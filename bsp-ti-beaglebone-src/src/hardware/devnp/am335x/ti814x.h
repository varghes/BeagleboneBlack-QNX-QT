/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems. 
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"). You 
 * may not reproduce, modify or distribute this software except in 
 * compliance with the License. You may obtain a copy of the License 
 * at: http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" basis, 
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as 
 * contributors under the License or as licensors under other terms.  
 * Please review this entire file for other proprietary rights or license 
 * notices, as well as the QNX Development Suite License Guide at 
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <io-pkt/iopkt_driver.h>
#include <stdio.h>
#include <errno.h>
#include <atomic.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/siginfo.h>
#include <sys/syspage.h>
#include <sys/neutrino.h>
#include <sys/mbuf.h>
#include <sys/slogcodes.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <sys/io-pkt.h>
#include <sys/cache.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <hw/inout.h>
#include <drvr/mdi.h>
#include <drvr/eth.h>
#include <drvr/nicsupport.h>
#include <hw/nicinfo.h>
#include <sys/device.h>
#define _STDDEF_H_INCLUDED
#include <siglock.h>
#include <dev/mii/miivar.h>


#ifdef  __cplusplus
extern "C" {
#endif

/* Macros */
#define get_mac_hi(mac) (((mac)[0] << 0) | ((mac)[1] << 8) |    \
             ((mac)[2] << 16) | ((mac)[3] << 24))
#define get_mac_lo(mac) (((mac)[4] << 0) | ((mac)[5] << 8))

#define RGMII				1
#define GMII				2
#define RMII				3


/* DEBUG Facility */
#define DEBUG_MII_RW                     0x80000000
#define DEBUG_MII                        0x40000000
#define DEBUG_RX                         0x20000000
#define DEBUG_RX2                        0x10000000
#define DEBUG_DUMP_RX                    0x08000000
#define DEBUG_TX                         0x04000000
#define DEBUG_TX2                        0x02000000
#define DEBUG_TRACE                      0x01000000
#define DEBUG_MASK                       0x000000ff

#if 1
/* AM335X register address definitions */
#define TI814X_CPSW_BASE                0x4A100000
#define TI814X_CPSW_SIZE                0x8000

#define TI814X_CPSW_MDIO_OFFSET         0x00001000
#else
/* J5 register address definitions */
#define TI814X_CPSW_BASE                0x4a100000
#define TI814X_CPSW_SIZE                0x4000

#define TI814X_CPSW_MDIO_OFFSET         0x00000800
#endif

/* TI814x Interrupt Vectors */
#define TI814x_RX_THRESH_INT0   40      /* RX Threshold interrupt */
#define TI814x_RX_INT0          41      /* RX Pulse interrupt */
#define TI814x_TX_INT0          42      /* TX Pulse interrupt */
#define TI814x_MISC_INT0        43      /* Misc interrupt */
#if 0
#define TI814x_RX_THRESH_INT1   44
#define TI814x_RX_INT1          41
#define TI814x_TX_INT1          46
#define TI814x_MISC_INT1        47
#endif

//#define DEFAULT_NUM_RX_PKTS 128
//#define MAX_NUM_RX_PKTS     128
//#define DEFAULT_NUM_TX_PKTS 128
//#define MAX_NUM_TX_PKTS     128
//#define NUM_TX_PKTS_PER_IF  64
//#define TOTAL_NUM_TX_PKTS   256
//#define MAX_PKT_SIZE        1518
//#define DEFRAG_LIMIT        10
//#define MIN_PKT_SIZE        60

#define DEFAULT_NUM_RX_PKTS 128
#define MAX_NUM_RX_PKTS     128
#define DEFAULT_NUM_TX_PKTS 128
#define MAX_NUM_TX_PKTS     128
#define NUM_TX_PKTS_PER_IF  64
#define TOTAL_NUM_TX_PKTS   256
#define MAX_PKT_SIZE        1518
#define DEFRAG_LIMIT        20
#define MIN_PKT_SIZE        60		// [BA] 64 byte min frame = 60 visible + 4 byte FCS added by hardware
#define MAC_LEN             6

/* J5 GMAC offset Definitions */
#define CPSW_ID_VER                     0x00000000
    #define CPSW_ID_VER_ID(x)               ((x)&0xffff0000) >> 16
    #define CPSW_ID_VER_RTLVER(x)           ((x)&0x0000f800) >> 11
    #define CPSW_ID_VER_MAJOR(x)            ((x)&0x00000700) >> 8
    #define CPSW_ID_VER_MINOR(x)            ((x)&0x000000ff)
#define CPSW_CONTROL                    0x00000004
#define CPSW_SOFT_RESET                 0x00000008
    #define CPSW_SOFT_RESET_BIT             0x00000001
#define CPSW_STAT_PORT_EN               0x0000000c
    #define P2_STAT_EN                      0x00000004
    #define P1_STAT_EN                      0x00000002
    #define P0_STAT_EN                      0x00000001
#define CPSW_PTYPE                      0x00000010
#define CPSW_SOFT_IDLE                  0x00000014
#define CPSW_THRU_RATE                  0x00000018
#define CPSW_GAP_THRESH                 0x0000001c
#define CPSW_TX_START_WDS               0x00000020
#define CPSW_FLOW_CONTROL               0x00000024

#if 1

#define AM335X_PORTSOFFSET				0x0100

#define P0_MAX_BLKS                     (AM335X_PORTSOFFSET+0x00000008)
#define P0_BLK_CNT                      (AM335X_PORTSOFFSET+0x0000000c)
#define P0_TX_IN_CTL                    (AM335X_PORTSOFFSET+0x00000010)
#define P0_PORT_VLAN                    (AM335X_PORTSOFFSET+0x00000014)
#define P0_TX_PRI_MAP                   (AM335X_PORTSOFFSET+0x00000018)
#define CPDMA_TX_PRI_MAP                (AM335X_PORTSOFFSET+0x0000001c)
#define CPDMA_RX_CH_MAP                 (AM335X_PORTSOFFSET+0x00000020)

#define P1_MAX_BLKS                     (AM335X_PORTSOFFSET+0x00000108)
#define P1_BLK_CNT                      (AM335X_PORTSOFFSET+0x0000010c)
#define P1_TX_IN_CTL                    (AM335X_PORTSOFFSET+0x00000110)
#define P1_PORT_VLAN                    (AM335X_PORTSOFFSET+0x00000114)
#define P1_TX_PRI_MAP                   (AM335X_PORTSOFFSET+0x00000118)
//#define P1_TS_CTL_CPSW_3GF              0x00000064
//#define P1_TS_SEQ_LTYPE                 0x00000068
//#define P1_TS_VLAN                      0x0000006c
#define SL1_SA_LO                       (AM335X_PORTSOFFSET+0x00000120)
#define SL1_SA_HI                       (AM335X_PORTSOFFSET+0x00000124)
#define P1_SEND_PERCENT                 (AM335X_PORTSOFFSET+0x00000128)

#define P2_MAX_BLKS                     (AM335X_PORTSOFFSET+0x00000208)
#define P2_BLK_CNT                      (AM335X_PORTSOFFSET+0x0000020c)
#define P2_TX_IN_CTL                    (AM335X_PORTSOFFSET+0x00000210)
#define P2_PORT_VLAN                    (AM335X_PORTSOFFSET+0x00000214)
#define P2_TX_PRI_MAP                   (AM335X_PORTSOFFSET+0x00000218)
//#define P2_TS_CTL_CPSW_3GF              0x000000a4
//#define P2_TS_SEQ_LTYPE                 0x000000a8
//#define P2_TS_VLAN                      0x000000ac
#define SL2_SA_LO                       (AM335X_PORTSOFFSET+0x00000220)
#define SL2_SA_HI                       (AM335X_PORTSOFFSET+0x00000224)
#define P2_SEND_PERCENT                 (AM335X_PORTSOFFSET+0x00000228)
#else
#define P0_MAX_BLKS                     0x00000028
#define P0_BLK_CNT                      0x0000002c
#define P0_TX_IN_CTL                    0x00000030
#define P0_PORT_VLAN                    0x00000034
#define P0_TX_PRI_MAP                   0x00000038
#define CPDMA_TX_PRI_MAP                0x0000003c
#define CPDMA_RX_CH_MAP                 0x00000040

