#include "stm8s.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"

#define NUM_LEDS 128
#define BRIGHTNESS 50

// -------------
// I2C Registers
// -------------
//
// 0-127 - [3R:3G:2B] encoding of LEDs 0-127
//
// 128 - LED pattern
//       - 0: Pinwheel
//       - 1: Horizontally-scrolling rainbow
//       - 2: Bullseye pattern
//       - 3: Vertically-scrolling rainbow
//       - 4: [3R:3G:2B] encoded values from I2C; updating live
//       - 5: Cycle through patterns 0-3
//       - 6: Set LED driver pin high-Z so you can drive it from GPIO2
// 129 - LED brightness divisor (wouldn't recommend going above 5; 12 is easy enough on the eyes)
#define I2C_MODE 128
#define I2C_BRIGHTNESS 129
static uint8_t I2C_Regs[256] = {0};
static uint8_t I2C_Index = 0;

typedef enum {
    I2C_REGISTER,
    I2C_DATA
} I2C_State_t;

static I2C_State_t I2C_State;


void I2C_interrupt(void) __interrupt (ITC_IRQ_I2C)
{
    uint8_t reg;

    if (I2C->SR1 & I2C_SR1_ADDR) // address match
    {
        reg = I2C->SR1; // clear status regs by reading from them
        reg = I2C->SR3;
        I2C_State = I2C_REGISTER;
        return;
    }

    if (I2C->SR1 & I2C_SR1_RXNE) // receive queue not empty
    {
        if (I2C_State == I2C_REGISTER)
        {
            I2C_Index = I2C->DR;
            I2C_State = I2C_DATA;
        } else {
            I2C_Regs[I2C_Index++] = I2C->DR;
        }

        return;
    }

    if (I2C->SR1 & I2C_SR1_TXE) // transmit queue empty
    {
        I2C->DR = I2C_Regs[I2C_Index++];
        return;
    }

    if (I2C->SR2 & I2C_SR2_AF) // NACK
    {
        I2C->SR2 &= ~I2C_SR2_AF;
    }

    if (I2C->SR1 & I2C_SR1_STOPF) // end of each I2C transaction; has to get cleared or the system hangs up
    {
        reg = I2C->SR1; // need to read SR1
        reg = I2C->CR2; // and write CR2
        I2C->CR2 = reg;
    }
}

// Drawn from https://blog.mark-stevens.co.uk/2015/05/stm8s-i2c-slave-device/
// Interestingly, he ignores the STOPF event interrupt
void I2C_Setup(void)
{
    I2C->CR1 &= ~I2C_CR1_PE; // disable peripheral

    I2C->FREQR = 16;
    I2C->CCRH &= ~I2C_CCRH_FS; // slave mode

    I2C->CCRH &= ~(0 | I2C_CCRH_CCR); // set clock to standard mode (50kHz)
    I2C->CCRL = 0xA0;

    I2C->OARH &= ~I2C_OARH_ADDMODE; // slave mode
    I2C->OARH |= I2C_OARH_ADDCONF; // "must be set by software"
    I2C->OARH &= ~(0 | I2C_OARH_ADD); // clearing extended address bits
    I2C->OARL = 0x37 << 1;

    I2C->TRISER = 17; //MAGIC

    I2C->ITR |= I2C_ITR_ITBUFEN | I2C_ITR_ITEVTEN | I2C_ITR_ITERREN;

    I2C->CR1 |= I2C_CR1_PE; // re-enable peripheral
    
    I2C->CR2 |= I2C_CR2_ACK; // automatic ACKs
}

void _delay_us(uint16_t microseconds) {
    TIM4->PSCR = TIM4_PRESCALER_1; // Set prescaler

    // Set count to approximately 1uS (clock/microseconds in 1 second)
    // The -1 adjusts for other runtime costs of this function
    TIM4->ARR = ((16000000L)/1000000) - 1;

    TIM4->CR1 = TIM4_CR1_CEN; // Enable counter

    for (; microseconds > 1; --microseconds) {
        while ((TIM4->SR1 & TIM4_SR1_UIF) == 0);

        // Clear overflow flag
        TIM4->SR1 &= ~TIM4_SR1_UIF;
    }
}

void ws_write_byte(uint8_t byte)
{
    for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
        if ((byte & mask) != 0) {
            // Set pin high
            __asm__("bset 20490, #5");

            // Delay for 850ns minus overheads
            nop(); nop(); nop(); nop();
            nop(); nop(); nop(); nop();

            // Set pin low
            __asm__("bres 20490, #5");

            // Delay 400ns minus overhead
            nop(); nop(); nop(); nop();

        } else {
            // Set pin high
            __asm__("bset 20490, #5");

            // Delay for 400ns minus overheads
            nop(); nop(); nop(); nop();
            nop(); nop();

            // Set pin low
            __asm__("bres 20490, #5");

            // Delay for 850ns minus overheads
            nop(); nop(); nop(); nop();

        }
    }
}

void ws_write_grb(uint8_t* colour)
{
    for (uint8_t i = 0; i < 3; ++i) {
        ws_write_byte(colour[i]);
    }
}

inline volatile void ws_reset(void)
{
    GPIOC->ODR &= ~(1<<5);

    // Pull the line low for >50uS to reset
    _delay_us(55);
}

