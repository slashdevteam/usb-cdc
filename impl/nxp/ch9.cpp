#include "ch9.h"

#include "deviceclass.h"

extern "C"
{

usb_status_t USB_DeviceControlCallback(usb_device_handle handle,
                                       usb_device_endpoint_callback_message_struct_t* message,
                                       void* callbackParam);
USB_GLOBAL usb::Ch9* gCh9;

}

namespace usb
{

Ch9::Ch9(DeviceClass* _parent)
      : parent(_parent)
{
    gCh9 = this;
};

int32_t Ch9::deviceControlPipeInit(usb_device_handle handle, void* param)
{
    usb_device_endpoint_init_struct_t epInitStruct;
    usb_device_endpoint_callback_struct_t endpointCallback;
    int32_t error;

    endpointCallback.callbackFn = USB_DeviceControlCallback;
    endpointCallback.callbackParam = param;

    epInitStruct.zlt = 1U;
    epInitStruct.transferType = USB_ENDPOINT_CONTROL;
    epInitStruct.endpointAddress = USB_CONTROL_ENDPOINT | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
    epInitStruct.maxPacketSize = USB_CONTROL_MAX_PACKET_SIZE;
    /* Initialize the control IN pipe */
    error = USB_DeviceInitEndpoint(handle, &epInitStruct, &endpointCallback);

    if(kStatus_USB_Success != error)
    {
        return error;
    }
    epInitStruct.endpointAddress = USB_CONTROL_ENDPOINT | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
    /* Initialize the control OUT pipe */
    error = USB_DeviceInitEndpoint(handle, &epInitStruct, &endpointCallback);

    if(kStatus_USB_Success != error)
    {
        USB_DeviceDeinitEndpoint(handle,
                                 USB_CONTROL_ENDPOINT | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT));
        return error;
    }
    return kStatus_USB_Success;
}