#define P1_MAX_BLKS                     0x00000050
#define P1_BLK_CNT                      0x00000054
#define P1_TX_IN_CTL                    0x00000058
#define P1_PORT_VLAN                    0x0000005c
#define P1_TX_PRI_MAP                   0x00000060
#define P1_TS_CTL_CPSW_3GF              0x00000064
#define P1_TS_SEQ_LTYPE                 0x00000068
#define P1_TS_VLAN                      0x0000006c
#define SL1_SA_LO                       0x00000070
#define SL1_SA_HI                       0x00000074
#define P1_SEND_PERCENT                 0x00000078

#define P2_MAX_BLKS                     0x00000090
#define P2_BLK_CNT                      0x00000094
#define P2_TX_IN_CTL                    0x00000098
#define P2_PORT_VLAN                    0x0000009c
#define P2_TX_PRI_MAP                   0x000000a0
#define P2_TS_CTL_CPSW_3GF              0x000000a4
#define P2_TS_SEQ_LTYPE                 0x000000a8
#define P2_TS_VLAN                      0x000000ac
#define SL2_SA_LO                       0x000000b0
#define SL2_SA_HI                       0x000000b4
#define P2_SEND_PERCENT                 0x000000b8
#endif

#define AM335X_CPDMAOFF					0x0800

/* CPDMA_REGS @ CPSW + 0x100 */
#if 1
#define TX_IDVER                        (AM335X_CPDMAOFF+0x00000000)
#define TX_CONTROL                      (AM335X_CPDMAOFF+0x00000004)
    #define TX_EN                           0x00000001
#define TX_TEARDOWN                     (AM335X_CPDMAOFF+0x00000008)
#define RX_IDVER                        (AM335X_CPDMAOFF+0x00000010)
#define RX_CONTROL                      (AM335X_CPDMAOFF+0x00000014)
    #define RX_EN                           0x00000001
#define RX_TEARDOWN                     (AM335X_CPDMAOFF+0x00000018)
#define CPDMA_SOFT_RESET                (AM335X_CPDMAOFF+0x0000001c)
#define DMA_SOFT_RESET                  (AM335X_CPDMAOFF+0x0000001c)
	#define CPDMA_SOFT_RESET_BIT		0x00000001
	#define DMA_SOFT_RESET_BIT          0x00000001
#define DMA_CONTROL                     (AM335X_CPDMAOFF+0x00000020)
#define DMA_STATUS                      (AM335X_CPDMAOFF+0x00000024)
    #define DMA_STAT_RXERR_CH_MSK           0x00000700
    #define DMA_STAT_RXERR_CODE_MSK         0x0000f000
        #define DMA_STAT_RXERR_NOOWN            0x0010
        #define DMA_STAT_RXERR_ZERO_BUF         0x0100
        #define DMA_STAT_RXERR_ZERO_LEN         0x0101
        #define DMA_STAT_RXERR_SOP              0x0110
    #define DMA_STAT_TXERR_CH_MSK           0x00070000
    #define DMA_STAT_TXERR_CODE_MSK         0x00f00000
    #define DMA_STAT_IDLE                   0x80000000
#define RX_BUFFER_OFFSET                (AM335X_CPDMAOFF+0x00000028)
#define EM_CONTROL                      (AM335X_CPDMAOFF+0x0000002c)

#define TX_INTSTAT_RAW                  (AM335X_CPDMAOFF+0x00000080)
#define TX_INTSTAT_MASKED               (AM335X_CPDMAOFF+0x00000084)
#define TX_INTSTAT_SET                  (AM335X_CPDMAOFF+0x00000088)
#define TX_INTSTAT_CLEAR                (AM335X_CPDMAOFF+0x0000008c)
#define CPDMA_IN_VECTOR                 (AM335X_CPDMAOFF+0x00000090)
#define CPDMA_EOI_VECTOR                (AM335X_CPDMAOFF+0x00000094)

#define RX_INTSTAT_RAW                  (AM335X_CPDMAOFF+0x000000a0)
    #define RX7_THRESH_PEND             0x00008000
    #define RX6_THRESH_PEND             0x00004000
    #define RX5_THRESH_PEND             0x00002000
    #define RX4_THRESH_PEND             0x00001000
    #define RX3_THRESH_PEND             0x00000800
    #define RX2_THRESH_PEND             0x00000400
    #define RX1_THRESH_PEND             0x00000200
    #define RX0_THRESH_PEND             0x00000100
    #define RX7_PEND                    0x00000080
    #define RX6_PEND                    0x00000040
    #define RX5_PEND                    0x00000020
    #define RX4_PEND                    0x00000010
    #define RX3_PEND                    0x00000008
    #define RX2_PEND                    0x00000004
    #define RX1_PEND                    0x00000002
    #define RX0_PEND                    0x00000001
#define RX_INTSTAT_MASKED               (AM335X_CPDMAOFF+0x000000a4)
#define RX_INTMASK_SET                  (AM335X_CPDMAOFF+0x000000a8)
    #define RX7_THRESH_PEND_MASK        0x00008000
    #define RX6_THRESH_PEND_MASK        0x00004000
    #define RX5_THRESH_PEND_MASK        0x00002000
    #define RX4_THRESH_PEND_MASK        0x00001000
    #define RX3_THRESH_PEND_MASK        0x00000800
    #define RX2_THRESH_PEND_MASK        0x00000400
    #define RX1_THRESH_PEND_MASK        0x00000200
    #define RX0_THRESH_PEND_MASK        0x00000100
    #define RX7_PEND_MASK               0x00000080
    #define RX6_PEND_MASK               0x00000040
    #define RX5_PEND_MASK               0x00000020
    #define RX4_PEND_MASK               0x00000010
    #define RX3_PEND_MASK               0x00000008
    #define RX2_PEND_MASK               0x00000004
    #define RX1_PEND_MASK               0x00000002
    #define RX0_PEND_MASK               0x00000001
#define RX_INTMASK_CLEAR                (AM335X_CPDMAOFF+0x000000ac)
#define DMA_INTSTAT_RAW                 (AM335X_CPDMAOFF+0x000000b0)
#define DMA_INTSTAT_MASKED              (AM335X_CPDMAOFF+0x000000b4)
#define DMA_INTSTAT_SET                 (AM335X_CPDMAOFF+0x000000b8)
    #define HOST_ERROR_INT_MASK             0x00000002
#define DMA_INTSTAT_CLEAR               (AM335X_CPDMAOFF+0x000000bc)
#else
#define TX_IDVER                        0x00000100
#define TX_CONTROL                      0x00000104
    #define TX_EN                           0x00000001
#define TX_TEARDOWN                     0x00000108
#define RX_IDVER                        0x00000110
#define RX_CONTROL                      0x00000114
    #define RX_EN                           0x00000001
#define RX_TEARDOWN                     0x00000118
#define RX_SOFT_RESET                   0x0000011c
#define DMA_CONTROL                     0x00000120
#define DMA_STATUS                      0x00000124
    #define DMA_STAT_RXERR_CH_MSK           0x00000700
    #define DMA_STAT_RXERR_CODE_MSK         0x0000f000
        #define DMA_STAT_RXERR_NOOWN            0x0010
        #define DMA_STAT_RXERR_ZERO_BUF         0x0100
        #define DMA_STAT_RXERR_ZERO_LEN         0x0101
        #define DMA_STAT_RXERR_SOP              0x0110
    #define DMA_STAT_TXERR_CH_MSK           0x00070000
    #define DMA_STAT_TXERR_CODE_MSK         0x00f00000
    #define DMA_STAT_IDLE                   0x80000000
#define RX_BUFFER_OFFSET                0x00000128
#define EM_CONTROL                      0x0000012c

#define TX_INTSTAT_RAW                  0x00000180
#define TX_INTSTAT_MASKED               0x00000184
#define TX_INTSTAT_SET                  0x00000188
#define TX_INTSTAT_CLEAR                0x0000018c
#define CPDMA_IN_VECTOR                 0x00000190
#define CPDMA_EOI_VECTOR                0x00000194

