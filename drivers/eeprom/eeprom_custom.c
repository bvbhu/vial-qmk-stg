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
 * @brief   N32H481RE on-chip Flash EEPROM driver (dual-page mirror).
 *
 * Layout (last 16 KB of the 512 KB Flash, reserved by the linker):
 *   page 0: 0x0807C000 - 0x0807DFFF (8 KB)
 *   page 1: 0x0807E000 - 0x0807FFFF (8 KB)
 *
 *   0x0000..0x001F  header (4 double words, written first, ascending)
 *   0x0020..0x1FF7  data area (8152 bytes physical); only the first
 *                   N32_EEPROM_USED_SIZE == 2304 bytes are mirrored, the
 *                   remainder stays erased (QMK addresses <= 2048 bytes)
 *   0x1FF8..0x1FFF  commit trailer: magic ('N32E') + sequence
 *
 * The N32H48x flash controller only accepts double-word programs in
 * ASCENDING address order after a page erase.  The previous version wrote
 * the magic+sequence word at offset 0 LAST (an address that is lower than
 * every data word just written).  Measured result: exactly one PGERR per
 * eeprom_write_block(), first_err_addr == 0x0807E000, and both page headers
 * left at 0xFFFFFFFF -- so no page ever became valid and every read returned
 * 0xFF (the "keycode reverts after refresh" symptom).
 *
 * Putting the commit marker at the very END of the page keeps the atomic
 * commit property (a half-written page has no valid trailer) while making
 * the whole programming sequence strictly ascending, exactly like the
 * vendor SDK's Flash_Program example (unlock once -> erase -> loop
 * FLASH_ProgramdoubleWord() from the page base -> lock once).
 */

#include <stdint.h>
#include <string.h>

#include "hal.h"

#include "eeprom_driver.h"

/*===========================================================================*/
/* Flash access helpers (register definitions from n32h47x_48x.h).          */
/*===========================================================================*/

#define N32_FLASH_KEY1              0x45670123U
#define N32_FLASH_KEY2              0xCDEF89ABU

#define N32_EEPROM_BASE             0x0807C000U /* Flash 512K - 16K */
#define N32_EEPROM_PAGE_SIZE        8192U
#define N32_EEPROM_HEADER_SIZE      32U
#define N32_EEPROM_TRAILER_OFF      (N32_EEPROM_PAGE_SIZE - 8U) /* 0x1FF8 */
#define N32_EEPROM_DATA_SIZE        (N32_EEPROM_TRAILER_OFF - N32_EEPROM_HEADER_SIZE)

/* QMK never addresses more than EEPROM_SIZE bytes (2048 on this board), so
   mirroring the whole 8152-byte data area on EVERY write is pure waste:
   ~1019 double-word programs, and because instructions are fetched from the
   same flash bank the CPU stalls for the whole operation.  That stall is
   long enough to make an animated RGB effect visibly stutter whenever Vial
   saves a lighting/keymap setting.  Mirror only the used window plus 256
   bytes of headroom (still 8-byte aligned); the rest of the page simply
   stays erased.  Keep N32_EEPROM_DATA_SIZE as the physical page limit. */
#ifndef N32_EEPROM_USED_SIZE
#    define N32_EEPROM_USED_SIZE 2304U /* 2048 + 256 spare, multiple of 8 */
#endif
#if defined(EEPROM_SIZE) && (EEPROM_SIZE > N32_EEPROM_USED_SIZE)
#    error "N32_EEPROM_USED_SIZE is smaller than EEPROM_SIZE"
#endif
#if N32_EEPROM_USED_SIZE > N32_EEPROM_DATA_SIZE
#    error "N32_EEPROM_USED_SIZE exceeds the mirrored data area"
#endif

#define N32_EEPROM_MAGIC            0x4E333245U /* 'N32E' - commit trailer */
#define N32_EEPROM_START            0x4E333253U /* 'N32S' - header start    */

/* Vendor SDK FLASH_STS_CLRFLAG:
 *   PGERR | PVERR | WRPERR | EOP | EVERR | RPADDERR = 0x0001003D. */
#define N32_FLASH_STS_CLRFLAG       0x0001003DU

/* Flags that mean the program operation failed.  EOP/BUSY/FWORDF/ECC1 are
 * deliberately not treated as errors. */
#define N32_FLASH_STS_ERRORS        (0x0000003CU | 0x00001000U | 0x00010000U)

#define N32_FLASH_WAIT_LIMIT        0x00400000U

/* Diagnostics, readable over SWD via the ELF symbol addresses. */
volatile uint32_t n32_eedbg_write_cnt;
volatile uint32_t n32_eedbg_program_cnt;
volatile uint32_t n32_eedbg_error_cnt;
volatile uint32_t n32_eedbg_last_sts;
volatile uint32_t n32_eedbg_verify_bad;
volatile uint32_t n32_eedbg_first_err_addr;
volatile uint32_t n32_eedbg_first_err_sts;
volatile uint32_t n32_eedbg_commit_cnt;
volatile uint32_t n32_eedbg_commit_ok;

