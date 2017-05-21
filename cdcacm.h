#pragma once

#include <cstdint>
#include "usbtypes.h"
#include "devicedescriptor.h"

#include "usb.h"
#include "usb_osa.h"
#include "usb_device.h"

#define USB_DEVICE_CONFIG_CDC_ACM_MAX_INSTANCE (1)
#define USB_DEVICE_CONFIG_CDC_COMM_CLASS_CODE (0x02)
#define USB_DEVICE_CONFIG_CDC_DATA_CLASS_CODE (0x0A)

#define USB_DEVICE_CDC_REQUEST_SEND_ENCAPSULATED_COMMAND (0x00)
#define USB_DEVICE_CDC_REQUEST_GET_ENCAPSULATED_RESPONSE (0x01)
#define USB_DEVICE_CDC_REQUEST_SET_COMM_FEATURE (0x02)
#define USB_DEVICE_CDC_REQUEST_GET_COMM_FEATURE (0x03)
#define USB_DEVICE_CDC_REQUEST_CLEAR_COMM_FEATURE (0x04)
#define USB_DEVICE_CDC_REQUEST_SET_AUX_LINE_STATE (0x10)
#define USB_DEVICE_CDC_REQUEST_SET_HOOK_STATE (0x11)
#define USB_DEVICE_CDC_REQUEST_PULSE_SETUP (0x12)
#define USB_DEVICE_CDC_REQUEST_SEND_PULSE (0x13)
#define USB_DEVICE_CDC_REQUEST_SET_PULSE_TIME (0x14)
#define USB_DEVICE_CDC_REQUEST_RING_AUX_JACK (0x15)
#define USB_DEVICE_CDC_REQUEST_SET_LINE_CODING (0x20)
#define USB_DEVICE_CDC_REQUEST_GET_LINE_CODING (0x21)
#define USB_DEVICE_CDC_REQUEST_SET_CONTROL_LINE_STATE (0x22)
#define USB_DEVICE_CDC_REQUEST_SEND_BREAK (0x23)
#define USB_DEVICE_CDC_REQUEST_SET_RINGER_PARAMS (0x30)
#define USB_DEVICE_CDC_REQUEST_GET_RINGER_PARAMS (0x31)
#define USB_DEVICE_CDC_REQUEST_SET_OPERATION_PARAM (0x32)
#define USB_DEVICE_CDC_REQUEST_GET_OPERATION_PARAM (0x33)
#define USB_DEVICE_CDC_REQUEST_SET_LINE_PARAMS (0x34)
#define USB_DEVICE_CDC_REQUEST_GET_LINE_PARAMS (0x35)
#define USB_DEVICE_CDC_REQUEST_DIAL_DIGITS (0x36)
#define USB_DEVICE_CDC_REQUEST_SET_UNIT_PARAMETER (0x37)
#define USB_DEVICE_CDC_REQUEST_GET_UNIT_PARAMETER (0x38)
#define USB_DEVICE_CDC_REQUEST_CLEAR_UNIT_PARAMETER (0x39)
#define USB_DEVICE_CDC_REQUEST_SET_ETHERNET_MULTICAST_FILTERS (0x40)
#define USB_DEVICE_CDC_REQUEST_SET_ETHERNET_POW_PATTER_FILTER (0x41)
#define USB_DEVICE_CDC_REQUEST_GET_ETHERNET_POW_PATTER_FILTER (0x42)
#define USB_DEVICE_CDC_REQUEST_SET_ETHERNET_PACKET_FILTER (0x43)
#define USB_DEVICE_CDC_REQUEST_GET_ETHERNET_STATISTIC (0x44)
#define USB_DEVICE_CDC_REQUEST_SET_ATM_DATA_FORMAT (0x50)
#define USB_DEVICE_CDC_REQUEST_GET_ATM_DEVICE_STATISTICS (0x51)
#define USB_DEVICE_CDC_REQUEST_SET_ATM_DEFAULT_VC (0x52)
#define USB_DEVICE_CDC_REQUEST_GET_ATM_VC_STATISTICS (0x53)
#define USB_DEVICE_CDC_REQUEST_MDLM_SPECIFIC_REQUESTS_MASK (0x7F)

