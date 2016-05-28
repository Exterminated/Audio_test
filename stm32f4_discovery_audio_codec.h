#include <stm32f4xx_rcc.h>
#include <stm32f4xx_dma.h>
#include <stm32f4xx_adc.h>
#include <stm32f4xx_dac.h>
#include <stm32f4xx.h>
#include <misc.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_tim.h>

// Макроопределения
#define LED4_PIN                         GPIO_Pin_12
#define LED4_GPIO_PORT                   GPIOD
#define LED4_GPIO_CLK                    RCC_AHB1Periph_GPIOD

#define LED3_PIN                         GPIO_Pin_13
#define LED3_GPIO_PORT                   GPIOD
#define LED3_GPIO_CLK                    RCC_AHB1Periph_GPIOD

#define LED5_PIN                         GPIO_Pin_14
#define LED5_GPIO_PORT                   GPIOD
#define LED5_GPIO_CLK                    RCC_AHB1Periph_GPIOD

#define LED6_PIN                         GPIO_Pin_15
#define LED6_GPIO_PORT                   GPIOD
#define LED6_GPIO_CLK                    RCC_AHB1Periph_GPIOD

// Для Таймера
#define APB1_TRIGGER_TIMER 	RCC_APB1Periph_TIM2
#define TIMER TIM2
#define TIMER_PERIOD  		100

// Для АЦП
#define ADC_TRIGGER 		ADC_ExternalTrigConv_T2_TRGO
#define APB2_ADC 			RCC_APB2Periph_ADC3
#define RECORDER_ADC 		ADC3
#define RECORDER_ADC_CHANNEL 	ADC_Channel_12

// Для ЦАП
#define DAC_TRIGGER 		DAC_Trigger_T2_TRGO
#define Rec_IRQHandler		DMA2_Stream1_IRQHandler
#define Rec_DMA_IRQ			DMA2_Stream1_IRQn

// Настройка рекордера
// Вход: размер буфера, адрес первого буфера, адрес второго буфера
void Recorder_config(int buf_size,uint16_t *First_buffer, uint16_t *Second_buffer);

// Настройка ЦАП и DMA из памяти в ЦАП
// Вход: размер буфера DMA, адрес первого буфера, адрес второго буфера
void DAC_player_config(int buf_size,uint16_t *First_buffer, uint16_t *Second_buffer);

// Настройка проигрывателя
// Вход: размер буфера, адрес первого буфера, адрес второго буфера
void Player_config(int buf_size,uint16_t *First_buffer, uint16_t *Second_buffer);

// Настраиваем таймер 2
// Вход: частота дискретизации, таймер будет срабатывать в 2 раза чаще
void General_config(int freq);

// Запускает рекордер
void Start_record(void);

// Запускает проигрыватель
void Start_playing();

// Настройка рекордера и проигрывателя
// Вход: размер буфера, частота дискретизации, адрес первого буфера, адрес второго буфера
void Init_record_and_play(int buf_size, int freq, uint16_t *RFirst_buffer, uint16_t *RSecond_buffer,
 uint16_t *PFirst_buffer, uint16_t *PSecond_buffer);

// Завершение записи и воспроизведения
void End_record_and_playing(void);