int32_t Ch9::controlCallback(usb_device_handle handle,
                             usb_device_endpoint_callback_message_struct_t* message,
                             void* callbackParam)
{
    usb_setup_struct_t* deviceSetup;
    usb_device_common_class_struct_t* classHandle;
    uint8_t* buffer = (uint8_t*)nullptr;
    uint32_t length = 0U;
    int32_t error = kStatus_USB_InvalidRequest;
    uint8_t state;

    if((0xFFFFFFFFU == message->length) || (nullptr == callbackParam))
    {
        return error;
    }

    classHandle = (usb_device_common_class_struct_t*)callbackParam;
    deviceSetup = (usb_setup_struct_t*)&classHandle->setupBuffer[0];
    USB_DeviceGetStatus(handle, kUSB_DeviceStatusDeviceState, &state);

    if(message->isSetup)
    {
        if((USB_SETUP_PACKET_SIZE != message->length) || (nullptr == message->buffer))
        {
            return error;
        }
        /* Receive a setup request */
        usb_setup_struct_t* setup = (usb_setup_struct_t*)(message->buffer);

        /* Copy the setup packet to the application buffer */
        deviceSetup->wValue = USB_SHORT_FROM_LITTLE_ENDIAN(setup->wValue);
        deviceSetup->wIndex = USB_SHORT_FROM_LITTLE_ENDIAN(setup->wIndex);
        deviceSetup->wLength = USB_SHORT_FROM_LITTLE_ENDIAN(setup->wLength);
        deviceSetup->bRequest = setup->bRequest;
        deviceSetup->bmRequestType = setup->bmRequestType;

        if((deviceSetup->bmRequestType & USB_REQUEST_TYPE_TYPE_MASK) == USB_REQUEST_TYPE_TYPE_STANDARD)
        {
            /* Handle the standard request */
            error = handleRequest(deviceSetup->bRequest, classHandle, deviceSetup, &buffer, &length);
        }
        else
        {
            if((deviceSetup->wLength) &&
                ((deviceSetup->bmRequestType & USB_REQUEST_TYPE_DIR_MASK) == USB_REQUEST_TYPE_DIR_OUT))
            {
                /* Class or vendor request with the OUT data phase. */
                if((deviceSetup->wLength) &&
                    ((deviceSetup->bmRequestType & USB_REQUEST_TYPE_TYPE_CLASS) == USB_REQUEST_TYPE_TYPE_CLASS))
                {
                    /* Get data buffer to receive the data from the host. */
                    usb_device_control_request_struct_t controlRequest;
                    controlRequest.buffer = (uint8_t*)nullptr;
                    controlRequest.isSetup = 1U;
                    controlRequest.setup = deviceSetup;
                    controlRequest.length = deviceSetup->wLength;
                    error = parent->event(handle, kUSB_DeviceClassEventClassRequest, &controlRequest);
                    length = controlRequest.length;
                    buffer = controlRequest.buffer;
                }
                else if((deviceSetup->wLength) &&
                         ((deviceSetup->bmRequestType & USB_REQUEST_TYPE_TYPE_VENDOR) == USB_REQUEST_TYPE_TYPE_VENDOR))
                {
                    /* Get data buffer to receive the data from the host. */
                    usb_device_control_request_struct_t controlRequest;
                    controlRequest.buffer = (uint8_t*)nullptr;
                    controlRequest.isSetup = 1U;
                    controlRequest.setup = deviceSetup;
                    controlRequest.length = deviceSetup->wLength;
                    error = parent->callback(handle, kUSB_DeviceEventVendorRequest, &controlRequest);
                    length = controlRequest.length;
                    buffer = controlRequest.buffer;
                }
                else
                {
                }
                if(kStatus_USB_Success == error)
                {
                    /* Prime an OUT transfer */
                    error = USB_DeviceRecvRequest(handle, USB_CONTROL_ENDPOINT, buffer, deviceSetup->wLength);
                    return error;
                }
            }
            else
            {
                /* Class or vendor request with the IN data phase. */
                if(((deviceSetup->bmRequestType & USB_REQUEST_TYPE_TYPE_CLASS) == USB_REQUEST_TYPE_TYPE_CLASS))
                {
                    /* Get data buffer to response the host. */
                    usb_device_control_request_struct_t controlRequest;
                    controlRequest.buffer = (uint8_t*)nullptr;
                    controlRequest.isSetup = 1U;
                    controlRequest.setup = deviceSetup;
                    controlRequest.length = deviceSetup->wLength;
                    error = parent->event(handle, kUSB_DeviceClassEventClassRequest, &controlRequest);
                    length = controlRequest.length;
                    buffer = controlRequest.buffer;
                }
                else if(((deviceSetup->bmRequestType & USB_REQUEST_TYPE_TYPE_VENDOR) == USB_REQUEST_TYPE_TYPE_VENDOR))
                {
                    /* Get data buffer to response the host. */
                    usb_device_control_request_struct_t controlRequest;
                    controlRequest.buffer = (uint8_t*)nullptr;
                    controlRequest.isSetup = 1U;
                    controlRequest.setup = deviceSetup;
                    controlRequest.length = deviceSetup->wLength;
                    error = parent->callback(handle, kUSB_DeviceEventVendorRequest, &controlRequest);
                    length = controlRequest.length;
                    buffer = controlRequest.buffer;
                }
                else
                {
                }
            }
        }
        /* Send the reponse to the host. */
        error = controlCallbackFeedback(handle,
                                        deviceSetup,
                                        error,
                                        kUSB_DeviceControlPipeSetupStage,
                                        &buffer,
                                        &length);
    }
    else if(kUSB_DeviceStateAddressing == state)
    {
        /* Set the device address to controller. */
        error = handleRequest(deviceSetup->bRequest, classHandle, deviceSetup, &buffer, &length);
    }
    else if((message->length) && (deviceSetup->wLength) &&
             ((deviceSetup->bmRequestType & USB_REQUEST_TYPE_DIR_MASK) == USB_REQUEST_TYPE_DIR_OUT))
    {
        if(((deviceSetup->bmRequestType & USB_REQUEST_TYPE_TYPE_CLASS) == USB_REQUEST_TYPE_TYPE_CLASS))
        {
            /* Data received in OUT phase, and notify the class driver. */
            usb_device_control_request_struct_t controlRequest;
            controlRequest.buffer = message->buffer;
            controlRequest.isSetup = 0U;
            controlRequest.setup = deviceSetup;
            controlRequest.length = message->length;
            error = parent->event(handle, kUSB_DeviceClassEventClassRequest, &controlRequest);
        }
        else if(((deviceSetup->bmRequestType & USB_REQUEST_TYPE_TYPE_VENDOR) == USB_REQUEST_TYPE_TYPE_VENDOR))
        {
            /* Data received in OUT phase, and notify the application. */
            usb_device_control_request_struct_t controlRequest;
            controlRequest.buffer = message->buffer;
            controlRequest.isSetup = 0U;
            controlRequest.setup = deviceSetup;
            controlRequest.length = message->length;
            error = parent->callback(handle, kUSB_DeviceEventVendorRequest, &controlRequest);
        }
        else
        {
        }
        /* Send the reponse to the host. */
        error = controlCallbackFeedback(handle,
                                        deviceSetup,
                                        error,
                                        kUSB_DeviceControlPipeDataStage,
                                        &buffer,
                                        &length);
    }
    else
    {
    }
    return error;
}

