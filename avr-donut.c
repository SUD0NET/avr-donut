/*
 * avr-donut.c: bare-metal avr port of andy sloane's donut.c
 * specifically ported from donut w/o math lib: https://www.a1k0n.net/2021/01/13/optimizing-donut.html
 * (not so) important stuff:
 * - line-by-line rasterisation, reducing sram footprint to 160 bytes
 * - fixed-point minsky rotation macro (no float math lib)
 * - compressed render matrix iteration cycles tuned for atmega328p mcu
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>

#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 22

// 32-bit math rotation macro
#define R(mul,shift,x,y) \
    _ = x; \
    x -= (int32_t)mul * y >> shift; \
    y += (int32_t)mul * _ >> shift; \
    _ = 3145728L - (int32_t)x * x - (int32_t)y * y >> 11; \
    x = (int32_t)x * _ >> 10; \
    y = (int32_t)y * _ >> 10;

const char charset[] PROGMEM = ".,-~:;=!*#$@";

char row_buffer[SCREEN_WIDTH];
int8_t z_buffer[SCREEN_WIDTH];

void uart_init(uint32_t baud) {
    UCSR0A = (1 << U2X0); 
    uint16_t ubrr_value = (F_CPU / (8UL * baud)) - 0.5; 
    
    UBRR0H = (uint8_t)(ubrr_value >> 8);
    UBRR0L = (uint8_t)ubrr_value;
    UCSR0B = (1 << TXEN0);                  
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); 
}

void uart_putchar(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_puts_p(const char *str) {
    char c;
    while ((c = pgm_read_byte(str++))) {
        uart_putchar(c);
    }
}

int main(void) {
    uart_init(115200);
    int32_t sA = 1024, cA = 0, sB = 1024, cB = 0, _;
    for (;;) {
        uart_puts_p(PSTR("\x1b[H"));
        for (int current_y = 0; current_y < SCREEN_HEIGHT; current_y++) {
            memset(row_buffer, ' ', SCREEN_WIDTH);
            memset(z_buffer, 127, SCREEN_WIDTH);
            int32_t sj = 0, cj = 1024;
            for (int j = 0; j < 90; j += 6) {
                int32_t si = 0, ci = 1024;
                for (int i = 0; i < 324; i += 8) {
                    int32_t R1 = 1, R2 = 2048, K2 = 5120L * 1024L;
                    int32_t x0 = R1 * cj + R2;
                    int32_t x1 = ci * x0 >> 10;
                    int32_t x2 = cA * sj >> 10;
                    int32_t x3 = si * x0 >> 10;
                    int32_t x4 = R1 * x2 - (sA * x3 >> 10);
                    int32_t x5 = sA * sj >> 10;
                    int32_t x6 = K2 + R1 * 1024L * x5 + cA * x3;
                    int32_t x7 = cj * si >> 10;
                    int32_t x6_shift = x6 >> 12; 
                    if (x6_shift == 0) x6_shift = 1;
                    int32_t y = 12 + ((15 * (cB * x4 + sB * x1)) >> 12) / x6_shift;
                    if (y == current_y) {
                        int32_t x = 40 + ((30 * (cB * x1 - sB * x4)) >> 12) / x6_shift;
                        if (x > 0 && x < SCREEN_WIDTH) {
                            int8_t zz = (x6 - K2) >> 15;
                            if (zz < z_buffer[x]) {
                                z_buffer[x] = zz;
                                int32_t N = (-cA * x7 - cB * ((-sA * x7 >> 10) + x2) - ci * (cj * sB >> 10) >> 10) - x5 >> 7;
                                int32_t luminance = N > 0 ? N : 0;
                                row_buffer[x] = pgm_read_byte(&charset[luminance]);
                            }
                        }
                    }
                    R(40, 8, ci, si); 
                }
                R(54, 7, cj, sj);
            }
            for (int k = 0; k < SCREEN_WIDTH; k++) {
                uart_putchar(row_buffer[k]);
            }
            uart_putchar('\r');
            uart_putchar('\n');
        }
        R(5, 7, cA, sA);
        R(5, 8, cB, sB);
    }
}

