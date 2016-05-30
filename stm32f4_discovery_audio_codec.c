#include "stm32f4_discovery_audio_codec.h"

static void General_Handler(void);

// Обработчик прерываний (общий)
static void General_Handler(void)
{
    if (DMA_GetFlagStatus(DMA2_Stream1, DMA_FLAG_TCIF1) != RESET)
    {
    	// Обработка прерывания от DMA2 (рекордера)
    	GPIO_ToggleBits(GPIOD,LED3_PIN | LED4_PIN);
    	DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_TCIF1); // Сбрасываем флаг прерывания
    }
    if (DMA_GetFlagStatus(DMA1_Stream6, DMA_FLAG_TCIF6) != RESET)
    {
    	// Обработка прерывания от DMA1 (проигрывателя)
    	GPIO_ToggleBits(GPIOD,LED5_PIN | LED6_PIN);
    	DMA_ClearFlag(DMA1_Stream6, DMA_FLAG_TCIF6); // Сбрасываем флаг прерывания
    }
}

// Завершение записи и воспроизведения
void End_record_and_playing(void)
{
    ADC_DMACmd(ADC3, DISABLE); // Выключаем АЦП3 DMA
    ADC_Cmd(ADC3, DISABLE); // Выключаем АЦП3
    DMA_Cmd(DMA2_Stream1, DISABLE); // Выключаем DMA2 (рекордер)
    DMA_Cmd(DMA1_Stream6, DISABLE); // Выключаем DMA1 (проигрыватель)
    DAC_DMACmd(DAC_Channel_2, DISABLE); // Выключаем ЦАП DMA
    DAC_Cmd(DAC_Channel_2, DISABLE); // Выключаем ЦАП
}

// Настройка рекордера и проигрывателя
// Вход: размер буфера
// Частота дискретизации
// Адрес первого буфера
// Адрес второго буфера
void Init_record_and_play(int buf_size,int freq, uint16_t *RFirst_buffer, uint16_t *RSecond_buffer, uint16_t *PFirst_buffer, uint16_t *PSecond_buffer)
{
    General_config(freq); // Настраиваем таймер 2 (частоту опроса)
    if(RSecond_buffer!= 0 && RFirst_buffer!= 0)
    	Recorder_config(buf_size, RFirst_buffer, RSecond_buffer); // Настраиваем рекордер
    if(PFirst_buffer!=0 && PSecond_buffer!=0)
    	Player_config(buf_size, PFirst_buffer, PSecond_buffer); // Настраиваем проигрыватель
}

// Настраиваем таймер 2
// Вход: частота дискретизации
// Таймер будет срабатывать в 2 раза чаще
void General_config(int freq)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  RCC_APB1PeriphClockCmd(APB1_TRIGGER_TIMER, ENABLE);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/(freq*2*TIMER_PERIOD)-1;
  TIM_TimeBaseStructure.TIM_Period = TIMER_PERIOD;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
  TIM_TimeBaseInit(TIMER, &TIM_TimeBaseStructure);
  TIM_SelectOutputTrigger(TIMER, TIM_TRGOSource_Update);
  TIM_Cmd(TIMER, ENABLE);
}

// Инициализация АЦП для рекордера
void ADC_recorder_init(void)
{
  RCC_APB2PeriphClockCmd(APB2_ADC, ENABLE);// Включение тактирования АЦП
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  /* ADC Common Init **********************************************************/
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;// Работаем не в Dual режиме.
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2; // Предделитель 2
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_10Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);
  
  // Настройка АЦП3
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; // Разрешение АЦП 12 разрядов
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;// Выключаем сканирование.
  //Выключаем повторное преобразование по окончании преобразования.
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_TRIGGER;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // Выравнивание по правому краю
  // число преобразований после поступления команды запуска преобразования
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  
  ADC_Init(RECORDER_ADC, &ADC_InitStructure);
  
  // Настройка АЦП3 регулярный канал 12
  // указывем канал, подлежащий оцифровке, время выборки сигнала 28 циклов
  ADC_RegularChannelConfig(RECORDER_ADC, RECORDER_ADC_CHANNEL, 1, ADC_SampleTime_28Cycles);
  // разрешаем выполнять запросы DMA
  ADC_DMARequestAfterLastTransferCmd(RECORDER_ADC, ENABLE);
}

// Запускает рекордер
void Start_record(void)
{
  ADC_DMACmd(ADC3, ENABLE);// Запуск DMA (АЦП3 -> Память)
  /* Enable ADC3 */
  ADC_Cmd(ADC3, ENABLE);// Запуск АЦП3
}

