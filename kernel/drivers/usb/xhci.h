/**
 * @file xhci.h — Xenithra OS — xHCI USB 3.0 Host Controller Driver
 *
 * USB 3.0 eXtensible Host Controller Interface driver.
 * Supports USB 1.1 (FS), USB 2.0 (HS), and USB 3.0 (SS) devices.
 */

#ifndef _DRIVER_XHCI_H_
#define _DRIVER_XHCI_H_

#include <stdint.h>
#include <stddef.h>

/* ── PCI IDs ─────────────────────────────────────────────────────────── */
#define XHCI_PCI_CLASS    0x0C   /* Serial Bus */
#define XHCI_PCI_SUBCLASS 0x03   /* USB */
#define XHCI_PCI_PROGIF   0x30   /* xHCI */

/* ── Capability Register Offsets ─────────────────────────────────────── */
#define XHCI_CAP_CAPLENGTH  0x00  /* Capability Registers Length */
#define XHCI_CAP_HCIVERSION 0x02  /* Interface Version Number */
#define XHCI_CAP_HCSPARAMS1 0x04  /* Structural Parameters 1 */
#define XHCI_CAP_HCSPARAMS2 0x08  /* Structural Parameters 2 */
#define XHCI_CAP_HCSPARAMS3 0x0C  /* Structural Parameters 3 */
#define XHCI_CAP_HCCPARAMS1 0x10  /* Capability Parameters 1 */
#define XHCI_CAP_DBOFF      0x14  /* Doorbell Array Offset */
#define XHCI_CAP_RTSOFF     0x18  /* Runtime Register Space Offset */

/* ── Operational Register Offsets (relative to Cap base + CAPLENGTH) ── */
#define XHCI_OP_USBCMD   0x00   /* USB Command */
#define XHCI_OP_USBSTS   0x04   /* USB Status */
#define XHCI_OP_PAGESIZE 0x08   /* Page Size */
#define XHCI_OP_DNCTRL   0x14   /* Device Notification Control */
#define XHCI_OP_CRCR     0x18   /* Command Ring Control */
#define XHCI_OP_DCBAAP   0x30   /* Device Context Base Address Array Pointer */
#define XHCI_OP_CONFIG   0x38   /* Configure */

/* ── USBCMD bits ─────────────────────────────────────────────────────── */
#define XHCI_CMD_RUN     (1U << 0)   /* Run/Stop */
#define XHCI_CMD_HCRST   (1U << 1)   /* Host Controller Reset */
#define XHCI_CMD_INTE    (1U << 2)   /* Interrupter Enable */
#define XHCI_CMD_HSEE    (1U << 3)   /* Host System Error Enable */

/* ── USBSTS bits ─────────────────────────────────────────────────────── */
#define XHCI_STS_HCH     (1U << 0)   /* HC Halted */
#define XHCI_STS_HSE     (1U << 2)   /* Host System Error */
#define XHCI_STS_EINT    (1U << 3)   /* Event Interrupt */
#define XHCI_STS_CNR     (1U << 11)  /* Controller Not Ready */

/* ── TRB (Transfer Request Block) types ──────────────────────────────── */
#define TRB_TYPE_NORMAL          1
#define TRB_TYPE_SETUP_STAGE     2
#define TRB_TYPE_DATA_STAGE      3
#define TRB_TYPE_STATUS_STAGE    4
#define TRB_TYPE_LINK           23
#define TRB_TYPE_ENABLE_SLOT    9
#define TRB_TYPE_DISABLE_SLOT  10
#define TRB_TYPE_ADDRESS_DEV   11
#define TRB_TYPE_NO_OP_CMD     23
#define TRB_TYPE_TRANSFER_EVT  32
#define TRB_TYPE_CMD_COMPLETION 33
#define TRB_TYPE_PORT_STATUS   34

/* ── TRB structure (16 bytes) ────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint64_t param;     /* Address or data parameter */
    uint32_t status;    /* Transfer/completion data */
    uint32_t ctrl;      /* Cycle bit, TRB type, flags */
} XhciTrb;

/* ── Port Status Register (PORTSC) ──────────────────────────────────── */
#define XHCI_PORTSC_CCS    (1U << 0)   /* Current Connect Status */
#define XHCI_PORTSC_PED    (1U << 1)   /* Port Enabled/Disabled */
#define XHCI_PORTSC_OCA    (1U << 3)   /* Over-current Active */
#define XHCI_PORTSC_PR     (1U << 4)   /* Port Reset */
#define XHCI_PORTSC_PP     (1U << 9)   /* Port Power */
#define XHCI_PORTSC_CSC    (1U << 17)  /* Connect Status Change */
#define XHCI_PORTSC_PEC    (1U << 18)  /* Port Enable/Disable Change */
#define XHCI_PORTSC_PRC    (1U << 21)  /* Port Reset Change */
#define XHCI_PORTSC_SPEED(x) (((x) >> 10) & 0xF)  /* Port Speed */

/* ── USB Speed constants ─────────────────────────────────────────────── */
#define USB_SPEED_FULL  1  /* USB 1.1 Full Speed  12 Mbps */
#define USB_SPEED_LOW   2  /* USB 1.1 Low Speed  1.5 Mbps */
#define USB_SPEED_HI    3  /* USB 2.0 High Speed 480 Mbps */
#define USB_SPEED_SUPER 4  /* USB 3.0 Super Speed 5 Gbps */

/* ── USB Device descriptor (simplified) ─────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;    /* 0x01 = Device */
    uint16_t bcdUSB;             /* USB Spec version BCD */
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;    /* Max EP0 packet size */
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} UsbDeviceDescriptor;

/* ── xHCI device tracking ────────────────────────────────────────────── */
#define XHCI_MAX_PORTS  16
#define XHCI_MAX_SLOTS  32
#define XHCI_RING_SIZE  256

typedef struct {
    volatile uint8_t *mmio_base;  /* MMIO base (PCI BAR0) */
    volatile uint8_t *op_regs;    /* Operational regs (base + CAPLENGTH) */
    volatile uint8_t *rt_regs;    /* Runtime regs */
    volatile uint32_t *doorbell;  /* Doorbell array */
    uint8_t  cap_len;
    uint8_t  max_slots;
    uint8_t  max_ports;
    uint8_t  max_intrs;
    uint64_t page_size;

    /* Command ring */
    XhciTrb *cmd_ring;
    uint32_t  cmd_cycle;
    uint32_t  cmd_enqueue;

    /* Event ring (primary) */
    XhciTrb *event_ring;
    uint32_t  event_cycle;
    uint32_t  event_dequeue;

    /* Port enumeration */
    uint8_t  port_connected[XHCI_MAX_PORTS];
    uint8_t  port_speed[XHCI_MAX_PORTS];
    uint8_t  slot_id[XHCI_MAX_PORTS];

    uint32_t irq;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
} XhciController;

/* ── Public API ──────────────────────────────────────────────────────── */

/** Find and initialize xHCI controller via PCI. Returns 0 on success. */
int  xhci_init(void);

/** Process pending xHCI events (call from IRQ handler or poll loop). */
void xhci_poll_events(void);

/** IRQ handler. */
void xhci_irq_handler(void);

/** Enumerate devices on all ports. Sends driver events via IPC. */
void xhci_enumerate_ports(void);

/** Submit a USB control transfer. Returns 0 on success. */
int  xhci_control_transfer(uint8_t slot, uint8_t request_type, uint8_t request,
                             uint16_t value, uint16_t index,
                             void *data, uint16_t len);

/** Get the global controller instance. */
XhciController *xhci_get_controller(void);

#endif /* _DRIVER_XHCI_H_ */