#define RX_INTSTAT_RAW                  0x000001a0
    #define RX7_THRESH_PEND             0x00008000
    #define RX6_THRESH_PEND             0x00004000
    #define RX5_THRESH_PEND             0x00002000
    #define RX4_THRESH_PEND             0x00001000
    #define RX3_THRESH_PEND             0x00000800
    #define RX2_THRESH_PEND             0x00000400
    #define RX1_THRESH_PEND             0x00000200
    #define RX0_THRESH_PEND             0x00000100
    #define RX7_PEND                    0x00000080
    #define RX6_PEND                    0x00000040
    #define RX5_PEND                    0x00000020
    #define RX4_PEND                    0x00000010
    #define RX3_PEND                    0x00000008
    #define RX2_PEND                    0x00000004
    #define RX1_PEND                    0x00000002
    #define RX0_PEND                    0x00000001
#define RX_INTSTAT_MASKED               0x000001a4
#define RX_INTMASK_SET                  0x000001a8
    #define RX7_THRESH_PEND_MASK        0x00008000
    #define RX6_THRESH_PEND_MASK        0x00004000
    #define RX5_THRESH_PEND_MASK        0x00002000
    #define RX4_THRESH_PEND_MASK        0x00001000
    #define RX3_THRESH_PEND_MASK        0x00000800
    #define RX2_THRESH_PEND_MASK        0x00000400
    #define RX1_THRESH_PEND_MASK        0x00000200
    #define RX0_THRESH_PEND_MASK        0x00000100
    #define RX7_PEND_MASK               0x00000080
    #define RX6_PEND_MASK               0x00000040
    #define RX5_PEND_MASK               0x00000020
    #define RX4_PEND_MASK               0x00000010
    #define RX3_PEND_MASK               0x00000008
    #define RX2_PEND_MASK               0x00000004
    #define RX1_PEND_MASK               0x00000002
    #define RX0_PEND_MASK               0x00000001
#define RX_INTMASK_CLEAR                0x000001ac
#define DMA_INTSTAT_RAW                 0x000001b0
#define DMA_INTSTAT_MASKED              0x000001b4
#define DMA_INTSTAT_SET                 0x000001b8
    #define HOST_ERROR_INT_MASK             0x00000002
#define DMA_INTSTAT_CLEAR               0x000001bc
#endif

/* CPDMA_STATERAM @ CPSW + 0x200 */
#if 1
#define TX0_HDP                         (AM335X_CPDMAOFF+0x00000200)
#define TX1_HDP                         (AM335X_CPDMAOFF+0x00000204)
#define TX2_HDP                         (AM335X_CPDMAOFF+0x00000208)
#define TX3_HDP                         (AM335X_CPDMAOFF+0x0000020c)
#define TX4_HDP                         (AM335X_CPDMAOFF+0x00000210)
#define TX5_HDP                         (AM335X_CPDMAOFF+0x00000214)
#define TX6_HDP                         (AM335X_CPDMAOFF+0x00000218)
#define TX7_HDP                         (AM335X_CPDMAOFF+0x0000021c)
#define RX0_HDP                         (AM335X_CPDMAOFF+0x00000220)
#define RX1_HDP                         (AM335X_CPDMAOFF+0x00000224)
#define RX2_HDP                         (AM335X_CPDMAOFF+0x00000228)
#define RX3_HDP                         (AM335X_CPDMAOFF+0x0000022c)
#define RX4_HDP                         (AM335X_CPDMAOFF+0x00000230)
#define RX5_HDP                         (AM335X_CPDMAOFF+0x00000234)
#define RX6_HDP                         (AM335X_CPDMAOFF+0x00000238)
#define RX7_HDP                         (AM335X_CPDMAOFF+0x0000023c)
#define TX0_CP                          (AM335X_CPDMAOFF+0x00000240)
#define TX1_CP                          (AM335X_CPDMAOFF+0x00000244)
#define TX2_CP                          (AM335X_CPDMAOFF+0x00000248)
#define TX3_CP                          (AM335X_CPDMAOFF+0x0000024C)
#define TX4_CP                          (AM335X_CPDMAOFF+0x00000250)
#define TX5_CP                          (AM335X_CPDMAOFF+0x00000254)
#define TX6_CP                          (AM335X_CPDMAOFF+0x00000258)
#define TX7_CP                          (AM335X_CPDMAOFF+0x0000025C)
#define RX0_CP                          (AM335X_CPDMAOFF+0x00000260)
#define RX1_CP                          (AM335X_CPDMAOFF+0x00000264)
#define RX2_CP                          (AM335X_CPDMAOFF+0x00000268)
#define RX3_CP                          (AM335X_CPDMAOFF+0x0000026C)
#define RX4_CP                          (AM335X_CPDMAOFF+0x00000270)
#define RX5_CP                          (AM335X_CPDMAOFF+0x00000274)
#define RX6_CP                          (AM335X_CPDMAOFF+0x00000278)
#define RX7_CP                          (AM335X_CPDMAOFF+0x0000027C)

#if 0
/* CPSW_STATS @ CPSW + 0x400 */
#define RXGOODFRAMES                    (AM335X_CPDMAOFF+0x00000C00)
#define RXBROADCASTFRAMES               (AM335X_CPDMAOFF+0x00000C04)
#define RXMULTICASTFRAMES               (AM335X_CPDMAOFF+0x00000C08)
#define RXPAUSEFRAMES                   (AM335X_CPDMAOFF+0x00000C0c)
#define RXCRCERRORS                     (AM335X_CPDMAOFF+0x00000C10)
#define RXALIGNCODEERRORS               (AM335X_CPDMAOFF+0x00000C14)
#define RXOVERSIZEDFRAMES               (AM335X_CPDMAOFF+0x00000C18)
#define RXJABBERFRAMES                  (AM335X_CPDMAOFF+0x00000C1c)
#define RXUNDERSIZEDFRAMES              (AM335X_CPDMAOFF+0x00000C20)
#define RXFRAGMENTS                     (AM335X_CPDMAOFF+0x00000C24)
#define RXOCTETS                        (AM335X_CPDMAOFF+0x00000C30)
#define TXGOODFRAMES                    (AM335X_CPDMAOFF+0x00000C34)
#define TXBROADCASTFRAMES               (AM335X_CPDMAOFF+0x00000C38)
#define TXMULTICASTFRAMES               (AM335X_CPDMAOFF+0x00000C3c)
#define TXPAUSEFRAMES                   (AM335X_CPDMAOFF+0x00000C40)
#define TXDEFERREDFRAMES                (AM335X_CPDMAOFF+0x00000C44)
#define TXCOLLISIONFRAMES               (AM335X_CPDMAOFF+0x00000C48)
#define TXSINGLECOLLFRAMES              (AM335X_CPDMAOFF+0x00000C4c)
#define TXMULTCOLLFRAMES                (AM335X_CPDMAOFF+0x00000C50)
#define TXEXCESSIVECOLLISIONS           (AM335X_CPDMAOFF+0x00000C54)
#define TXLATECOLLISION                 (AM335X_CPDMAOFF+0x00000C58)
#define TXUNDERRUN                      (AM335X_CPDMAOFF+0x00000C5c)
#define TXCARRIERSENSEERRORS            (AM335X_CPDMAOFF+0x00000C60)
#define TXOCTETS                        (AM335X_CPDMAOFF+0x00000C64)
#define OCTETFRAMES64                   (AM335X_CPDMAOFF+0x00000C68)
#define OCTETFRAMES65TO127              (AM335X_CPDMAOFF+0x00000C6c)
#define OCTETFRAMES128TO255             (AM335X_CPDMAOFF+0x00000C70)
#define OCTETFRAMES256TO511             (AM335X_CPDMAOFF+0x00000C74)
#define OCTETFRAMES512TO1023            (AM335X_CPDMAOFF+0x00000C78)
#define OCTETFRAMES1024TUP              (AM335X_CPDMAOFF+0x00000C7c)
#define NETOCTETS                       (AM335X_CPDMAOFF+0x00000C80)
#define RXSOFOVERRUNS                   (AM335X_CPDMAOFF+0x00000C84)
#define RXMOFOVERRUNS                   (AM335X_CPDMAOFF+0x00000C88)
#define RXDMAOVERRUNS                   (AM335X_CPDMAOFF+0x00000C8c)
#endif
#else
#define TX0_HDP                         0x00000200
#define TX1_HDP                         0x00000204
#define TX2_HDP                         0x00000208
#define TX3_HDP                         0x0000020c
#define TX4_HDP                         0x00000210
#define TX5_HDP                         0x00000214
#define TX6_HDP                         0x00000218
#define TX7_HDP                         0x0000021c
#define RX0_HDP                         0x00000220
#define RX1_HDP                         0x00000224
#define RX2_HDP                         0x00000228
#define RX3_HDP                         0x0000022c
#define RX4_HDP                         0x00000230
#define RX5_HDP                         0x00000234
#define RX6_HDP                         0x00000238
#define RX7_HDP                         0x0000023c
#define TX0_CP                          0x00000240
#define TX1_CP                          0x00000244
#define TX2_CP                          0x00000248
#define TX3_CP                          0x0000024C
#define TX4_CP                          0x00000250
#define TX5_CP                          0x00000254
#define TX6_CP                          0x00000258
#define TX7_CP                          0x0000025C
#define RX0_CP                          0x00000260
#define RX1_CP                          0x00000264
#define RX2_CP                          0x00000268
#define RX3_CP                          0x0000026C
#define RX4_CP                          0x00000270
#define RX5_CP                          0x00000274
#define RX6_CP                          0x00000278
#define RX7_CP                          0x0000027C

