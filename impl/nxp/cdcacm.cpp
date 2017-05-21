#include "cdcacm.h"

#include "usb_device.h"
#include "usb_spec.h"

#define USB_CDC_ACM_ENTER_CRITICAL() \
    \
USB_OSA_SR_ALLOC();                  \
    \
USB_OSA_ENTER_CRITICAL()

#define USB_CDC_ACM_EXIT_CRITICAL() USB_OSA_EXIT_CRITICAL()

extern "C"
{

usb_status_t CdcAcmInterruptIn(usb_device_handle handle,
                               usb_device_endpoint_callback_message_struct_t* message,
                               void* callbackParam);
usb_status_t CdcAcmBulkOut(usb_device_handle handle,
                           usb_device_endpoint_callback_message_struct_t* message,
                           void* callbackParam);
usb_status_t CdcAcmBulkIn(usb_device_handle handle,
                          usb_device_endpoint_callback_message_struct_t* message,
                          void* callbackParam);
}

namespace usb
{

CdcAcm::CdcAcm(uint8_t controllerId,
               usb_device_handle deviceHandle,
               class_handle_t* handle)
    : usbCdcAcmInfo{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0},
      recvSize(0),
      sendSize(0),
      countryCode{(COUNTRY_SETTING >> 0U) & 0x00FFU,
                  (COUNTRY_SETTING >> 8U) & 0x00FFU},
      abstractState{(STATUS_ABSTRACT_STATE >> 0U) & 0x00FFU,
                    (STATUS_ABSTRACT_STATE >> 8U) & 0x00FFU},
      lineCoding{/* E.g. 0x00,0xC2,0x01,0x00 : 0x0001C200 is 115200 bits per second */
                (LINE_CODING_DTERATE >> 0U) & 0x000000FFU,
                (LINE_CODING_DTERATE >> 8U) & 0x000000FFU,
                (LINE_CODING_DTERATE >> 16U) & 0x000000FFU,
                (LINE_CODING_DTERATE >> 24U) & 0x000000FFU,
                LINE_CODING_CHARFORMAT,
                LINE_CODING_PARITYTYPE,
                LINE_CODING_DATABITS},
    cicEndpoints{{USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT | (USB_IN << 7U),
                  USB_ENDPOINT_INTERRUPT,
                  FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE}},
    dicEndpoints{{USB_CDC_VCOM_BULK_IN_ENDPOINT | (USB_IN << 7U),
                  USB_ENDPOINT_BULK,
                  FS_CDC_VCOM_BULK_IN_PACKET_SIZE},
                 {USB_CDC_VCOM_BULK_OUT_ENDPOINT | (USB_OUT << 7U),
                  USB_ENDPOINT_BULK,
                  FS_CDC_VCOM_BULK_OUT_PACKET_SIZE}},
    cicInterface{0,
                 {USB_CDC_VCOM_ENDPOINT_CIC_COUNT,
                  cicEndpoints,},
                 nullptr},
     dicInterface{0,
                  {USB_CDC_VCOM_ENDPOINT_DIC_COUNT,
                   dicEndpoints,},
                  nullptr},
     interfaces{{USB_CDC_VCOM_CIC_CLASS,
                 USB_CDC_VCOM_CIC_SUBCLASS,
                 USB_CDC_VCOM_CIC_PROTOCOL,
                 USB_CDC_VCOM_COMM_INTERFACE_INDEX,
                 &cicInterface,
                 sizeof(cicInterface) / sizeof(usb_device_interfaces_struct_t)},
                {USB_CDC_VCOM_DIC_CLASS,
                 USB_CDC_VCOM_DIC_SUBCLASS,
                 USB_CDC_VCOM_DIC_PROTOCOL,
                 USB_CDC_VCOM_DATA_INTERFACE_INDEX,
                 &dicInterface,
                 sizeof(dicInterface) / sizeof(usb_device_interfaces_struct_t)}},
    interfaceList{{USB_CDC_VCOM_INTERFACE_COUNT, interfaces}},
    config{interfaceList, kUSB_DeviceClassTypeCdc, USB_DEVICE_CONFIGURATION_COUNT},
    cdcAcmConfig{nullptr, (class_handle_t)&cdcAcmHandle, &config}
{
    cdcAcmHandle.configStruct = &cdcAcmConfig[0];
    cdcAcmHandle.configuration = 0;
    cdcAcmHandle.alternate = 0xFF;
    cdcAcmHandle.commInterfaceHandle = nullptr;

    USB_OsaMutexCreate(&(cdcAcmHandle.bulkIn.mutex));
    USB_OsaMutexCreate(&(cdcAcmHandle.bulkOut.mutex));
    USB_OsaMutexCreate(&(cdcAcmHandle.interruptIn.mutex));

    cdcAcmHandle.handle = deviceHandle;
    *handle = (class_handle_t)&cdcAcmHandle;
}

CdcAcm::~CdcAcm()
{
    USB_OsaMutexDestroy((cdcAcmHandle.bulkIn.mutex));
    USB_OsaMutexDestroy((cdcAcmHandle.bulkOut.mutex));
    USB_OsaMutexDestroy((cdcAcmHandle.interruptIn.mutex));
    endpointsDeinit(&cdcAcmHandle);
}

int32_t CdcAcm::send(class_handle_t handle,
                     uint8_t ep,
                     uint8_t* buffer,
                     uint32_t length)
{
    usb_device_cdc_acm_struct_t* cdcAcmHandle;
    int32_t error = kStatus_USB_Error;
    usb_device_cdc_acm_pipe_t* cdcAcmPipe = nullptr;

    if(!handle)
    {
        return kStatus_USB_InvalidHandle;
    }

    cdcAcmHandle = (usb_device_cdc_acm_struct_t*)handle;

    if(cdcAcmHandle->bulkIn.ep == ep)
    {
        cdcAcmPipe = &(cdcAcmHandle->bulkIn);
    }
    else if(cdcAcmHandle->interruptIn.ep == ep)
    {
        cdcAcmPipe = &(cdcAcmHandle->interruptIn);
    }
    else
    {
    }

    if(nullptr != cdcAcmPipe)
    {
        if(1 == cdcAcmPipe->isBusy)
        {
            return kStatus_USB_Busy;
        }
        USB_CDC_ACM_ENTER_CRITICAL();
        error = USB_DeviceSendRequest(cdcAcmHandle->handle, ep, buffer, length);
        if(kStatus_USB_Success == error)
        {
            cdcAcmPipe->isBusy = 1;
        }
        USB_CDC_ACM_EXIT_CRITICAL();
    }
    return error;
}

int32_t CdcAcm::recv(class_handle_t handle,
                     uint8_t ep,
                     uint8_t* buffer,
                     uint32_t length)
{
    usb_device_cdc_acm_struct_t* cdcAcmHandle;
    int32_t error = kStatus_USB_Error;
    if(!handle)
    {
        return kStatus_USB_InvalidHandle;
    }
    cdcAcmHandle = (usb_device_cdc_acm_struct_t*)handle;

    if(1 == cdcAcmHandle->bulkOut.isBusy)
    {
        return kStatus_USB_Busy;
    }
    USB_CDC_ACM_ENTER_CRITICAL();
    error = USB_DeviceRecvRequest(cdcAcmHandle->handle,
                                  ep,
                                  buffer,
                                  length);
    if(kStatus_USB_Success == error)
    {
        cdcAcmHandle->bulkOut.isBusy = 1;
    }
    USB_CDC_ACM_EXIT_CRITICAL();
    return error;
}

int32_t CdcAcm::event(void *handle, uint32_t _event, void *param)
{
    usb_device_cdc_acm_struct_t* cdcAcmHandle;
    usb_device_cdc_acm_request_param_struct_t reqParam;
    int32_t error = kStatus_USB_Error;
    uint16_t interfaceAlternate;
    uint8_t* temp8;
    uint8_t alternate;

    if((!param) || (!handle))
    {
        return kStatus_USB_InvalidHandle;
    }

    cdcAcmHandle = (usb_device_cdc_acm_struct_t*)handle;

    switch(_event)
    {
        case kUSB_DeviceClassEventDeviceReset:
            /* Bus reset, clear the configuration. */
            cdcAcmHandle->configuration = 0;
            break;
        case kUSB_DeviceClassEventSetConfiguration:
            temp8 = ((uint8_t*)param);
            if(!cdcAcmHandle->configStruct)
            {
                break;
            }
            if(*temp8 == cdcAcmHandle->configuration)
            {
                break;
            }

            error = endpointsDeinit(cdcAcmHandle);
            cdcAcmHandle->configuration = *temp8;
            cdcAcmHandle->alternate = 0;
            error = endpointsInit(cdcAcmHandle);
            break;
        case kUSB_DeviceClassEventSetInterface:
            if(!cdcAcmHandle->configStruct)
            {
                break;
            }

            interfaceAlternate = *((uint16_t*)param);
            alternate = (uint8_t)(interfaceAlternate & 0xFFU);

            if(cdcAcmHandle->interfaceNumber != ((uint8_t)(interfaceAlternate >> 8U)))
            {
                break;
            }
            if(alternate == cdcAcmHandle->alternate)
            {
                break;
            }
            error = endpointsDeinit(cdcAcmHandle);
            cdcAcmHandle->alternate = alternate;
            error = endpointsInit(cdcAcmHandle);
            break;
        case kUSB_DeviceClassEventSetEndpointHalt:
            if((!cdcAcmHandle->configStruct) || (!cdcAcmHandle->commInterfaceHandle) ||
                (!cdcAcmHandle->dataInterfaceHandle))
            {
                break;
            }
            temp8 = ((uint8_t*)param);
            for(int count = 0; count < cdcAcmHandle->commInterfaceHandle->endpointList.count; count++)
            {
                if(*temp8 == cdcAcmHandle->commInterfaceHandle->endpointList.endpoint[count].endpointAddress)
                {
                    error = USB_DeviceStallEndpoint(cdcAcmHandle->handle, *temp8);
                }
            }
            for(int count = 0; count < cdcAcmHandle->dataInterfaceHandle->endpointList.count; count++)
            {
                if(*temp8 == cdcAcmHandle->dataInterfaceHandle->endpointList.endpoint[count].endpointAddress)
                {
                    error = USB_DeviceStallEndpoint(cdcAcmHandle->handle, *temp8);
                }
            }
            break;
        case kUSB_DeviceClassEventClearEndpointHalt:
            if((!cdcAcmHandle->configStruct) || (!cdcAcmHandle->commInterfaceHandle) ||
                (!cdcAcmHandle->dataInterfaceHandle))
            {
                break;
            }
            temp8 = ((uint8_t*)param);
            for(int count = 0; count < cdcAcmHandle->commInterfaceHandle->endpointList.count; count++)
            {
                if(*temp8 == cdcAcmHandle->commInterfaceHandle->endpointList.endpoint[count].endpointAddress)
                {
                    error = USB_DeviceUnstallEndpoint(cdcAcmHandle->handle, *temp8);
                }
            }
            for(int count = 0; count < cdcAcmHandle->dataInterfaceHandle->endpointList.count; count++)
            {
                if(*temp8 == cdcAcmHandle->dataInterfaceHandle->endpointList.endpoint[count].endpointAddress)
                {
                    error = USB_DeviceUnstallEndpoint(cdcAcmHandle->handle, *temp8);
                }
            }
            break;
        case kUSB_DeviceClassEventClassRequest:
            if(param)
            {
                usb_device_control_request_struct_t* controlRequest = (usb_device_control_request_struct_t*)param;

                if((controlRequest->setup->wIndex & 0xFFU) != cdcAcmHandle->interfaceNumber)
                {
                    break;
                }
                /* Standard CDC request */
                if(USB_REQUEST_TYPE_TYPE_CLASS == (controlRequest->setup->bmRequestType & USB_REQUEST_TYPE_TYPE_MASK))
                {
                    reqParam.buffer = &(controlRequest->buffer);
                    reqParam.length = &(controlRequest->length);
                    reqParam.interfaceIndex = controlRequest->setup->wIndex;
                    reqParam.setupValue = controlRequest->setup->wValue;
                    reqParam.isSetup = controlRequest->isSetup;
                    switch(controlRequest->setup->bRequest)
                    {
                        case USB_DEVICE_CDC_REQUEST_SEND_ENCAPSULATED_COMMAND:
                            error = callback(
                                (class_handle_t)cdcAcmHandle, kUSB_DeviceCdcEventSendEncapsulatedCommand, &reqParam);
                            break;
                        case USB_DEVICE_CDC_REQUEST_GET_ENCAPSULATED_RESPONSE:
                            error = callback(
                                (class_handle_t)cdcAcmHandle, kUSB_DeviceCdcEventGetEncapsulatedResponse, &reqParam);
                            break;
                        case USB_DEVICE_CDC_REQUEST_SET_COMM_FEATURE:
                            error = callback(
                                (class_handle_t)cdcAcmHandle, kUSB_DeviceCdcEventSetCommFeature, &reqParam);
                            break;
                        case USB_DEVICE_CDC_REQUEST_GET_COMM_FEATURE:
                            error = callback(
                                (class_handle_t)cdcAcmHandle, kUSB_DeviceCdcEventGetCommFeature, &reqParam);
                            break;
                        case USB_DEVICE_CDC_REQUEST_CLEAR_COMM_FEATURE:
                            error = callback(
                                (class_handle_t)cdcAcmHandle, kUSB_DeviceCdcEventClearCommFeature, &reqParam);
                            break;
                        case USB_DEVICE_CDC_REQUEST_GET_LINE_CODING:
                            error = callback(
                                (class_handle_t)cdcAcmHandle, kUSB_DeviceCdcEventGetLineCoding, &reqParam);
                            break;
                        case USB_DEVICE_CDC_REQUEST_SET_LINE_CODING:
                            error = callback(
                                (class_handle_t)cdcAcmHandle, kUSB_DeviceCdcEventSetLineCoding, &reqParam);
                            break;
                        case USB_DEVICE_CDC_REQUEST_SET_CONTROL_LINE_STATE:
                            error = callback(
                                (class_handle_t)cdcAcmHandle, kUSB_DeviceCdcEventSetControlLineState, &reqParam);
                            break;
                        case USB_DEVICE_CDC_REQUEST_SEND_BREAK:
                            error = callback((class_handle_t)cdcAcmHandle,
                                                                              kUSB_DeviceCdcEventSendBreak, &reqParam);
                            break;
                        default:
                            error = kStatus_USB_InvalidRequest;
                            break;
                    }
                }
            }
            break;
        default:
            break;
    }
    return error;
}

int32_t CdcAcm::endpointsDeinit(usb_device_cdc_acm_struct_t* cdcAcmHandle)
{
    int32_t error = kStatus_USB_Error;

    if((!cdcAcmHandle->commInterfaceHandle) || (!cdcAcmHandle->dataInterfaceHandle))
    {
        return error;
    }
    for(int count = 0; count < cdcAcmHandle->commInterfaceHandle->endpointList.count; count++)
    {
        error = USB_DeviceDeinitEndpoint(
            cdcAcmHandle->handle, cdcAcmHandle->commInterfaceHandle->endpointList.endpoint[count].endpointAddress);
    }
    for(int count = 0; count < cdcAcmHandle->dataInterfaceHandle->endpointList.count; count++)
    {
        error = USB_DeviceDeinitEndpoint(
            cdcAcmHandle->handle, cdcAcmHandle->dataInterfaceHandle->endpointList.endpoint[count].endpointAddress);
    }
    cdcAcmHandle->commInterfaceHandle = nullptr;
    cdcAcmHandle->dataInterfaceHandle = nullptr;

    return error;
}

int32_t CdcAcm::endpointsInit(usb_device_cdc_acm_struct_t* cdcAcmHandle)
{
    usb_device_interface_list_t* interfaceList;
    usb_device_interface_struct_t* interface = nullptr;
    int32_t error = kStatus_USB_Error;

    if(!cdcAcmHandle)
    {
        return error;
    }

    /* return error when configuration is invalid (0 or more than the configuration number) */
    if((cdcAcmHandle->configuration == 0U) ||
        (cdcAcmHandle->configuration > cdcAcmHandle->configStruct->classInfomation->configurations))
    {
        return error;
    }

    interfaceList = &cdcAcmHandle->configStruct->classInfomation->interfaceList[cdcAcmHandle->configuration - 1];

    for(int count = 0; count < interfaceList->count; count++)
    {
        if(USB_DEVICE_CONFIG_CDC_COMM_CLASS_CODE == interfaceList->interfaces[count].classCode)
        {
            for(int index = 0; index < interfaceList->interfaces[count].count; index++)
            {
                if(interfaceList->interfaces[count].interface[index].alternateSetting == cdcAcmHandle->alternate)
                {
                    interface = &interfaceList->interfaces[count].interface[index];
                    break;
                }
            }
            cdcAcmHandle->interfaceNumber = interfaceList->interfaces[count].interfaceNumber;
            break;
        }
    }
    if(!interface)
    {
        return error;
    }

    cdcAcmHandle->commInterfaceHandle = interface;

    for(int count = 0; count < interface->endpointList.count; count++)
    {
        usb_device_endpoint_init_struct_t epInitStruct;
        usb_device_endpoint_callback_struct_t epCallback;
        epInitStruct.zlt = 0;
        epInitStruct.endpointAddress = interface->endpointList.endpoint[count].endpointAddress;
        epInitStruct.maxPacketSize = interface->endpointList.endpoint[count].maxPacketSize;
        epInitStruct.transferType = interface->endpointList.endpoint[count].transferType;

        if((USB_IN == ((epInitStruct.endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >>
                        USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT)) &&
            (USB_ENDPOINT_INTERRUPT == epInitStruct.transferType))
        {
            cdcAcmHandle->interruptIn.ep = (epInitStruct.endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_NUMBER_MASK);
            cdcAcmHandle->interruptIn.isBusy = 0;
            epCallback.callbackFn = CdcAcmInterruptIn;
        }

        epCallback.callbackParam = cdcAcmHandle;

        error = USB_DeviceInitEndpoint(cdcAcmHandle->handle, &epInitStruct, &epCallback);
    }

    for(int count = 0; count < interfaceList->count; count++)
    {
        if(USB_DEVICE_CONFIG_CDC_DATA_CLASS_CODE == interfaceList->interfaces[count].classCode)
        {
            for(int index = 0; index < interfaceList->interfaces[count].count; index++)
            {
                if(interfaceList->interfaces[count].interface[index].alternateSetting == cdcAcmHandle->alternate)
                {
                    interface = &interfaceList->interfaces[count].interface[index];
                    break;
                }
            }
            break;
        }
    }

    cdcAcmHandle->dataInterfaceHandle = interface;

    for(int count = 0; count < interface->endpointList.count; count++)
    {
        usb_device_endpoint_init_struct_t epInitStruct;
        usb_device_endpoint_callback_struct_t epCallback;
        epInitStruct.zlt = 0;
        epInitStruct.endpointAddress = interface->endpointList.endpoint[count].endpointAddress;
        epInitStruct.maxPacketSize = interface->endpointList.endpoint[count].maxPacketSize;
        epInitStruct.transferType = interface->endpointList.endpoint[count].transferType;

        if((USB_IN == ((epInitStruct.endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >>
                        USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT)) &&
            (USB_ENDPOINT_BULK == epInitStruct.transferType))
        {
            cdcAcmHandle->bulkIn.ep = (epInitStruct.endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_NUMBER_MASK);
            cdcAcmHandle->bulkIn.isBusy = 0;
            epCallback.callbackFn = CdcAcmBulkIn;
        }
        else if((USB_OUT == ((epInitStruct.endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >>
                              USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT)) &&
                 (USB_ENDPOINT_BULK == epInitStruct.transferType))
        {
            cdcAcmHandle->bulkOut.ep = (epInitStruct.endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_NUMBER_MASK);
            cdcAcmHandle->bulkOut.isBusy = 0;
            epCallback.callbackFn = CdcAcmBulkOut;
        }
        else
        {
        }
        epCallback.callbackParam = this;

        error = USB_DeviceInitEndpoint(cdcAcmHandle->handle, &epInitStruct, &epCallback);
    }
    return error;
}

int32_t CdcAcm::bulkOut(usb_device_endpoint_callback_message_struct_t* message)
{
    int32_t error = kStatus_USB_Error;
    cdcAcmHandle.bulkOut.isBusy = 0;
    callback((class_handle_t)&cdcAcmHandle,
             kUSB_DeviceCdcEventRecvResponse,
             message);
    return error;
}

int32_t CdcAcm::bulkIn(usb_device_endpoint_callback_message_struct_t* message)
{
    int32_t error = kStatus_USB_Error;
    cdcAcmHandle.bulkIn.isBusy = 0;
    callback((class_handle_t)&cdcAcmHandle,
             kUSB_DeviceCdcEventSendResponse,
             message);
    return error;
}

int32_t CdcAcm::interruptIn(usb_device_endpoint_callback_message_struct_t* message)
{
    int32_t error = kStatus_USB_Error;
    cdcAcmHandle.interruptIn.isBusy = 0;
    error = callback((class_handle_t)&cdcAcmHandle,
                     kUSB_DeviceCdcEventSerialStateNotif,
                     message);
    return error;
}

int32_t CdcAcm::callback(class_handle_t handle, uint32_t event, void *param)
{
    uint32_t len;
    uint16_t* uartBitmap;
    usb_device_cdc_acm_request_param_struct_t* acmReqParam;
    usb_device_endpoint_callback_message_struct_t* epCbParam;
    int32_t error = kStatus_USB_Error;
    usb_cdc_acm_info_t* acmInfo = &usbCdcAcmInfo;
    acmReqParam = (usb_device_cdc_acm_request_param_struct_t*)param;
    epCbParam = (usb_device_endpoint_callback_message_struct_t*)param;
    switch(event)
    {
        case kUSB_DeviceCdcEventSendResponse:
            {
                if((epCbParam->length != 0) && (!(epCbParam->length % dicEndpoints[0].maxPacketSize)))
                {
                    /* If the last packet is the size of endpoint, then send also zero-ended packet,
                     ** meaning that we want to inform the host that we do not have any additional
                     ** data, so it can flush the output.
                     */
                    error = send(handle, USB_CDC_VCOM_BULK_IN_ENDPOINT, nullptr, 0);
                }
                else if((1 == cdcVcom.attach) && (1 == cdcVcom.startTransactions))
                {
                    if((epCbParam->buffer != nullptr) || ((epCbParam->buffer == nullptr) && (epCbParam->length == 0)))
                    {
                        /* User: add your own code for send complete event */
                        /* Schedule buffer for next receive event */
                        error = recv(handle,
                                     USB_CDC_VCOM_BULK_OUT_ENDPOINT,
                                     currRecvBuf,
                                     dicEndpoints[0].maxPacketSize);
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
                        s_waitForDataReceive = 1;
                        USB0->INTEN &= ~USB_INTEN_SOFTOKEN_MASK;
#endif
                    }
                }
                else
                {
                }
            }
            break;
        case kUSB_DeviceCdcEventRecvResponse:
            {
                if((1 == cdcVcom.attach) && (1 == cdcVcom.startTransactions))
                {
                    recvSize = epCbParam->length;

#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
                    s_waitForDataReceive = 0;
                    USB0->INTEN |= USB_INTEN_SOFTOKEN_MASK;
#endif
                    if(!recvSize)
                    {
                        /* Schedule buffer for next receive event */
                        error = recv(handle,
                                     USB_CDC_VCOM_BULK_OUT_ENDPOINT,
                                     currRecvBuf,
                                     dicEndpoints[0].maxPacketSize);
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
                        s_waitForDataReceive = 1;
                        USB0->INTEN &= ~USB_INTEN_SOFTOKEN_MASK;
#endif
                    }
                }
            }
            break;
        case kUSB_DeviceCdcEventSerialStateNotif:
            ((usb_device_cdc_acm_struct_t*)handle)->hasSentState = 0;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceCdcEventSendEncapsulatedCommand:
            break;
        case kUSB_DeviceCdcEventGetEncapsulatedResponse:
            break;
        case kUSB_DeviceCdcEventSetCommFeature:
            if(USB_DEVICE_CDC_FEATURE_ABSTRACT_STATE == acmReqParam->setupValue)
            {
                if(1 == acmReqParam->isSetup)
                {
                    *(acmReqParam->buffer) = abstractState;
                }
                else
                {
                    *(acmReqParam->length) = 0;
                }
            }
            else if(USB_DEVICE_CDC_FEATURE_COUNTRY_SETTING == acmReqParam->setupValue)
            {
                if(1 == acmReqParam->isSetup)
                {
                    *(acmReqParam->buffer) = countryCode;
                }
                else
                {
                    *(acmReqParam->length) = 0;
                }
            }
            else
            {
            }
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceCdcEventGetCommFeature:
            if(USB_DEVICE_CDC_FEATURE_ABSTRACT_STATE == acmReqParam->setupValue)
            {
                *(acmReqParam->buffer) = abstractState;
                *(acmReqParam->length) = COMM_FEATURE_DATA_SIZE;
            }
            else if(USB_DEVICE_CDC_FEATURE_COUNTRY_SETTING == acmReqParam->setupValue)
            {
                *(acmReqParam->buffer) = countryCode;
                *(acmReqParam->length) = COMM_FEATURE_DATA_SIZE;
            }
            else
            {
            }
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceCdcEventClearCommFeature:
            break;
        case kUSB_DeviceCdcEventGetLineCoding:
            *(acmReqParam->buffer) = lineCoding;
            *(acmReqParam->length) = LINE_CODING_SIZE;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceCdcEventSetLineCoding:
            {
                if(1 == acmReqParam->isSetup)
                {
                    *(acmReqParam->buffer) = lineCoding;
                }
                else
                {
                    *(acmReqParam->length) = 0;
                }
            }
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceCdcEventSetControlLineState:
            {
                usbCdcAcmInfo.dteStatus = acmReqParam->setupValue;
                /* activate/deactivate Tx carrier */
                if(acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_CARRIER_ACTIVATION)
                {
                    acmInfo->uartState |= USB_DEVICE_CDC_UART_STATE_TX_CARRIER;
                }
                else
                {
                    acmInfo->uartState &= (uint16_t)~USB_DEVICE_CDC_UART_STATE_TX_CARRIER;
                }

                /* activate carrier and DTE */
                if(acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE)
                {
                    acmInfo->uartState |= USB_DEVICE_CDC_UART_STATE_RX_CARRIER;
                }
                else
                {
                    acmInfo->uartState &= (uint16_t)~USB_DEVICE_CDC_UART_STATE_RX_CARRIER;
                }

                /* Indicates to DCE if DTE is present or not */
                acmInfo->dtePresent = (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE) ? true : false;

                /* Initialize the serial state buffer */
                acmInfo->serialStateBuf[0] = NOTIF_REQUEST_TYPE;                /* bmRequestType */
                acmInfo->serialStateBuf[1] = USB_DEVICE_CDC_NOTIF_SERIAL_STATE; /* bNotification */
                acmInfo->serialStateBuf[2] = 0x00;                              /* wValue */
                acmInfo->serialStateBuf[3] = 0x00;
                acmInfo->serialStateBuf[4] = 0x00; /* wIndex */
                acmInfo->serialStateBuf[5] = 0x00;
                acmInfo->serialStateBuf[6] = UART_BITMAP_SIZE; /* wLength */
                acmInfo->serialStateBuf[7] = 0x00;
                /* Notifiy to host the line state */
                acmInfo->serialStateBuf[4] = acmReqParam->interfaceIndex;
                /* Lower byte of UART BITMAP */
                uartBitmap = (uint16_t*)&acmInfo->serialStateBuf[NOTIF_PACKET_SIZE + UART_BITMAP_SIZE - 2];
                *uartBitmap = acmInfo->uartState;
                len = (uint32_t)(NOTIF_PACKET_SIZE + UART_BITMAP_SIZE);
                if(0 == ((usb_device_cdc_acm_struct_t*)handle)->hasSentState)
                {
                    error = send(handle, USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT, acmInfo->serialStateBuf, len);
                    ((usb_device_cdc_acm_struct_t*)handle)->hasSentState = 1;
                }

                /* Update status */
                if(acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_CARRIER_ACTIVATION)
                {
                    /*  To do: CARRIER_ACTIVATED */
                }
                else
                {
                    /* To do: CARRIER_DEACTIVATED */
                }
                if(acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE)
                {
                    /* DTE_ACTIVATED */
                    if(1 == cdcVcom.attach)
                    {
                        cdcVcom.startTransactions = 1;
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
                        s_waitForDataReceive = 1;
                        USB0->INTEN &= ~USB_INTEN_SOFTOKEN_MASK;
                        s_comOpen = 1;
#endif
                    }
                }
                else
                {
                    /* DTE_DEACTIVATED */
                    if(1 == cdcVcom.attach)
                    {
                        cdcVcom.startTransactions = 0;
                    }
                }
            }
            break;
        case kUSB_DeviceCdcEventSendBreak:
            break;
        default:
            break;
    }

    return error;
}

int32_t CdcAcm::deviceCallback(class_handle_t handle, uint32_t event, void *param)
{
    int32_t error = kStatus_USB_Error;
    uint16_t* temp16 = (uint16_t*)param;
    uint8_t* temp8 = (uint8_t*)param;

    switch(event)
    {
        case kUSB_DeviceEventBusReset:
            {
                cdcVcom.attach = 0;
            }
            break;
        case kUSB_DeviceEventSetConfiguration:
            cdcVcom.attach = 1;
            cdcVcom.currentConfiguration = *temp8;
            if(USB_CDC_VCOM_CONFIGURE_INDEX == (*temp8))
            {
                /* Schedule buffer for receive */
                recv(reinterpret_cast<uint32_t>(&cdcAcmHandle),
                     USB_CDC_VCOM_BULK_OUT_ENDPOINT,
                     currRecvBuf,
                     dicEndpoints[0].maxPacketSize);
            }
        case kUSB_DeviceEventSetInterface:
            if(cdcVcom.attach)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FFU);
                if(interface < USB_CDC_VCOM_INTERFACE_COUNT)
                {
                    cdcVcom.currentInterfaceAlternateSetting[interface] = alternateSetting;
                }
            }
            break;
        default:
            break;
    }
    return error;
}

int32_t CdcAcm::setSpeed(uint8_t speed)
{
    for(int i = 0; i < USB_CDC_VCOM_ENDPOINT_CIC_COUNT; i++)
    {
        if(USB_SPEED_HIGH == speed)
        {
            cicEndpoints[i].maxPacketSize = HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE;
        }
        else
        {
            cicEndpoints[i].maxPacketSize = FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE;
        }
    }
    for(int i = 0; i < USB_CDC_VCOM_ENDPOINT_DIC_COUNT; i++)
    {
        if(USB_SPEED_HIGH == speed)
        {
            dicEndpoints[i].maxPacketSize = HS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
        }
        else
        {
            dicEndpoints[i].maxPacketSize = FS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
        }
    }
    return kStatus_USB_Success;
}

void CdcAcm::echo()
{
    if((0 != recvSize) && (0xFFFFFFFFU != recvSize))
    {
        uint32_t i;

        /* Copy Buffer to Send Buff */
        for(i = 0; i < recvSize; i++)
        {
            currSendBuf[sendSize++] = currRecvBuf[i];
        }
        recvSize = 0;
    }

    if(sendSize)
    {
        uint32_t size = sendSize;
        sendSize = 0;
        send((class_handle_t)&cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, currSendBuf, size);
    }
}

}

extern "C"
{
usb_status_t CdcAcmInterruptIn(usb_device_handle handle,
                                         usb_device_endpoint_callback_message_struct_t* message,
                                         void* callbackParam)
{
    return static_cast<usb_status_t>(reinterpret_cast<usb::CdcAcm*>(callbackParam)->interruptIn(message));
}

usb_status_t CdcAcmBulkOut(usb_device_handle handle,
                               usb_device_endpoint_callback_message_struct_t* message,
                               void* callbackParam)
{
    return static_cast<usb_status_t>(reinterpret_cast<usb::CdcAcm*>(callbackParam)->bulkOut(message));
}

usb_status_t CdcAcmBulkIn(usb_device_handle handle,
                          usb_device_endpoint_callback_message_struct_t* message,
                          void* callbackParam)
{
    return static_cast<usb_status_t>(reinterpret_cast<usb::CdcAcm*>(callbackParam)->bulkIn(message));
}

}