int32_t Ch9::handleRequest(uint8_t request,
                           usb_device_common_class_struct_t* classHandle,
                           usb_setup_struct_t* setup,
                           uint8_t** buffer,
                           uint32_t* length)
{
    int32_t error = kStatus_USB_Success;

    switch(request)
    {
        case 0:
            getStatus(classHandle, setup, buffer, length);
            break;
        case 1:
            setClearFeature(classHandle, setup, buffer, length);
            break;
        case 3:
            setClearFeature(classHandle, setup, buffer, length);
            break;
        case 5:
            setAddress(classHandle, setup, buffer, length);
            break;
        case 6:
            getDescriptor(classHandle, setup, buffer, length);
            break;
        case 8:
            getConfiguration(classHandle, setup, buffer, length);
            break;
        case 9:
            setConfiguration(classHandle, setup, buffer, length);
            break;
        case 10:
            getInterface(classHandle, setup, buffer, length);
            break;
        case 11:
            setInterface(classHandle, setup, buffer, length);
            break;
        case 12:
            synchFrame(classHandle, setup, buffer, length);
            break;
        default:
            break;
    }
    return error;
}

int32_t Ch9::controlCallbackFeedback(usb_device_handle handle,
                                     usb_setup_struct_t* setup,
                                     int32_t error,
                                     usb_device_control_read_write_sequence_t stage,
                                     uint8_t** buffer,
                                     uint32_t* length)
{
    int32_t errorCode = kStatus_USB_Error;
    uint8_t direction = USB_IN;

    if(kStatus_USB_InvalidRequest == error)
    {
        /* Stall the control pipe when the request is unsupported. */
        if((!((setup->bmRequestType & USB_REQUEST_TYPE_TYPE_MASK) == USB_REQUEST_TYPE_TYPE_STANDARD)) &&
            ((setup->bmRequestType & USB_REQUEST_TYPE_DIR_MASK) == USB_REQUEST_TYPE_DIR_OUT) && (setup->wLength) &&
            (kUSB_DeviceControlPipeSetupStage == stage))
        {
            direction = USB_OUT;
        }
        errorCode = USB_DeviceStallEndpoint(handle,
            (USB_CONTROL_ENDPOINT) | (uint8_t)((uint32_t)direction << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT));
    }
    else
    {
        if(*length > setup->wLength)
        {
            *length = setup->wLength;
        }
        errorCode = USB_DeviceSendRequest(handle, (USB_CONTROL_ENDPOINT), *buffer, *length);

        if((kStatus_USB_Success == errorCode) &&
            (USB_REQUEST_TYPE_DIR_IN == (setup->bmRequestType & USB_REQUEST_TYPE_DIR_MASK)))
        {
            errorCode = USB_DeviceRecvRequest(handle, (USB_CONTROL_ENDPOINT), (uint8_t*)nullptr, 0U);
        }
    }
    return errorCode;
}

