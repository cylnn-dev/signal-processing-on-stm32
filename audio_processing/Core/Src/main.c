/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "pdm2pcm.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */
#include "tusb_config.h"
#include "device/usbd.h"
#include "class/audio/audio.h"
#include "../../tinyusb/bsp/board_api.h"
#include "cs43l22.h"
#include <math.h>



/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void audio_task(void (*feed)(void));

bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; 				          // +1 for master channel 0
uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; 					// +1 for master channel 0
uint32_t sampFreq;
uint8_t bytesPerSample;
uint8_t clkValid;

// Range states
// List of supported sample rates
static const uint32_t sampleRatesList[] = {32000, 48000, 96000};

#define N_sampleRates  TU_ARRAY_SIZE(sampleRatesList)

// Bytes per format of every Alt settings
static const uint8_t bytesPerSampleAltList[CFG_TUD_AUDIO_FUNC_1_N_FORMATS] =
        {
                CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX,
                CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_TX,
        };

//audio_control_range_2_n_t(1) volumeRng[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX+1]; // Volume range state


// audio buffer
CFG_TUSB_MEM_ALIGN uint8_t test_buffer_audio[CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX];

#define SAMPLE_RATE 48000  // 96000
#define SAMPLE_PERIOD (1.0f / SAMPLE_RATE)
#define DC_POINT 0
#define TX_SIZE (SAMPLE_RATE / 1000)

#define CODEC_ADDR 0x94U
uint8_t initVolume = 70;
// each packet could contain 32, 48, or 96 samples according to the sample size defined in recording

typedef enum {
    DATA_REQUEST = 0x00U,
    FIRST_HALF_READY = 0x01U,
    SECOND_HALF_READY = 0x02U,
    BUFFER_FULL = 0x03U,
    BUFFER_ERROR = 0x04U,
} buffer_state;

static float t = 0;
uint16_t rxBuff[TX_SIZE * 2] = {0};
int16_t pcmBuff[TX_SIZE] = {0};
//int16_t signalBuff[TX_SIZE] = {0};
buffer_state rx_state = FIRST_HALF_READY;
buffer_state pcm_state = DATA_REQUEST;


void (*feed)(void) = &feed_nothing;
void setup_codec();
void enable_mic_dma();


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2S3_Init();
  MX_I2S2_Init();
  MX_CRC_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_PDM2PCM_Init();
  MX_SPI1_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

    tud_init(BOARD_TUD_RHPORT);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    sampFreq = sampleRatesList[0];
    clkValid = 1;

    /* enable DMA from Mic to rxBuff */
    enable_mic_dma();

    /* activate CODEC and enable DMA from pcmBuff */
    setup_codec();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  print_welcome_message();
  print_menu_message();
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
      user_input_control();
      audio_task(feed);
      tud_task();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 301;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void enable_mic_dma() { HAL_I2S_Receive_DMA(&hi2s2, &rxBuff[0], TX_SIZE * 2); }

void setup_codec(void) {
    cs43l22_Init(CODEC_ADDR, OUTPUT_DEVICE_HEADPHONE, initVolume, SAMPLE_RATE);
    cs43l22_Play(CODEC_ADDR, NULL, 0);
    HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t *) ((void *) &pcmBuff[0]), TX_SIZE);
    HAL_I2S_DMAPause(&hi2s3);
}

void start_codec(void) {
    HAL_I2S_DMAResume(&hi2s3);
}

void audio_task(void (*feed)(void)) {
    (*feed)();
}



void feed_mic_data(void) {
    uint16_t *txBuffPtr = (rx_state == FIRST_HALF_READY) ? (&rxBuff[0]) : (&rxBuff[TX_SIZE]);
    PDM_Filter((uint8_t*) txBuffPtr, &pcmBuff[0], &PDM1_filter_handler);
    pcm_state = BUFFER_FULL;
}

void feed_signal_data(void) {
    /* direct approach for generating a test signal */
    if (pcm_state == DATA_REQUEST) {
        for (int i = 0; i < TX_SIZE; ++i) {
            float f_sample = 5 * cosf(2 * M_PI * 100 * t);
            int16_t sample = (int16_t) (f_sample * 1000 + DC_POINT);
            t += SAMPLE_PERIOD;

            pcmBuff[i] = sample;
        }
        pcm_state = BUFFER_FULL;
    }

}