/* CPSW_STATS @ CPSW + 0x400 */
#define RXGOODFRAMES                    0x00000400
#define RXBROADCASTFRAMES               0x00000404
#define RXMULTICASTFRAMES               0x00000408
#define RXPAUSEFRAMES                   0x0000040c
#define RXCRCERRORS                     0x00000410
#define RXALIGNCODEERRORS               0x00000414
#define RXOVERSIZEDFRAMES               0x00000418
#define RXJABBERFRAMES                  0x0000041c
#define RXUNDERSIZEDFRAMES              0x00000420
#define RXFRAGMENTS                     0x00000424
#define RXOCTETS                        0x00000430
#define TXGOODFRAMES                    0x00000434
#define TXBROADCASTFRAMES               0x00000438
#define TXMULTICASTFRAMES               0x0000043c
#define TXPAUSEFRAMES                   0x00000440
#define TXDEFERREDFRAMES                0x00000444
#define TXCOLLISIONFRAMES               0x00000448
#define TXSINGLECOLLFRAMES              0x0000044c
#define TXMULTCOLLFRAMES                0x00000450
#define TXEXCESSIVECOLLISIONS           0x00000454
#define TXLATECOLLISION                 0x00000458
#define TXUNDERRUN                      0x0000045c
#define TXCARRIERSENSEERRORS            0x00000460
#define TXOCTETS                        0x00000464
#define OCTETFRAMES64                   0x00000468
#define OCTETFRAMES65TO127              0x0000046c
#define OCTETFRAMES128TO255             0x00000470
#define OCTETFRAMES256TO511             0x00000474
#define OCTETFRAMES512TO1023            0x00000478
#define OCTETFRAMES1024TUP              0x0000047c
#define NETOCTETS                       0x00000480
#define RXSOFOVERRUNS                   0x00000484
#define RXMOFOVERRUNS                   0x00000488
#define RXDMAOVERRUNS                   0x0000048c  
#endif

/* CPTS @ CPSW + 0x500 */

#define AM335X_ALEOFFSET				0x0D00

/* ALE_REGS @ CPSW + 0x600 */
#if 1
#define ALE_IDVER                       (AM335X_ALEOFFSET+0x00000000)
#define ALE_CONTROL                     (AM335X_ALEOFFSET+0x00000008)
    #define ENABLE_ALE                      0x80000000
    #define CLEAR_TABLE                     0x40000000
    #define ALE_BYPASS                      0x00000010
#define ALE_PRESCALE                    (AM335X_ALEOFFSET+0x00000010)
#define ALE_UNKNOWN_VLAN                (AM335X_ALEOFFSET+0x00000018)
#define ALE_TBLCTL                      (AM335X_ALEOFFSET+0x00000020)
    #define WRITE_RDZ_WRITE                 0x80000000
    #define ENTRY_MASK                      0x000003ff
#define ALE_TBLW2                       (AM335X_ALEOFFSET+0x00000034)
    #define TBLW2_MASK                      0x000000ff
    #define TBLW2_PORT_MASK                 0x00000003
    #define TBLW2_START_BIT                 64
#define ALE_TBLW1                       (AM335X_ALEOFFSET+0x00000038)
    #define ENTRY_TYPE_UNICAST              0x10000000
#define ALE_TBLW0                       (AM335X_ALEOFFSET+0x0000003c)
#define ALE_PORTCTL0                    (AM335X_ALEOFFSET+0x00000040)
    #define PORT_STATE_FORWARD              0x00000003
    #define PORT_STATE_LEARN                0x00000002
    #define PORT_STATE_BLOCKED              0x00000001
    #define PORT_STATE_DISABLED             0x00000000
#define ALE_PORTCTL1                    (AM335X_ALEOFFSET+0x00000044)
#define ALE_PORTCTL2                    (AM335X_ALEOFFSET+0x00000048)
#else
#define ALE_IDVER                       0x00000600
#define ALE_CONTROL                     0x00000608
    #define ENABLE_ALE                      0x80000000
    #define CLEAR_TABLE                     0x40000000
    #define ALE_BYPASS                      0x00000010
#define ALE_PRESCALE                    0x00000610
#define ALE_UNKNOWN_VLAN                0x00000618
#define ALE_TBLCTL                      0x00000620
    #define WRITE_RDZ_WRITE                 0x80000000
    #define ENTRY_MASK                      0x000003ff
#define ALE_TBLW2                       0x00000634
    #define TBLW2_MASK                      0x000000ff
    #define TBLW2_PORT_MASK                 0x00000003
    #define TBLW2_START_BIT                 64
#define ALE_TBLW1                       0x00000638
    #define ENTRY_TYPE_UNICAST              0x10000000
#define ALE_TBLW0                       0x0000063c
#define ALE_PORTCTL0                    0x00000640
    #define PORT_STATE_FORWARD              0x00000003
    #define PORT_STATE_LEARN                0x00000002
    #define PORT_STATE_BLOCKED              0x00000001
    #define PORT_STATE_DISABLED             0x00000000
#define ALE_PORTCTL1                    0x00000644
#define ALE_PORTCTL2                    0x00000648
#endif

/* ALE Table entry definitions */
#define PORT_MASK_BIT_OFFSET            66
/* Table entry types */
#define ALE_TYPE_FREE                   0x00000000
#define ALE_TYPE_ADDR                   0x00000001
#define ALE_TYPE_VLAN                   0x00000002
#define ALE_TYPE_VLAN_ADDR              0x00000003
/* Unicast Types */
#define ALE_UNICAST                     0x00000000
#define ALE_UNICAST_AGE_NOT_TOUCHED     0x00000001
#define ALE_UNICAST_OUI                 0x00000002
#define ALE_UNICAST_AGE_TOUCHED         0x00000003
/* Multicast/FWD states */
#define ALE_STATE_FWD                   0x00000000
#define ALE_STATE_BLK_FWD_LRN           0x00000001
#define ALE_STATE_FWD_LRN               0x00000002
#define ALE_STATE_FWD_2                 0x00000003

#define ALE_SECURE_FLAG                 0x00000001
#define ALE_BLOCKED_FLAG                0x00000002

#define ALE_ENTRY_WORDS                 3
#define ALE_ENTRIES                     1024

#define AM335X_SLX1OFFSET				0x0D80
#define AM335X_SLX2OFFSET				0x0DC0

/* SLX_REGS @ CPSW + 0x700 */
#if 1
#define SL1_IDVER                       (AM335X_SLX1OFFSET+0x00000000)
#define SL1_MAC_CONTROL                 (AM335X_SLX1OFFSET+0x00000004)
    #define FULLDUPLEX                      0x00000001
    #define GMII_EN                         0x00000020
    #define GIG_MODE                        0x00000080
    #define MACIFCTL_A                      0x00008000
    #define EXT_EN                          0x00040000
#define SL1_MAC_STATUS                  (AM335X_SLX1OFFSET+0x00000008)
#define SL1_SOFT_RESET                  (AM335X_SLX1OFFSET+0x0000000c)
    #define SL_SOFT_RESET_BIT               0x00000001