int32_t Ch9::getStatus(usb_device_common_class_struct_t* classHandle,
                       usb_setup_struct_t* setup,
                       uint8_t** buffer,
                       uint32_t* length)
{
    int32_t error = kStatus_USB_InvalidRequest;
    uint8_t state;

    USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);

    if((kUSB_DeviceStateAddress != state) && (kUSB_DeviceStateConfigured != state))
    {
        return error;
    }

    if((setup->bmRequestType & USB_REQUEST_TYPE_RECIPIENT_MASK) == USB_REQUEST_TYPE_RECIPIENT_DEVICE)
    {
#if(defined(USB_DEVICE_CONFIG_OTG) && (USB_DEVICE_CONFIG_OTG))
        if(setup->wIndex == USB_REQUEST_STANDARD_GET_STATUS_OTG_STATUS_SELECTOR)
        {
            error = USB_DeviceGetStatus(classHandle->handle,
                                        kUSB_DeviceStatusOtg,
                                        &classHandle->standardTranscationBuffer);
            classHandle->standardTranscationBuffer = USB_SHORT_TO_LITTLE_ENDIAN(classHandle->standardTranscationBuffer);
            /* The device status length must be USB_DEVICE_STATUS_SIZE. */
            *length = 1;
        }
        else /* Get the device status */
        {
#endif
            error = USB_DeviceGetStatus(classHandle->handle,
                                        kUSB_DeviceStatusDevice,
                                        &classHandle->standardTranscationBuffer);
            classHandle->standardTranscationBuffer =
                classHandle->standardTranscationBuffer & USB_GET_STATUS_DEVICE_MASK;
            classHandle->standardTranscationBuffer = USB_SHORT_TO_LITTLE_ENDIAN(classHandle->standardTranscationBuffer);
            /* The device status length must be USB_DEVICE_STATUS_SIZE. */
            *length = USB_DEVICE_STATUS_SIZE;
#if(defined(USB_DEVICE_CONFIG_OTG) && (USB_DEVICE_CONFIG_OTG))
        }
#endif
    }
    else if((setup->bmRequestType & USB_REQUEST_TYPE_RECIPIENT_MASK) == USB_REQUEST_TYPE_RECIPIENT_INTERFACE)
    {
        /* Get the interface status */
        error = kStatus_USB_Success;
        classHandle->standardTranscationBuffer = 0U;
        /* The interface status length must be USB_INTERFACE_STATUS_SIZE. */
        *length = USB_INTERFACE_STATUS_SIZE;
    }
    else if((setup->bmRequestType & USB_REQUEST_TYPE_RECIPIENT_MASK) == USB_REQUEST_TYPE_RECIPIENT_ENDPOINT)
    {
        /* Get the endpoint status */
        usb_device_endpoint_status_struct_t endpointStatus;
        endpointStatus.endpointAddress = (uint8_t)setup->wIndex;
        endpointStatus.endpointStatus = kUSB_DeviceEndpointStateIdle;
        error = USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusEndpoint, &endpointStatus);
        classHandle->standardTranscationBuffer = endpointStatus.endpointStatus & USB_GET_STATUS_ENDPOINT_MASK;
        classHandle->standardTranscationBuffer = USB_SHORT_TO_LITTLE_ENDIAN(classHandle->standardTranscationBuffer);
        /* The endpoint status length must be USB_INTERFACE_STATUS_SIZE. */
        *length = USB_ENDPOINT_STATUS_SIZE;
    }
    else
    {
    }
    *buffer = (uint8_t*)&classHandle->standardTranscationBuffer;
    return error;
};

int32_t Ch9::setClearFeature(usb_device_common_class_struct_t* classHandle,
                             usb_setup_struct_t* setup,
                             uint8_t** buffer,
                             uint32_t* length)
{
    int32_t error = kStatus_USB_InvalidRequest;
    uint8_t state;
    uint8_t isSet = 0U;

    USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);

    if((kUSB_DeviceStateAddress != state) && (kUSB_DeviceStateConfigured != state))
    {
        return error;
    }

    /* Identify the request is set or clear the feature. */
    if(USB_REQUEST_STANDARD_SET_FEATURE == setup->bRequest)
    {
        isSet = 1U;
    }

    if((setup->bmRequestType & USB_REQUEST_TYPE_RECIPIENT_MASK) == USB_REQUEST_TYPE_RECIPIENT_DEVICE)
    {
        /* Set or Clear the device featrue. */
        if(USB_REQUEST_STANDARD_FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP == setup->wValue)
        {
#if((defined(USB_DEVICE_CONFIG_REMOTE_WAKEUP)) && (USB_DEVICE_CONFIG_REMOTE_WAKEUP > 0U))
            USB_DeviceSetStatus(classHandle->handle, kUSB_DeviceStatusRemoteWakeup, &isSet);
#endif
            /* Set or Clear the device remote wakeup featrue. */
            error = parent->callback(classHandle->handle, kUSB_DeviceEventSetRemoteWakeup, &isSet);
        }
#if((defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)) || \
    (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))) && \
    (defined(USB_DEVICE_CONFIG_USB20_TEST_MODE) && (USB_DEVICE_CONFIG_USB20_TEST_MODE > 0U))
        else if(USB_REQUEST_STANDARD_FEATURE_SELECTOR_DEVICE_TEST_MODE == setup->wValue)
        {
            state = kUSB_DeviceStateTestMode;
            error = USB_DeviceSetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);
        }
