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
#define MASTERCLOCK   (150000000)
#define I2C_BAUDRATE  (400000)
#define I2S_BITRATE   (8000)

#define SAMP_PER      (50)
#define BUFF_SIZE     (8192) //1 second sample

//PINS_SSC_CODEC
//PIN_PCK2
//PINS_TWI0

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

/*~~~~~~  Local functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void fft_process(void);
void CODEC_Init(void);
void I2C_Init(void);
void I2S_Init(void);

void I2S_AudioRecord();

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
	//Wdg_Disable();
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
    
	printf( "\n\r-- Scheduler Project %s --\n\r", SOFTPACK_VERSION ) ;
	printf( "-- %s\n\r", BOARD_NAME ) ;
	printf( "-- Compiled: %s %s With %s --\n\r", __DATE__, __TIME__ , COMPILER_NAME);

  /* Scheduler Inititalization */
  printf( "-- Scheduler Initialization --\n\r" ) ;
	//SchM_Init(ScheduleConfig);
	
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
  //SSC_ConfigureTransmitter(SSC, uint32_t tcmr, uint32_t tfmr);
  //SSC_ConfigureReceiver(SSC, uint32_t rcmr, uint32_t rfmr);
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
  WM8904_Init(&pTwid, WM8904_SLAVE_ADDRESS, PMC_MCKR_CSS_MAIN_CLK);
  WM8904_Write((&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_BIAS_CTRL0, WM8904_ISEL_HP_BIAS);
	WM8904_Write(&pTwid, WM8904_SLAVE_ADDRESS, WM8904_REG_VMID_CTRL0, WM8904_VMID_BUF_ENA | WM8904_VMID_RES_FAST | WM8904_VMID_ENA);
}