    .section .data
delay_counter: .word 0                    @ Variable global para contar segundos (usada por SysTick)

    .section .text
    .syntax unified
    .thumb

    .global main
    .global init_led
    .global init_button
    .global init_systick
    .global SysTick_Handler

// --- Definiciones de registros ------------------------------------------------
    .equ RCC_BASE,       0x40021000        @ Base del sistema de relojes (RCC)
    .equ RCC_AHB2ENR,    RCC_BASE + 0x4C   @ Registro para habilitar el reloj de GPIOA y GPIOC
    .equ GPIOA_BASE,     0x48000000        @ Base de GPIOA
    .equ GPIOA_MODER,    GPIOA_BASE + 0x00 @ Registro de modo de GPIOA
    .equ GPIOA_ODR,      GPIOA_BASE + 0x14 @ Registro de salida (Output Data Register)
    .equ LD2_PIN,        5                 @ Pin del LED LD2 (PA5)

    .equ GPIOC_BASE,     0x48000800        @ Base de GPIOC
    .equ GPIOC_MODER,    GPIOC_BASE + 0x00 @ Registro de modo de GPIOC
    .equ GPIOC_IDR,      GPIOC_BASE + 0x10 @ Registro de entrada (Input Data Register)
    .equ B1_PIN,         13                @ Pin del botón B1 (PC13)

    .equ SYST_CSR,       0xE000E010        @ Registro de control del SysTick
    .equ SYST_RVR,       0xE000E014        @ Registro de recarga (Reload value)
    .equ SYST_CVR,       0xE000E018        @ Registro de valor actual (no usado aquí)
    .equ HSI_FREQ,       4000000           @ Frecuencia interna del reloj (4 MHz)

// --- Función principal --------------------------------------------------------
main:
    bl init_led                            @ Inicializa GPIOA PA5 como salida para LED
    bl init_button                         @ Inicializa GPIOC PC13 como entrada para el botón
    bl init_systick                        @ Configura SysTick para contar cada 1 segundo

loop:
    movw r0, #:lower16:GPIOC_IDR           @ Dirección baja de GPIOC_IDR
    movt r0, #:upper16:GPIOC_IDR           @ Dirección alta de GPIOC_IDR
    ldr  r1, [r0]                          @ Leer el valor de entrada de los pines de GPIOC
    tst  r1, #(1 << B1_PIN)                @ Verifica si el pin 13 está en 1 (sin presionar)
    bne  loop                              @ Si no está presionado, sigue esperando

    @ Encender LED LD2 (PA5)
    movw r0, #:lower16:GPIOA_ODR
    movt r0, #:upper16:GPIOA_ODR
    ldr  r1, [r0]
    orr  r1, r1, #(1 << LD2_PIN)           @ Establece a 1 el bit 5 (PA5)
    str  r1, [r0]

    @ Iniciar contador de 3 segundos
    ldr r0, =delay_counter
    movs r1, #3
    str r1, [r0]

wait_led_off:
    ldr r0, =delay_counter
    ldr r1, [r0]
    cmp r1, #0
    bne wait_led_off                       @ Espera hasta que SysTick lo reduzca a 0

    @ Apagar LED LD2 (PA5)
    movw r0, #:lower16:GPIOA_ODR
    movt r0, #:upper16:GPIOA_ODR
    ldr  r1, [r0]
    bic  r1, r1, #(1 << LD2_PIN)           @ Limpia el bit 5 (PA5)
    str  r1, [r0]

    b loop                                 @ Volver al inicio y esperar otra pulsación

// --- Inicializa GPIOA PA5 como salida (LED LD2) -------------------------------
init_led:
    movw  r0, #:lower16:RCC_AHB2ENR
    movt  r0, #:upper16:RCC_AHB2ENR
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 0)                @ Habilita el reloj para GPIOA
    str   r1, [r0]

    movw  r0, #:lower16:GPIOA_MODER
    movt  r0, #:upper16:GPIOA_MODER
    ldr   r1, [r0]
    bic   r1, r1, #(0b11 << (LD2_PIN * 2)) @ Limpia los 2 bits para PA5
    orr   r1, r1, #(0b01 << (LD2_PIN * 2)) @ Configura PA5 como salida (01)
    str   r1, [r0]
    bx    lr

// --- Inicializa GPIOC PC13 como entrada (botón B1) ----------------------------
init_button:
    movw  r0, #:lower16:RCC_AHB2ENR
    movt  r0, #:upper16:RCC_AHB2ENR
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 2)                @ Habilita el reloj para GPIOC
    str   r1, [r0]

    movw  r0, #:lower16:GPIOC_MODER
    movt  r0, #:upper16:GPIOC_MODER
    ldr   r1, [r0]
    bic   r1, r1, #(0b11 << (B1_PIN * 2))  @ Configura PC13 como entrada (00)
    str   r1, [r0]
    bx    lr

// --- Configura SysTick para generar interrupciones cada 1 segundo -------------
init_systick:
    movw  r0, #:lower16:SYST_RVR
    movt  r0, #:upper16:SYST_RVR
    movw  r1, #:lower16:HSI_FREQ
    movt  r1, #:upper16:HSI_FREQ
    subs  r1, r1, #1                       @ Recarga = 4_000_000 - 1
    str   r1, [r0]

    movw  r0, #:lower16:SYST_CSR
    movt  r0, #:upper16:SYST_CSR
    movs  r1, #(1 << 0)|(1 << 1)|(1 << 2)  @ ENABLE, TICKINT y CLKSOURCE = 1
    str   r1, [r0]
    bx    lr

// --- Manejador de interrupción SysTick ----------------------------------------
    .thumb_func
SysTick_Handler:
    ldr r0, =delay_counter                 @ Carga dirección del contador
    ldr r1, [r0]                           @ Lee valor actual
    cmp r1, #0
    beq end_systick                        @ Si es 0, no decrementa
    subs r1, r1, #1                        @ Resta 1 al contador
    str r1, [r0]                           @ Guarda el nuevo valor
end_systick:
    bx lr                                  @ Retorna de la interrupción
