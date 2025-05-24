/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HID_H
#define HID_H

#include "config.h"

void hid_init(void);

void send_encoded_keys(struct encoded_keys keys);

#endif // HID_H