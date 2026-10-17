/*
    Copyright (C) 2026 bvbhu (N32H48x port for vial-qmk-stg-n32h481)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    eeprom_custom.c
 * @brief   N32H481RE on-chip Flash EEPROM driver (dual-page mirrored
 *          wear leveling).
 *
 * Layout (last 16 KB of the 512 KB Flash, reserved by the linker):
 *   page 0: 0x0807C000 - 0x0807DFFF (8 KB)
 *   page 1: 0x0807E000 - 0x0807FFFF (8 KB)
 *
 * Each page starts with a 32-byte header:
 *   0x00: magic  ('N32E')
 *   0x04: sequence (incremented on each write)
 *   0x08: XOR checksum of the data area
 *   0x0C: reserved
 * Data area follows at offset 0x20 (8160 bytes, 8-byte doubleWord
 * programming granularity per N32H481 Flash).
 *
 * Wear leveling: writes always target the page that is NOT currently
 * active, alternating between the two pages on every erase. Flash can
 * only transition bits from 1 -> 0, so an 8 KB page must be erased
 * before it can be reprogrammed; the dual-page mirror halves the erase
 * count per page versus a single-sector scheme.
 */

#include <stdint.h>
#include <string.h>

#include "hal.h"

#include "eeprom_driver.h"

/*===========================================================================*/
/* Flash access helpers (registers from n32h47x_48x.h).                     */
/*===========================================================================*/

#define N32_FLASH_KEY1              0x45670123U
#define N32_FLASH_KEY2              0xCDEF89ABU

#define N32_EEPROM_BASE             0x0807C000U /* Flash 512K - 16K */
#define N32_EEPROM_PAGE_SIZE        8192U
#define N32_EEPROM_PAGES            2U
#define N32_EEPROM_HEADER_SIZE      32U
#define N32_EEPROM_DATA_SIZE        (N32_EEPROM_PAGE_SIZE - N32_EEPROM_HEADER_SIZE)

#define N32_EEPROM_MAGIC            0x4E333245U /* 'N32E' */

/* Writable status flags (write 1 to clear), matching the SDK's
   FLASH_STS_CLRFLAG (0x1FC7F): EOP|PGERR|WRPERR|PVERR|EVERR|ECC1/2ERR|
   RDKEYERR|RDXKEYERR|JSERR|RTPKEYERR|DECCERR|FWORDF|RPADDERR etc. */
#define N32_FLASH_STS_CLRFLAG       0x0001FC7FU

static inline void n32_flash_unlock(void) {
    FLASH->KEY = N32_FLASH_KEY1;
    FLASH->KEY = N32_FLASH_KEY2;
}

static inline void n32_flash_lock(void) {
    FLASH->CTRL |= FLASH_CTRL_LOCK;
}

static void n32_flash_wait(void) {
    while (FLASH->STS & FLASH_STS_BUSY) {
    }
}

static void n32_flash_clear_status(void) {
    FLASH->STS = N32_FLASH_STS_CLRFLAG;
}

static void n32_flash_erase_page(uint32_t addr) {
    n32_flash_unlock();
    n32_flash_clear_status();
    n32_flash_wait();
    FLASH->CTRL |= FLASH_CTRL_PER;
    FLASH->ADD = addr;
    FLASH->CTRL |= FLASH_CTRL_START;
    n32_flash_wait();
    FLASH->CTRL &= (uint32_t)~FLASH_CTRL_PER;
    n32_flash_lock();
}

static void n32_flash_program_dword(uint32_t addr, uint32_t lo, uint32_t hi) {
    n32_flash_unlock();
    n32_flash_clear_status();
    n32_flash_wait();
    FLASH->CTRL |= FLASH_CTRL_PG;
    *(__IO uint32_t *)addr = lo;
    *(__IO uint32_t *)(addr + 4U) = hi;
    n32_flash_wait();
    FLASH->CTRL &= (uint32_t)~FLASH_CTRL_PG;
    n32_flash_lock();
}

/*===========================================================================*/
/* Page management.                                                          */
/*===========================================================================*/

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t checksum;
    uint32_t reserved[5];
} n32_eeprom_header_t;

static uint32_t n32_eeprom_page_addr(uint32_t page) {
    return N32_EEPROM_BASE + page * N32_EEPROM_PAGE_SIZE;
}

static uint32_t n32_eeprom_data_addr(uint32_t page) {
    return n32_eeprom_page_addr(page) + N32_EEPROM_HEADER_SIZE;
}