// https://github.com/j3qq4hch/STM8_WS2812B/blob/master/ws2812b/ws2812b_fx.c
void wheel(uint8_t wheelPos, uint8_t *target)
{
    wheelPos = 256 - wheelPos;
    if (wheelPos < 85)
    {
        target[0] = 0; // green
        target[1] = 255 - wheelPos * 3; // red
        target[2] = wheelPos * 3;
    }
    else if (wheelPos < 170)
    {
        wheelPos -= 85;
        target[1] = 0;
        target[0] = wheelPos * 3;
        target[2] = 255 - wheelPos * 3;
    }
    else
    {
        wheelPos -= 170;
        target[1] = wheelPos * 3;
        target[0] = 255 - wheelPos * 3;
        target[2] = 0;
    }

    for (uint8_t i = 0; i<3; ++i)
    {
        target[i] /= I2C_Regs[I2C_BRIGHTNESS];
    }
}

static uint8_t lights[NUM_LEDS][3];
static uint8_t lights_index = 0;

void verticalRainbow(void)
{
    for (uint8_t i = 0; i < NUM_LEDS; ++i)
    {
        wheel((coordsY[i] + lights_index) & 0xFF, lights[i]);
    }
}

void horizontalRainbow(void)
{
    for (uint8_t i = 0; i < NUM_LEDS; ++i)
    {
        wheel((coordsX[i] + lights_index) & 0xFF, lights[i]);
    }
}

void fibonacci(void)
{
    for (uint8_t i = 0; i < NUM_LEDS; ++i)
    {
        wheel((i*2 + lights_index) & 0xFF, lights[fibonacciToPhysical[127-i]]);
    }
}

void pinwheel(void)
{
    for (uint8_t i = 0; i < NUM_LEDS; ++i)
    {
        wheel((i*2 + lights_index) & 0xFF, lights[i]);
    }
}

void i2c_fb(void)
{
    uint8_t pixel;
    for (uint8_t i=0; i < NUM_LEDS; ++i)
    {
        pixel = I2C_Regs[i];
        lights[i][0] = ((pixel & 0x1C) << 3) / I2C_Regs[I2C_BRIGHTNESS];
        lights[i][1] = (pixel & 0xE0) / I2C_Regs[I2C_BRIGHTNESS];
        lights[i][2] = ((pixel & 0x3) << 5) / I2C_Regs[I2C_BRIGHTNESS];
    }
}

static void (*patterns[5]) (void) = { pinwheel, horizontalRainbow, fibonacci, verticalRainbow, i2c_fb };
static uint8_t patternIndex = 0;
static uint8_t outputEnabled = 1;
static uint16_t timer2_counter = 0;

void timer2_overflow_interrupt(void) __interrupt (ITC_IRQ_TIM2_OVF)
{
    if (I2C_Regs[I2C_MODE] < 6)
    {
        if (!outputEnabled)
        {
            GPIOC->DDR |= (1<<5); // set to "output, fast push-pull"
            GPIOC->CR1 |= (1<<5);
            GPIOC->CR2 |= (1<<5);
            outputEnabled = 1;
        }

        if (I2C_Regs[I2C_MODE] < 5)
        {
            patterns[I2C_Regs[I2C_MODE]]();
        } else { // must be exactly 5, in which case it's supposed to cycle
            patterns[patternIndex]();
        }

        lights_index++; // naturally overflows at 255; sue me

        // Write to LED strip
        ws_reset();
        for (uint8_t i = 0; i < NUM_LEDS; ++i) {
            ws_write_grb(lights[i]);
        }
    }
    else if (I2C_Regs[I2C_MODE] == 6)
    {
        GPIOC->DDR &= ~(1<<5); // set to "input, floating without interrupt"
        GPIOC->CR1 &= ~(1<<5);
        GPIOC->CR2 &= ~(1<<5);
        outputEnabled = 0;
    }


    ++timer2_counter;
    // Clear the timer overflow so the interrupt doesn't fire again
    TIM2->SR1 &= ~TIM2_SR1_UIF;
}


int main(void)
{
    // Configure the clock for maximum speed on the 16MHz HSI oscillator
    // At startup the clock output is divided by 8
    CLK->CKDIVR = 0x0;

    // Pin C5 in fast push-pull mode
    // This starts high as the light looks for low signals
    GPIOC->DDR |= (1<<5);
    GPIOC->CR1 |= (1<<5);
    GPIOC->CR2 |= (1<<5);
    GPIOC->ODR &= ~(1<<5);

    // Pins B5, B4 as fast push-pull outputs
    GPIOB->DDR |= (1<<5) | (1<<4);
    GPIOB->CR1 |= (1<<5) | (1<<4);
    GPIOB->CR2 |= (1<<5) | (1<<4);
    GPIOB->ODR &= ~((1<<5) | (1<<4));

    // Configure timer 2 (16-bit general purpose)
    TIM2->PSCR = TIM2_PRESCALER_256; // Set prescaler
//    TIM2->ARRH = 0x30; // Set auto-reload value for 200ms
//    TIM2->ARRL = 0xD4;
    TIM2->ARRH = 0x02; // Set auto-reload value for 40ms
    TIM2->ARRL = 0xC4;
    TIM2->IER = TIM2_IER_UIE; // Enable interrupt
    TIM2->CR1 = TIM2_CR1_CEN; // Enable counter

    // Initialise lights array
    memset(lights, 0, sizeof(lights));

    I2C_Setup();
    I2C_Regs[I2C_BRIGHTNESS] = 12;
    I2C_Regs[I2C_MODE] = 5;

    enableInterrupts();

    outputEnabled = 1;

    uint8_t first_cycle = 1;

    for (;;) {
        if (patternIndex == 3)
        {
            patternIndex = 0;
        } else {
            ++patternIndex;
        }

        // 40ms should go into 1000ms 25 times, but it's five times faster empirically
        // So, uh, MAGIC
        while (timer2_counter <= 625);

        timer2_counter = 0;
    }
}
