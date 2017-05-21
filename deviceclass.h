#pragma once

#include "cdcacm.h"
#include "ch9.h"
#include <memory> // unique_ptr
#include "devicedescriptor.h"

struct usb_device_get_descriptor_common_struct_t
{
    uint8_t *buffer;
    uint32_t length;
};

struct usb_device_get_device_qualifier_descriptor_struct_t
{
    uint8_t *buffer;
    uint32_t length;
};

struct usb_device_get_bos_descriptor_struct_t
{
    uint8_t *buffer;
    uint32_t length;
};

struct usb_device_get_hid_descriptor_struct_t
{
    uint8_t *buffer;
    uint32_t length;
    uint8_t interfaceNumber;
};

struct usb_device_get_hid_report_descriptor_struct_t
{
    uint8_t *buffer;
    uint32_t length;
    uint8_t interfaceNumber;
};

struct usb_device_get_hid_physical_descriptor_struct_t
{
    uint8_t *buffer;
    uint32_t length;
    uint8_t index;
    uint8_t interfaceNumber;
};

union usb_device_get_descriptor_common_union_t
{
    usb_device_get_descriptor_common_struct_t commonDescriptor;
    usb_device_get_device_descriptor_struct_t deviceDescriptor;
    usb_device_get_device_qualifier_descriptor_struct_t deviceQualifierDescriptor;
    usb_device_get_configuration_descriptor_struct_t configurationDescriptor;
    usb_device_get_string_descriptor_struct_t stringDescriptor;
    usb_device_get_hid_descriptor_struct_t hidDescriptor;
    usb_device_get_hid_report_descriptor_struct_t hidReportDescriptor;
    usb_device_get_hid_physical_descriptor_struct_t hidPhysicalDescriptor;
};

using usb_device_class_init_call_t = usb_status_t(*)(uint8_t controllerId,
                                                     usb_device_class_config_struct_t *classConfig,
                                                     class_handle_t *classHandle);
using usb_device_class_deinit_call_t = usb_status_t(*)(class_handle_t handle);
using usb_device_class_event_callback_t = usb_status_t(*)(void* classHandle, uint32_t event, void* param);

struct usb_device_class_map_t
{
    usb_device_class_init_call_t classInit;
    usb_device_class_deinit_call_t classDeinit;
    usb_device_class_event_callback_t classEventCallback;
    usb_device_class_type_t type;
};



namespace usb
{

class DeviceClass
{
public:
    DeviceClass(uint8_t _controllerId,
                device_specific_descriptors& _deviceSpecificDescriptor);
    ~DeviceClass();
    void run();

    int32_t getSpeed(uint8_t controllerId, uint8_t *speed);
    int32_t callback(usb_device_handle handle, uint32_t _event, void* param);
    int32_t event(usb_device_handle handle, usb_device_class_event_t _event, void* param);
    int32_t getDeviceHandle(uint8_t controllerId, usb_device_handle *handle);
    int32_t getHandleByDeviceHandle(usb_device_handle deviceHandle,
                                    usb_device_common_class_struct_t** handle);
    int32_t getHandleByControllerId(uint8_t controllerId,
                                    usb_device_common_class_struct_t** handle);

    int32_t deviceCallback(usb_device_handle handle, uint32_t _event, void* param);
    int32_t setSpeed(uint8_t speed);

    int32_t send(uint8_t* buffer, uint32_t length);
    int32_t recv(uint8_t* buffer, uint32_t length);

    void echo();

private:
    usb_device_common_class_struct_t commonClassStruct;
    std::unique_ptr<CdcAcm> cdcAcm;
    class_handle_t cdcAcmHandle;
    Ch9 ch9;
    uint8_t controllerId;
    DeviceDescriptor descriptor;
};

}
