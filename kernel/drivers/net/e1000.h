/**
 * @file e1000.h — Xenithra OS — Intel e1000 Gigabit Ethernet Driver
 *
 * Kernel-mode driver for the Intel 82540EM / 82574L family (QEMU e1000).
 * Implements: PCI enumeration, MMIO map, TX/RX ring descriptors, IRQ handler.
 */

#ifndef _DRIVER_E1000_H_
#define _DRIVER_E1000_H_

#include <stdint.h>
#include <stddef.h>

/* ── PCI Vendor/Device IDs ───────────────────────────────────────────── */
#define E1000_VENDOR_ID   0x8086   /* Intel */
#define E1000_DEVICE_82540 0x100E  /* 82540EM (QEMU default) */
#define E1000_DEVICE_82574 0x10D3  /* 82574L */

/* ── MMIO Register Offsets ───────────────────────────────────────────── */
#define E1000_CTRL    0x0000   /* Device Control */
#define E1000_STATUS  0x0008   /* Device Status */
#define E1000_EECD    0x0010   /* EEPROM/Flash Control */
#define E1000_EERD    0x0014   /* EEPROM Read */
#define E1000_ICR     0x00C0   /* Interrupt Cause Read */
#define E1000_ICS     0x00C8   /* Interrupt Cause Set */
#define E1000_IMS     0x00D0   /* Interrupt Mask Set/Read */
#define E1000_IMC     0x00D8   /* Interrupt Mask Clear */
#define E1000_RCTL    0x0100   /* Receive Control */
#define E1000_RDBAL   0x2800   /* RX Descriptor Base Addr Low */
#define E1000_RDBAH   0x2804   /* RX Descriptor Base Addr High */
#define E1000_RDLEN   0x2808   /* RX Descriptor Ring Length */
#define E1000_RDH     0x2810   /* RX Descriptor Head */
#define E1000_RDT     0x2818   /* RX Descriptor Tail */
#define E1000_TCTL    0x0400   /* Transmit Control */
#define E1000_TDBAL   0x3800   /* TX Descriptor Base Addr Low */
#define E1000_TDBAH   0x3804   /* TX Descriptor Base Addr High */
#define E1000_TDLEN   0x3808   /* TX Descriptor Ring Length */
#define E1000_TDH     0x3810   /* TX Descriptor Head */
#define E1000_TDT     0x3818   /* TX Descriptor Tail */
#define E1000_RAL     0x5400   /* Receive Address Low (MAC) */
#define E1000_RAH     0x5404   /* Receive Address High (MAC) + AV bit */
#define E1000_MTA     0x5200   /* Multicast Table Array (128 regs × 4 bytes) */

/* ── Control Register Bits ───────────────────────────────────────────── */
#define E1000_CTRL_RST    (1U << 26)   /* Full reset */
#define E1000_CTRL_SLU    (1U << 6)    /* Set Link Up */
#define E1000_CTRL_ASDE   (1U << 5)    /* Auto Speed Detect Enable */

/* ── Receive Control Bits ────────────────────────────────────────────── */
#define E1000_RCTL_EN     (1U << 1)    /* Receiver Enable */
#define E1000_RCTL_SBP    (1U << 2)    /* Store Bad Packets */
#define E1000_RCTL_UPE    (1U << 3)    /* Unicast Promiscuous Enable */
#define E1000_RCTL_MPE    (1U << 4)    /* Multicast Promiscuous Enable */
#define E1000_RCTL_BAM    (1U << 15)   /* Accept Broadcast */
#define E1000_RCTL_BSIZE_2048  0x0     /* Buffer size 2048 */
#define E1000_RCTL_SECRC  (1U << 26)   /* Strip Ethernet CRC */

/* ── Transmit Control Bits ───────────────────────────────────────────── */
#define E1000_TCTL_EN     (1U << 1)    /* Transmit Enable */
#define E1000_TCTL_PSP    (1U << 3)    /* Pad Short Packets */
#define E1000_TCTL_CT_VAL (0x0F << 4) /* Collision Threshold = 15 */
#define E1000_TCTL_COLD_VAL (0x3F << 12) /* Collision Distance */

/* ── Interrupt Bits ──────────────────────────────────────────────────── */
#define E1000_ICR_TXDW    (1U << 0)    /* TX Descriptor Written Back */
#define E1000_ICR_RXDMT0  (1U << 4)    /* RX Descriptor Min Threshold */
#define E1000_ICR_RXT0    (1U << 7)    /* RX Timer Interrupt */
#define E1000_ICR_LSC     (1U << 2)    /* Link Status Change */

/* ── Descriptor sizes ────────────────────────────────────────────────── */
#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8
#define E1000_RX_BUF_SIZE 2048

/* ── RX Descriptor (16 bytes) ────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint64_t addr;     /* Buffer address (physical) */
    uint16_t length;   /* Bytes received */
    uint16_t checksum;
    uint8_t  status;   /* Descriptor Done = bit0, EOP = bit1 */
    uint8_t  errors;
    uint16_t special;
} E1000RxDesc;

/* ── TX Descriptor (16 bytes) ────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint64_t addr;     /* Buffer address (physical) */
    uint16_t length;   /* Bytes to send */
    uint8_t  cso;      /* Checksum Offset */
    uint8_t  cmd;      /* EOP | IFCS | RS */
    uint8_t  status;   /* Descriptor Done = bit0 */
    uint8_t  css;
    uint16_t special;
} E1000TxDesc;

/* ── Driver state ────────────────────────────────────────────────────── */
typedef struct {
    volatile uint8_t *mmio_base;   /* Mapped MMIO region */
    uint8_t           mac[6];      /* Discovered MAC from EEPROM */
    uint32_t          irq;         /* PCI IRQ line */

    /* RX ring */
    E1000RxDesc  *rx_descs;        /* RX descriptor ring (DMA-accessible) */
    uint8_t      *rx_bufs[E1000_NUM_RX_DESC]; /* DMA buffers */
    uint32_t      rx_tail;

    /* TX ring */
    E1000TxDesc  *tx_descs;
    uint32_t      tx_tail;

    /* Stats */
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_errors;
    uint64_t tx_errors;
    uint8_t  link_up;
} E1000Device;

/* ── Public API ──────────────────────────────────────────────────────── */

/** Initialize the e1000 NIC. Returns 0 on success, -1 if not found. */
int  e1000_init(void);

/** Send a raw Ethernet frame. Returns 0 on success. */
int  e1000_send(const void *data, uint16_t len);

/** Poll for a received frame. Copies into buf (max buf_len). Returns bytes. */
int  e1000_recv(void *buf, uint16_t buf_len);

/** IRQ handler — called from kernel interrupt dispatch. */
void e1000_irq_handler(void);

/** Get the MAC address (6 bytes). */
void e1000_get_mac(uint8_t mac[6]);

/** Check link status. Returns 1 if up. */
int  e1000_link_up(void);

/** Dump driver stats via kernel IPC notification. */
void e1000_dump_stats(void);

#endif /* _DRIVER_E1000_H_ */