void feed_nothing(void) {
    ;
}



bool tud_audio_tx_done_post_load_cb(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
    (void) rhport;
    (void) n_bytes_copied;
    (void) itf;
    (void) ep_in;
    (void) cur_alt_setting;

    int16_t* usbBuff = (int16_t*)((void*)test_buffer_audio);

    /* transmit mic data */
    if (pcm_state == BUFFER_FULL) {
        for (size_t i = 0; i < TX_SIZE; i++) {
            *usbBuff++ = pcmBuff[i];
        }
        clear_buffer(pcmBuff, TX_SIZE);
        pcm_state = DATA_REQUEST;
    }


    /* tinyUSB reference */
//    if(bytesPerSample == 2)
//    {
//        uint16_t* pData_16 = (uint16_t*)((void*)test_buffer_audio);
//        for (size_t cnt = 0; cnt < sampFreq / 1000; cnt++)
//        {
//            pData_16[cnt] = dummy_data--;
//        }
//    }
//        // 24bit in 32bit slot
//    else if(bytesPerSample == 4)
//    {
//        uint32_t* pData_32 = (uint32_t*)((void*)test_buffer_audio);
//        for (size_t cnt = 0; cnt < sampFreq / 1000; cnt++)
//        {
//            pData_32[cnt] = (uint32_t)startVal++ << 16U;
//        }
//    }
    return true;
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (hi2s->Instance == SPI2) {
        rx_state = FIRST_HALF_READY;
    }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (hi2s->Instance == SPI2) {
        rx_state = SECOND_HALF_READY;
    }
}


//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    ;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    ;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void) remote_wakeup_en;
    ;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    ;
}

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+

// Invoked when set interface is called, typically on start/stop streaming or format change
bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
    (void)rhport;
    //uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

    // Clear buffer when streaming format is changed
    if(alt != 0)
    {
        bytesPerSample = bytesPerSampleAltList[alt-1];
    }
    return true;
}

// Invoked when audio class specific set request received for an EP
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
    (void) rhport;
    (void) pBuff;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t ep = TU_U16_LOW(p_request->wIndex);

    (void) channelNum; (void) ctrlSel; (void) ep;

    return false; 	// Yet not implemented
}

// Invoked when audio class specific set request received for an interface
bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
    (void) rhport;
    (void) pBuff;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);

    (void) channelNum; (void) ctrlSel; (void) itf;

    return false; 	// Yet not implemented
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
    (void) rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

    (void) itf;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

    // If request is for our feature unit
    if ( entityID == UAC2_ENTITY_FEATURE_UNIT )
    {
        switch ( ctrlSel )
        {
            case AUDIO_FU_CTRL_MUTE:
                // Request uses format layout 1
                TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_1_t));

                mute[channelNum] = ((audio_control_cur_1_t*) pBuff)->bCur;

                TU_LOG2("    Set Mute: %d of channel: %u\r\n", mute[channelNum], channelNum);
                return true;

            case AUDIO_FU_CTRL_VOLUME:
                // Request uses format layout 2
                TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_2_t));

                volume[channelNum] = (uint16_t) ((audio_control_cur_2_t*) pBuff)->bCur;

                TU_LOG2("    Set Volume: %d dB of channel: %u\r\n", volume[channelNum], channelNum);
                return true;

                // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    // Clock Source unit
    if ( entityID == UAC2_ENTITY_CLOCK )
    {
        switch ( ctrlSel )
        {
            case AUDIO_CS_CTRL_SAM_FREQ:
                TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_4_t));

                sampFreq = (uint32_t)((audio_control_cur_4_t *)pBuff)->bCur;

                TU_LOG2("Clock set current freq: %lu\r\n", sampFreq);

                return true;
                break;

                // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    return false;    // Yet not implemented
}

// Invoked when audio class specific get request received for an EP
bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
    (void) rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t ep = TU_U16_LOW(p_request->wIndex);

    (void) channelNum; (void) ctrlSel; (void) ep;

    //	return tud_control_xfer(rhport, p_request, &tmp, 1);

    return false; 	// Yet not implemented
}

