#include "deviceclass.h"

#include "mbed.h"
#include "usb.h"
#include "usb_device_config.h"
#include "usb_device.h"
#include "MK24F12.h"
#include "usb_device_dci.h"
// includes above just for usb_device_khci!!!
#include "usb_device_khci.h"
#include "fsl_clock.h"

#define USB_DEVICE_INTERRUPT_PRIORITY (3U)

extern "C"
{

extern void USB_DeviceKhciIsrFunction(void* deviceHandle);
void USBIRQHandler();
usb_status_t USB_DeviceClassCallback(usb_device_handle handle, uint32_t event, void* param);
USB_GLOBAL void* gHandle;
USB_GLOBAL usb::DeviceClass* gDeviceClass;

// needed by NXP USB stack
int DbgConsole_Printf(const char *fmt_s, ...)
{
    return 0;
}

}

namespace usb
{

DeviceClass::DeviceClass(uint8_t _controllerId,
                         device_specific_descriptors& _deviceSpecificDescriptor)
    : ch9(this),
      controllerId(_controllerId),
      descriptor(this, _deviceSpecificDescriptor)
{
    // With MPU enabled NXP stack is not receiving TOKDNE interrupts
    SYSMPU->CESR &= ~SYSMPU_CESR_VLD_MASK;

    usb_device_common_class_struct_t *classHandle;

    /* Allocate a common class driver handle. */
    commonClassStruct.controllerId = controllerId;
    classHandle = &commonClassStruct;

    NVIC_SetVector(USB0_IRQn, reinterpret_cast<uint32_t>(&USBIRQHandler));
    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, 48000000U);
    /* Initialize the device stack. */
    USB_DeviceInit(controllerId, USB_DeviceClassCallback, &classHandle->handle);

    cdcAcm = std::make_unique<CdcAcm>(controllerId, classHandle->handle, &cdcAcmHandle);

    gHandle = classHandle->handle;
    gDeviceClass = this;

