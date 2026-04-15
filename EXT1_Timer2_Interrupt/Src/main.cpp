#include <stdint.h>
#include "STM32F407xx.h"

int b_pressed=0;
inline volatile uint32_t *NVIC_ISER0=reinterpret_cast<volatile uint32_t*>(0xE000E100);
inline volatile uint32_t *NVIC_ICPR0=reinterpret_cast<volatile uint32_t*>(0xE000E280);//Interrupt clear pending interrupts
// LED INIT
void LED_Init(void)
{
    RCC->AHB1ENR |= (1 << 3);   // Enable GPIOD clock

    GPIOD->MODER &= ~(3 << (15 * 2));
    GPIOD->MODER |=  (1 << (15 * 2));

    GPIOD->MODER &= ~(3 << (14 * 2));
       GPIOD->MODER |=  (1 << (14 * 2));// Output mode

    GPIOD->ODR &= ~(1 << 15); // LED OFF
    GPIOD->ODR &= ~(1 << 14);
}
void TIM2_Init(){

    RCC->APB1ENR  |=  (1 << 0);           // TIM2 clock on
    TIM2->PSC      =  1599;
    TIM2->ARR      =  9999;
    TIM2->CNT      =  0;
    TIM2->EGR     |=  (1 << 0);
    TIM2->SR      &= ~(1 << 0);
    TIM2->DIER    |=  (1 << 0);           // UIE bit
    *NVIC_ICPR0  |=  (1 << 28);
    *NVIC_ISER0  |=  (1 << 28);

    // Step 7 — start timer
    TIM2->CR1     |=  (1 << 0);           // CEN bit
}

//BUTTON EXTI INIT
void Button_EXTI_Init(void)
{
    /* Enable clocks */
    RCC->AHB1ENR |= (1 << 1);   // GPIOB
    RCC->APB2ENR |= (1 << 14);  // SYSCFG

    /* PB1 as input */
    GPIOB->MODER &= ~(3 << (1 * 2));

    /* Pull-down */
    GPIOB->PUPDR &= ~(3 << (1 * 2));
    GPIOB->PUPDR |=  (2 << (1 * 2));  // Pull-down

    /* Map EXTI1 to PB1 */
    SYSCFG->EXTICR[0] &= ~(0xF << 4);  // 0001 = PB1
    SYSCFG->EXTICR[0]|=(1<<4);
    //MAP EXTI4 to PC4
    SYSCFG->EXTICR[1]&=~(0XF<<0);
    SYSCFG->EXTICR[1]|=(2<<0);


    /* Rising edge trigger */
    EXTI->RTSR |= (1 << 1);
    EXTI->RTSR|=(1<<4);

    /* Unmask interrupt */
    EXTI->IMR |= (1 << 1);
    EXTI->IMR |= (1 << 4);

    /* NVIC Enable (IRQ6 for EXTI0) */
    *NVIC_ISER0|= (1 << 7);//ISER0 EXTI1
    *NVIC_ISER0|=(1<<10);
}

//ISR
extern "C" void EXTI1_IRQHandler(void)
{
    if (EXTI->PR & (1 << 1))  // Check pending
    {
        GPIOD->ODR |= (1 << 14); // SET LED

        EXTI->PR |= (1 << 1); // Clear pending
    }
}
extern "C" void EXTI4_IRQHandler(void)
{
    if (EXTI->PR & (1 << 4))  // Check pending
    {
    	GPIOD->ODR &= ~(1 << 14);
        EXTI->PR |= (1 << 4); // Clear pending
    }
}
extern "C" void TIM2_IRQHandler(void) {
    if(TIM2->SR & (1 << 0)) {
        TIM2->SR   &= ~(1 << 0);
                   GPIOD->ODR |= (1 << 15);     // toggle PD15
           }
 }
int main(void)
{
    LED_Init();
    TIM2_Init();
    Button_EXTI_Init();

    while (1)
    {

    }
}