/* Erase/program helpers below assume the controller is already unlocked.
   A whole page update is wrapped in exactly one begin/end pair. */
static inline void n32_flash_begin(void) {
    FLASH->KEY = N32_FLASH_KEY1;
    FLASH->KEY = N32_FLASH_KEY2;
}

static inline void n32_flash_end(void) {
    FLASH->CTRL |= FLASH_CTRL_LOCK;
}

static uint32_t n32_flash_wait(void) {
    uint32_t guard = N32_FLASH_WAIT_LIMIT;
    /* The N32 flash controller asserts BUSY a few cycles AFTER the last
       register store.  Polling only for BUSY==0 can therefore return while
       the operation has not started yet; the caller then clears PG/START and
       aborts the program part-way through.  Measured symptom: the trailing
       1-3 bytes of one double word stayed 0xFF, read-back verification
       failed and the commit trailer was never written (Vial keycode edits
       silently reverted).  Wait for BUSY to appear first, then to clear. */
    uint32_t start = 4096U;
    while (((FLASH->STS & FLASH_STS_BUSY) == 0U) && --start) {
    }
    while ((FLASH->STS & FLASH_STS_BUSY) && --guard) {
    }
    return FLASH->STS;
}

static void n32_flash_erase_page_raw(uint32_t addr) {
    FLASH->STS = N32_FLASH_STS_CLRFLAG;
    n32_flash_wait();
    FLASH->CTRL |= FLASH_CTRL_PER;
    FLASH->ADD = addr;
    FLASH->CTRL |= FLASH_CTRL_START;
    n32_flash_wait();
    FLASH->CTRL &= (uint32_t)~FLASH_CTRL_PER;
}

static void n32_flash_program_dword(uint32_t addr, uint32_t lo, uint32_t hi) {
    if (addr & 7U) {
        /* SDK FLASH_ProgramdoubleWord() rejects non-8-byte-aligned targets. */
        n32_eedbg_error_cnt++;
        n32_eedbg_last_sts = 0xADD00001U;
        return;
    }

    FLASH->STS = N32_FLASH_STS_CLRFLAG;
    n32_flash_wait();
    FLASH->CTRL |= FLASH_CTRL_PG;
    *(__IO uint32_t *)addr = lo;
    *(__IO uint32_t *)(addr + 4U) = hi;
    /* Latch window for the second word store (the controller samples both
       halves as one 8-byte program). */
    for (volatile uint32_t settle = 0U; settle < 64U; settle++) {
    }
    uint32_t sts = n32_flash_wait();
    FLASH->CTRL &= (uint32_t)~FLASH_CTRL_PG;

    n32_eedbg_program_cnt++;
    if (sts & N32_FLASH_STS_ERRORS) {
        if (n32_eedbg_error_cnt == 0U) {
            n32_eedbg_first_err_addr = addr;
            n32_eedbg_first_err_sts = sts;
        }
        n32_eedbg_error_cnt++;
        n32_eedbg_last_sts = sts;
    }
}

/*===========================================================================*/
/* Page management.                                                          */
/*===========================================================================*/

typedef struct {
    uint32_t magic;
    uint32_t sequence;
} n32_eeprom_trailer_t;

static uint32_t n32_eeprom_page_addr(uint32_t page) {
    return N32_EEPROM_BASE + page * N32_EEPROM_PAGE_SIZE;
}

static uint32_t n32_eeprom_data_addr(uint32_t page) {
    return n32_eeprom_page_addr(page) + N32_EEPROM_HEADER_SIZE;
}

static const n32_eeprom_trailer_t *n32_eeprom_trailer(uint32_t page) {
    return (const n32_eeprom_trailer_t *)(n32_eeprom_page_addr(page) + N32_EEPROM_TRAILER_OFF);
}

static bool n32_eeprom_page_valid(uint32_t page) {
    return (n32_eeprom_trailer(page)->magic == N32_EEPROM_MAGIC);
}

static uint32_t n32_eeprom_active_page(void) {
    bool v0 = n32_eeprom_page_valid(0U);
    bool v1 = n32_eeprom_page_valid(1U);
    if (v0 && v1) {
        uint32_t s0 = n32_eeprom_trailer(0U)->sequence;
        uint32_t s1 = n32_eeprom_trailer(1U)->sequence;
        /* Signed difference stays correct across a 32-bit sequence wrap. */
        return ((int32_t)(s0 - s1) >= 0) ? 0U : 1U;
    }
    if (v0) {
        return 0U;
    }
    if (v1) {
        return 1U;
    }
    return 0U; /* both empty: start from page 0 */
}

/* Expected byte of the new image at data offset `off`.  The source page is
   read directly from Flash, so no 8 KB RAM copy is needed. */
static uint8_t n32_eeprom_expected(const uint8_t *src, uint32_t off,
                                   const uint8_t *patch, uint32_t patch_off,
                                   uint32_t patch_len) {
    if (off >= patch_off && off < patch_off + patch_len) {
        return patch[off - patch_off];
    }
    return src[off];
}

