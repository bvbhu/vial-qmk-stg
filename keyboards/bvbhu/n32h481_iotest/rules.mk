# Copyright 2026 Talo
# SPDX-License-Identifier: GPL-3.0-or-later

# Exercise both the N32 SPIv1 low-level driver (enabled in halconf.h) and
# QMK's spi_master API on this pin-test board.  Driver/pin defaults live in
# platforms/chibios/chibios_config.h (SPID1, PA5/PA6/PA7 AF1) and can be
# overridden in config.h when those pins are needed by a matrix.
SPI_DRIVER_REQUIRED = yes
