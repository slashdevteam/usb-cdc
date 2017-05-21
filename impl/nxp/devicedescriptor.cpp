#include "devicedescriptor.h"
#include "usb_device_config.h"
#include "usb_device.h"
#include "deviceclass.h"

namespace usb
{
DeviceDescriptor::DeviceDescriptor(DeviceClass* _parent, device_specific_descriptors& _descriptors)
    : parent(_parent),
    deviceDescriptors(_descriptors),
    languageList{deviceDescriptors.languageString,
                 sizeof(deviceDescriptors.languageString),
                 deviceDescriptors.deviceLanguage,
                 USB_DEVICE_LANGUAGE_COUNT}
{
}

int32_t DeviceDescriptor::getDeviceDescriptor(usb_device_handle handle,
                                             usb_device_get_device_descriptor_struct_t* _deviceDescriptor)
{
    _deviceDescriptor->buffer = deviceDescriptors.deviceDescriptor;
    _deviceDescriptor->length = USB_DESCRIPTOR_LENGTH_DEVICE;
    return kStatus_USB_Success;
}

int32_t DeviceDescriptor::getConfigurationDescriptor(usb_device_handle handle,
                                                     usb_device_get_configuration_descriptor_struct_t* _configurationDescriptor)
{
    if (USB_CDC_VCOM_CONFIGURE_INDEX > _configurationDescriptor->configuration)
    {
        _configurationDescriptor->buffer = deviceDescriptors.configurationDescriptor;
        _configurationDescriptor->length = USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL;
        return kStatus_USB_Success;
    }
    return kStatus_USB_InvalidRequest;
}

int32_t DeviceDescriptor::getStringDescriptor(usb_device_handle handle,
                                              usb_device_get_string_descriptor_struct_t* stringDescriptor)
{
    if (stringDescriptor->stringIndex == 0U)
    {
        stringDescriptor->buffer = (uint8_t*)languageList.languageString;
        stringDescriptor->length = languageList.stringLength;
    }
    else
    {
        uint8_t languageId = 0U;
        uint8_t languageIndex = USB_DEVICE_STRING_COUNT;

        for (; languageId < USB_DEVICE_LANGUAGE_COUNT; languageId++)
        {
            if (stringDescriptor->languageId == languageList.languageList[languageId].languageId)
            {
                if (stringDescriptor->stringIndex < USB_DEVICE_STRING_COUNT)
                {
                    languageIndex = stringDescriptor->stringIndex;
                }
                break;
            }
        }

        if (USB_DEVICE_STRING_COUNT == languageIndex)
        {
            return kStatus_USB_InvalidRequest;
        }
        stringDescriptor->buffer = (uint8_t*)languageList.languageList[languageId].string[languageIndex];
        stringDescriptor->length = languageList.languageList[languageId].length[languageIndex];
    }
    return kStatus_USB_Success;
}

int32_t DeviceDescriptor::setSpeed(usb_device_handle handle, uint8_t speed)
{
    usb_descriptor_union_t* ptr1;
    usb_descriptor_union_t* ptr2;

    ptr1 = (usb_descriptor_union_t*)(&deviceDescriptors.configurationDescriptor[0]);
    ptr2 = (usb_descriptor_union_t*)(&deviceDescriptors.configurationDescriptor[USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL - 1]);

    while (ptr1 < ptr2)
    {
        if (ptr1->common.bDescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT)
        {
            if (USB_SPEED_HIGH == speed)
            {
                if (USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT == (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK))
                {
                    ptr1->endpoint.bInterval = HS_CDC_VCOM_INTERRUPT_IN_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE,
                                                       ptr1->endpoint.wMaxPacketSize);
                }
                else if (USB_CDC_VCOM_BULK_IN_ENDPOINT == (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK))
                {
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_CDC_VCOM_BULK_IN_PACKET_SIZE, ptr1->endpoint.wMaxPacketSize);
                }
                else if (USB_CDC_VCOM_BULK_OUT_ENDPOINT == (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK))
                {
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_CDC_VCOM_BULK_OUT_PACKET_SIZE, ptr1->endpoint.wMaxPacketSize);
                }
                else
                {
                }
            }
            else
            {
                if (USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT == (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK))
                {
                    ptr1->endpoint.bInterval = FS_CDC_VCOM_INTERRUPT_IN_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE,
                                                       ptr1->endpoint.wMaxPacketSize);
                }
                else if (USB_CDC_VCOM_BULK_IN_ENDPOINT == (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK))
                {
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_CDC_VCOM_BULK_IN_PACKET_SIZE, ptr1->endpoint.wMaxPacketSize);
                }
                else if (USB_CDC_VCOM_BULK_OUT_ENDPOINT == (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK))
                {
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_CDC_VCOM_BULK_OUT_PACKET_SIZE, ptr1->endpoint.wMaxPacketSize);
                }
                else
                {
                }
            }
        }
        ptr1 = (usb_descriptor_union_t*)((uint8_t*)ptr1 + ptr1->common.bLength);
    }

    parent->setSpeed(speed);

    return kStatus_USB_Success;
}

}
