/****************************************************************************************************/
/**
  \mainpage
  \n 
  \brief        Main application (main module)
  \author       Abraham Tezmol Otero, M.S.E.E
  \project      Tau 
  \version      1.0
  \date         12/Jun/2016
   

/*~~~~~~  Headers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Main group of includes for board definitions, chip definitions and type definitions */
#include    "Std_types.h"
/** Task scheduler definitions */
#include    "SchM.h"
/** LED control definitions */ 
#include    "Led_Ctrl.h"
/** Watchdog control function prototypes definitions */
#include    "Wdg.h"
/** Button control operations */
#include    "Button_Ctrl.h"
/** Floating Point Unit */
#include    "Fpu.h"
/**I2C Controller*/
//#include    "twi.h"
/**I2S Controller*/
//#include    "ssc.h"
/**Codec Driver*/
#include    "wm8904.h"

/*~~~~~~  Local definitions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define MASTERCLOCK   	(150000000)
#define I2C_BAUDRATE  	(400000)
#define I2S_BITRATE   	(8000 * 32)

#define SAMP_PER      	(50)
#define BUFF_SIZE     	(8192) //1 second sample
#define FFT_SIZE		(1024)

#define SSC_RCMR_CONFIG	(SSC_RCMR_CKS_MCK | SSC_RCMR_CKO_NONE | SSC_RCMR_CKG_CONTINUOUS | SSC_RCMR_START_RF_EDGE | SSC_RCMR_STTDLY(1) | SSC_RCMR_PERIOD(0))
#define SSC_RFMR_CONFIG	(SSC_RFMR_DATLEN(15) | SSC_RFMR_MSBF | SSC_TFMR_FSOS_NONE | SSC_TFMR_FSEDGE_POSITIVE)
#define	SSC_TCMR_CONFIG (SSC_TCMR_CKS_TK | SSC_TCMR_CKO_NONE | SSC_TCMR_CKG_CONTINUOUS | SSC_TCMR_START_TF_EDGE | SSC_TCMR_STTDLY(1) | SSC_TCMR_PERIOD(0))
#define SSC_TFMR_CONFIG (SSC_RFMR_DATLEN(15) | SSC_TFMR_MSBF | SSC_RFMR_FSLEN(15) | SSC_TFMR_FSOS_NONE | SSC_TFMR_FSEDGE_POSITIVE)

/*~~~~~~  Global variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
Pin SSC_Pins[] = PINS_SSC_CODEC;
Pin PCK2_Pins[] = PIN_PCK2;
Pin TWI0_Pins[] = PINS_TWI0;

/** Auxiliary input buffer to accomodate data as FFT function expects it */
float       fft_inputData[BUFF_SIZE];
/** Output magnitude data */
float       fft_signalPower[BUFF_SIZE/2];
/** Auxiliary output variable that holds the frequency bin with the highest level of signal power */
uint32_t    u32fft_maxPowerIndex;
/** Auxiliary output variable that holds the maximum level of signal power */
float       fft_maxPower;

uint32_t ssc_inputData[BUFF_SIZE];

/*~~~~~~  Local functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void fft_process(void);
void CODEC_Init(void);
void I2C_Init(void);
void I2S_Init(void);

void I2S_AudioRecord();
void uinttofloat(float * floatbuffer, uint32_t * sscdata);

pfun pFFT = &fft_process;

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/
/**
 *  \brief getting-started Application entry point.
 *
 *  \return Unused (ANSI-C compatibility).
 */
extern int main( void )
{
	/* Disable watchdog */
	Wdg_Disable();
	/* Configure LEDs */
	LedCtrl_Configure();
  /* Configure Button */  
  ButtonCtrl_ConfigureSW0Button();
  /* Enable I and D cache */
	SCB_EnableICache();
	SCB_EnableDCache(); 
  /* Enable Floating Point Unit */
  Fpu_Enable();
  PMC_EnablePeripheral(ID_PMC);

	I2C_Init();
   	CODEC_Init();
    
	printf( "\n\r-- Scheduler Project %s --\n\r", SOFTPACK_VERSION ) ;
	printf( "-- %s\n\r", BOARD_NAME ) ;
	printf( "-- Compiled: %s %s With %s --\n\r", __DATE__, __TIME__ , COMPILER_NAME);

	I2S_Init();
	I2S_AudioRecord();
	uinttofloat(fft_inputData, ssc_inputData);

  /* Scheduler Inititalization */
  printf( "-- Scheduler Initialization --\n\r" ) ;
	//SchM_Init(ScheduleConfig);

	//FFT
	fft_process();
	
	/* Should never reach this code */
	for(;;)
    {
	//	printf( "-- Unexpected Error at Scheduler Initialization --\n\r" ) ;
	}
}

void fft_process(void)
{
  /** Perform FFT on the input signal */
  fft(fft_inputData, fft_signalPower, BUFF_SIZE/2, &u32fft_maxPowerIndex, &fft_maxPower);
        
  /* Publish through emulated Serial */
  printf("%5d  %5.4f \r\n", u32fft_maxPowerIndex, fft_maxPower);
}

void I2C_Init(void)
{
  PMC_EnablePeripheral(ID_PIOA);
  PMC_EnablePeripheral(ID_PIOB);
  PMC_EnablePeripheral(ID_PIOC);
  PMC_EnablePeripheral(ID_PIOD);
  PMC_EnablePeripheral(ID_PIOE);
  PMC_EnablePeripheral(ID_TWIHS0);
  PIO_Configure(TWI0_Pins, 2);
  TWI_ConfigureMaster( TWIHS0, I2C_BAUDRATE, MASTERCLOCK );
}

