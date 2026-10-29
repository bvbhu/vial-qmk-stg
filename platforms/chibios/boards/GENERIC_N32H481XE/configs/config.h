// Copyright 2026 bvbhu (vial-qmk-stg-n32h481 fork)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// N32H481 USBHS is a DWC OTG core with 6 endpoints available
// (N32_USBHS_ENDPOINTS = 6), so there is no need to share IN/OUT endpoint
// numbers (USB_ENDPOINTS_ARE_REORDERABLE).  Keeping the standalone OUT
// endpoint layout is what this port's USBOTGv1 LL driver supports
// properly: usb_endpoint_out_configure_cb() calls usbInitEndpointI(),
// usb_lld_init_endpoint() then programs DOEPCTL/DOEPTSIZ and unmasks the
// OUT endpoint in DAINTMSK.  With REORDERABLE defined, OUT configuration
// relies on the shared IN endpoint table entry and, combined with the N32
// RSTDIF reset reporting, the OUT data endpoint interrupts stayed masked
// forever (device enumerated, Vial hung - no raw HID OUT delivery).
//
// NOTE: with REORDERABLE removed, RAW_OUT_EPNUM becomes a standalone
// endpoint number (usb_descriptor.h NEXT_EPNUM), which is exactly what the
// endpoint count allows here.