#endif
#if(defined(USB_DEVICE_CONFIG_OTG) && (USB_DEVICE_CONFIG_OTG))
        else if(USB_REQUEST_STANDARD_FEATURE_SELECTOR_B_HNP_ENABLE == setup->wValue)
        {
            error = parent->callback(classHandle->handle, kUSB_DeviceEventSetBHNPEnable, &isSet);
        }
#endif
        else
        {
        }
    }
    else if((setup->bmRequestType & USB_REQUEST_TYPE_RECIPIENT_MASK) == USB_REQUEST_TYPE_RECIPIENT_ENDPOINT)
    {
        /* Set or Clear the endpoint featrue. */
        if(USB_REQUEST_STANDARD_FEATURE_SELECTOR_ENDPOINT_HALT == setup->wValue)
        {
            if(USB_CONTROL_ENDPOINT == (setup->wIndex & USB_ENDPOINT_NUMBER_MASK))
            {
                /* Set or Clear the control endpoint status(halt or not). */
                if(isSet)
                {
                    USB_DeviceStallEndpoint(classHandle->handle, (uint8_t)setup->wIndex);
                }
                else
                {
                    USB_DeviceUnstallEndpoint(classHandle->handle, (uint8_t)setup->wIndex);
                }
            }

            /* Set or Clear the endpoint status featrue. */
            if(isSet)
            {
                error = parent->event(classHandle->handle, kUSB_DeviceClassEventSetEndpointHalt, &setup->wIndex);
            }
            else
            {
                error = parent->event(classHandle->handle, kUSB_DeviceClassEventClearEndpointHalt, &setup->wIndex);
            }
        }
        else
        {
        }
    }
    else
    {
    }
    return error;
};

int32_t Ch9::setAddress(usb_device_common_class_struct_t* classHandle,
                        usb_setup_struct_t* setup,
                        uint8_t** buffer,
                        uint32_t* length)
{
    int32_t error = kStatus_USB_InvalidRequest;
    uint8_t state;

    USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);

    if((kUSB_DeviceStateAddressing != state) && (kUSB_DeviceStateAddress != state) &&
        (kUSB_DeviceStateDefault != state) && (kUSB_DeviceStateConfigured != state))
    {
        return error;
    }

    if(kUSB_DeviceStateAddressing != state)
    {
        /* If the device address is not setting, pass the address and the device state will change to
         * kUSB_DeviceStateAddressing internally. */
        state = setup->wValue & 0xFFU;
        error = USB_DeviceSetStatus(classHandle->handle, kUSB_DeviceStatusAddress, &state);
    }
    else
    {
        /* If the device address is setting, set device address and the address will be write into the controller
         * internally. */
        error = USB_DeviceSetStatus(classHandle->handle, kUSB_DeviceStatusAddress, nullptr);
        /* And then change the device state to kUSB_DeviceStateAddress. */
        if(kStatus_USB_Success == error)
        {
            state = kUSB_DeviceStateAddress;
            error = USB_DeviceSetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);
        }
    }
    return error;
};

