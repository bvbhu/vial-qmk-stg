// Copyright 2026 bvbhu (vial-qmk-stg-n32h481 fork)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// N32H481 USBHS is a DWC OTG core, same as STM32/AT32/WB32. QMK only uses
// shared IN/OUT endpoints when USB_ENDPOINTS_ARE_REORDERABLE is defined;
// tmk_core/protocol/usb_descriptor.h auto-defines it only for
// STM32_USB_USE_OTG1. Without it the RAW HID OUT endpoint is allocated as a
// standalone endpoint and, on this port, it was never enabled in DAINTMSK:
// the device enumerated but never received raw HID reports, so Vial hung.
// Define it here to match GENERIC_AT32_F415XX / GENERIC_WB32_*.
#define USB_ENDPOINTS_ARE_REORDERABLE