#define SL1_RX_MAXLEN                   (AM335X_SLX1OFFSET+0x00000010)
#define SL1_BOFFTEST                    (AM335X_SLX1OFFSET+0x00000014)
#define SL1_RX_PAUSE                    (AM335X_SLX1OFFSET+0x00000018)
#define SL1_TX_PAUSE                    (AM335X_SLX1OFFSET+0x0000001c)
#define SL1_EM_CONTROL                  (AM335X_SLX1OFFSET+0x00000020)
#define SL1_RX_PRI_MAP                  (AM335X_SLX1OFFSET+0x00000024)

#define SL2_IDVER                       (AM335X_SLX2OFFSET+0x00000000)
#define SL2_MAC_CONTROL                 (AM335X_SLX2OFFSET+0x00000004)
#define SL2_MAC_STATUS                  (AM335X_SLX2OFFSET+0x00000008)
#define SL2_SOFT_RESET                  (AM335X_SLX2OFFSET+0x0000000c)
#define SL2_RX_MAXLEN                   (AM335X_SLX2OFFSET+0x00000010)
#define SL2_BOFFTEST                    (AM335X_SLX2OFFSET+0x00000014)
#define SL2_RX_PAUSE                    (AM335X_SLX2OFFSET+0x00000018)
#define SL2_TX_PAUSE                    (AM335X_SLX2OFFSET+0x0000001c)
#define SL2_EM_CONTROL                  (AM335X_SLX2OFFSET+0x00000010)
#define SL2_RX_PRI_MAP                  (AM335X_SLX2OFFSET+0x00000024)
#else
#define SL1_IDVER                       0x00000700
#define SL1_MAC_CONTROL                 0x00000704
    #define FULLDUPLEX                      0x00000001
    #define GMII_EN                         0x00000020
    #define GIG_MODE                        0x00000080
    #define MACIFCTL_A                      0x00008000
    #define EXT_EN                          0x00040000
#define SL1_MAC_STATUS                  0x00000708
#define SL1_SOFT_RESET                  0x0000070c
    #define SL_SOFT_RESET_BIT               0x00000001
#define SL1_RX_MAXLEN                   0x00000710
#define SL1_BOFFTEST                    0x00000714
#define SL1_RX_PAUSE                    0x00000718
#define SL1_TX_PAUSE                    0x0000071c
#define SL1_EM_CONTROL                  0x00000720
#define SL1_RX_PRI_MAP                  0x00000724

#define SL2_IDVER                       0x00000740
#define SL2_MAC_CONTROL                 0x00000744
#define SL2_MAC_STATUS                  0x00000748
#define SL2_SOFT_RESET                  0x0000074c
#define SL2_RX_MAXLEN                   0x00000750
#define SL2_BOFFTEST                    0x00000754
#define SL2_RX_PAUSE                    0x00000758
#define SL2_TX_PAUSE                    0x0000075c
#define SL2_EM_CONTROL                  0x00000760
#define SL2_RX_PRI_MAP                  0x00000764
#endif

#define AM335X_MDIOOFFSET				0x1000

/* MDIO REgisters @ 0x800 */
#if 1
#define MDIOVER                         (AM335X_MDIOOFFSET+0x00000000)
#define MDIOCONTROL                     (AM335X_MDIOOFFSET+0x00000004)
    #define MDIO_IDLE                       0x80000000
    #define MDIO_ENABLE                     0x40000000
    #define MDIO_FAULT                      0x00800000
    #define MDIO_FAULTENB                   0x00400000
    #define MDIO_CLK
    #define MDIO_ENABLE                     0x40000000
    #define MDIO_FAULT                      0x00800000
    #define MDIO_FAULTENB                   0x00400000
	// RMII Interface Clock: 50 Mhz
	// Peripheral Clock Frequency: 166 Mhz
	// MDIO Typical: 1Mhz, works up to 2.5Mhz
	// MDIO_CLK frequency = peripheral clock frequency/(CLKDIV + 1).
//    #define PER_CLK_FREQ_HZ     166000000
	#define PER_CLK_FREQ_HZ     125000000
    #define MDIO_CLK_FREQ_HZ    1000000
    #define MDIO_CLK_DIVISOR                ((PER_CLK_FREQ_HZ / MDIO_CLK_FREQ_HZ) - 1)

#define MDIOALIVE                       (AM335X_MDIOOFFSET+0x00000008)
#define MDIOLINK                        (AM335X_MDIOOFFSET+0x0000000c)
#define MDIOLINKINTRAW                  (AM335X_MDIOOFFSET+0x00000010)
#define MDIOLINKINTMASKED               (AM335X_MDIOOFFSET+0x00000014)
#define MDIOUSERINTRAW                  (AM335X_MDIOOFFSET+0x00000020)
#define MDIOUSERINTMASKED               (AM335X_MDIOOFFSET+0x00000024)
#define MDIOUSERINTMASKSET              (AM335X_MDIOOFFSET+0x00000028)
#define MDIOUSERINTMASKCLR              (AM335X_MDIOOFFSET+0x0000002c)
#define MDIOUSERACCESS0                 (AM335X_MDIOOFFSET+0x00000080)
    #define GO                              0x80000000
    #define WRITE                           0x40000000
    #define ACK                             0x20000000
    #define DATAMASK                        0x0000ffff
    #define REGADR(x)                      (x << 21)
    #define PHYADR(x)                      (x << 16)
#define MDIOUSERPHYSEL0                 (AM335X_MDIOOFFSET+0x00000084)
#define MDIOUSERACCESS1                 (AM335X_MDIOOFFSET+0x00000088)
#define MDIOUSERPHYSEL1                 (AM335X_MDIOOFFSET+0x0000008c)
#else
#define MDIOVER                         0x00000800
#define MDIOCONTROL                     0x00000804
    #define MDIO_IDLE                       0x8000000
    #define MDIO_ENABLE                     0x40000000
    #define MDIO_FAULT                      0x00800000
    #define MDIO_FAULTENB                   0x00400000
    #define MDIO_CLK
    #define MDIO_ENABLE                     0x40000000
    #define MDIO_FAULT                      0x00800000
    #define MDIO_FAULTENB                   0x00400000
    // MDIO Typical: 1Mhz, works up to 2.5Mhz
    // MDIO_CLK frequency = peripheral clock frequency/(CLKDIV + 1).
    #define PER_CLK_FREQ_HZ     166000000
    #define MDIO_CLK_FREQ_HZ    1000000
    #define MDIO_CLK_DIVISOR                ((PER_CLK_FREQ_HZ / MDIO_CLK_FREQ_HZ) - 1)
#define MDIOALIVE                       0x00000808
#define MDIOLINK                        0x0000080c
#define MDIOLINKINTRAW                  0x00000810
#define MDIOLINKINTMASKED               0x00000814
#define MDIOUSERINTRAW                  0x00000820
#define MDIOUSERINTMASKED               0x00000824
#define MDIOUSERINTMASKSET              0x00000828
#define MDIOUSERINTMASKCLR              0x0000082c
#define MDIOUSERACCESS0                 0x00000880
    #define GO                              0x80000000
    #define WRITE                           0x40000000
    #define ACK                             0x20000000
    #define DATAMASK                        0x0000ffff
    #define REGADR(x)                      (x << 21)
    #define PHYADR(x)                      (x << 16)
#define MDIOUSERPHYSEL0                 0x00000884
#define MDIOUSERACCESS1                 0x00000888
#define MDIOUSERPHYSEL1                 0x0000088c
#endif

#define AM335X_WLOFFSET					0x1200

/* GMAC Subsystem Register Offset Definitions @ CPSW+0x900*/
#if 1
#define CPSW_SS_IDVER                   (AM335X_WLOFFSET+0x00000000)
#define CPSW_SS_SOFT_RESET              (AM335X_WLOFFSET+0x00000004)
    #define CPSW_SS_SOFT_RESET_BIT               0x00000001