/* Write the four header double words strictly in ascending order, from the
   very first word of the page. */
static void n32_eeprom_write_header(uint32_t base, uint32_t seq) {
    n32_flash_program_dword(base + 0x00U, N32_EEPROM_START, seq);
    n32_flash_program_dword(base + 0x08U, 0U, 0U);
    n32_flash_program_dword(base + 0x10U, 0U, 0U);
    n32_flash_program_dword(base + 0x18U, 0U, 0U);
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void eeprom_driver_init(void) {
    /* Nothing to do: the active page is resolved lazily. */
}

void eeprom_driver_format(bool erase) {
    (void)erase;

    n32_flash_begin();
    /* Both pages are always invalidated, otherwise a stale page with a higher
       sequence could win the next n32_eeprom_active_page() call. */
    n32_flash_erase_page_raw(n32_eeprom_page_addr(0U));
    n32_flash_erase_page_raw(n32_eeprom_page_addr(1U));

    /* Empty image: data area stays erased (0xFF == empty EEPROM). */
    n32_eeprom_write_header(n32_eeprom_page_addr(0U), 1U);
    n32_flash_program_dword(n32_eeprom_page_addr(0U) + N32_EEPROM_TRAILER_OFF,
                            N32_EEPROM_MAGIC, 1U);
    n32_flash_end();
}

void eeprom_driver_erase(void) {
    eeprom_driver_format(true);
}

void eeprom_read_block(void *buf, const void *addr, size_t len) {
    uint32_t page   = n32_eeprom_active_page();
    uint32_t offset = (uint32_t)(uintptr_t)addr;
    if (offset + len > N32_EEPROM_USED_SIZE) {
        return;
    }
    const uint8_t *src = (const uint8_t *)n32_eeprom_data_addr(page) + offset;
    memcpy(buf, src, len);
}

void eeprom_write_block(const void *buf, void *addr, size_t len) {
    uint32_t data_off = (uint32_t)(uintptr_t)addr;
    if (len == 0U || data_off + len > N32_EEPROM_USED_SIZE) {
        return;
    }

    const uint8_t *patch = (const uint8_t *)buf;
    uint32_t src_page = n32_eeprom_active_page();
    uint32_t dst_page = (src_page == 0U) ? 1U : 0U;
    uint32_t dst_base = n32_eeprom_page_addr(dst_page);
    const uint8_t *src = (const uint8_t *)n32_eeprom_data_addr(src_page);
    uint8_t *dst       = (uint8_t *)n32_eeprom_data_addr(dst_page);

    uint32_t seq = n32_eeprom_trailer(src_page)->sequence;
    /* A blank source page reads 0xFFFFFFFF; never let it wrap to zero. */
    seq = (seq == 0xFFFFFFFFU) ? 1U : (seq + 1U);
    if (seq == 0U) {
        seq = 1U;
    }

    n32_eedbg_write_cnt++;

    n32_flash_begin();
    n32_flash_erase_page_raw(dst_base);

    /* Header first, starting at the lowest address of the page. */
    n32_eeprom_write_header(dst_base, seq);

    /* Copy source -> destination one double word at a time, patching the
       changed range on the fly.  Erased (all-0xFF) double words are skipped:
       the fresh page already reads 0xFF.  Addresses still increase, which is
       what the flash controller requires. */
    for (uint32_t off = 0U; off < N32_EEPROM_USED_SIZE; off += 8U) {
        uint8_t tmp[8];
        for (uint32_t i = 0U; i < 8U; i++) {
            tmp[i] = n32_eeprom_expected(src, off + i, patch, data_off, (uint32_t)len);
        }
        uint32_t lo, hi;
        memcpy(&lo, tmp, 4U);
        memcpy(&hi, tmp + 4U, 4U);
        if (lo == 0xFFFFFFFFU && hi == 0xFFFFFFFFU) {
            continue;
        }
        n32_flash_program_dword((uint32_t)(uintptr_t)(dst + off), lo, hi);
    }

    /* Verify the written image before committing. */
    uint32_t bad = 0U;
    for (uint32_t off = 0U; off < N32_EEPROM_USED_SIZE; off++) {
        if (dst[off] != n32_eeprom_expected(src, off, patch, data_off, (uint32_t)len)) {
            bad++;
        }
    }
    n32_eedbg_verify_bad = bad;

    /* Commit marker is the LAST double word of the page: highest address, so
       the whole sequence stays ascending.  A power loss before this program
       leaves the page without a valid trailer and the old page stays active. */
    if (bad == 0U) {
        n32_flash_program_dword(dst_base + N32_EEPROM_TRAILER_OFF,
                                N32_EEPROM_MAGIC, seq);
        n32_eedbg_commit_cnt++;
        if (n32_eeprom_page_valid(dst_page)) {
            n32_eedbg_commit_ok++;
        }
    }
    n32_flash_end();
}
