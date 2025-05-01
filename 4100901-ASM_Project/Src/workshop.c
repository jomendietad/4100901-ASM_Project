#include <stdint.h>

#define WFI()       __asm volatile("wfi")

void init_led(void);
void init_button_exti(void);
void init_systick(void);

// --- Direcciones base ---
#define RCC_BASE      0x40021000U
#define RCC_AHB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0x4CU)) // Enable GPIOA, GPIOC
#define RCC_APB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0x60U)) // Enable SYSCFG clock

#define GPIOA_BASE    0x48000000U
#define GPIOA_MODER   (*(volatile uint32_t *)(GPIOA_BASE + 0x00U))
#define GPIOA_ODR     (*(volatile uint32_t *)(GPIOA_BASE + 0x14U))
#define LD2_PIN       5U  // PA5

#define GPIOC_BASE    0x48000800U
#define GPIOC_MODER   (*(volatile uint32_t *)(GPIOC_BASE + 0x00U))
#define GPIOC_IDR     (*(volatile uint32_t *)(GPIOC_BASE + 0x10U))
#define B1_PIN        13U // PC13

// --- SysTick ---
#define SYST_BASE     0xE000E010U
#define SYST_CSR      (*(volatile uint32_t *)(SYST_BASE + 0x00U))
#define SYST_RVR      (*(volatile uint32_t *)(SYST_BASE + 0x04U))
#define HSI_FREQ      4000000U // 4 MHz

// --- NVIC para interrupciones ---
#define NVIC_ISER1    (*(volatile uint32_t *)(0xE000E104U))  // ISER[1]
#define NVIC_EXTI15_10_Pos   (8U) // IRQ40 está en ISER1 bit 8

// --- SYSCFG para configurar EXTI ---
#define SYSCFG_BASE   0x40010000U
#define SYSCFG_EXTICR4 (*(volatile uint32_t *)(SYSCFG_BASE + 0x14U))

// --- EXTI (External Interrupt) ---
#define EXTI_BASE     0x40010400U
#define EXTI_IMR1     (*(volatile uint32_t *)(EXTI_BASE + 0x00U)) // Interrupt mask
#define EXTI_FTSR1    (*(volatile uint32_t *)(EXTI_BASE + 0x0CU)) // Falling trigger
#define EXTI_PR1      (*(volatile uint32_t *)(EXTI_BASE + 0x14U)) // Pending register

// --- Variables ---
volatile uint32_t delay_counter = 0;

// --- Programa Principal ---
int main(void)
{
    init_led();
    init_button_exti();
    init_systick();
    while (1) {
        WFI(); // Espera a interrupciones para ahorrar energía
    }
}

// --- Inicializa LED LD2 (PA5 como salida) ---
void init_led(void)
{
    RCC_AHB2ENR |= (1 << 0);                  // Habilita reloj GPIOA
    GPIOA_MODER &= ~(3 << (LD2_PIN * 2));      // PA5 salida
    GPIOA_MODER |=  (1 << (LD2_PIN * 2));
}

// --- Inicializa Botón B1 (PC13) con interrupción EXTI ---
void init_button_exti(void)
{
    RCC_AHB2ENR |= (1 << 2);                   // Habilita reloj GPIOC
    RCC_APB2ENR |= (1 << 0);                   // Habilita reloj SYSCFG

    GPIOC_MODER &= ~(3 << (B1_PIN * 2));        // PC13 entrada

    SYSCFG_EXTICR4 &= ~(0xF << 4);              // Limpiar bits EXTI13
    SYSCFG_EXTICR4 |=  (0x2 << 4);              // PC13 --> EXTI13

    EXTI_IMR1 |= (1 << B1_PIN);                 // Habilitar interrupción de línea 13
    EXTI_FTSR1 |= (1 << B1_PIN);                // Disparar en flanco de bajada

    NVIC_ISER1 |= (1 << NVIC_EXTI15_10_Pos);    // Habilitar IRQ EXTI15_10
}

// --- Inicializa Systick para cada 1 segundo ---
void init_systick(void)
{
    SYST_RVR = HSI_FREQ - 1;                    // 4MHz -> 1s
    SYST_CSR = (1 << 0) | (1 << 1) | (1 << 2);   // ENABLE | TICKINT | CLKSOURCE
}

// --- Manejador de Interrupción SysTick ---
void SysTick_Handler(void)
{
    if (delay_counter > 0) {
        delay_counter--;
        if (delay_counter == 0) {
            GPIOA_ODR &= ~(1 << LD2_PIN); // Apagar LED
        }
    }
}

// --- Manejador de Interrupción EXTI para PC13 (Botón B1) ---
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_PR1 & (1 << B1_PIN)) {
        EXTI_PR1 = (1 << B1_PIN);              // Limpiar bandera de interrupción
        GPIOA_ODR |= (1 << LD2_PIN);            // Encender LED
        delay_counter = 3;                     // Empezar conteo de 3 segundos
    }
}
