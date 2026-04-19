#include "usbacm.h"
#include "gpio.h"
#include <algorithm>

USB_ACM *USB_ACM::instance = nullptr;

void USB_ACM::unplug()
{
    HAL_Delay(100);
    Pin{GPIOA, 12}.setAsInput();
    HAL_Delay(100);
    Pin{GPIOA, 12}.setAsAlt(10); // OTG FS
}

int8_t USB_ACM::itfInit()
{
    instance->active = true;
    instance->rx_active = false;
    instance->ctl_line_state = 1;
    instance->outgoing_pkt_size = 0;
    instance->tx_fifo.clear();
    instance->rx_fifo.clear();
    USBD_CDC_SetRxBuffer(&instance->USBD_Device, instance->rx_buffer);
    USBD_CDC_SetTxBuffer(&instance->USBD_Device, instance->tx_fifo.data, 0);
    instance->tryTransmit();
    instance->tryReceive();
    return USBD_OK;
}

int8_t USB_ACM::itfDeinit(void)
{
    instance->active = false;
    return USBD_OK;
}

int8_t USB_ACM::itfControl(uint8_t cmd, uint8_t * pbuf, uint16_t length)
{
    if (cmd == CDC_SET_CONTROL_LINE_STATE)
    {
        USB_ACM::instance->ctl_line_state = ((USBD_SetupReqTypedef *)pbuf)->wValue;
        return USBD_OK;
    }
    return USBD_OK;
}

int8_t USB_ACM::itfReceive(uint8_t* pbuf, uint32_t *Len)
{
    for (uint32_t i = 0; i < *Len; ++i) {
        USB_ACM::instance->rx_fifo.put(pbuf[i]);
    }
    USB_ACM::instance->rx_active = false;
    USB_ACM::instance->tryReceive();
    return USBD_OK;
}

int8_t USB_ACM::itfTransmitCplt(uint8_t *pbuf, uint32_t *Len, uint8_t epnum)
{
    USB_ACM::instance->txComplete();
    return USBD_OK;
}


USBD_CDC_ItfTypeDef USB_ACM::CDC_fops = {
    itfInit,
    itfDeinit,
    itfControl,
    itfReceive,
    itfTransmitCplt,
};

void USB_ACM::init()
{
    instance = this;
    active = false;
    outgoing_pkt_size = 0;
    rx_active = false;
    
    unplug();

    Pin{GPIOA, 11}.setAsAlt(10); // OTG FS
    Pin{GPIOA, 12}.setAsAlt(10); // OTG FS

    USBD_Init(&USBD_Device, &ACM_Desc, 0);
    USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);
    USBD_CDC_RegisterInterface(&USBD_Device, &CDC_fops);
    USBD_Start(&USBD_Device);
}

void USB_ACM::deinit()
{
    USBD_Stop(&USBD_Device);
}

uint32_t USB_ACM::read(uint8_t *buf, uint32_t max_len)
{
    bool before = rx_active || rx_fifo.putSize() < CDC_DATA_FS_OUT_PACKET_SIZE;

    uint8_t *start = buf;
    uint32_t len = std::min(max_len, rx_fifo.getSizeContiguous());
    buf = rx_fifo.getSpan(buf, len);
    max_len -= len;
    if (max_len) {
        len = std::min(max_len, rx_fifo.getSizeContiguous());
        buf = rx_fifo.getSpan(buf, len);
    }
    bool after = rx_active || rx_fifo.putSize() < CDC_DATA_FS_OUT_PACKET_SIZE;
    if (before && !after)
        tryReceive();
    return buf - start;
}

uint32_t USB_ACM::write(const uint8_t *buf, uint32_t len)
{
    len = std::min(len, tx_fifo.putSize());
    for (uint32_t i = 0; i < len; ++i) {
        tx_fifo.put(buf[i]);
    }
    if (tx_fifo.getSize() >= CDC_DATA_FS_IN_PACKET_SIZE)
        tryTransmit();
    return len;
}

void USB_ACM::writeHex(uint32_t value)
{
    char buf[16];
    snprintf(buf, 16, "%X", (unsigned)value);
    writeStr(buf);
}

void USB_ACM::flush()
{
    tryTransmit();
}

void USB_ACM::tryReceive()
{
    if (rx_active || rx_fifo.putSize() < CDC_DATA_FS_OUT_PACKET_SIZE)
        return;
    rx_active = true;
    USBD_CDC_ReceivePacket(&USBD_Device);
}

void USB_ACM::tryTransmit()
{
    if (outgoing_pkt_size)
        return;
    uint32_t max_size = std::min(tx_fifo.getSize(), (uint32_t)CDC_DATA_FS_IN_PACKET_SIZE);
    if (!max_size)
        return;
    //outgoing_pkt_size += std::min(max_size, (uint32_t)CDC_DATA_FS_IN_PACKET_SIZE);
    // outgoing_pkt_size = max_size;
    uint32_t part1_size = std::min(max_size, tx_fifo.getSizeContiguous());
    tx_fifo.getSpan(tx_buffer, part1_size);
    if (part1_size < max_size) {
        uint32_t part2_size = max_size - part1_size;
        tx_fifo.getSpan(tx_buffer + part1_size, part2_size);
    }
    outgoing_pkt_size = max_size;
    USBD_CDC_SetTxBuffer(&USBD_Device, tx_buffer, max_size);
    USBD_CDC_TransmitPacket(&USBD_Device);
}

void USB_ACM::txComplete()
{
    outgoing_pkt_size = 0;
    tryTransmit();
}