#define USB_DEVICE_CDC_NOTIF_NETWORK_CONNECTION (0x00)
#define USB_DEVICE_CDC_NOTIF_RESPONSE_AVAIL (0x01)
#define USB_DEVICE_CDC_NOTIF_AUX_JACK_HOOK_STATE (0x08)
#define USB_DEVICE_CDC_NOTIF_RING_DETECT (0x09)
#define USB_DEVICE_CDC_NOTIF_SERIAL_STATE (0x20)
#define USB_DEVICE_CDC_NOTIF_CALL_STATE_CHANGE (0x28)
#define USB_DEVICE_CDC_NOTIF_LINE_STATE_CHANGE (0x29)
#define USB_DEVICE_CDC_NOTIF_CONNECTION_SPEED_CHANGE (0x2A)

#define USB_DEVICE_CDC_FEATURE_ABSTRACT_STATE (0x01)
#define USB_DEVICE_CDC_FEATURE_COUNTRY_SETTING (0x02)

#define USB_DEVICE_CDC_CONTROL_SIG_BITMAP_CARRIER_ACTIVATION (0x02)
#define USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE (0x01)
#define USB_DEVICE_CDC_UART_STATE_RX_CARRIER (0x01)
#define USB_DEVICE_CDC_UART_STATE_TX_CARRIER (0x02)
#define USB_DEVICE_CDC_UART_STATE_BREAK (0x04)
#define USB_DEVICE_CDC_UART_STATE_RING_SIGNAL (0x08)
#define USB_DEVICE_CDC_UART_STATE_FRAMING (0x10)
#define USB_DEVICE_CDC_UART_STATE_PARITY (0x20)
#define USB_DEVICE_CDC_UART_STATE_OVERRUN (0x40)

#define DATA_BUFF_SIZE FS_CDC_VCOM_BULK_OUT_PACKET_SIZE
#define NOTIF_PACKET_SIZE (0x08)
#define UART_BITMAP_SIZE (0x02)
#define COMM_FEATURE_DATA_SIZE (0x02)
#define NOTIF_REQUEST_TYPE (0xA1)

#define LINE_CODING_SIZE (0x07)
#define LINE_CODING_DTERATE (115200)
#define LINE_CODING_CHARFORMAT (0x00)
#define LINE_CODING_PARITYTYPE (0x00)
#define LINE_CODING_DATABITS (0x08)

#define STATUS_ABSTRACT_STATE (0x0000)
#define COUNTRY_SETTING (0x0000)

enum usb_device_cdc_acm_event_t
{
    kUSB_DeviceCdcEventSendResponse = 0x01,
    kUSB_DeviceCdcEventRecvResponse,
    kUSB_DeviceCdcEventSerialStateNotif,
    kUSB_DeviceCdcEventSendEncapsulatedCommand,
    kUSB_DeviceCdcEventGetEncapsulatedResponse,
    kUSB_DeviceCdcEventSetCommFeature,
    kUSB_DeviceCdcEventGetCommFeature,
    kUSB_DeviceCdcEventClearCommFeature,
    kUSB_DeviceCdcEventGetLineCoding,
    kUSB_DeviceCdcEventSetLineCoding,
    kUSB_DeviceCdcEventSetControlLineState,
    kUSB_DeviceCdcEventSendBreak
} ;

struct usb_device_cdc_acm_request_param_struct_t
{
    uint8_t **buffer;
    uint32_t *length;
    uint16_t interfaceIndex;
    uint16_t setupValue;
    uint8_t isSetup;
};