// Invoked when audio class specific get request received for an interface
bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
    (void) rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);

    (void) channelNum; (void) ctrlSel; (void) itf;

    return false; 	// Yet not implemented
}

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
    (void) rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    // uint8_t itf = TU_U16_LOW(p_request->wIndex); 			// Since we have only one audio function implemented, we do not need the itf value
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

    // Input terminal (Microphone input)
    if (entityID == UAC2_ENTITY_INPUT_TERMINAL)
    {
        switch ( ctrlSel )
        {
            case AUDIO_TE_CTRL_CONNECTOR:
            {
                // The terminal connector control only has a get request with only the CUR attribute.
                audio_desc_channel_cluster_t ret;

                // Those are dummy values for now
                ret.bNrChannels = 1;
                ret.bmChannelConfig = 0;
                ret.iChannelNames = 0;

                TU_LOG2("    Get terminal connector\r\n");

                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void*) &ret, sizeof(ret));
            }
                break;

                // Unknown/Unsupported control selector
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    // Feature unit
    if (entityID == UAC2_ENTITY_FEATURE_UNIT)
    {
        switch ( ctrlSel )
        {
            case AUDIO_FU_CTRL_MUTE:
                // Audio control mute cur parameter block consists of only one byte - we thus can send it right away
                // There does not exist a range parameter block for mute
                TU_LOG2("    Get Mute of channel: %u\r\n", channelNum);
                return tud_control_xfer(rhport, p_request, &mute[channelNum], 1);

            case AUDIO_FU_CTRL_VOLUME:
                switch ( p_request->bRequest )
                {
                    case AUDIO_CS_REQ_CUR:
                        TU_LOG2("    Get Volume of channel: %u\r\n", channelNum);
                        return tud_control_xfer(rhport, p_request, &volume[channelNum], sizeof(volume[channelNum]));

                    case AUDIO_CS_REQ_RANGE:
                        TU_LOG2("    Get Volume range of channel: %u\r\n", channelNum);

                        // Copy values - only for testing - better is version below
                        audio_control_range_2_n_t(1)
                                ret;

                        ret.wNumSubRanges = 1;
                        ret.subrange[0].bMin = -90;    // -90 dB
                        ret.subrange[0].bMax = 30;		// +30 dB
                        ret.subrange[0].bRes = 1; 		// 1 dB steps

                        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void*) &ret, sizeof(ret));

                        // Unknown/Unsupported control
                    default:
                        TU_BREAKPOINT();
                        return false;
                }
                break;

                // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    // Clock Source unit
    if ( entityID == UAC2_ENTITY_CLOCK )
    {
        switch ( ctrlSel )
        {
            case AUDIO_CS_CTRL_SAM_FREQ:
                // channelNum is always zero in this case
                switch ( p_request->bRequest )
                {
                    case AUDIO_CS_REQ_CUR:
                        TU_LOG2("    Get Sample Freq.\r\n");
                        return tud_control_xfer(rhport, p_request, &sampFreq, sizeof(sampFreq));

                    case AUDIO_CS_REQ_RANGE:
                    {
                        TU_LOG2("    Get Sample Freq. range\r\n");
                        audio_control_range_4_n_t(N_sampleRates) rangef =
                                {
                                        .wNumSubRanges = tu_htole16(N_sampleRates)
                                };
                        TU_LOG1("Clock get %d freq ranges\r\n", N_sampleRates);
                        for(uint8_t i = 0; i < N_sampleRates; i++)
                        {
                            rangef.subrange[i].bMin = (int32_t)sampleRatesList[i];
                            rangef.subrange[i].bMax = (int32_t)sampleRatesList[i];
                            rangef.subrange[i].bRes = 0;
                            TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax, (int)rangef.subrange[i].bRes);
                        }
                        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &rangef, sizeof(rangef));
                    }
                        // Unknown/Unsupported control
                    default:
                        TU_BREAKPOINT();
                        return false;
                }
                break;

            case AUDIO_CS_CTRL_CLK_VALID:
                // Only cur attribute exists for this request
                TU_LOG2("    Get Sample Freq. valid\r\n");
                return tud_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));

                // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    TU_LOG2("  Unsupported entity: %d\r\n", entityID);
    return false; 	// Yet not implemented
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
    (void) rhport;
    (void) itf;
    (void) ep_in;
    (void) cur_alt_setting;

    tud_audio_write((uint8_t *)test_buffer_audio, (uint16_t)(sampFreq / (TUD_OPT_HIGH_SPEED ? 8000 : 1000) * bytesPerSample));

    return true;
}



bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
    (void) rhport;
    (void) p_request;

    return true;
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