#define CPSW_SS_CONTROL                 (AM335X_WLOFFSET+0x00000008)
#define CPSW_SS_INT_CONTROL             (AM335X_WLOFFSET+0x0000000c)
#define CO_RX_THRES_EN                  (AM335X_WLOFFSET+0x00000010)
#define C0_RX_EN                        (AM335X_WLOFFSET+0x00000014)
    #define RX_EN_CH0                       0x00000001
    #define RX_EN_CH1                       0x00000002
    #define RX_EN_CH2                       0x00000004
    #define RX_EN_CH3                       0x00000008
    #define RX_EN_CH4                       0x00000010
    #define RX_EN_CH5                       0x00000020
    #define RX_EN_CH6                       0x00000040
    #define RX_EN_CH7                       0x00000080
#define C0_TX_EN                        (AM335X_WLOFFSET+0x00000018)
    #define TX_EN_CH0                       0x00000001
    #define TX_EN_CH1                       0x00000002
    #define TX_EN_CH2                       0x00000004
    #define TX_EN_CH3                       0x00000008
    #define TX_EN_CH4                       0x00000010
    #define TX_EN_CH5                       0x00000020
    #define TX_EN_CH6                       0x00000040
    #define TX_EN_CH7                       0x00000080
#define C0_MISC_EN                      (AM335X_WLOFFSET+0x0000001c)
#define C1_RX_THRESH_EN                 (AM335X_WLOFFSET+0x00000020)
#define C1_RX_EN                        (AM335X_WLOFFSET+0x00000024)
#define C2_RX_EN                        (AM335X_WLOFFSET+0x00000034)
#else
#define CPSW_SS_IDVER                    0x00000900
#define CPSW_SS_SOFT_RESET               0x00000904
    #define CPSW_SS_SOFT_RESET_BIT               0x00000001
#define CPSW_SS_CONTROL                 0x00000908
#define CPSW_SS_INT_CONTROL             0x0000090c
#define CO_RX_THRES_EN                  0x00000910
#define C0_RX_EN                        0x00000914
    #define RX_EN_CH0                       0x00000001
    #define RX_EN_CH1                       0x00000002
    #define RX_EN_CH2                       0x00000004
    #define RX_EN_CH3                       0x00000008
    #define RX_EN_CH4                       0x00000010
    #define RX_EN_CH5                       0x00000020
    #define RX_EN_CH6                       0x00000040
    #define RX_EN_CH7                       0x00000080
#define C0_TX_EN                        0x00000918
    #define TX_EN_CH0                       0x00000001
    #define TX_EN_CH1                       0x00000002
    #define TX_EN_CH2                       0x00000004
    #define TX_EN_CH3                       0x00000008
    #define TX_EN_CH4                       0x00000010
    #define TX_EN_CH5                       0x00000020
    #define TX_EN_CH6                       0x00000040
    #define TX_EN_CH7                       0x00000080
#define C0_MISC_EN                      0x0000091c
#define C1_RX_THRESH_EN                 0x00000920
#define C1_RX_EN                        0x00000924
#define C2_RX_EN                        0x00000934
#endif

// mdio clock divide down value.

#if 1
// RMII Interface Clock: 50 Mhz
// Peripheral Clock Frequency: 166 Mhz
// MDIO Typical: 1Mhz, works up to 2.5Mhz
// MDIO_CLK frequency = peripheral clock frequency/(CLKDIV + 1).
//#define PER_CLK_FREQ_HZ     166000000
//#define PER_CLK_FREQ_HZ     200000000
//#define MDIO_CLK_FREQ_HZ    1000000
//#define MDIO_CLK_DIVISOR    ((PER_CLK_FREQ_HZ / MDIO_CLK_FREQ_HZ) - 1)

/* CPPI Descriptor */
#define CPPI_DESC_PHYS      0x4A102000      /* Physical address of descriptors in CPPI RAM */
#define CPPI_DESC_MEM_SIZE  0x00002000      /* Size of CPPI RAM memory for descriptors */
#define CPPI_RX_DESC_OFFSET 0x00000000
#define CPPI_TX_DESC_OFFSET 0x00001000
typedef struct _cppi_desc_t {
    uint32_t    next;               /* Pointer to next descriptor */
    uint32_t    buffer;             /* Pointer to data buffer */
    uint32_t    off_len;            /* Buffer offset and length */
    uint32_t    flag_len;           /* Packet flag and length */
    } cppi_desc_t;

/* Packet flags */
#define DESC_FLAG_SOP           (1 << 31)   /* Start of packet */
#define DESC_FLAG_EOP           (1 << 30)   /* End of packet */
#define DESC_FLAG_OWN           (1 << 29)   /* Ownership flag */
#define DESC_FLAG_EOQ           (1 << 28)   /* End of queue flag */
#define DESC_FLAG_TDOWNCMPLT    (1 << 27)   /* Teardown complete flag */
#define DESC_FLAG_PASSCRC       (1 << 26)   /* Pass CRC flag */
#define DESC_FLAG_JABBER        (1 << 25)   /* Jabber flag */
#define DESC_FLAG_OVERSIZE      (1 << 24)   /* Oversize flag */
#define DESC_FLAG_FRAGMENT      (1 << 23)   /* Fragment flag */
#define DESC_FLAG_UNDERSIZE     (1 << 22)   /* Undersize flag */
#define DESC_FLAG_CONTROL       (1 << 21)   /* Control flag */
#define DESC_FLAG_OVERRUN       (1 << 20)   /* Overrun flag */
#define DESC_FLAG_TOPORT_EN     (1 << 20)   /* to-port-enable for TX descriptors */
#define DESC_FLAG_CODE_ERR      (1 << 19)   /* Code error flag */
#define DESC_FLAG_ALIGN_ERR     (1 << 18)   /* Alignment error flag */
#define DESC_FLAG_CRC_ERR       (1 << 17)   /* CRC Error flag */
#define DESC_FLAG_NOMATCH       (1 << 16)   /* No match flag */
#define DESC_FLAG_TO_PORT(x)    (x << 16)   /* To port */

#define RX_ERROR    (DESC_FLAG_OVERSIZE | DESC_FLAG_UNDERSIZE | DESC_FLAG_OVERRUN | \
                    DESC_FLAG_ALIGN_ERR | DESC_FLAG_CRC_ERR)

/* Control Module Peripheral Register Definitions */
/* Addresses */
#define TI814X_CMPR_BASE                0x44E10000
#define TI814X_CMPR_SATA_OFFSET         0x00000720
#define TI814X_CMPR_SIZE                0x00002000
/* Register Offsets and bit definitions */
#define CMPR_MMR_LOCK1                  0x00000064
    /* This seemingly arbitraty value has no documentation to
     * support it, it was provided by TI Engineers */
    #define MMR_LOCK1_VALUE                 0xF757FDC0
#define CMPR_SATA_PLLCFG0               0x00000720
    #define SATA_PLLCFG0_ENPLL              0x00000001
    #define SATA_PLLCFG0_ENPLLLDO           0x00000002
    #define SATA_PLLCFG0_ENBGSC_REF         0x00000004
    #define SATA_PLLCFG0_SELBC              0x00000008
    #define SATA_PLLCFG0_ENDIGLDO           0x00000010
    #define SATA_PLLCFG0_SELINFREQ_20M      0x80000000
    #define SATA_PLLCFG0_DIGCLRZ            0x40000000
#define CMPR_SATA_PLLCFG1               0x00000724
    #define SATA_PLLCFG1_MDIVPULSE          0x00000001
    #define SATA_PLLCFG1_ENSSC              0x00000002
    #define SATA_PLLCFG1_EN_CLK50M          0x00000004
    #define SATA_PLLCFG1_EN_CLK100M         0x00000008
    #define SATA_PLLCFG1_EN_CLK125M         0x00000010
    #define SATA_PLLCFG1_EN_CLKAUX          0x00000020
    #define SATA_PLLCFG1_MDIVINT                0x012c0000
    #define SATA_PLLCFG1_PLLREFSEL              0x40000000
    #define SATA_PLLCFG1_ENSATAMODE             0x80000000
    #define SATA_PLLCFG1_VALUE                  SATA_PLLCFG1_ENSATAMODE | SATA_PLLCFG1_PLLREFSEL  | \
                                                SATA_PLLCFG1_MDIVINT    | SATA_PLLCFG1_EN_CLKAUX  | \
                                                SATA_PLLCFG1_EN_CLK125M | SATA_PLLCFG1_EN_CLK100M | \
                                                SATA_PLLCFG1_EN_CLK50M
