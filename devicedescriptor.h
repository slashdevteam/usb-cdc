#pragma once

#include "usbtypes.h"

#include <cstdint>
#include "usb.h"

#define USB_DEVICE_SPECIFIC_BCD_VERSION (0x0200)
#define USB_DEVICE_DEMO_BCD_VERSION (0x0101U)

#define CDC_COMM_CLASS (0x02)
#define CDC_DATA_CLASS (0x0A)

#define USB_CDC_DIRECT_LINE_CONTROL_MODEL (0x01)
#define USB_CDC_ABSTRACT_CONTROL_MODEL (0x02)
#define USB_CDC_TELEPHONE_CONTROL_MODEL (0x03)
#define USB_CDC_MULTI_CHANNEL_CONTROL_MODEL (0x04)
#define USB_CDC_CAPI_CONTROL_MOPDEL (0x05)
#define USB_CDC_ETHERNET_NETWORKING_CONTROL_MODEL (0x06)
#define USB_CDC_ATM_NETWORKING_CONTROL_MODEL (0x07)
#define USB_CDC_WIRELESS_HANDSET_CONTROL_MODEL (0x08)
#define USB_CDC_DEVICE_MANAGEMENT (0x09)
#define USB_CDC_MOBILE_DIRECT_LINE_MODEL (0x0A)
#define USB_CDC_OBEX (0x0B)
#define USB_CDC_ETHERNET_EMULATION_MODEL (0x0C)

#define USB_CDC_NO_CLASS_SPECIFIC_PROTOCOL (0x00)
#define USB_CDC_AT_250_PROTOCOL (0x01)
#define USB_CDC_AT_PCCA_101_PROTOCOL (0x02)
#define USB_CDC_AT_PCCA_101_ANNEX_O (0x03)
#define USB_CDC_AT_GSM_7_07 (0x04)
#define USB_CDC_AT_3GPP_27_007 (0x05)
#define USB_CDC_AT_TIA_CDMA (0x06)
#define USB_CDC_ETHERNET_EMULATION_PROTOCOL (0x07)
#define USB_CDC_EXTERNAL_PROTOCOL (0xFE)
#define USB_CDC_VENDOR_SPECIFIC (0xFF)

#define USB_CDC_PYHSICAL_INTERFACE_PROTOCOL (0x30)
#define USB_CDC_HDLC_PROTOCOL (0x31)
#define USB_CDC_TRANSPARENT_PROTOCOL (0x32)
#define USB_CDC_MANAGEMENT_PROTOCOL (0x50)
#define USB_CDC_DATA_LINK_Q931_PROTOCOL (0x51)
#define USB_CDC_DATA_LINK_Q921_PROTOCOL (0x52)
#define USB_CDC_DATA_COMPRESSION_V42BIS (0x90)
#define USB_CDC_EURO_ISDN_PROTOCOL (0x91)
#define USB_CDC_RATE_ADAPTION_ISDN_V24 (0x92)
#define USB_CDC_CAPI_COMMANDS (0x93)
#define USB_CDC_HOST_BASED_DRIVER (0xFD)
#define USB_CDC_UNIT_FUNCTIONAL (0xFE)

#define USB_CDC_HEADER_FUNC_DESC (0x00)
#define USB_CDC_CALL_MANAGEMENT_FUNC_DESC (0x01)
#define USB_CDC_ABSTRACT_CONTROL_FUNC_DESC (0x02)
#define USB_CDC_DIRECT_LINE_FUNC_DESC (0x03)
#define USB_CDC_TELEPHONE_RINGER_FUNC_DESC (0x04)
#define USB_CDC_TELEPHONE_REPORT_FUNC_DESC (0x05)
#define USB_CDC_UNION_FUNC_DESC (0x06)
#define USB_CDC_COUNTRY_SELECT_FUNC_DESC (0x07)
#define USB_CDC_TELEPHONE_MODES_FUNC_DESC (0x08)
#define USB_CDC_TERMINAL_FUNC_DESC (0x09)
#define USB_CDC_NETWORK_CHANNEL_FUNC_DESC (0x0A)
#define USB_CDC_PROTOCOL_UNIT_FUNC_DESC (0x0B)
#define USB_CDC_EXTENSION_UNIT_FUNC_DESC (0x0C)
#define USB_CDC_MULTI_CHANNEL_FUNC_DESC (0x0D)
#define USB_CDC_CAPI_CONTROL_FUNC_DESC (0x0E)
#define USB_CDC_ETHERNET_NETWORKING_FUNC_DESC (0x0F)
#define USB_CDC_ATM_NETWORKING_FUNC_DESC (0x10)
#define USB_CDC_WIRELESS_CONTROL_FUNC_DESC (0x11)
#define USB_CDC_MOBILE_DIRECT_LINE_FUNC_DESC (0x12)
#define USB_CDC_MDLM_DETAIL_FUNC_DESC (0x13)
#define USB_CDC_DEVICE_MANAGEMENT_FUNC_DESC (0x14)
#define USB_CDC_OBEX_FUNC_DESC (0x15)
#define USB_CDC_COMMAND_SET_FUNC_DESC (0x16)
#define USB_CDC_COMMAND_SET_DETAIL_FUNC_DESC (0x17)
#define USB_CDC_TELEPHONE_CONTROL_FUNC_DESC (0x18)
#define USB_CDC_OBEX_SERVICE_ID_FUNC_DESC (0x19)