int32_t Ch9::getDescriptor(usb_device_common_class_struct_t* classHandle,
                           usb_setup_struct_t* setup,
                           uint8_t** buffer,
                           uint32_t* length)
{
    usb_device_get_descriptor_common_union_t commonDescriptor;
    int32_t error = kStatus_USB_InvalidRequest;
    uint8_t state;
    uint8_t descriptorType = (uint8_t)((setup->wValue & 0xFF00U) >> 8U);
    uint8_t descriptorIndex = (uint8_t)((setup->wValue & 0x00FFU));

    USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);

    if((kUSB_DeviceStateAddress != state) && (kUSB_DeviceStateConfigured != state) &&
        (kUSB_DeviceStateDefault != state))
    {
        return error;
    }
    commonDescriptor.commonDescriptor.length = setup->wLength;
    if(USB_DESCRIPTOR_TYPE_DEVICE == descriptorType)
    {
        /* Get the device descriptor */
        error = parent->callback(classHandle->handle,
                                 kUSB_DeviceEventGetDeviceDescriptor,
                                 &commonDescriptor.deviceDescriptor);
    }
    else if(USB_DESCRIPTOR_TYPE_CONFIGURE == descriptorType)
    {
        /* Get the configuration descriptor */
        commonDescriptor.configurationDescriptor.configuration = descriptorIndex;
        error = parent->callback(classHandle->handle,
                                 kUSB_DeviceEventGetConfigurationDescriptor,
                                 &commonDescriptor.configurationDescriptor);
    }
    else if(USB_DESCRIPTOR_TYPE_STRING == descriptorType)
    {
        /* Get the string descriptor */
        commonDescriptor.stringDescriptor.stringIndex = descriptorIndex;
        commonDescriptor.stringDescriptor.languageId = setup->wIndex;
        error = parent->callback(classHandle->handle,
                                 kUSB_DeviceEventGetStringDescriptor,
                                 &commonDescriptor.stringDescriptor);
    }
#if(defined(USB_DEVICE_CONFIG_HID) && (USB_DEVICE_CONFIG_HID > 0U))
    else if(USB_DESCRIPTOR_TYPE_HID == descriptorType)
    {
        /* Get the hid descriptor */
        commonDescriptor.hidDescriptor.interfaceNumber = setup->wIndex;
        error = parent->callback(classHandle->handle,
                                 kUSB_DeviceEventGetHidDescriptor,
                                 &commonDescriptor.hidDescriptor);
    }
    else if(USB_DESCRIPTOR_TYPE_HID_REPORT == descriptorType)
    {
        /* Get the hid report descriptor */
        commonDescriptor.hidReportDescriptor.interfaceNumber = setup->wIndex;
        error = parent->callback(classHandle->handle,
                                 kUSB_DeviceEventGetHidReportDescriptor,
                                 &commonDescriptor.hidReportDescriptor);
    }
    else if(USB_DESCRIPTOR_TYPE_HID_PHYSICAL == descriptorType)
    {
        /* Get the hid physical descriptor */
        commonDescriptor.hidPhysicalDescriptor.index = descriptorIndex;
        commonDescriptor.hidPhysicalDescriptor.interfaceNumber = setup->wIndex;
        error = parent->callback(classHandle->handle,
                                 kUSB_DeviceEventGetHidPhysicalDescriptor,
                                 &commonDescriptor.hidPhysicalDescriptor);
    }
#endif
#if(defined(USB_DEVICE_CONFIG_USB20_TEST_MODE) && (USB_DEVICE_CONFIG_USB20_TEST_MODE > 0U))
    else if(USB_DESCRIPTOR_TYPE_DEVICE_QUALITIER == descriptorType)
    {
        /* Get the device descriptor */
        error = parent->callback(classHandle->handle,
                                 kUSB_DeviceEventGetDeviceQualifierDescriptor,
                                 &commonDescriptor.deviceDescriptor);
    }
#endif
#if(defined(USB_DEVICE_CONFIG_LPM_L1) && (USB_DEVICE_CONFIG_LPM_L1 > 0U))
    else if(USB_DESCRIPTOR_TYPE_BOS == descriptorType)
    {
        /* Get the configuration descriptor */
        commonDescriptor.configurationDescriptor.configuration = descriptorIndex;
        error = parent->callback(classHandle->handle,
                                 kUSB_DeviceEventGetBOSDescriptor,
                                 &commonDescriptor.configurationDescriptor);
    }
#endif
    else
    {
    }
    *buffer = commonDescriptor.commonDescriptor.buffer;
    *length = commonDescriptor.commonDescriptor.length;
    return error;
};

int32_t Ch9::getConfiguration(usb_device_common_class_struct_t* classHandle,
                              usb_setup_struct_t* setup,
                              uint8_t** buffer,
                              uint32_t* length)
{
    uint8_t state;

    USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);

    if((kUSB_DeviceStateAddress != state) && ((kUSB_DeviceStateConfigured != state)))
    {
        return kStatus_USB_InvalidRequest;
    }

    *length = USB_CONFIGURE_SIZE;
    *buffer = (uint8_t*)&classHandle->standardTranscationBuffer;
    return parent->callback(classHandle->handle, kUSB_DeviceEventGetConfiguration,
                                   &classHandle->standardTranscationBuffer);
};