#define CMPR_SATA_PLLCFG2               0x00000728
#define CMPR_SATA_PLLCFG3               0x0000072c
    #define SATA_PLLCFG3_PLLDO_CTRL         0x000008e0
    #define SATA_PLLCFG3_DIGLDO_ENFUNC1     0x00400000
#define CMPR_SATA_PLLCFG4               0x00000730
#define CMPR_SATA_PLLSTATUS             0x00000734
#define SATA_PLLSTATUS_APLL_LOCKED      0x00000001
#else
// RMII Interface Clock: 50 Mhz
// Peripheral Clock Frequency: 166 Mhz
// MDIO Typical: 1Mhz, works up to 2.5Mhz
// MDIO_CLK frequency = peripheral clock frequency/(CLKDIV + 1).
#define PER_CLK_FREQ_HZ     166000000
#define MDIO_CLK_FREQ_HZ    1000000
#define MDIO_CLK_DIVISOR    ((PER_CLK_FREQ_HZ / MDIO_CLK_FREQ_HZ) - 1)

/* CPPI Descriptor */ 
//#define CPPI_DESC_PHYS      0x02c82000    /* Physical address of descriptors in CPPI RAM */
#define CPPI_DESC_PHYS      0x4A102000      /* Physical address of descriptors in CPPI RAM */
#define CPPI_DESC_MEM_SIZE  0x00002000      /* Size of CPPI RAM memory for descriptors */
#define CPPI_RX_DESC_OFFSET 0x00000000
#define CPPI_TX_DESC_OFFSET 0x00001000
typedef struct _cppi_desc_t {
    uint32_t    next;               /* Pointer to next descriptor */
    uint32_t    buffer;             /* Pointer to data buffer */
    uint32_t    off_len;            /* Buffer offset and length */
    uint32_t    flag_len;           /* Packet flag and length */
    } cppi_desc_t;

/* Packet flags */
#define DESC_FLAG_SOP           (1 << 31)   /* Start of packet */
#define DESC_FLAG_EOP           (1 << 30)   /* End of packet */
#define DESC_FLAG_OWN           (1 << 29)   /* Ownership flag */
#define DESC_FLAG_EOQ           (1 << 28)   /* End of queue flag */
#define DESC_FLAG_TDOWNCMPLT    (1 << 27)   /* Teardown complete flag */
#define DESC_FLAG_PASSCRC       (1 << 26)   /* Pass CRC flag */
#define DESC_FLAG_JABBER        (1 << 25)   /* Jabber flag */
#define DESC_FLAG_OVERSIZE      (1 << 24)   /* Oversize flag */
#define DESC_FLAG_FRAGMENT      (1 << 23)   /* Fragment flag */
#define DESC_FLAG_UNDERSIZE     (1 << 22)   /* Undersize flag */
#define DESC_FLAG_CONTROL       (1 << 21)   /* Control flag */
#define DESC_FLAG_OVERRUN       (1 << 20)   /* Overrun flag */
#define DESC_FLAG_TOPORT_EN     (1 << 20)   /* to-port-enable for TX descriptors */
#define DESC_FLAG_CODE_ERR      (1 << 19)   /* Code error flag */
#define DESC_FLAG_ALIGN_ERR     (1 << 18)   /* Alignment error flag */
#define DESC_FLAG_CRC_ERR       (1 << 17)   /* CRC Error flag */
#define DESC_FLAG_NOMATCH       (1 << 16)   /* No match flag */
#define DESC_FLAG_TO_PORT(x)    (x << 16)   /* To port */

#define RX_ERROR    (DESC_FLAG_OVERSIZE | DESC_FLAG_UNDERSIZE | DESC_FLAG_OVERRUN | \
                    DESC_FLAG_ALIGN_ERR | DESC_FLAG_CRC_ERR)

/* Control Module Peripheral Register Definitions */
/* Addresses */
#define TI814X_CMPR_BASE                0x48140000
#define TI814X_CMPR_SATA_OFFSET         0x00000720
#define TI814X_CMPR_SIZE                0x00002000
/* Register Offsets and bit definitions */
#define CMPR_MMR_LOCK1                  0x00000064
    /* This seemingly arbitraty value has no documentation to
     * support it, it was provided by TI Engineers */
    #define MMR_LOCK1_VALUE                 0xF757FDC0
#define CMPR_SATA_PLLCFG0               0x00000720
    #define SATA_PLLCFG0_ENPLL              0x00000001
    #define SATA_PLLCFG0_ENPLLLDO           0x00000002
    #define SATA_PLLCFG0_ENBGSC_REF         0x00000004
    #define SATA_PLLCFG0_SELBC              0x00000008
    #define SATA_PLLCFG0_ENDIGLDO           0x00000010
    #define SATA_PLLCFG0_SELINFREQ_20M      0x80000000
    #define SATA_PLLCFG0_DIGCLRZ            0x40000000
#define CMPR_SATA_PLLCFG1               0x00000724
    #define SATA_PLLCFG1_MDIVPULSE          0x00000001
    #define SATA_PLLCFG1_ENSSC              0x00000002
    #define SATA_PLLCFG1_EN_CLK50M          0x00000004
    #define SATA_PLLCFG1_EN_CLK100M         0x00000008
    #define SATA_PLLCFG1_EN_CLK125M         0x00000010
    #define SATA_PLLCFG1_EN_CLKAUX          0x00000020
    #define SATA_PLLCFG1_MDIVINT                0x012c0000
    #define SATA_PLLCFG1_PLLREFSEL              0x40000000
    #define SATA_PLLCFG1_ENSATAMODE             0x80000000
    #define SATA_PLLCFG1_VALUE                  SATA_PLLCFG1_ENSATAMODE | SATA_PLLCFG1_PLLREFSEL  | \
                                                SATA_PLLCFG1_MDIVINT    | SATA_PLLCFG1_EN_CLKAUX  | \
                                                SATA_PLLCFG1_EN_CLK125M | SATA_PLLCFG1_EN_CLK100M | \
                                                SATA_PLLCFG1_EN_CLK50M                                 
#define CMPR_SATA_PLLCFG2               0x00000728
#define CMPR_SATA_PLLCFG3               0x0000072c
    #define SATA_PLLCFG3_PLLDO_CTRL         0x000008e0
    #define SATA_PLLCFG3_DIGLDO_ENFUNC1     0x00400000
#define CMPR_SATA_PLLCFG4               0x00000730
#define CMPR_SATA_PLLSTATUS             0x00000734
#define SATA_PLLSTATUS_APLL_LOCKED      0x00000001
#endif

#if 1

#define CMR_BASE                        0x44E10000      /* Control Module Regsiters */
#define CMR_MODULE_SIZE                 0x00002000      /* only a subset of the regs are needed */
/* Register Definitions */
#define CMR_DEVICE_ID                   0x00000600
#define DEV_FEATURE                     0x00000604
#define INIT_PRIORITY_0                 0x00000608
#define INIT_PRIORITY_1                 0x0000060c
#define MMU_CFG                         0x00000610
#define TPTC_CFG                        0x00000614
#define DSP_IDLE_CFG                    0x0000061c
#define USB_CTRL0                       0x00000620
#define USB_STS0                        0x00000624
#define USB_CTRL1                       0x00000628
#define USB_STS1                        0x0000062c
#define MAC_ID0_LO                      0x00000630
#define MAC_ID0_HI                      0x00000634
#define MAC_ID1_LO                      0x00000638
#define MAC_ID1_HI                      0x0000063c
#define GMII_SET                        0x00000650
#define         GMII0_SEL_RGMII                 0x00000002
#define         GMII1_SEL_RGMII                 0x00000008
#define         RMII0_SEL_RMII                  0x00000001
#define         RMII1_SEL_RMII                  0x00000004
#define         RMII0_SEL_GMII                  0x00000001
#define         RMII1_SEL_GMII                  0x00000004
#define         GMII0_ID_MODE                   0x00000010
#define         GMII1_ID_MODE                   0x00000020
#define         RGMII0_EN                       0x00000100
#define         RGMII1_EN                       0x00000200
// AM335X:
#define         PORT1_MII_MODE					0x00000000
#define         PORT1_GMII_MODE					0x00000000
#define         PORT1_RMII_MODE					0x00000001
#define         PORT1_RGMII_MODE				0x00000002
#define         PORT2_MII_MODE					0x00000000
#define         PORT2_GMII_MODE					0x00000000
#define         PORT2_RMII_MODE					0x00000004
#define         PORT2_RGMII_MODE				0x00000008
#define         RGMII1_INTERNAL_DELAY			0x00000000
#define         RGMII1_NO_INTERNAL_DELAY		0x00000010
#define         RGMII2_INTERNAL_DELAY			0x00000000
#define         RGMII2_NO_INTERNAL_DELAY		0x00000020
#define         RMII1_CLOCK_CHIP				0x00000040
#define         RMII1_CLOCK_PLL					0x00000000
#define         RMII2_CLOCK_CHIP				0x00000080
#define         RMII2_CLOCK_PLL					0x00000000

