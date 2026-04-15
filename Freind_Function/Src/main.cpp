#include <stdint.h>
#include "STM32F407xx.h"

inline volatile uint32_t *NVIC_ISER0=reinterpret_cast<volatile uint32_t*>(0xE000E100);
inline volatile uint32_t *NVIC_ICPR0=reinterpret_cast<volatile uint32_t*>(0xE000E280);


class SimulatedSensor;   // forward declare

class LEDController {
private:
    uint32_t green_pin;    // PD12
    uint32_t orange_pin;   // PD13
    uint32_t red_pin;      // PD14

public:
    LEDController(uint32_t g, uint32_t o, uint32_t r)
        : green_pin(g), orange_pin(o), red_pin(r) {
    }

    void init() {
        RCC->AHB1ENR  |=  (1 << 3);           // GPIOD clock

        // PD12 output
        GPIOD->MODER  &= ~(3 << (12 * 2));
        GPIOD->MODER  |=  (1 << (12 * 2));

        // PD13 output
        GPIOD->MODER  &= ~(3 << (13 * 2));
        GPIOD->MODER  |=  (1 << (13 * 2));

        // PD14 output
        GPIOD->MODER  &= ~(3 << (14 * 2));
        GPIOD->MODER  |=  (1 << (14 * 2));

        // all LEDs OFF
        GPIOD->ODR    &= ~(1 << 12);
        GPIOD->ODR    &= ~(1 << 13);
        GPIOD->ODR    &= ~(1 << 14);
    }

    void allOFF() {
        GPIOD->ODR &= ~(1 << green_pin);
        GPIOD->ODR &= ~(1 << orange_pin);
        GPIOD->ODR &= ~(1 << red_pin);
    }

    // friend function declaration
    friend void updateLEDFromSensor(LEDController& led,SimulatedSensor& sensor);
};

class SimulatedSensor {
private:
    uint32_t counter;          // simulated value 0-100
    uint32_t threshold_warm;   // 30
    uint32_t threshold_hot;    // 60
    uint32_t max_count;        // resets at 100

public:
    SimulatedSensor() {
        counter        = 0;
        threshold_warm = 10;
        threshold_hot  = 20;
        max_count      = 50;
    }
    void increment() {
        counter++;
        if(counter > max_count) {
            counter = 0;   // reset back to 0
        }
    }

    // public getter
    uint32_t getCount() {
        return counter;
    }

    // friend function declaration
    friend void updateLEDFromSensor(LEDController& led,SimulatedSensor& sensor);
};

LEDController   led(12, 13, 14);
SimulatedSensor sensor;

void USART3_init(void)
{
    RCC->AHB1ENR |= (1 << 2);   // Enable GPIOC clock
    RCC->APB1ENR |= (1 << 18);  // Enable USART3 clock


    GPIOC->MODER &= ~(3 << 20);
    GPIOC->MODER |=  (2<< 20);   // Alternate function -10
    // created Array :AFR[2]  , AFR[0]=AFRL ,AFR[1]= AFRH
    GPIOC->AFR[1] &= ~(0xF << 8); // AFRH =AFR[1]- Clearing 4 bits
    GPIOC->AFR[1] |=  (7 << 8);   // AF7 (USART3) // 0111 for AF7

    // PA3 → RX
    GPIOC->MODER &= ~(3 << 22);
    GPIOC->MODER |=  (2 << 22);// 10 AFR mode
    GPIOC->AFR[1] &= ~(0xF << 12);// AFRH Register
    GPIOC->AFR[1] |=  (7 << 12); // AF7 -0111

    // USART settings
    USART3->BRR = 0x0683;      // 9600 baud @ 16 MHz(HSI)
    USART3->CR1 |= (1 << 3);   // TE
    //USART3->CR1 |= (1 << 2);   // RE
    USART3->CR1 |= (1 << 13);  // UE - USART enable
}

void USART3_write(uint8_t ch) {
    while(!(USART3->SR & (1 << 7))) {}
    USART3->DR = ch;
    while(!(USART3->SR & (1 << 6))) {}
}

void USART3_write_string(const char* str) {
    while(*str) {
        USART3_write(*str++);
    }
}

// print uint32 value
void USART3_write_number(uint32_t num) {
    char buf[12];
    int  idx = 0;

    if(num == 0) {
        USART3_write('0');
        return;
    }

    while(num > 0) {
        buf[idx++] = '0' + (num % 10);
        num /= 10;
    }

    for(int i = idx - 1; i >= 0; i--) {
        USART3_write(buf[i]);
    }
}

void TIM2_Init(void) {
    RCC->APB1ENR  |=  (1 << 0);    // TIM2 clock

    TIM2->PSC      =  1599;         // tick = 100µs @ 16MHz
    TIM2->ARR      =  9999;         // 1 second
    TIM2->CNT      =  0;

    TIM2->EGR     |=  (1 << 0);    // UG bit
    TIM2->SR      &= ~(1 << 0);    // clear UIF

    TIM2->DIER    |=  (1 << 0);    // UIE

    *NVIC_ICPR0    |=  (1 << 28);   // clear pending
   // NVIC_IPR7     &= ~(0xFF);      // clear priority
    //NVIC_IPR7     |=  (1 << 4);    // priority = 1
    *NVIC_ISER0    |=  (1 << 28);   // enable IRQ28

    TIM2->CR1     |=  (1 << 0);    // CEN — start
}

void updateLEDFromSensor(LEDController&   led,SimulatedSensor& sensor) {

    uint32_t count= sensor.counter;         // private!
    uint32_t warm_limit= sensor.threshold_warm;  // private!
    uint32_t hot_limit= sensor.threshold_hot;   // private!

    // turn off all LEDs first
    led.allOFF();

    if(count < warm_limit) {
        // GREEN — normal zone
        GPIOD->ODR |= (1 << led.green_pin);   // private!
        USART3_write_number(count);
        USART3_write_string(" ");

    }
    else if(count >= warm_limit && count < hot_limit) {

        // ORANGE — warm zone
        GPIOD->ODR |= (1 << led.orange_pin);   // private!
        USART3_write_number(count);
        USART3_write_string(" ");
    }
    else {

        // RED — hot zone
        GPIOD->ODR |= (1 << led.red_pin);   // private!
        USART3_write_number(count);
        USART3_write_string(" ");
    }
}

extern "C" void TIM2_IRQHandler(void) {
    if(TIM2->SR & (1 << 0)) {

        TIM2->SR &= ~(1 << 0);         // clear UIF FIRST

        sensor.increment();             // count++

        updateLEDFromSensor(led, sensor); // friend function
    }
}
int main(void) {

    led.init();
    USART3_init();
    TIM2_Init();
    USART3_write_string("  Friend Function Case Study    \r\n");
    USART3_write_string("  Timer Based LED Alert System  \r\n");

    while(1) {
        // everything handled in TIM2_IRQHandler
        // CPU free here
    }

    return 0;
}