    NVIC_SetPriority((IRQn_Type)USB0_IRQn, USB_DEVICE_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(USB0_IRQn);
}

DeviceClass::~DeviceClass()
{
    usb_device_common_class_struct_t *classHandle;
    /* Get the common class handle according to the controller id. */
    getHandleByControllerId(controllerId, &classHandle);
    cdcAcm.reset(nullptr);
    /* De-initialize the USB device stack. */
    USB_DeviceDeinit(classHandle->handle);
}

void DeviceClass::run()
{
    USB_DeviceRun(gHandle);
}

int32_t DeviceClass::getHandleByControllerId(uint8_t controllerId,
                                             usb_device_common_class_struct_t **handle)
{
    USB_OSA_SR_ALLOC();

    USB_OSA_ENTER_CRITICAL();
    if(controllerId == commonClassStruct.controllerId)
    {
        *handle = &commonClassStruct;
        USB_OSA_EXIT_CRITICAL();
        return kStatus_USB_Success;
    }
    USB_OSA_EXIT_CRITICAL();
    return kStatus_USB_InvalidParameter;
}

int32_t DeviceClass::getHandleByDeviceHandle(usb_device_handle deviceHandle,
                                             usb_device_common_class_struct_t **handle)
{
    USB_OSA_SR_ALLOC();

    USB_OSA_ENTER_CRITICAL();
    if (deviceHandle == commonClassStruct.handle)
    {
        *handle = &commonClassStruct;
        USB_OSA_EXIT_CRITICAL();
        return kStatus_USB_Success;
    }
    USB_OSA_EXIT_CRITICAL();
    return kStatus_USB_InvalidParameter;
}

int32_t DeviceClass::getDeviceHandle(uint8_t controllerId, usb_device_handle *handle)
{
    USB_OSA_SR_ALLOC();

    USB_OSA_ENTER_CRITICAL();
    if(controllerId == commonClassStruct.controllerId)
    {
        *handle = &commonClassStruct.handle;
        USB_OSA_EXIT_CRITICAL();
        return kStatus_USB_Success;
    }
    USB_OSA_EXIT_CRITICAL();
    return kStatus_USB_InvalidParameter;
}

int32_t DeviceClass::event(usb_device_handle handle, usb_device_class_event_t _event, void* param)
{
    usb_device_common_class_struct_t *classHandle;

    int32_t errorReturn = kStatus_USB_Error;
    int32_t error = kStatus_USB_Error;

    if (NULL == param)
    {
        return kStatus_USB_InvalidParameter;
    }

    /* Get the common class handle according to the device handle. */
    errorReturn = getHandleByDeviceHandle(handle, &classHandle);
    if (kStatus_USB_Success != errorReturn)
    {
        return kStatus_USB_InvalidParameter;
    }

    error = cdcAcm->event((void* )cdcAcmHandle, _event, param);

    return error;
}

int32_t DeviceClass::callback(usb_device_handle handle, uint32_t _event, void* param)
{
    usb_device_common_class_struct_t *classHandle;
    int32_t error = kStatus_USB_Error;

    /* Get the common class handle according to the device handle. */
    error = getHandleByDeviceHandle(handle, &classHandle);
    if (kStatus_USB_Success != error)
    {
        return error;
    }

    if (kUSB_DeviceEventBusReset == _event)
    {
        /* Initialize the control pipes */
        ch9.deviceControlPipeInit(handle, classHandle);

        /* Notify the classes the USB bus reset signal detected. */
        event(handle, kUSB_DeviceClassEventDeviceReset, classHandle);
    }

    /* Call the application device callback function. */
    error = deviceCallback(handle, _event, param);
    return error;
}

int32_t DeviceClass::deviceCallback(usb_device_handle handle, uint32_t _event, void* param)
{
    int32_t error = kStatus_USB_Error;
    switch (_event)
    {
        case kUSB_DeviceEventBusReset:
        {
            cdcAcm->deviceCallback(0, _event, param);
        }
        break;
        case kUSB_DeviceEventSetConfiguration:
            if (param)
            {
                cdcAcm->deviceCallback(0, _event, param);
            }
            break;
        case kUSB_DeviceEventSetInterface:
            cdcAcm->deviceCallback(0, _event, param);
            break;
        case kUSB_DeviceEventGetConfiguration:
            break;
        case kUSB_DeviceEventGetInterface:
            break;
        case kUSB_DeviceEventGetDeviceDescriptor:
            if (param)
            {
                error = descriptor.getDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetConfigurationDescriptor:
            if (param)
            {
                error = descriptor.getConfigurationDescriptor(handle,
                                                             (usb_device_get_configuration_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetStringDescriptor:
            if (param)
            {
                /* Get device string descriptor request */
                error = descriptor.getStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
            }
            break;
        default:
            break;
    }
    return error;
}

int32_t DeviceClass::getSpeed(uint8_t controllerId, uint8_t *speed)
{
    usb_device_common_class_struct_t *classHandle;
    int32_t error = kStatus_USB_Error;

    /* Get the common class handle according to the controller id. */
    error = getHandleByControllerId(controllerId, &classHandle);

    if (kStatus_USB_Success != error)
    {
        return error;
    }

    /* Get the current speed. */
    error = USB_DeviceGetStatus(classHandle->handle, kUSB_DeviceStatusSpeed, speed);

    return error;
}

int32_t DeviceClass::setSpeed(uint8_t speed)
{
    return cdcAcm->setSpeed(speed);
}

int32_t DeviceClass::send(uint8_t* buffer, uint32_t length)
{
    return cdcAcm->send(buffer, length);
}

int32_t DeviceClass::recv(uint8_t* buffer, uint32_t length)
{
    return cdcAcm->recv(buffer, length);
}

void DeviceClass::echo()
{
    cdcAcm->echo();
}

}

extern "C"
{

void USBIRQHandler()
{
    USB_DeviceKhciIsrFunction(gHandle);
}

usb_status_t USB_DeviceClassCallback(usb_device_handle handle, uint32_t event, void* param)
{
    return static_cast<usb_status_t>(gDeviceClass->callback(handle, event, param));
}

}