int32_t Ch9::setConfiguration(usb_device_common_class_struct_t* classHandle,
                              usb_setup_struct_t* setup,
                              uint8_t** buffer,
                              uint32_t* length)
{
    uint8_t state;

    USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);

    if((kUSB_DeviceStateAddress != state) && (kUSB_DeviceStateConfigured != state))
    {
        return kStatus_USB_InvalidRequest;
    }

    /* The device state is changed to kUSB_DeviceStateConfigured */
    state = kUSB_DeviceStateConfigured;
    USB_DeviceSetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);
    if(!setup->wValue)
    {
        /* If the new configuration is zero, the device state is changed to kUSB_DeviceStateAddress */
        state = kUSB_DeviceStateAddress;
        USB_DeviceSetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);
    }

    /* Notify the class layer the configuration is changed */
    parent->event(classHandle->handle, kUSB_DeviceClassEventSetConfiguration, &setup->wValue);
    /* Notify the application the configuration is changed */
    return parent->callback(classHandle->handle, kUSB_DeviceEventSetConfiguration, &setup->wValue);
};

int32_t Ch9::getInterface(usb_device_common_class_struct_t* classHandle,
                          usb_setup_struct_t* setup,
                          uint8_t** buffer,
                          uint32_t* length)
{
    usb_status_t error = kStatus_USB_InvalidRequest;
    uint8_t state;

    USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);

    if(state != kUSB_DeviceStateConfigured)
    {
        return error;
    }
    *length = USB_INTERFACE_SIZE;
    *buffer = (uint8_t*)&classHandle->standardTranscationBuffer;
    classHandle->standardTranscationBuffer = setup->wIndex & 0xFFU;
    /* The Bit[15~8] is used to save the interface index, and the alternate setting will be saved in Bit[7~0] by
     * application. */
    return parent->callback(classHandle->handle,
                            kUSB_DeviceEventGetInterface,
                            &classHandle->standardTranscationBuffer);
};

int32_t Ch9::setInterface(usb_device_common_class_struct_t* classHandle,
                          usb_setup_struct_t* setup,
                          uint8_t** buffer,
                          uint32_t* length)
{
    uint8_t state;

    USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);

    if(state != kUSB_DeviceStateConfigured)
    {
        return kStatus_USB_InvalidRequest;
    }
    classHandle->standardTranscationBuffer = ((setup->wIndex & 0xFFU) << 8U) | (setup->wValue & 0xFFU);
    /* Notify the class driver the alternate setting of the interface is changed. */
    /* The Bit[15~8] is used to save the interface index, and the alternate setting is saved in Bit[7~0]. */
    parent->event(classHandle->handle,
                  kUSB_DeviceClassEventSetInterface,
                  &classHandle->standardTranscationBuffer);
    /* Notify the application the alternate setting of the interface is changed. */
    /* The Bit[15~8] is used to save the interface index, and the alternate setting will is saved in Bit[7~0]. */
    return parent->callback(classHandle->handle,
                            kUSB_DeviceEventSetInterface,
                            &classHandle->standardTranscationBuffer);
};

int32_t Ch9::synchFrame(usb_device_common_class_struct_t* classHandle,
                        usb_setup_struct_t* setup,
                        uint8_t** buffer,
                        uint32_t* length)
{
    int32_t error = kStatus_USB_InvalidRequest;
    uint8_t state;

    USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusDeviceState, &state);

    if(state != kUSB_DeviceStateConfigured)
    {
        return error;
    }

    classHandle->standardTranscationBuffer = setup->wIndex;
    /* Get the sync frame value */
    error = USB_DeviceGetStatus(classHandle->handle,
                                kUSB_DeviceStatusSynchFrame,
                                &classHandle->standardTranscationBuffer);
    *buffer = (uint8_t*)&classHandle->standardTranscationBuffer;
    *length = sizeof(classHandle->standardTranscationBuffer);
    return error;
};

}

extern "C"
{

usb_status_t USB_DeviceControlCallback(usb_device_handle handle,
                                       usb_device_endpoint_callback_message_struct_t* message,
                                       void* callbackParam)
{
    return static_cast<usb_status_t>(gCh9->controlCallback(handle, message, callbackParam));
}

}
