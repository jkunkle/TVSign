
// main functionalities
#include <avr/io.h>

#define SHIFT_RESET PB0
#define SHIFT_COPY PD7
#define SHIFT_ENABLE PD6
#define SHIFT_DATA PD5

void update_pattern(void);
void update_speed(void);
void update_brightness(void);
void run_turnon(void);
void run_wave(void);
void run_switch(void);
void run_breathe(void);
void run_race(uint8_t thickenss, int direction);
void run_sparkle(void);

// random helpers
uint8_t fill_random( struct cRGB *led, int brightness, uint8_t seed );
uint8_t random( uint8_t seed );

// sound input functions
//uint16_t ReadADC(uint8_t ADCchannel);

void USART_Init(uint16_t ubrr);
uint8_t USART_Receive(void);
void USART_Transmit( unsigned char data );

void change_color(uint8_t index, uint8_t col1, uint8_t col2, uint8_t col3);