#define USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL (67)
#define USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC (5)
#define USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG (5)
#define USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT (4)
#define USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC (5)


#define USB_DEVICE_STRING_COUNT (3)
#define USB_DEVICE_LANGUAGE_COUNT (1)
#define USB_CDC_VCOM_CONFIGURE_INDEX (1)

#define HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE (16)
#define FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE (16)
#define HS_CDC_VCOM_INTERRUPT_IN_INTERVAL (0x07)
#define FS_CDC_VCOM_INTERRUPT_IN_INTERVAL (0x08)


#define USB_DESCRIPTOR_LENGTH_STRING0 (4)
#define USB_DESCRIPTOR_LENGTH_STRING1 (36)
#define USB_DESCRIPTOR_LENGTH_STRING2 (42)

#define USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE (0x24)
#define USB_DESCRIPTOR_TYPE_CDC_CS_ENDPOINT (0x25)

#define USB_DEVICE_CLASS (0x02)
#define USB_DEVICE_SUBCLASS (0x00)
#define USB_DEVICE_PROTOCOL (0x00)

#define USB_DEVICE_MAX_POWER (0x32)

#define USB_CDC_VCOM_ENDPOINT_CIC_COUNT (1)
#define USB_CDC_VCOM_ENDPOINT_DIC_COUNT (2)

struct usb_device_get_device_descriptor_struct_t
{
    uint8_t* buffer;
    uint32_t length;
};

struct usb_device_get_string_descriptor_struct_t
{
    uint8_t* buffer;
    uint32_t length;
    uint16_t languageId;
    uint8_t stringIndex;
};

struct usb_device_get_configuration_descriptor_struct_t
{
    uint8_t* buffer;
    uint32_t length;
    uint8_t configuration;
};

struct device_specific_descriptors
{
    uint8_t deviceDescriptor[USB_DESCRIPTOR_LENGTH_DEVICE];
    uint8_t configurationDescriptor[USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL];
    uint8_t languageString[USB_DESCRIPTOR_LENGTH_STRING0];
    uint8_t manufacturerString[USB_DESCRIPTOR_LENGTH_STRING1];
    uint8_t productString[USB_DESCRIPTOR_LENGTH_STRING2];
    uint8_t* stringDescriptorArray[USB_DEVICE_STRING_COUNT];
    uint32_t stringDescriptorLength[USB_DEVICE_STRING_COUNT];
    usb_language_t deviceLanguage[USB_DEVICE_LANGUAGE_COUNT];
};

namespace usb
{

class DeviceClass;

class DeviceDescriptor
{
public:
    DeviceDescriptor(DeviceClass* _parent, device_specific_descriptors& _descriptors);
    int32_t setSpeed(usb_device_handle handle, uint8_t speed);
    int32_t getDeviceDescriptor(usb_device_handle handle,
                                usb_device_get_device_descriptor_struct_t* deviceDescriptor);
    int32_t getStringDescriptor(usb_device_handle handle,
                                usb_device_get_string_descriptor_struct_t* stringDescriptor);
    int32_t getConfigurationDescriptor(usb_device_handle handle,
                                       usb_device_get_configuration_descriptor_struct_t* configurationDescriptor);

private:
    DeviceClass* parent;
    device_specific_descriptors& deviceDescriptors;
    usb_language_list_t languageList;
};

}
