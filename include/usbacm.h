#include "usbd_cdc.h"
#include "usbd_desc.h"

template<unsigned SIZE>
struct FIFO
{
    const unsigned size = SIZE;
    
    uint8_t data[SIZE];
    uint16_t read_ptr;
    uint16_t write_ptr;
    
    FIFO()
    : read_ptr{}
    , write_ptr{}
    {}
    
    void put(uint8_t item) {
        data[write_ptr] = item;
        write_ptr = (write_ptr < SIZE - 1) ? write_ptr + 1 : 0;
    }
    uint8_t get() {
        uint8_t value = data[read_ptr];
        read_ptr = (read_ptr < SIZE - 1) ? read_ptr + 1 : 0;
        return value;
    }
    uint32_t putSize() const {
        if (write_ptr < read_ptr)
            return read_ptr - write_ptr - 1;
        return read_ptr - write_ptr + SIZE - 1;
    }
    uint32_t getSize() const {
        if (read_ptr <= write_ptr)
            return write_ptr - read_ptr;
        return write_ptr + SIZE - read_ptr;
    }
    uint32_t getSizeContiguous() const {
        uint32_t size = getSize();
        if (size > (SIZE - read_ptr))
            size = SIZE - read_ptr;
        return size;
    }
    uint8_t *getSpan(uint8_t *buf, uint32_t size) {
        const uint8_t *ptr = &data[read_ptr];
        read_ptr += size;
        if (read_ptr >= SIZE)
            read_ptr = 0;
        memcpy(buf, ptr, size);
        return buf + size;
    }
    void clear() {
        write_ptr = 0;
        read_ptr = 0;
    }
};

class USB_ACM
{
protected:
    static constexpr int TX_FIFO_SIZE = 256;
    static constexpr int RX_FIFO_SIZE = 256;
    FIFO<TX_FIFO_SIZE> tx_fifo;
    FIFO<RX_FIFO_SIZE> rx_fifo;
    uint32_t outgoing_pkt_size;
    bool rx_active;
    bool active;
    uint8_t rx_buffer[CDC_DATA_FS_OUT_PACKET_SIZE];
    uint8_t tx_buffer[CDC_DATA_FS_IN_PACKET_SIZE];
    uint32_t ctl_line_state;

    void unplug();
    void tryTransmit();
    void txComplete();

    static int8_t itfInit();
    static int8_t itfDeinit(void);
    static int8_t itfControl(uint8_t cmd, uint8_t * pbuf, uint16_t length);
    static int8_t itfReceive(uint8_t* pbuf, uint32_t *Len);
    static int8_t itfTransmitCplt(uint8_t *pbuf, uint32_t *Len, uint8_t epnum);
    static USBD_CDC_ItfTypeDef CDC_fops;

public:
    USBD_HandleTypeDef USBD_Device;
    static USB_ACM *instance;
    void init();
    void deinit();
    bool isActive() const { return active; }
    uint32_t read(uint8_t *buf, uint32_t max_len);
    uint32_t write(const uint8_t *buf, uint32_t len);
    inline uint32_t writeStr(const char *buf) { return write((const uint8_t *)buf, strlen(buf)); }
    void writeHex(uint32_t value);
    uint32_t getRxCount() const { return rx_fifo.getSize(); }
    uint32_t getTxSpace() const { return tx_fifo.putSize(); }
    uint8_t readChar() { return rx_fifo.get(); }
    void writeChar(uint8_t data) { tx_fifo.put(data); }
    void flush();
    void tryReceive();
    bool dtr() const { return ctl_line_state & 1; }
    bool rts() const { return ctl_line_state & 2; }
};