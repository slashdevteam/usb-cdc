/*
 * This code is adapted from usb_device_cdc_vcom example application
 * from NXP SDK_2.2_MK24FN1M0xxx12
 *
 * Adaptations: Copyright (c) 2018 Slashdev SDG UG
 *
 * Original copyright:
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "usbtypes.h"
#include "usb_device.h"
#include <cstdint>
#include "usb.h"

#define USB_DEVICE_STATUS_SIZE (0x02U)
#define USB_INTERFACE_STATUS_SIZE (0x02U)
#define USB_ENDPOINT_STATUS_SIZE (0x02U)
#define USB_CONFIGURE_SIZE (0X01U)
#define USB_INTERFACE_SIZE (0X01U)
#define USB_GET_STATUS_DEVICE_MASK (0x03U)
#define USB_GET_STATUS_INTERFACE_MASK (0x03U)
#define USB_GET_STATUS_ENDPOINT_MASK (0x03U)

enum usb_device_control_read_write_sequence_t
{
    kUSB_DeviceControlPipeSetupStage = 0U,
    kUSB_DeviceControlPipeDataStage,
    kUSB_DeviceControlPipeStatusStage,
};

struct usb_device_common_class_struct_t
{
    usb_device_handle handle;
    uint8_t setupBuffer[USB_SETUP_PACKET_SIZE];
    uint16_t standardTranscationBuffer;
    uint8_t controllerId;
};

namespace usb
{

class DeviceClass;

class Ch9
{

public:
    Ch9(DeviceClass* _parent);

    int32_t deviceControlPipeInit(usb_device_handle handle, void* param);
    int32_t controlCallback(usb_device_handle handle,
                            usb_device_endpoint_callback_message_struct_t *message,
                            void* callbackParam);
private:
    int32_t getStatus(usb_device_common_class_struct_t* classHandle,
                      usb_setup_struct_t* setup,
                      uint8_t** buffer,
                      uint32_t* length);
    int32_t setClearFeature(usb_device_common_class_struct_t* classHandle,
                            usb_setup_struct_t* setup,
                            uint8_t** buffer,
                            uint32_t* length);
    int32_t setAddress(usb_device_common_class_struct_t* classHandle,
                       usb_setup_struct_t* setup,
                       uint8_t** buffer,
                       uint32_t* length);
    int32_t getDescriptor(usb_device_common_class_struct_t* classHandle,
                          usb_setup_struct_t* setup,
                          uint8_t** buffer,
                          uint32_t* length);
    int32_t getConfiguration(usb_device_common_class_struct_t* classHandle,
                             usb_setup_struct_t* setup,
                             uint8_t** buffer,
                             uint32_t* length);
    int32_t setConfiguration(usb_device_common_class_struct_t* classHandle,
                             usb_setup_struct_t* setup,
                             uint8_t** buffer,
                             uint32_t* length);
    int32_t getInterface(usb_device_common_class_struct_t* classHandle,
                         usb_setup_struct_t* setup,
                         uint8_t** buffer,
                         uint32_t* length);
    int32_t setInterface(usb_device_common_class_struct_t* classHandle,
                         usb_setup_struct_t* setup,
                         uint8_t** buffer,
                         uint32_t* length);
    int32_t synchFrame(usb_device_common_class_struct_t* classHandle,
                       usb_setup_struct_t* setup,
                       uint8_t** buffer,
                       uint32_t* length);
    int32_t controlCallbackFeedback(usb_device_handle handle,
                                    usb_setup_struct_t* setup,
                                    int32_t error,
                                    usb_device_control_read_write_sequence_t stage,
                                    uint8_t** buffer,
                                    uint32_t* length);

    int32_t handleRequest(uint8_t request,
                          usb_device_common_class_struct_t* classHandle,
                          usb_setup_struct_t* setup,
                          uint8_t** buffer,
                          uint32_t* length);

private:
    DeviceClass* parent;
};

}