/* PRCM Register definitions */
//#define TI814X_PRCM_BASE                0x44E00000
//#define TI814X_PRCM_SIZE                0x00004000
//#define CM_ETHERNET_CLKSTCTRL           0x00001404
//#define         CLKCTRL_SW_WKUP                 0x00000002
//#define         CLKCTRL_CLKACT_RFT_GCLK         0x00000200
//#define         CLKCTRL_CLKACT_ETH_GCLK         0x00000100
//#define CM_ALWON_ETHERNET_0_CLKCTRL     0x000015d4
//#define CM_ALWON_ETHERNET_1_CLKCTRL     0x000015d8
//#define         MODULEMODE_ENABLED      0x00000002

#else

#define CMR_BASE                        0x48140000      /* Control Module Regsiters */
#define CMR_MODULE_SIZE                 0x00002000      /* only a subset of the regs are needed */
/* Register Definitions */
#define CMR_DEVICE_ID                   0x00000600
#define DEV_FEATURE                     0x00000604
#define INIT_PRIORITY_0                 0x00000608
#define INIT_PRIORITY_1                 0x0000060c
#define MMU_CFG                         0x00000610
#define TPTC_CFG                        0x00000614
#define DSP_IDLE_CFG                    0x0000061c
#define USB_CTRL0                       0x00000620
#define USB_STS0                        0x00000624
#define USB_CTRL1                       0x00000628
#define USB_STS1                        0x0000062c
#define MAC_ID0_LO                      0x00000630
#define MAC_ID0_HI                      0x00000634
#define MAC_ID1_LO                      0x00000638
#define MAC_ID1_HI                      0x0000063c
#define GMII_SET                        0x00000650
#define         GMII0_SEL_RGMII                 0x00000002
#define         GMII1_SEL_RGMII                 0x00000008
#define         GMII0_ID_MODE                   0x00000010
#define         GMII1_ID_MODE                   0x00000020
#define         RGMII0_EN                       0x00000100
#define         RGMII1_EN                       0x00000200
    
/* PRCM Register definitions */
#define TI814X_PRCM_BASE                0x48180000
#define TI814X_PRCM_SIZE                0x00002000
#define CM_ETHERNET_CLKSTCTRL           0x00001404
#define         CLKCTRL_SW_WKUP                 0x00000002
#define         CLKCTRL_CLKACT_RFT_GCLK         0x00000200
#define         CLKCTRL_CLKACT_ETH_GCLK         0x00000100
#define CM_ALWON_ETHERNET_0_CLKCTRL     0x000015d4
#define CM_ALWON_ETHERNET_1_CLKCTRL     0x000015d8
#define         MODULEMODE_ENABLED      0x00000002
#endif

typedef struct {
    struct cache_ctrl       cachectl;
    struct mbuf             **rx_mbuf;
    struct mbuf             **tx_mbuf;
    cppi_desc_t             *rx_desc;
    cppi_desc_t             *tx_desc;
    uint32_t                tx_phys;
} meminfo_t;

typedef struct {
    struct device                               dev;            /* Common device */
    struct ethercom                             ecom;           /* Common Ethernet */
    struct ethercom                             *common_ecom[2];
    uint16_t                                    num_rx_pkts;
    uint16_t                                    num_tx_pkts;
    int32_t                                     stop_miimon;
    int32_t                                     tx_q_len;
    int32_t                                     force_link;
    int32_t                                     linkup;
    int32_t                                     start_running;
    int32_t                                     pause_xmit;
    int32_t                                     pause_receive;
    int32_t                                     iid[2];
    int32_t                                     dying;
    int32_t                                     tx_pidx;
    int32_t                                     tx_cidx;
    int32_t                                     rx_cidx;
    int32_t                                     probe_phy;          // cmd line option to force periodic probing
    int32_t                                     pkts_received;      // optimization to not probe phy
    uint32_t                                    rx_phys;
    uint32_t                                    tx_freed;
    uint32_t                                    tx_alloc;
    uint32_t                                    tx_tracked;
    uint32_t                                    mcast_hash[2];
    uint32_t                                    num_if;
    uintptr_t                                   cpsw_regs;
    uintptr_t                                   cppi_base;
    uintptr_t                                   cmr_base;           // control module registers
    void                                        *sd_hook;
    mdi_t                                       *mdi;
    nic_config_t                                cfg;
    nic_stats_t                                 stats;
    volatile cppi_desc_t                        *last_tx;
    struct mbuf                                 **tx_mbuf;
    struct callout                              mii_callout;
    struct callout                              hk_callout;
    struct mii_data                             bsd_mii;
    struct _iopkt_self                          *iopkt;
    struct _iopkt_inter                         inter;
    struct evcnt                                ev_txdrop __attribute__((aligned (NET_CACHELINE_SIZE)));
    const struct sigevent                       *(*isrp)(void *, int);
    meminfo_t                                   meminfo;
} ti814x_dev_t;

struct  ti814x_dev {
        struct  device  sc_dev;
        ti814x_dev_t    *sc_ti814x;
        char            filler [sizeof (ti814x_dev_t) + NET_CACHELINE_SIZE];
};



/******************************************************************************
 *  Prototypes.
 *****************************************************************************/

/* ti814x.c */
int32_t init_pll(void);
int32_t init_prcm_emac(void);
void device_flush_mcast_bits (ti814x_dev_t *);

/* bsd_media.c */
void     bsd_mii_initmedia(ti814x_dev_t *);

/* devctl.c */
int ti814x_ioctl (struct ifnet *, unsigned long, caddr_t);

/* event.c */
int ti814x_process_interrupt (void *, struct nw_work_thread *);
const struct sigevent *ti814x_isr (void *, int);
const struct sigevent *ti814x_misc_isr (void *arg, int iid);
int ti814x_enable_interrupt (void *);
void ti814x_hk_callout (void *);

/* mii.c */
uint16_t ti814x_mdi_read (void *, uint8_t, uint8_t);
void ti814x_mdi_write (void *, uint8_t, uint8_t, uint16_t);
void ti814x_MDI_MonitorPhy (void *);
int ti814x_findphy (ti814x_dev_t *);

/* receive.c */
void ti814x_receive(ti814x_dev_t *ti814x, struct nw_work_thread *wtp);

/* transmit.c */
void ti814x_reap_pkts (ti814x_dev_t *);
void ti814x_start (struct ifnet *);

/* ale.h */
int32_t ti814x_ale_init(ti814x_dev_t *ti814x);
int8_t  ti814x_ale_get_port_field(uint32_t *entry);
int32_t ti814x_ale_set_entry_type(uint32_t *entry, uint8_t entry_type);
int32_t ti814x_ale_get_entry_type(uint32_t *entry);
void ti814x_ale_flush_ucast(uint32_t *entry, int port_mask);
int32_t ti814x_ale_match_addr(ti814x_dev_t *ti814x, uint8_t* addr);
int32_t ti814x_ale_match_free(ti814x_dev_t *ti814x);
int32_t ti814x_ale_find_ageable(ti814x_dev_t *ti814x);
int32_t ti814x_ale_add_mcast(ti814x_dev_t *ti814x, uint8_t *addr, int32_t port_mask);
int32_t ti814x_ale_print_table(ti814x_dev_t *ti814x);
int32_t ti814x_ale_add_ucast(ti814x_dev_t *ti814x, uint8_t *addr, int32_t index, int32_t port, int32_t flags);

#ifdef __cplusplus
};
#endif

#endif
