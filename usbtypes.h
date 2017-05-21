#pragma once

#include <cstdint>
#include "usb.h"
#include "usb_device.h"

#define USB_CDC_VCOM_INTERFACE_COUNT (2)
#define USB_DEVICE_CONFIGURATION_COUNT (1)
#define USB_CDC_VCOM_CIC_CLASS (CDC_COMM_CLASS)
#define USB_CDC_VCOM_CIC_SUBCLASS (USB_CDC_ABSTRACT_CONTROL_MODEL)
#define USB_CDC_VCOM_CIC_PROTOCOL (USB_CDC_NO_CLASS_SPECIFIC_PROTOCOL)

#define USB_CDC_VCOM_DIC_CLASS (CDC_DATA_CLASS)
#define USB_CDC_VCOM_DIC_SUBCLASS (0x00)
#define USB_CDC_VCOM_DIC_PROTOCOL (USB_CDC_NO_CLASS_SPECIFIC_PROTOCOL)
#define USB_CDC_VCOM_COMM_INTERFACE_INDEX (0)
#define USB_CDC_VCOM_DATA_INTERFACE_INDEX (1)

#define USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT (1)
#define USB_CDC_VCOM_BULK_IN_ENDPOINT (2)
#define USB_CDC_VCOM_BULK_OUT_ENDPOINT (3)
#define HS_CDC_VCOM_BULK_IN_PACKET_SIZE (512)
#define FS_CDC_VCOM_BULK_IN_PACKET_SIZE (64)
#define HS_CDC_VCOM_BULK_OUT_PACKET_SIZE (512)
#define FS_CDC_VCOM_BULK_OUT_PACKET_SIZE (64)

#define class_handle_t uint32_t

using usb_device_class_callback_t = usb_status_t(*)(class_handle_t classHandle,
                                                    uint32_t callbackEvent,
                                                    void *eventParam);

struct usb_device_endpoint_struct_t
{
    uint8_t endpointAddress;
    uint8_t transferType;
    uint16_t maxPacketSize;
};

struct usb_device_endpoint_list_t
{
    uint8_t count;
    usb_device_endpoint_struct_t *endpoint;
};

struct usb_device_interface_struct_t
{
    uint8_t alternateSetting;
    usb_device_endpoint_list_t endpointList;
    void *classSpecific;
};

struct usb_device_interfaces_struct_t
{
    uint8_t classCode;
    uint8_t subclassCode;
    uint8_t protocolCode;
    uint8_t interfaceNumber;
    usb_device_interface_struct_t *interface;
    uint8_t count;
};

struct usb_device_interface_list_t
{
    uint8_t count;
    usb_device_interfaces_struct_t *interfaces;
};

enum usb_device_class_type_t
{
    kUSB_DeviceClassTypeHid = 1U,
    kUSB_DeviceClassTypeCdc,
    kUSB_DeviceClassTypeMsc,
    kUSB_DeviceClassTypeAudio,
    kUSB_DeviceClassTypePhdc,
    kUSB_DeviceClassTypeVideo,
    kUSB_DeviceClassTypePrinter,
    kUSB_DeviceClassTypeDfu,
    kUSB_DeviceClassTypeCcid,
};

enum usb_device_class_event_t
{
    kUSB_DeviceClassEventClassRequest = 1U,
    kUSB_DeviceClassEventDeviceReset,
    kUSB_DeviceClassEventSetConfiguration,
    kUSB_DeviceClassEventSetInterface,
    kUSB_DeviceClassEventSetEndpointHalt,
    kUSB_DeviceClassEventClearEndpointHalt,
};


struct usb_device_class_struct_t
{
    usb_device_interface_list_t *interfaceList;
    usb_device_class_type_t type;
    uint8_t configurations;
};

struct usb_device_class_config_struct_t
{
    usb_device_class_callback_t classCallback;
    class_handle_t classHandle;
    usb_device_class_struct_t *classInfomation;
};

struct usb_device_class_config_list_struct_t
{
    usb_device_class_config_struct_t *config;
    usb_device_callback_t deviceCallback;
    uint8_t count;
};


struct usb_device_control_request_struct_t
{
    usb_setup_struct_t *setup;
    uint8_t *buffer;
    uint32_t length;
    uint8_t isSetup;
};
