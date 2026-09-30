/**
 * @file rtl8139.h — Xenithra OS — Realtek RTL8139 Fast Ethernet Driver
 *
 * Kernel driver for the Realtek RTL8139 100Mbps NIC.
 * Commonly used in QEMU with -netdev=rtl8139 option.
 * Uses I/O port access (no MMIO). Ring-mode RX, 4-descriptor TX.
 */

#ifndef _DRIVER_RTL8139_H_
#define _DRIVER_RTL8139_H_

#include <stdint.h>
#include <stddef.h>

/* ── PCI IDs ─────────────────────────────────────────────────────────── */
#define RTL8139_VENDOR_ID  0x10EC   /* Realtek Semiconductor */
#define RTL8139_DEVICE_ID  0x8139   /* RTL8139 */
#define RTL8100E_DEVICE_ID 0x8136   /* RTL8100E (compatible) */

/* ── I/O Register Offsets ─────────────────────────────────────────────── */
#define RTL_MAC0         0x00   /* MAC address (6 bytes, read as 4+2) */
#define RTL_MAR0         0x08   /* Multicast address register */
#define RTL_TSD0         0x10   /* TX Status Descriptor 0 */
#define RTL_TSD1         0x14
#define RTL_TSD2         0x18
#define RTL_TSD3         0x1C
#define RTL_TSAD0        0x20   /* TX Start Address 0 */
#define RTL_TSAD1        0x24
#define RTL_TSAD2        0x28
#define RTL_TSAD3        0x2C
#define RTL_RBSTART      0x30   /* RX Buffer Start Address */
#define RTL_ERBCR        0x34   /* Early RX Byte Count */
#define RTL_ERSR         0x36   /* Early RX Status */
#define RTL_CR           0x37   /* Command Register */
#define RTL_CAPR         0x38   /* Current Address of Packet Read */
#define RTL_CBR          0x3A   /* Current Buffer Address */
#define RTL_IMR          0x3C   /* Interrupt Mask Register */
#define RTL_ISR          0x3E   /* Interrupt Status Register */
#define RTL_TCR          0x40   /* TX Configuration Register */
#define RTL_RCR          0x44   /* RX Configuration Register */
#define RTL_TCTR         0x48   /* Timer Count */
#define RTL_MPC          0x4C   /* Missed Packet Counter */
#define RTL_9346CR       0x50   /* 93C46 Command Register */
#define RTL_CONFIG0      0x51
#define RTL_CONFIG1      0x52
#define RTL_MSR          0x58   /* Media Status Register */
#define RTL_BMCR         0x62   /* Basic Mode Control Register */
#define RTL_BMSR         0x64   /* Basic Mode Status Register */

/* ── Command Register bits (CR) ──────────────────────────────────────── */
#define RTL_CR_BUFE      (1U << 0)   /* Buffer Empty */
#define RTL_CR_TE        (1U << 2)   /* Transmitter Enable */
#define RTL_CR_RE        (1U << 3)   /* Receiver Enable */
#define RTL_CR_RST       (1U << 4)   /* Reset */

/* ── Interrupt bits (IMR / ISR) ──────────────────────────────────────── */
#define RTL_INT_ROK      (1U << 0)   /* Receive OK */
#define RTL_INT_RER      (1U << 1)   /* Receive Error */
#define RTL_INT_TOK      (1U << 2)   /* Transmit OK */
#define RTL_INT_TER      (1U << 3)   /* Transmit Error */
#define RTL_INT_RXOVW    (1U << 4)   /* RX Buffer Overflow */
#define RTL_INT_LINK     (1U << 5)   /* Link Change */
#define RTL_INT_FOVW     (1U << 6)   /* RX FIFO Overflow */
#define RTL_INT_LENCHG   (1U << 13)  /* Cable Length Change */
#define RTL_INT_TIMEOUT  (1U << 14)  /* Timer Timeout */
#define RTL_INT_SERR     (1U << 15)  /* System Error */

/* ── RX Configuration ─────────────────────────────────────────────────── */
#define RTL_RCR_AAP     (1U << 0)    /* Accept All Packets (promiscuous) */
#define RTL_RCR_APM     (1U << 1)    /* Accept Physical Match */
#define RTL_RCR_AM      (1U << 2)    /* Accept Multicast */
#define RTL_RCR_AB      (1U << 3)    /* Accept Broadcast */
#define RTL_RCR_WRAP    (1U << 7)    /* Wrap ring buffer */
#define RTL_RCR_MXDMA_UNLIMITED (7U << 8)  /* Unlimited DMA burst */
#define RTL_RCR_RBLEN_64K (3U << 11) /* RX buffer 64K */
#define RTL_RCR_RXFTH_NONE (7U << 13) /* No RX FIFO threshold */

/* ── TX Status bits ───────────────────────────────────────────────────── */
#define RTL_TSD_OWN      (1U << 13)  /* DMA operation complete */
#define RTL_TSD_TOK      (1U << 15)  /* TX OK */
#define RTL_TSD_TUN      (1U << 14)  /* TX FIFO Underrun */

/* ── RX packet header ─────────────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint16_t status;    /* bit0=ROK, bit1=FAE, bit2=CRC, etc. */
    uint16_t size;      /* Packet size including CRC (4 bytes) */
} Rtl8139RxHeader;

/* ── TX slots (RTL8139 has 4 TX descriptors) ─────────────────────────── */
#define RTL8139_NUM_TX  4
#define RTL8139_TX_BUF_SIZE 1536   /* Max Ethernet frame */

/* ── RX ring buffer size ──────────────────────────────────────────────── */
#define RTL8139_RX_BUF_SIZE  (64 * 1024 + 16 + 1500)  /* 64KB + wrap */

/* ── Driver state ─────────────────────────────────────────────────────── */
typedef struct {
    uint16_t  iobase;          /* PCI BAR0 (I/O space) */
    uint8_t   mac[6];
    uint32_t  irq;

    /* RX ring */
    uint8_t  *rx_buf;          /* Physically contiguous 64KB ring */
    uint32_t  rx_buf_phys;
    uint16_t  rx_offset;       /* Current read position in ring */

    /* TX ring (4 slots, round-robin) */
    uint8_t  *tx_buf[RTL8139_NUM_TX];
    uint32_t  tx_buf_phys[RTL8139_NUM_TX];
    uint8_t   tx_current;      /* Next TX slot to use */

    /* Statistics */
    uint64_t  rx_packets;
    uint64_t  tx_packets;
    uint64_t  rx_bytes;
    uint64_t  tx_bytes;
    uint32_t  rx_errors;
    uint32_t  tx_errors;
    uint8_t   link_up;
} Rtl8139Device;

/* ── Public API ──────────────────────────────────────────────────────── */

/** Find RTL8139 via PCI and initialize. Returns 0 on success. */
int  rtl8139_init(void);

/** Send a raw Ethernet frame (max 1500 bytes). Returns 0 on success. */
int  rtl8139_send(const void *data, uint16_t len);

/** Poll for received frame. Returns bytes received, 0 if empty. */
int  rtl8139_recv(void *buf, uint16_t buf_len);

/** IRQ handler (called from interrupt dispatch). */
void rtl8139_irq_handler(void);

/** Get MAC address. */
void rtl8139_get_mac(uint8_t mac[6]);

/** Check link state (1=up). */
int  rtl8139_link_up(void);

#endif /* _DRIVER_RTL8139_H_ */
