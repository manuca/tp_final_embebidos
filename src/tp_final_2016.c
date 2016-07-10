#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>
#include "signal_data.h"

/*
 * Software configuration
 */
#define BUFFER_SIZE    (1000)
#define BASE_FREQUENCY (1e6)
#define MAX_MULTIPLIER (3)

#define TICKS_BETWEEN_FREQUENCY_CHANGE BASE_FREQUENCY

/*
 * Hardware mappings
 */

#ifdef __LPC17XX__
#define DAC_REGISTER LPC_DAC
#define ADC_PERIPH  LPC_ADC
#define ADC_CHANNEL ADC_CH0
#define ADC_IRQ_HANDLER ADC_IRQHandler
#endif

#ifdef __LPC43XX__
#define DAC_REGISTER LPC_DAC
#define ADC_PERIPH  LPC_ADC
#define ADC_CHANNEL ADC_CH0
#define ADC_IRQ_HANDLER ADC0_IRQHandler
#endif


uint16_t signal_data[BUFFER_SIZE];

void initDAC()
{
#ifdef __LPC43XX__
  Board_DAC_Init(LPC_DAC);
#endif
  Chip_DAC_Init(LPC_DAC);
}

void RIT_IRQHandler()
{
  static unsigned int index = 0;
  static unsigned int ticks = 0;
  static uint8_t current_multiplier = 1;

  // Clear interrupt bit
  LPC_RITIMER->CTRL |= RIT_CTRL_INT;

  if(ticks < BASE_FREQUENCY) {
    ticks++;
  }
  else {
    ticks = 0;

    if (current_multiplier < MAX_MULTIPLIER) {
      current_multiplier++;
    }
    else {
      current_multiplier = 1;
    }
  }

  Chip_DAC_UpdateValue(LPC_DAC, (uint32_t) signal_data[index]);
  index = (index < BUFFER_SIZE) ? (index + current_multiplier) : 0;
}

/*
 * Establish a target of 1MHz for DAC updates
 */
void initRIT()
{
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_RIT);
  Chip_Clock_SetPCLKDiv(SYSCTL_PCLK_RIT, SYSCTL_CLKDIV_1);

  LPC_RITIMER->COMPVAL = (SystemCoreClock / BASE_FREQUENCY);
  LPC_RITIMER->MASK    = 0xFFFFFFFF;
  LPC_RITIMER->CTRL   |= RIT_CTRL_INT | RIT_CTRL_ENCLR | RIT_CTRL_ENBR |  RIT_CTRL_TEN;
}

void ADC_IRQ_HANDLER()
{
  uint16_t data;

  Chip_ADC_ReadValue(ADC_PERIPH, ADC_CHANNEL, &data);
  Chip_ADC_SetStartMode(ADC_PERIPH, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
}

void initADC()
{
  ADC_CLOCK_SETUP_T setup;
#ifdef __LPC17XX__
  Chip_IOCON_PinMux(LPC_IOCON, 0, 23, IOCON_MODE_INACT, IOCON_FUNC1);
#endif
  Chip_ADC_Init(ADC_PERIPH, &setup);
  Chip_ADC_EnableChannel(ADC_PERIPH, ADC_CHANNEL, ENABLE);
  Chip_ADC_Int_SetChannelCmd(ADC_PERIPH, ADC_CHANNEL, ENABLE);
  Chip_ADC_SetBurstCmd(ADC_PERIPH, DISABLE);
  Chip_ADC_SetStartMode(ADC_PERIPH, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
}

int main(void)
{
    SystemCoreClockUpdate();
    Board_Init();

    initDAC();
    calculateSignal(signal_data, BUFFER_SIZE);

    initRIT();
    initADC();

    NVIC_EnableIRQ(ADC_IRQn);
    NVIC_EnableIRQ(RITIMER_IRQn);

    while(1) {
      __WFI();
    }

    Chip_DAC_DeInit(LPC_DAC);
}