// Настройка рекордера
// Вход: размер буфера
// Адрес первого буфера
// Адрес второго буфера
void Recorder_config(int buf_size,uint16_t *First_buffer, uint16_t *Second_buffer)
{
  DMA_InitTypeDef DMA2_InitStructure; 
  GPIO_InitTypeDef      GPIO_InitStructure;
  
  // Включаем тактирование DMA2
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
  
  // Настройка DMA2 Stream 0 канал 0
  DMA2_InitStructure.DMA_Channel = DMA_Channel_2;
  // адрес периферии - АЦП буфер полученных измерений
  DMA2_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &ADC3->DR;
  DMA2_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;// направление периферия (АЦП) -> память
  DMA2_InitStructure.DMA_BufferSize = buf_size;//размер буфера DMA (все отсчеты)
  // не инкрементируем указатели на данные в периферии (ЦАП)
  DMA2_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  // инкрементируем указатели на данные в памяти
  DMA2_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
  // размер элемента данных - 2 байта
  DMA2_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA2_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA2_InitStructure.DMA_Mode = DMA_Mode_Circular;// циклический режим
  // Существует 2 режима:
  // DMA_Mode_Normal - однократное срабатывание DMA.
  // DMA_Mode_Circular - многократное срабатывание DMA. 
  // В этом случае DMA может выдавать прерывание, но продолжит повторять свою работу вне зависимости 
  // отреагируете ли вы на него до тех пор пока вы его не выключите.
  DMA2_InitStructure.DMA_Priority = DMA_Priority_High;// высокий приоритет
  DMA2_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;// фифо не исп.
  DMA2_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;// относится к фифо, фифо не исп.
  DMA2_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;// относится к пакетному режиму
  DMA2_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;// относится к пакетному режиму
  
  DMA_Init(DMA2_Stream1, &DMA2_InitStructure);
  DMA_DoubleBufferModeCmd(DMA2_Stream1,ENABLE);// включаем режим двойного буфера
  DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);// устанавливаем прерывания от DMA по окончанию передачи
  
  DMA2_Stream1->M0AR = (uint32_t)First_buffer;// указатель на первый буфер
  DMA2_Stream1->M1AR = (uint32_t)Second_buffer;// указатель на второй буфер
  DMA_Cmd(DMA2_Stream1, ENABLE);
  
  NVIC_InitTypeDef NVIC_InitStructure;
  //Enable DMA2 channel IRQ Channel 
  NVIC_InitStructure.NVIC_IRQChannel = Rec_DMA_IRQ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
    
  // Настройка АЦП 3 Канал 12 вывод как аналоговый вход
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOC, &GPIO_InitStructure);


  GPIO_InitStructure.GPIO_Pin = LED3_PIN | LED4_PIN | LED5_PIN |LED6_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  ADC_recorder_init();
}

// Запускает проигрыватель
void Start_playing()
{
  DMA_Cmd(DMA1_Stream6, ENABLE); // Запуск DMA1 (Память -> ЦАП)
  DAC_DMACmd(DAC_Channel_2, ENABLE); // Запуск ЦАП, канал 2 + DMA
  DAC_Cmd(DAC_Channel_2, ENABLE); // Запуск ЦАП, канал 2
}

// Настройка проигрывателя
// Вход: размер буфера
// Адрес первого буфера
// Адрес второго буфера
void Player_config(int buf_size,uint16_t *First_buffer, uint16_t *Second_buffer)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  // Включаем тактирование DMA1 (Память -> ЦАП)
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1 | RCC_AHB1Periph_GPIOD, ENABLE);
  // Включаем тактирование ЦАП
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
  // Настраиваем 5 вывод порта A как выход ЦАП
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  DAC_DeInit();
  // Настройка ЦАП и DMA из памяти в ЦАП
  DAC_player_config(buf_size,First_buffer,Second_buffer);
}

// Настройка ЦАП и DMA из памяти в ЦАП
// Вход: размер буфера DMA,
// Адрес первого буфера,
// Адрес второго буфера
void DAC_player_config(int buf_size, uint16_t *First_buffer, uint16_t *Second_buffer)
{
  DAC_InitTypeDef  DAC_InitStructure;
  DMA_InitTypeDef DMA1_InitStructure; 

  /* ЦАП channel 1 Configuration */
  DAC_InitStructure.DAC_Trigger = DAC_TRIGGER;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_TriangleAmplitude_1; 
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_2, &DAC_InitStructure);
 
  NVIC_InitTypeDef NVIC_InitStructure;
  //Enable DMA2 channel IRQ Channel 
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream6_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
 
  /* DMA1_Stream5 channel7 configuration **************************************/
  DMA_DeInit(DMA1_Stream6);
  DMA1_InitStructure.DMA_Channel = DMA_Channel_7;
  // адрес периферии - ЦАП (12-битовый формат с выравниванием вправо) буфер для загрузки
  DMA1_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)& DAC->DHR12R2;
  DMA1_InitStructure.DMA_BufferSize = buf_size;//размер буфера DMA (все отсчеты)
  // размер элемента данных - 2 байта
  DMA1_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA1_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  
  DMA1_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;// направление память -> периферия (ЦАП)
  // не инкрементируем указатели на данные в периферии (ЦАП)
  DMA1_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
  // инкрементируем указатели на данные в памяти
  DMA1_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA1_InitStructure.DMA_Mode = DMA_Mode_Circular;// циклический режим
  // Существует 2 режима:
  // DMA_Mode_Normal - однократное срабатывание DMA.
  // DMA_Mode_Circular - многократное срабатывание DMA. 
  // В этом случае DMA может выдавать прерывание, но продолжит повторять свою работу вне зависимости 
  // отреагируете ли вы на него до тех пор пока вы его не выключите.
  DMA1_InitStructure.DMA_Priority = DMA_Priority_High;// высокий приоритет
  DMA1_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; // фифо не исп.
  DMA1_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull; // относится к фифо, фифо не исп.
  DMA1_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single; // относится к пакетному режиму
  DMA1_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; // относится к пакетному режиму
  
  DMA_Init(DMA1_Stream6, &DMA1_InitStructure);
  DMA_DoubleBufferModeCmd(DMA1_Stream6,ENABLE); // включаем режим двойного буфера
  DMA_ITConfig(DMA1_Stream6, DMA_IT_TC, ENABLE); // устанавливаем прерывания от DMA по окончанию передачи
  DMA1_Stream6->M0AR = (uint32_t)First_buffer; // указатель на первый буфер
  DMA1_Stream6->M1AR = (uint32_t)Second_buffer; // указатель на второй буфер
}

// Обработчик прерывания рекордера
void Rec_IRQHandler (void)
{
    General_Handler();
}

// Обработчик прерывания проигрывателя
void DMA1_Stream6_IRQHandler(void)
{
    General_Handler();
}
