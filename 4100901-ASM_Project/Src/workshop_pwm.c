#include <stdint.h>

// GPIO
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;
    volatile uint32_t AFRH;
} GPIO_TypeDef;
#define GPIOA ((GPIO_TypeDef *)0x48000000U)

// RCC
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t ICSCR;
    volatile uint32_t CFGR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t PLLSAI1CFGR;
    volatile uint32_t PLLSAI2CFGR;
    volatile uint32_t CIER;
    volatile uint32_t CIFR;
    volatile uint32_t CICR;
    uint32_t RESERVED1;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED2;
    volatile uint32_t APB1RSTR1;
    volatile uint32_t APB1RSTR2;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED3;
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED4;
    volatile uint32_t APB1ENR1;
    volatile uint32_t APB1ENR2;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;
#define RCC ((RCC_TypeDef *)0x40021000U)

typedef struct {
    volatile uint32_t CTRL;    // Control y estado
    volatile uint32_t LOAD;    // Carga
    volatile uint32_t VAL;     // Valor actual
    volatile uint32_t CALIB;   // Calibración

} SYSYTICK_TypeDef;
#define SYSYTICK ((SYSYTICK_TypeDef *)0xE000E010U) // Dirección base de SysTick
#define HSI_FREQ    4000000U   // 4 MHz (Reloj interno)
uint32_t ms_counter = 0; // Contador de milisegundos
// TIM2
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RESERVED1;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t RESERVED2;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} TIM_TypeDef;
#define TIM2 ((TIM_TypeDef *)0x40000000U)


// Constantes
#define TIM2_CLK_FREQ   4000000U   // 4 MHz APB1 timer clock
#define PWM_FREQUENCY   100U      // 1 kHz PWM
#define PWM_PERIOD      (TIM2_CLK_FREQ / PWM_FREQUENCY)
#define PWM_DUTY_CYCLE  50         // 50% duty cycle
#define PWM_PIN         5U         // PA5 = TIM2_CH1

#define PWM_DC_TO_CCR(DC) ((PWM_PERIOD * (DC)) / 100) // Macro para calcular CCR


// Inicializa PA6 para TIM3_CH1 (AF2)
void init_gpio_pwm(void)
{
    RCC->AHB2ENR |= (1 << 0);                   // Enable GPIOA
    // PA6 alternate function mode
    GPIOA->MODER &= ~(3U << (PWM_PIN * 2));
    GPIOA->MODER |=  (2U << (PWM_PIN * 2));
    // AF2 on PA5 to TIM2_CH1
    GPIOA->AFRL  &= ~(0xFU << (PWM_PIN * 4));
    GPIOA->AFRL  |=  (1U << (PWM_PIN * 4));
}

// Inicializa TIM2 para PWM en CH1
void init_tim2_pwm(void)
{
    RCC->APB1ENR1 |= (1 << 0);                  // Enable TIM2 clock

    TIM2->PSC   = 0;                            // No prescaler
    TIM2->ARR   = PWM_PERIOD - 1;               // Period for 1 kHz

    TIM2->CCMR1 = (6U << 4);                    // PWM mode 1 on CH1
    TIM2->CCER  |= (1 << 0);                    // Enable CH1 output
    TIM2->CCR1  = PWM_DC_TO_CCR(PWM_DUTY_CYCLE);// Set duty cycle
    TIM2->CR1   |= (1 << 0);                    // Enable counter
}

void set_pwm_duty(uint8_t percent)
{
    if (percent > 100) percent = 100;
    TIM2->CCR1 = PWM_DC_TO_CCR(percent);  // Update duty cycle based on new percentage
}

void init_systick(void)
{
    SYSYTICK->LOAD = (HSI_FREQ/1000);              // Carga el valor de cuenta
    SYSYTICK->VAL  = 0;                            // Limpia el valor actual
    SYSYTICK->CTRL = (1 << 2) | (1 << 1) | (1 << 0); // Habilita SysTick, interrumpir y reloj HSI
}

int main(void)
{
    init_systick();
    init_gpio_pwm();
    init_tim2_pwm();
    uint32_t pwm_increase_tick = 0;
    uint32_t pwm_value = 0;
    while (1) {
        if (ms_counter >= pwm_increase_tick) {
            pwm_value += 5; // Incrementa el valor del ciclo de trabajo
            if (pwm_value > 100) {
                pwm_value = 0; // Reinicia el ciclo de trabajo
            }
            TIM2->CCR1 = PWM_DC_TO_CCR(pwm_value); // Actualiza el ciclo de trabajo
            pwm_increase_tick += 100; // Establece el siguiente tiempo de incremento
        }
    }
}

void SysTick_Handler(void)
{
    ms_counter++; // Incrementa el contador de milisegundos
}