struct usb_device_cdc_acm_pipe_t
{
    usb_osa_mutex_handle mutex;
    uint8_t ep;
    uint8_t isBusy;
};

struct usb_device_cdc_acm_struct_t
{
    usb_device_handle handle;
    usb_device_class_config_struct_t *configStruct;
    usb_device_interface_struct_t *commInterfaceHandle;
    usb_device_interface_struct_t *dataInterfaceHandle;
    usb_device_cdc_acm_pipe_t bulkIn;
    usb_device_cdc_acm_pipe_t bulkOut;
    usb_device_cdc_acm_pipe_t interruptIn;
    uint8_t configuration;
    uint8_t interfaceNumber;
    uint8_t alternate;
    uint8_t hasSentState;
};

struct usb_cdc_vcom_struct_t
{
    usb_device_handle deviceHandle;
    class_handle_t cdcAcmHandle;
    volatile uint8_t attach;
    uint8_t speed;
    volatile uint8_t
        startTransactions;
    uint8_t currentConfiguration;
    uint8_t currentInterfaceAlternateSetting
        [USB_CDC_VCOM_INTERFACE_COUNT];
};

struct usb_cdc_acm_info_t
{
    uint8_t serialStateBuf[NOTIF_PACKET_SIZE + UART_BITMAP_SIZE];
    bool dtePresent;
    uint16_t breakDuration;
    uint8_t dteStatus;
    uint8_t currentInterface;
    uint16_t uartState;
};

namespace usb
{

class CdcAcm
{
public:
    CdcAcm(uint8_t controllerId,
           usb_device_handle deviceHandle,
           class_handle_t* handle);
    ~CdcAcm();

    int32_t interruptIn(usb_device_endpoint_callback_message_struct_t* message);
    int32_t bulkIn(usb_device_endpoint_callback_message_struct_t* message);
    int32_t bulkOut(usb_device_endpoint_callback_message_struct_t* message);
    int32_t endpointsInit(usb_device_cdc_acm_struct_t* cdcAcmHandle);
    int32_t endpointsDeinit(usb_device_cdc_acm_struct_t* cdcAcmHandle);
    int32_t event(void* handle, uint32_t event, void *param);
    int32_t send(class_handle_t handle, uint8_t ep, uint8_t* buffer, uint32_t length);
    int32_t recv(class_handle_t handle, uint8_t ep, uint8_t* buffer, uint32_t length);
    int32_t callback(class_handle_t handle, uint32_t event, void* param);
    int32_t deviceCallback(class_handle_t handle, uint32_t event, void* param);
    int32_t setSpeed(uint8_t speed);

    void echo();

private:
    usb_device_cdc_acm_struct_t cdcAcmHandle;
    usb_cdc_vcom_struct_t cdcVcom;
    usb_cdc_acm_info_t usbCdcAcmInfo;

    uint8_t currRecvBuf[DATA_BUFF_SIZE];
    uint8_t currSendBuf[DATA_BUFF_SIZE];
    volatile uint32_t recvSize;
    volatile  uint32_t sendSize;

    uint8_t countryCode[COMM_FEATURE_DATA_SIZE];
    uint8_t abstractState[COMM_FEATURE_DATA_SIZE];
    uint8_t lineCoding[LINE_CODING_SIZE];

    usb_device_endpoint_struct_t cicEndpoints[USB_CDC_VCOM_ENDPOINT_CIC_COUNT];
    usb_device_endpoint_struct_t dicEndpoints[USB_CDC_VCOM_ENDPOINT_DIC_COUNT];
    usb_device_interface_struct_t cicInterface;
    usb_device_interface_struct_t dicInterface;
    usb_device_interfaces_struct_t interfaces[USB_CDC_VCOM_INTERFACE_COUNT];
    usb_device_interface_list_t interfaceList[USB_DEVICE_CONFIGURATION_COUNT];
    usb_device_class_struct_t config;
    usb_device_class_config_struct_t cdcAcmConfig[1];
};

}