static uint32_t n32_eeprom_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0U;
    for (size_t i = 0; i < len; i++) {
        sum ^= (uint32_t)data[i] << ((i % 4U) * 8U);
    }
    return sum;
}

static bool n32_eeprom_page_valid(uint32_t page) {
    const n32_eeprom_header_t *h = (const n32_eeprom_header_t *)n32_eeprom_page_addr(page);
    return (h->magic == N32_EEPROM_MAGIC);
}

static uint32_t n32_eeprom_active_page(void) {
    bool v0 = n32_eeprom_page_valid(0U);
    bool v1 = n32_eeprom_page_valid(1U);
    if (v0 && v1) {
        uint32_t s0 = ((const n32_eeprom_header_t *)n32_eeprom_page_addr(0U))->sequence;
        uint32_t s1 = ((const n32_eeprom_header_t *)n32_eeprom_page_addr(1U))->sequence;
        return (s0 >= s1) ? 0U : 1U;
    }
    if (v0) {
        return 0U;
    }
    if (v1) {
        return 1U;
    }
    return 0U; /* both empty: start from page 0 */
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void eeprom_driver_init(void) {
    /* Nothing to do: the active page is resolved lazily on the first
       read/write via n32_eeprom_active_page(). */
}

void eeprom_driver_format(bool erase) {
    if (erase) {
        n32_flash_erase_page(n32_eeprom_page_addr(0U));
        n32_flash_erase_page(n32_eeprom_page_addr(1U));
    }
    /* Bring page 0 up with an empty image. */
    uint8_t zeros[N32_EEPROM_DATA_SIZE];
    memset(zeros, 0xFFU, sizeof(zeros));
    n32_flash_erase_page(n32_eeprom_page_addr(0U));
    n32_flash_program_dword(n32_eeprom_page_addr(0U), N32_EEPROM_MAGIC, 1U);
    n32_flash_program_dword(n32_eeprom_page_addr(0U) + 4U, n32_eeprom_checksum(zeros, N32_EEPROM_DATA_SIZE), 0U);
    for (size_t off = N32_EEPROM_HEADER_SIZE; off < N32_EEPROM_PAGE_SIZE; off += 8U) {
        n32_flash_program_dword(n32_eeprom_page_addr(0U) + off, 0xFFFFFFFFU, 0xFFFFFFFFU);
    }
}

void eeprom_driver_erase(void) {
    eeprom_driver_format(true);
}

void eeprom_read_block(void *buf, const void *addr, size_t len) {
    uint32_t page = n32_eeprom_active_page();
    const uint8_t *src = (const uint8_t *)n32_eeprom_data_addr(page) + (uint32_t)(uintptr_t)addr;
    memcpy(buf, src, len);
}

void eeprom_write_block(const void *buf, void *addr, size_t len) {
    uint32_t src_page = n32_eeprom_active_page();
    uint32_t dst_page  = (src_page == 0U) ? 1U : 0U;
    uint32_t dst_addr  = n32_eeprom_data_addr(dst_page);
    uint32_t data_off  = (uint32_t)(uintptr_t)addr;

    /* Mirror the current page image into RAM, patch the changed range,
       then program the alternate page (must be erased first). */
    uint8_t image[N32_EEPROM_DATA_SIZE];
    memcpy(image, (const uint8_t *)n32_eeprom_data_addr(src_page), N32_EEPROM_DATA_SIZE);
    memcpy(image + data_off, buf, len);

    n32_flash_erase_page(n32_eeprom_page_addr(dst_page));

    n32_eeprom_header_t header;
    header.magic    = N32_EEPROM_MAGIC;
    header.sequence = ((const n32_eeprom_header_t *)n32_eeprom_page_addr(src_page))->sequence + 1U;
    header.checksum = n32_eeprom_checksum(image, N32_EEPROM_DATA_SIZE);
    memset(header.reserved, 0U, sizeof(header.reserved));

    n32_flash_program_dword(n32_eeprom_page_addr(dst_page) + 0x00U, header.magic, header.sequence);
    n32_flash_program_dword(n32_eeprom_page_addr(dst_page) + 0x08U, header.checksum, 0U);
    for (size_t i = 0U; i < 5U; i++) {
        n32_flash_program_dword(n32_eeprom_page_addr(dst_page) + 0x0CU + i * 8U, 0U, 0U);
    }
    for (size_t off = 0U; off < N32_EEPROM_DATA_SIZE; off += 8U) {
        n32_flash_program_dword(dst_addr + off, ((uint32_t *)image)[off / 4U], ((uint32_t *)image)[off / 4U + 1U]);
    }
}