void I2S_Init(void)
{
  	PMC_EnablePeripheral(ID_SSC);
  	PIO_Configure(SSC_Pins, 6);
  	SSC_Configure(SSC, I2S_BITRATE, MASTERCLOCK);
  	SSC_ConfigureTransmitter(SSC, SSC_TCMR_CONFIG, SSC_TFMR_CONFIG);
  	SSC_ConfigureReceiver(SSC, SSC_RCMR_CONFIG  , SSC_RFMR_CONFIG);

	PMC_ConfigurePCK2(1, 0);

  	SSC_EnableTransmitter(SSC);
  	SSC_EnableReceiver(SSC);
}

void CODEC_Init(void)
{
  /***/
  //PMC_ConfigurePCK2(MASTERCLOCK, uint32_t prescaler);
  Twid pTwid;
  pTwid.pTwi = TWIHS0;
  pTwid.pTransfer = NULL;

  	uint16_t data = 0;
	/* check that WM8904 is present */
	uint32_t status = TimeTick_Configure();
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_RESET, 0xFFFF);
	data = WM8904_Read(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_RESET);
	if(data != 0x8904) 
	{
		printf("WM8904 not found!\n\r");
	}

  	WM8904_Init(&pTwid, WM8904_SLAVE_ADDRESS, PMC_MCKR_CSS_MAIN_CLK);
  	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_BIAS_CTRL0, WM8904_ISEL_HP_BIAS);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_VMID_CTRL0, WM8904_VMID_BUF_ENA | WM8904_VMID_RES_FAST | WM8904_VMID_ENA);

  	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_BIAS_CTRL0, WM8904_ISEL_HP_BIAS | WM8904_BIAS_ENA);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_POWER_MANG0, WM8904_INL_ENA | WM8904_INR_ENA);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_POWER_MANG2, WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ADC_DIG1, WM8904_DEEMPH(0));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ANALOGUE_OUT12ZC, 0x0000);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_CHARGE_PUMP0, WM8904_CP_ENA);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_CLASS0, WM8904_CP_DYN_PWR);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_FLL_CRTL1, 0x0000);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_FLL_CRTL2, WM8904_FLL_OUTDIV(7)| WM8904_FLL_FRATIO(4));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_FLL_CRTL3, WM8904_FLL_K(0x8000));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_FLL_CRTL4, WM8904_FLL_N(0xBB));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_FLL_CRTL1, WM8904_FLL_FRACN_ENA | WM8904_FLL_ENA);

  	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_CLOCK_RATE1, WM8904_CLK_SYS_RATE(3) | WM8904_SAMPLE_RATE(5));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_CLOCK_RATE0, 0x0000);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_CLOCK_RATE2, WM8904_SYSCLK_SRC | WM8904_CLK_SYS_ENA | WM8904_CLK_DSP_ENA);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_AUD_INF1, WM8904_BCLK_DIR | WM8904_AIF_FMT_I2S | WM8904_AIF_WL_32BIT);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_AUD_INF2, WM8904_BCLK_DIV(8));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_AUD_INF3, WM8904_LRCLK_DIR | WM8904_LRCLK_RATE(0x20));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_POWER_MANG6, WM8904_DACL_ENA | WM8904_DACR_ENA | WM8904_ADCL_ENA | WM8904_ADCR_ENA);

  	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ANALOGUE_LIN0, WM8904_LIN_VOL(0x10));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ANALOGUE_RIN0, WM8904_RIN_VOL(0x10));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ANALOGUE_HP0, WM8904_HPL_ENA | WM8904_HPR_ENA);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ANALOGUE_HP0, WM8904_HPL_ENA_DLY 
                | WM8904_HPL_ENA |WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_DC_SERVO0, WM8904_DCS_ENA_CHAN_3 | WM8904_DCS_ENA_CHAN_2 
                | WM8904_DCS_ENA_CHAN_1 | WM8904_DCS_ENA_CHAN_0);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_DC_SERVO1, WM8904_DCS_TRIG_STARTUP_3 
                | WM8904_DCS_TRIG_STARTUP_2 |WM8904_DCS_TRIG_STARTUP_1 | WM8904_DCS_TRIG_STARTUP_0);
  
  	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ANALOGUE_HP0, WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA | 
						    WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ANALOGUE_HP0, WM8904_HPL_RMV_SHORT | WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA | 
					    	WM8904_HPR_RMV_SHORT | WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ANALOGUE_LOUT1, WM8904_HPOUT_VU | WM8904_HPOUTL_VOL(0x39));
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_ANALOGUE_ROUT1, WM8904_HPOUT_VU | WM8904_HPOUTR_VOL(0x39));
}

void I2S_AudioRecord()
{
	int i = 0;
	for( i = 0; i < BUFF_SIZE; i++ )
	{
		//ssc_inputData[i] = SSC_Read(SSC);
		while(!SSC_IsRxReady(SSC)) //waits until frame is received
		{
			ssc_inputData[i] = SSC_Read(SSC);
		}
	}
}

void uinttofloat(float * floatbuffer, uint32_t * sscdata)
{
	int i = 0;
	for( i = 0; i < BUFF_SIZE; i++ )
	{
		floatbuffer[i] = ((float)sscdata[i]) / (float) 32768;
		if( floatbuffer[i] > 1 ) floatbuffer[i] = 1.0;
		if( floatbuffer[i] < -1 ) floatbuffer[i] = -1.0;
	}
}
