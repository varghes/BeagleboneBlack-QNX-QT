#define VARIANT_AIC3106

#define PROTOCOL_TDM_I2S   0
#define PROTOCOL_TDM_LJ    1


#define OSC0_CLK_FREQ    45158400
#define OSC1_CLK_FREQ	 22579200
#define AUXCLK_FREQ      11289600
#define SAMPLE_RATE      44100
#define SAMPLE_SIZE		 2 // in bytes
#define PROTOCOL		 PROTOCOL_TDM_I2S
#define TDM_SLOT_SIZE  	 16
#define TDM_NSLOTS       2
#define RX_VOICES		 2
#define TX_VOICES		 2

#define CLOCK_MODE     SLAVE

#define RX_TDM_SLOT_START 0
#define TX_TDM_SLOT_START 0
 
#define RX_SERIALIZER  3
#define TX_SERIALIZER  2

#define CODEC_SLAVE_ADDR1 0x18
#define CODEC_SLAVE_ADDR2 0x19
#define CODEC_SLAVE_ADDR3 0x1a

/* Board Specific Mapping */
#define     EDMA3CC_ADDR        0x49000000
#define     MCASP5_BASEADDR     0x4A1AE000
#define     MCASP4_BASEADDR     0x4A1A8000
#define     MCASP3_BASEADDR     0x4A1A2000
#define     MCASP2_BASEADDR     0x48050000
#define     MCASP1_BASEADDR     0x4803C000
#define     MCASP0_BASEADDR     0x48038000
#define     MCASP_SIZE          0x2C0

#define     MCASP0_dMAX     (0x46000000)
#define     MCASP1_dMAX     (0x46400000)
#define     MCASP2_dMAX     (0x46800000)
#define     MCASP3_dMAX     (MCASP3_BASEADDR + 0x3000)
#define     MCASP4_dMAX     (MCASP4_BASEADDR + 0x3000)
#define     MCASP5_dMAX     (MCASP5_BASEADDR + 0x3000)

#define		MCASP0_DMA_PLAY 8
#define		MCASP0_DMA_CAP  9
#define 	MCASP1_DMA_PLAY	10
#define 	MCASP1_DMA_CAP	11
#define 	MCASP2_DMA_PLAY	12
#define 	MCASP2_DMA_CAP	13
#define 	MCASP3_DMA_PLAY	56
#define 	MCASP3_DMA_CAP	57
#define 	MCASP4_DMA_PLAY	62
#define 	MCASP4_DMA_CAP	63
#define 	MCASP5_DMA_PLAY	0
#define 	MCASP5_DMA_CAP	1

#define		MCASP0_TX_IRQ	80
#define		MCASP0_RX_IRQ	81
#define		MCASP1_TX_IRQ	82
#define		MCASP1_RX_IRQ	83
#define		MCASP2_TX_IRQ	84
#define		MCASP2_RX_IRQ	85
#define		MCASP3_TX_IRQ	105
#define		MCASP3_RX_IRQ	106
#define		MCASP4_TX_IRQ	108
#define		MCASP4_RX_IRQ	109
#define		MCASP5_TX_IRQ	110
#define		MCASP5_RX_IRQ	111

#define		DMA_RELOAD	64

#define		EDMA_BASE_IRQ 	 0x200
