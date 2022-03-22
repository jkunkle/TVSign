//
// LED Driver for TV sign
//
//
// Utilizes Light_WS2812 library for LED communication

// These are the main TV colors
// The colors must be matched to
// RGB values. Since this is light
// and not pigment, the produced
// colors will not perfectly replicate
// the original colors
// VIOLET : #8B3E6E RED=16, BLUE=2-4
// CYAN : #4DBAC3 BLUE=16 GREEN=12-16
// DIRTY YELLOW : #C6C474 RED=16 GREEN=5-6
// EGGSHELL WHITE : #E3E1CD: RED=16, GREEN=6 BLUE=1
//
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "light_ws2812.h"
#include "tvpatterns.h"

// Number of Violet LEDs
#define _N_LED_VIOLET 170
// Number of Beige LEDs
#define _N_LED_BEIGE 83
// Number of Yellow LEDs
#define _N_LED_YELLOW 116
// Number of Cyan LEDs
#define _N_LED_CYAN 170
#define _MAX_LED _N_LED_VIOLET + _N_LED_BEIGE + _N_LED_YELLOW + _N_LED_CYAN + 1
// Total number of patterns (increase if patterns are added)
#define _N_PAT 7
// Number of steps in race pattern
#define _N_RACE_STEPS 63
#define _N_RACE_STEPS_BEIGE 39
#define _N_RACE_STEPS_YELLOW 55

// Define cutoffs for when to move
// to the next pattern
// can be disabled by sending the 
// toggle_auto_update change
#define _MAX_WAVE 100
#define _MAX_SWITCH 20
#define _MAX_BREATHE 50
#define _MAX_RACE 5000
#define _MAX_SPARKLE 1000

// maximum brightness factor
// if it is set too high it 
// will draw too much current
// for the power supply
#define _MAX_BRIGHTNESS 4

// define the start LED of each color
#define _START_VIOLET 0
#define _START_BEIGE _N_LED_VIOLET
#define _START_YELLOW _N_LED_VIOLET + _N_LED_BEIGE
#define _START_CYAN _N_LED_VIOLET + _N_LED_BEIGE + _N_LED_YELLOW

// defines for buttons
#define BUTTON_PATTERN 0
#define BUTTON_SPEED 1
#define BUTTON_BRIGHT 2

FUSES = 
{
    .low = 0xff, 
    .high = HFUSE_DEFAULT,
    .extended = EFUSE_DEFAULT,
};


// Store the colors for all LEDs
struct cRGB led[_MAX_LED];


// Minimum brightness color patterns
// for each displayed color
// order is {R, G, B}
volatile uint8_t violet[3] = {8, 0, 1};
volatile uint8_t cyan[3] = {0, 3, 4};
volatile uint8_t yellow[3] = {8, 3, 0};
volatile uint8_t beige[3] = {8, 3, 1};

// define the delay limits for each pattern
// these should be set so that the pattern
// cannot become too slow or too fast
const uint8_t max_delays[_N_PAT] = {16,64, 64, 64, 64, 64, 32};
const uint8_t min_delays[_N_PAT] = {16,1 , 1 , 1 , 1 , 1, 1};
const uint8_t nom_delays[_N_PAT] = {4,16 , 64 , 64 , 4 , 4, 8};

// default the starting delay
volatile uint8_t DELAY = nom_delays[0];

const uint8_t MAX_RACE_WIDTH = 60;
volatile uint8_t race_width = 10;
volatile uint8_t sparkle_count = 8;
volatile int raceStepVioletCyan = 0;
volatile int raceStepBeige = 0;
volatile int raceStepYellow = 0;
volatile int disable_auto_update = 0;

volatile int n_wave = 0;
volatile int n_switch = 0;
volatile int n_breathe = 0;
volatile int n_race = 0;
volatile int n_sparkle = 0;

// Define the LED configurations for each step
// in the race pattern.  There are 12 entries
// per step, separated to have 3 per color where
// each of the 3 corresponds to a different LED ring
// entries 0-2 = violet
// entries 3-5 = beige 
// entries 6-8 = yellow
// entries 9-11 = cyan
// generally the first in a group corresponds to the
// inner ring, the second to the middle ring
// and the third to the outer ring
const uint16_t steps_race_violet[_N_RACE_STEPS][3] PROGMEM = {
                {0,   92,  120},
                {1,   93,  119},
                {2,   94,  118},
                {3,   95,  117},
                {4,   96,  116},
                {5,   97,  115},
                {6,   98,  114},
                {7,   99,  113},
                {8,   100, 112},
                {9,   101, 111},
                {9,   102, 110},
                {9,   102, 109},
                {10,  103, 108},
                {10,  104, 107},
                {10,  105, 106},
                {10,  11,  12},
                {10,  11,  13},
                {10,  52,  14},
                {53,  51,  15},
                {54,  50,  16},
                {55,  49,  17},
                {56,  48,  18},
                {57,  47,  19},
                {58,  46,  20},
                {59,  45,  21},
                {60,  44,  22},
                {61,  43,  23},
                {62,  42,  24},
                {63,  41,  25},
                {64,  40,  26},
                {65,  39,  27},
                {66,  38,  28},
                {67,  37,  29},
                {68,  36,  30},
                {69,  35,  31},
                {69,  34,  32},
                {69,  34,  33},
                {70,  144, 145},
                {70,  143, 146},
                {70,  143, 147},
                {70,  142, 148},
                {71,  141, 149},
                {72,  140, 150},
                {73,  139, 151},
                {74,  138, 152},
                {75,  137, 153},
                {76,  136, 154},
                {77,  135, 155},
                {78,  134, 156},
                {79,  133, 157},
                {80,  132, 158},
                {81,  131, 159},
                {82,  130, 160},
                {83,  129, 161},
                {84,  128, 162},
                {85,  127, 163},
                {86,  126, 164},
                {87,  125, 165},
                {88,  124, 166},
                {89,  123, 167},
                {89,  122, 168},
                {89,  122, 169},
                {89,  90,  91},
};

const uint16_t steps_race_beige[_N_RACE_STEPS_BEIGE][3] PROGMEM = {
                {172, 210, 237},
                {173, 211, 238},
                {174, 212, 239},
                {175, 213, 240},
                {176, 214, 241},
                {177, 215, 242},
                {178, 216, 243},
                {179, 217, 244},
                {180, 218, 244},
                {181, 219, 244},
                {182, 220, 244},
                {183, 221, 244},
                {184, 221, 244},
                {185, 221, 244},
                {186, 221, 244},
                {187, 221, 244},
                {188, 221, 244},
                {189, 221, 244},
                {190, 222, 244},
                {191, 223, 244},
                {192, 224, 244},
                {193, 225, 244},
                {194, 226, 245},
                {195, 227, 246},
                {196, 228, 247},
                {197, 229, 248},
                {198, 230, 249},
                {199, 231, 250},
                {200, 232, 251},
                {201, 233, 251},
                {202, 233, 251},
                {203, 233, 251},
                {204, 234, 251},
                {205, 235, 252},
                {206, 236, 252},
                {207, 236, 252},
                {208, 236, 252},
                {170, 209, 252},
                {171, 209, 252},
};

const uint16_t steps_race_yellow[_N_RACE_STEPS_YELLOW][3] PROGMEM = {
                {255, 308, 347},
                {256, 309, 348},
                {257, 310, 349},
                {258, 311, 350},
                {259, 312, 351},
                {260, 313, 352},
                {261, 314, 353},
                {262, 315, 354},
                {263, 316, 355},
                {264, 317, 356},
                {265, 318, 357},
                {266, 319, 357},
                {267, 320, 357},
                {268, 321, 357},
                {269, 322, 357},
                {270, 323, 357},
                {271, 324, 357},
                {272, 325, 357},
                {273, 325, 357},
                {274, 325, 357},
                {275, 325, 357},
                {276, 325, 357},
                {277, 325, 357},
                {278, 325, 357},
                {279, 325, 357},
                {280, 325, 357},
                {281, 325, 357},
                {282, 325, 357},
                {283, 326, 357},
                {284, 327, 357},
                {285, 328, 357},
                {286, 329, 357},
                {287, 330, 357},
                {288, 331, 357},
                {289, 332, 357},
                {290, 333, 358},
                {291, 334, 359},
                {292, 335, 360},
                {293, 336, 361},
                {294, 337, 362},
                {295, 338, 363},
                {296, 339, 364},
                {297, 340, 365},
                {298, 341, 366},
                {299, 342, 366},
                {300, 342, 366},
                {301, 343, 367},
                {302, 344, 367},
                {303, 344, 367},
                {304, 344, 367},
                {305, 345, 367},
                {306, 346, 368},
                {253, 307, 347},
                {254, 307, 347},
                {255, 308, 347},
                            };

const uint16_t steps_race_cyan[_N_RACE_STEPS][12] PROGMEM = {
                {539, 513, 512},
                {538, 514, 511},
                {537, 515, 510},
                {536, 516, 509},
                {535, 517, 508},
                {534, 518, 507},
                {533, 519, 506},
                {532, 520, 505},
                {531, 521, 504},
                {530, 522, 503},
                {530, 523, 502},
                {530, 523, 501},
                {529, 524, 500},
                {529, 525, 499},
                {529, 525, 498},
                {529, 528, 527},
                {529, 528, 461},
                {529, 462, 460},
                {497, 463, 459},
                {496, 464, 458},
                {495, 465, 457},
                {494, 466, 456},
                {493, 467, 455},
                {492, 468, 454},
                {491, 469, 453},
                {490, 470, 452},
                {489, 471, 451},
                {488, 472, 450},
                {487, 473, 449},
                {486, 474, 448},
                {485, 475, 447},
                {484, 476, 446},
                {483, 477, 445},
                {482, 478, 444},
                {481, 479, 443},
                {481, 480, 442},
                {481, 480, 441},
                {440, 394, 393},
                {440, 394, 392},
                {440, 395, 391},
                {439, 396, 390},
                {438, 397, 389},
                {437, 398, 388},
                {436, 399, 387},
                {435, 400, 386},
                {434, 401, 385},
                {433, 402, 384},
                {432, 403, 383},
                {431, 404, 382},
                {430, 405, 381},
                {429, 406, 380},
                {428, 407, 379},
                {427, 408, 378},
                {426, 409, 377},
                {425, 410, 376},
                {424, 411, 375},
                {423, 412, 374},
                {422, 413, 373},
                {421, 414, 372},
                {420, 415, 371},
                {420, 416, 370},
                {420, 416, 369},
                {420, 419, 418},
};


// define a mapping of colors
// to sides for the switch pattern
const uint8_t color_patterns[12][4] PROGMEM = {
                {0, 1, 2, 3},
                {1, 2, 3, 0},
                {2, 3, 0, 1},
                {3, 0, 1, 2},
                {0, 2, 1, 3},
                {1, 0, 3, 2},
                {2, 0, 1, 3},
                {3, 1, 0, 2},
                {0, 3, 1, 2},
                {1, 0, 2, 3},
                {2, 3, 0, 1},
                {3, 2, 1, 0},
};

// store the ranges of each set of
// colors for convenience in the
// switch pattern
const uint16_t color_ranges[4][2] PROGMEM = {
    {_START_VIOLET, _START_BEIGE},
    {_START_BEIGE, _START_YELLOW},
    {_START_YELLOW, _START_CYAN},
    {_START_CYAN, _MAX_LED},
};


//store the current pattern
volatile int ipat = 0; 
//store the current step
volatile uint16_t istep = 0;
volatile uint16_t iglobalStep = 0;
volatile uint16_t ibigGlobalStep = 0;




//a bool for ending updates
//currently only used for the startup pattern
volatile int stop_updates = 0;

// variables for startup pattern
volatile int idirection = 1;
volatile int isub = 0;

// brightness, default to maximum
volatile uint8_t brightness = _MAX_BRIGHTNESS;

// storage for active buttons
volatile uint8_t active_buttons = 0;

// define interrupts
uint8_t TCCR1B_SEL = (1 << CS11 ) | (1 << CS10 );

// define interrupt for receiving
// data from bluetooth module
ISR(USART_RX_vect)
{
    uint8_t res1 = USART_Receive();
    uint8_t res2 = USART_Receive();

    if( res1 == 0x4a && res2 == 0x01 ) {
        // Move to the next pattern
        update_pattern();
    }
    if( res1 == 0x4a && res2 == 0x02 ) {
        // Increase speed
        // If at max speed, return
        // to slowest
        update_speed();
    }
    if( res1 == 0x4a && res2 == 0x03 ) {
        // Decrease brightness
        // If at min brightness, return
        // to maximum
        update_brightness();
    }
    if( res1 == 0x4a && res2 == 0x04 ) {
        // do not auto update the pattern

        if( disable_auto_update == 0) {
            disable_auto_update = 1;
        }
        else if( disable_auto_update == 1 ){
            disable_auto_update = 0;
        }
        
    }

    if( res1 == 0xa4){
        // map one color (res2) 
        // into new RGB values
        // Each of 1 byte
        //
        // Transmit a byte to 
        // request additional
        // data
        USART_Transmit(1);
        uint8_t col1 = USART_Receive();
        uint8_t col2 = USART_Receive();
        uint8_t col3 = USART_Receive();
        change_color(res2, col1, col2, col3);
    }
    if( res1 == 0xa5){
        race_width = res2;
        if (race_width > MAX_RACE_WIDTH) {
            race_width = MAX_RACE_WIDTH;
        }
    }
    if( res1 == 0xa6){
        sparkle_count = res2;
    }
        
}

// define the interrupts for button
// pushes.  
// when a button is pushed, the 
// active_buttons variable is updated
// and a timer is started which will
// catch any additional button pushes
// within the window.  This prevents
// multiple activations from button bounce
// and should be smoother
ISR( TIMER1_OVF_vect ) {
    
    TCCR1B &= ~TCCR1B_SEL;

    if( active_buttons & (1 << BUTTON_PATTERN) ){
        update_pattern();
    }
    else if( active_buttons & ( 1 << BUTTON_SPEED ) ) {
        update_speed();
    }
    else if( active_buttons & (1 << BUTTON_BRIGHT ) ) {
        update_brightness();
    }
    active_buttons = 0;

}

ISR(INT0_vect)
{
    active_buttons |= (1 << BUTTON_PATTERN);
    TCCR1B |= TCCR1B_SEL;
}

ISR(INT1_vect)
{
    active_buttons |= (1 << BUTTON_SPEED);
    TCCR1B |= TCCR1B_SEL;
}

ISR(PCINT2_vect){
    
    active_buttons |= (1 << BUTTON_BRIGHT);
    TCCR1B |= TCCR1B_SEL;
}

// Update to the next pattern
// or return to the first pattern if
// at the end
// also set all LEDs to off in prep
// for next pattern
void update_pattern()
{
    ipat++;
    istep = 0;
    iglobalStep = 0;
    ibigGlobalStep = 0;
    if( ipat >= _N_PAT ) { 
        ipat = 1;
    }
    DELAY = nom_delays[ipat];

    for( int il = 0 ; il < _MAX_LED; il++ ) {
        led[il].r=0;
        led[il].g=0;
        led[il].b=0;
    }

    ws2812_setleds(led,_MAX_LED);
}

// update the speed of the pattern
// store the step at which the
// current pattern would be and
// fix any jumps in the pattern
// that the delay change would cause
void update_speed()
{
    int prev_step = istep/DELAY;
    if( DELAY == 1) {
        DELAY=max_delays[ipat];
    } else{
        DELAY /= 2;
    }
    if( istep/DELAY > (prev_step + 1) ) {
        istep = ( prev_step + 1)*DELAY;
    }
}

// decrease brightness
// if at minium go to maximum
void update_brightness()
{
    brightness = brightness - 1;
    if( brightness <= 0 )
    {
        brightness = _MAX_BRIGHTNESS;
    }
}

int main(void)
{
    TIMSK1 = ( 1 << TOIE1 );
    PORTD |= ( 1 << PD2 ) | (1 << PD3 ) | (1 << PD4 ); // enable PORTD.2, PORTD.3, PORTD.4 pin pull up resistor
    DDRD |= ( 1 << PD5 ) | ( 1 << PD6 ) | ( 1 << PD7 );
    DDRB |= ( 1 << PB0 );
    EIMSK |= (1<<INT0) | ( 1 << INT1 );  // enable external interrupt 0
    EICRA |= (1<<ISC01) | (1 << ISC11 ); // interrupt on falling edge
    // enable interrupt on PCINT20
    PCICR |= (1 << PCIE2);
    PCMSK2 = 0;
    PCMSK2 |= (1 << PCINT20);

    sei();

    // initialize bluetooth interface
    USART_Init(207);

    _delay_ms(100);
    while(1) {
        
        if( ipat == 0 ) { 
           run_turnon();
           if( ibigGlobalStep >= 10 ) {
               update_pattern();
           }
        }
        if( ipat == 1 ) { 
            run_wave();
        }
        if( ipat == 2 ) { 
            run_switch();
        }
        if( ipat == 3 ) { 
            run_breathe();
        }
        if( ipat == 4 ) { 
            run_race(race_width, 0);
        }
        if( ipat == 5 ) { 
            run_race(race_width, 1);
        }
        else if( ipat == 6 ) { 
            run_sparkle();
        }

        istep++;
        iglobalStep++;
        if( iglobalStep == 0 ){
            ibigGlobalStep++;
        }

    }

}


// Turnon pattern
//
// make a gradual increase
// to the maximal brightness
// and then stay there
void run_turnon(){

    if( istep > 13 ) { 
        stop_updates = 1;
    }
    if( stop_updates == 1 ) { 
        return;
    }
    for( int il = _START_VIOLET ; il < _START_BEIGE; il++ ) {

        led[il].r=violet[0]*istep;
        led[il].g=violet[1]*istep;
        led[il].b=violet[2]*istep;
    }
    for( int il = _START_BEIGE ; il < _START_YELLOW; il++ ) {
        led[il].r=beige[0]*istep;
        led[il].g=beige[1]*istep;
        led[il].b=beige[2]*istep;
    }
    for( int il = _START_YELLOW ; il < _START_CYAN; il++ ) {
        led[il].r=yellow[0]*istep;
        led[il].g=yellow[1]*istep;
        led[il].b=yellow[2]*istep;
    }
    for( int il = _START_CYAN ; il < _MAX_LED; il++ ) {
        led[il].r=cyan[0]*istep;
        led[il].g=cyan[1]*istep;
        led[il].b=cyan[2]*istep;
    }
    ws2812_setleds(led,_MAX_LED);
    //_delay_ms(DELAY); 

}

// Wave pattern
//
// Illuminate all LEDs on one ring 
// on each side.  At each step move
// one ring out.  Return to first ring 
// from outside ring
void run_wave() {

    if( istep/DELAY >= 3 ) { 
        istep = 0;
        n_wave++;
    }
    if( n_wave >= _MAX_WAVE && disable_auto_update == 0) {
        update_pattern();
        n_wave = 0;
    }

    for( int il = 0 ; il < _MAX_LED; il++ ) {
        led[il].r=0;
        led[il].g=0;
        led[il].b=0;
    }
    if( istep/DELAY == 0 ) { 
        for( int il = 0 ; il < 11; il++ ) {
            led[il].r=violet[0]*brightness;
            led[il].g=violet[1]*brightness;
            led[il].b=violet[2]*brightness;
        }
        for( int il = 53 ; il < 70; il++ ) {
            led[il].r=violet[0]*brightness;
            led[il].g=violet[1]*brightness;
            led[il].b=violet[2]*brightness;
        }
        for( int il = 74 ; il < 90; il++ ) {
            led[il].r=violet[0]*brightness;
            led[il].g=violet[1]*brightness;
            led[il].b=violet[2]*brightness;
        }
        for( int il = 237 ; il < 253; il++ ) {
            led[il].r=beige[0]*brightness;
            led[il].g=beige[1]*brightness;
            led[il].b=beige[2]*brightness;
        }
        for( int il = 347 ; il < 369; il++ ) {
            led[il].r=yellow[0]*brightness;
            led[il].g=yellow[1]*brightness;
            led[il].b=yellow[2]*brightness;
        }
        for( int il = 420 ; il < 437; il++ ) {
            led[il].r=cyan[0]*brightness;
            led[il].g=cyan[1]*brightness;
            led[il].b=cyan[2]*brightness;
        }
        for( int il = 481 ; il < 498; il++ ) {
            led[il].r=cyan[0]*brightness;
            led[il].g=cyan[1]*brightness;
            led[il].b=cyan[2]*brightness;
        }
        for( int il = 529 ; il < _MAX_LED; il++ ) {
            led[il].r=cyan[0]*brightness;
            led[il].g=cyan[1]*brightness;
            led[il].b=cyan[2]*brightness;
        }

    }
    else if( istep/DELAY == 1) {
        led[11].r=violet[0]*brightness;
        led[11].g=violet[1]*brightness;
        led[11].b=violet[2]*brightness;

        led[72].r=violet[0]*brightness;
        led[72].g=violet[1]*brightness;
        led[72].b=violet[2]*brightness;

        led[90].r=violet[0]*brightness;
        led[90].g=violet[1]*brightness;
        led[90].b=violet[2]*brightness;

        led[437].r=cyan[0]*brightness;
        led[437].g=cyan[1]*brightness;
        led[437].b=cyan[2]*brightness;

        led[419].r=cyan[0]*brightness;
        led[419].g=cyan[1]*brightness;
        led[419].b=cyan[2]*brightness;

        led[528].r=cyan[0]*brightness;
        led[528].g=cyan[1]*brightness;
        led[528].b=cyan[2]*brightness;

        for( int il = 34 ; il < 53; il++ ) {
            led[il].r=violet[0]*brightness;
            led[il].g=violet[1]*brightness;
            led[il].b=violet[2]*brightness;
        }
        for( int il = 92 ; il < 105; il++ ) {
            led[il].r=violet[0]*brightness;
            led[il].g=violet[1]*brightness;
            led[il].b=violet[2]*brightness;
        }
        for( int il = 122 ; il < 142; il++ ) {
            led[il].r=violet[0]*brightness;
            led[il].g=violet[1]*brightness;
            led[il].b=violet[2]*brightness;
        }
        for( int il = 209 ; il < 237; il++ ) {
            led[il].r=beige[0]*brightness;
            led[il].g=beige[1]*brightness;
            led[il].b=beige[2]*brightness;
        }
        for( int il = 307 ; il < 347; il++ ) {
            led[il].r=yellow[0]*brightness;
            led[il].g=yellow[1]*brightness;
            led[il].b=yellow[2]*brightness;
        }
        for( int il = 397 ; il < 417; il++ ) {
            led[il].r=cyan[0]*brightness;
            led[il].g=cyan[1]*brightness;
            led[il].b=cyan[2]*brightness;
        }
        for( int il = 462 ; il < 481; il++ ) {
            led[il].r=cyan[0]*brightness;
            led[il].g=cyan[1]*brightness;
            led[il].b=cyan[2]*brightness;
        }
        for( int il = 513 ; il < 527; il++ ) {
            led[il].r=cyan[0]*brightness;
            led[il].g=cyan[1]*brightness;
            led[il].b=cyan[2]*brightness;
        }
    }
    else if( istep/DELAY == 2) {
        led[12].r=violet[0]*brightness;
        led[12].g=violet[1]*brightness;
        led[12].b=violet[2]*brightness;

        led[70].r=violet[0]*brightness;
        led[70].g=violet[1]*brightness;
        led[70].b=violet[2]*brightness;

        led[91].r=violet[0]*brightness;
        led[91].g=violet[1]*brightness;
        led[91].b=violet[2]*brightness;

        led[110].r=violet[0]*brightness;
        led[110].g=violet[1]*brightness;
        led[110].b=violet[2]*brightness;

        led[418].r=cyan[0]*brightness;
        led[418].g=cyan[1]*brightness;
        led[418].b=cyan[2]*brightness;

        led[417].r=cyan[0]*brightness;
        led[417].g=cyan[1]*brightness;
        led[417].b=cyan[2]*brightness;
        
        led[440].r=cyan[0]*brightness;
        led[440].g=cyan[1]*brightness;
        led[440].b=cyan[2]*brightness;

        led[394].r=cyan[0]*brightness;
        led[394].g=cyan[1]*brightness;
        led[394].b=cyan[2]*brightness;

        led[512].r=cyan[0]*brightness;
        led[512].g=cyan[1]*brightness;
        led[512].b=cyan[2]*brightness;

        led[527].r=cyan[0]*brightness;
        led[527].g=cyan[1]*brightness;
        led[527].b=cyan[2]*brightness;

        led[526].r=cyan[0]*brightness;
        led[526].g=cyan[1]*brightness;
        led[526].b=cyan[2]*brightness;

        for( int il = 13 ; il < 34; il++ ) {
            led[il].r=violet[0]*brightness;
            led[il].g=violet[1]*brightness;
            led[il].b=violet[2]*brightness;
        }
        for( int il = 105 ; il < 122; il++ ) {
            led[il].r=violet[0]*brightness;
            led[il].g=violet[1]*brightness;
            led[il].b=violet[2]*brightness;
        }
        for( int il = 144 ; il < 170; il++ ) {
            led[il].r=violet[0]*brightness;
            led[il].g=violet[1]*brightness;
            led[il].b=violet[2]*brightness;
        }
        for( int il = 170 ; il < 209; il++ ) {
            led[il].r=beige[0]*brightness;
            led[il].g=beige[1]*brightness;
            led[il].b=beige[2]*brightness;
        }
        for( int il = 253 ; il < 307; il++ ) {
            led[il].r=yellow[0]*brightness;
            led[il].g=yellow[1]*brightness;
            led[il].b=yellow[2]*brightness;
        }
        for( int il = 369 ; il < 394; il++ ) {
            led[il].r=cyan[0]*brightness;
            led[il].g=cyan[1]*brightness;
            led[il].b=cyan[2]*brightness;
        }
        for( int il = 441 ; il < 462; il++ ) {
            led[il].r=cyan[0]*brightness;
            led[il].g=cyan[1]*brightness;
            led[il].b=cyan[2]*brightness;
        }
        for( int il = 498 ; il < 513; il++ ) {
            led[il].r=cyan[0]*brightness;
            led[il].g=cyan[1]*brightness;
            led[il].b=cyan[2]*brightness;
        }
    }
    ws2812_setleds(led,_MAX_LED);

}

// Switch pattern
// Light all LEDs
// swap colors between sides
void run_switch(){


    uint16_t patStep = istep/DELAY;
    if( patStep >= 12 ) { 
        n_switch++;
        istep = 0;
    }
    if(n_switch >= _MAX_SWITCH && disable_auto_update == 0){
        n_switch = 0;
        update_pattern();
    }

    uint8_t pat0  = pgm_read_byte(&(color_patterns[patStep][0]));
    uint8_t pat1  = pgm_read_byte(&(color_patterns[patStep][1]));
    uint8_t pat2  = pgm_read_byte(&(color_patterns[patStep][2]));
    uint8_t pat3  = pgm_read_byte(&(color_patterns[patStep][3]));

    uint16_t startv = pgm_read_word(&(color_ranges[pat0][0]));
    uint16_t endv = pgm_read_word(&(color_ranges[pat0][1]));
    uint16_t startb = pgm_read_word(&(color_ranges[pat1][0]));
    uint16_t endb = pgm_read_word(&(color_ranges[pat1][1]));
    uint16_t starty = pgm_read_word(&(color_ranges[pat2][0]));
    uint16_t endy = pgm_read_word(&(color_ranges[pat2][1]));
    uint16_t startc = pgm_read_word(&(color_ranges[pat3][0]));
    uint16_t endc = pgm_read_word(&(color_ranges[pat3][1]));

    for( int il = startv; il < endv; il++ ) {
        led[il].r=violet[0]*brightness;
        led[il].g=violet[1]*brightness;
        led[il].b=violet[2]*brightness;
    }
    for( int il = startb; il < endb; il++ ) {
        led[il].r=beige[0]*brightness;
        led[il].g=beige[1]*brightness;
        led[il].b=beige[2]*brightness;
    }
    for( int il = starty; il < endy; il++ ) {
        led[il].r=yellow[0]*brightness;
        led[il].g=yellow[1]*brightness;
        led[il].b=yellow[2]*brightness;
    }
    for( int il = startc; il < endc; il++ ) {
        led[il].r=cyan[0]*brightness;
        led[il].g=cyan[1]*brightness;
        led[il].b=cyan[2]*brightness;
    }

    ws2812_setleds(led,_MAX_LED);
}

// Breathe pattern
// Illuminate all LEDs
// gradually increase brightness
// with two speeds, then
// reverse 
void run_breathe(){

    
    // when at step 20
    // switch directions 
    if( istep >= 20 ) { 
        n_breathe++;
        istep = 0;
        if( idirection == 0 ) {
            idirection = 1 ;
        }
        else {
            idirection = 0 ;
        }
    }
    if( n_breathe >= _MAX_BREATHE && disable_auto_update == 0 ){
        n_breathe = 0;
        update_pattern();
    }

    // fast steps
    if( idirection == 0 ) {
        if( istep <= 10 ) {
            isub = istep;
        }
        else {
            isub = ( istep - 10 )/2 + 10;
        }

        for( int il = _START_VIOLET ; il < _START_BEIGE; il++ ) {
            led[il].r=violet[0]*(14-isub);
            led[il].g=violet[1]*(14-isub);
            led[il].b=violet[2]*(14-isub);
        }
        for( int il = _START_BEIGE; il < _START_YELLOW; il++ ) {
            led[il].r=beige[0]*(14-isub);
            led[il].g=beige[1]*(14-isub);
            led[il].b=beige[2]*(14-isub);
        }
        for( int il = _START_YELLOW; il < _START_CYAN; il++ ) {
            led[il].r=yellow[0]*(14-isub);
            led[il].g=yellow[1]*(14-isub);
            led[il].b=yellow[2]*(14-isub);
        }
        for( int il = _START_CYAN; il < _MAX_LED; il++ ) {
            led[il].r=cyan[0]*(14-isub);
            led[il].g=cyan[1]*(14-isub);
            led[il].b=cyan[2]*(14-isub);
        }
    } 
    //slow steps
    else {
        if( istep >= 5 ) {
            isub = istep;
        }
        else {
            isub = ( istep )/2;
        }
        for( int il = _START_VIOLET ; il < _START_BEIGE; il++ ) {
            led[il].r=violet[0]*(isub+1);
            led[il].g=violet[1]*(isub+1);
            led[il].b=violet[2]*(isub+1);
        }
        for( int il = _START_BEIGE; il < _START_YELLOW; il++ ) {
            led[il].r=beige[0]*(isub+1);
            led[il].g=beige[1]*(isub+1);
            led[il].b=beige[2]*(isub+1);
        }
        for( int il = _START_YELLOW; il < _START_CYAN; il++ ) {
            led[il].r=yellow[0]*(isub+1);
            led[il].g=yellow[1]*(isub+1);
            led[il].b=yellow[2]*(isub+1);
        }
        for( int il = _START_CYAN; il < _MAX_LED; il++ ) {
            led[il].r=cyan[0]*(isub+1);
            led[il].g=cyan[1]*(isub+1);
            led[il].b=cyan[2]*(isub+1);
        }
    }
    ws2812_setleds(led,_MAX_LED);
    _delay_ms(DELAY);

}
// Racetrack pattern
//
// Illuminate a section of LEDs
// that travels around the rings
// synchronously

void run_race(uint8_t thickness, int direction){


    int this_loc_violet_cyan = 0;
    int this_loc_beige = 0;
    int this_loc_yellow = 0;
       
    for( int il = 0 ; il < _MAX_LED; il++ ) {
        led[il].r=0;
        led[il].g=0;
        led[il].b=0;
    }

    if(n_race >= _MAX_RACE && disable_auto_update == 0 ){
        n_race = 0;
        update_pattern();
    }

    if(istep % DELAY == 0) {
        n_race++;

        // forward
        if( direction == 0 ){
            raceStepVioletCyan += 1;
            raceStepBeige += 1;
            raceStepYellow += 1;

            if( raceStepVioletCyan >= _N_RACE_STEPS ) { 
                raceStepVioletCyan = 0;
            }
            if( raceStepBeige >= _N_RACE_STEPS_BEIGE) { 
                raceStepBeige = 0;
            }
            if( raceStepYellow >= _N_RACE_STEPS_YELLOW) { 
                raceStepYellow = 0;
            }
        }
        //reverse
        if( direction == 1) {
            raceStepVioletCyan -= 1;
            raceStepBeige -= 1;
            raceStepYellow -= 1;

            if( raceStepVioletCyan < 0 ) { 
                raceStepVioletCyan = _N_RACE_STEPS - 1;
            }
            if( raceStepBeige < 0) { 
                raceStepBeige = _N_RACE_STEPS_BEIGE - 1;
            }
            if( raceStepYellow < 0 ) { 
                raceStepYellow = _N_RACE_STEPS_YELLOW - 1;
            }
        }
    }

    for(int ient=0;  ient < thickness; ient++){

        if( direction == 0) {
            this_loc_violet_cyan = raceStepVioletCyan + ient;
            if(this_loc_violet_cyan >= _N_RACE_STEPS){
                this_loc_violet_cyan = this_loc_violet_cyan - _N_RACE_STEPS;
            }
            this_loc_beige = raceStepBeige + ient;
            if(this_loc_beige >= _N_RACE_STEPS_BEIGE){
                this_loc_beige = this_loc_beige - _N_RACE_STEPS_BEIGE;
            }
            this_loc_yellow = raceStepYellow + ient;
            if(this_loc_yellow >= _N_RACE_STEPS_YELLOW){
                this_loc_yellow = this_loc_yellow - _N_RACE_STEPS_YELLOW;
            }
        }
        if( direction == 1) {
            this_loc_violet_cyan = raceStepVioletCyan - ient;
            if(this_loc_violet_cyan < 0){
                this_loc_violet_cyan = this_loc_violet_cyan + _N_RACE_STEPS;
            }
            this_loc_beige = raceStepBeige - ient;
            if(this_loc_beige < 0){
                this_loc_beige = this_loc_beige + _N_RACE_STEPS_BEIGE;
            }
            this_loc_yellow = raceStepYellow - ient;
            if(this_loc_yellow < 0){
                this_loc_yellow = this_loc_yellow + _N_RACE_STEPS_YELLOW;
            }
        }

        uint16_t violet0 = pgm_read_word(&(steps_race_violet[this_loc_violet_cyan][0]));
        uint16_t violet1 = pgm_read_word(&(steps_race_violet[this_loc_violet_cyan][1]));
        uint16_t violet2 = pgm_read_word(&(steps_race_violet[this_loc_violet_cyan][2]));

        uint16_t beige0 = pgm_read_word(&(steps_race_beige[this_loc_beige][0]));
        uint16_t beige1 = pgm_read_word(&(steps_race_beige[this_loc_beige][1]));
        uint16_t beige2 = pgm_read_word(&(steps_race_beige[this_loc_beige][2]));

        uint16_t yellow0 = pgm_read_word(&(steps_race_yellow[this_loc_yellow][0]));
        uint16_t yellow1 = pgm_read_word(&(steps_race_yellow[this_loc_yellow][1]));
        uint16_t yellow2 = pgm_read_word(&(steps_race_yellow[this_loc_yellow][2]));

        uint16_t cyan0 = pgm_read_word(&(steps_race_cyan[this_loc_violet_cyan][0]));
        uint16_t cyan1 = pgm_read_word(&(steps_race_cyan[this_loc_violet_cyan][1]));
        uint16_t cyan2 = pgm_read_word(&(steps_race_cyan[this_loc_violet_cyan][2]));

        if( ient == (thickness - 1)) {
            led[violet1].r=violet[0]*brightness;
            led[violet1].g=violet[1]*brightness;
            led[violet1].b=violet[2]*brightness;

            led[beige1].r=beige[0]*brightness;
            led[beige1].g=beige[1]*brightness;
            led[beige1].b=beige[2]*brightness;

            led[yellow1].r=yellow[0]*brightness;
            led[yellow1].g=yellow[1]*brightness;
            led[yellow1].b=yellow[2]*brightness;

            led[cyan1].r=cyan[0]*brightness;
            led[cyan1].g=cyan[1]*brightness;
            led[cyan1].b=cyan[2]*brightness;
        }
        else if( ient == 0 ){
            led[violet0].r=violet[0]*brightness;
            led[violet0].g=violet[1]*brightness;
            led[violet0].b=violet[2]*brightness;

            led[violet2].r=violet[0]*brightness;
            led[violet2].g=violet[1]*brightness;
            led[violet2].b=violet[2]*brightness;

            led[beige0].r=beige[0]*brightness;
            led[beige0].g=beige[1]*brightness;
            led[beige0].b=beige[2]*brightness;

            led[beige2].r=beige[0]*brightness;
            led[beige2].g=beige[1]*brightness;
            led[beige2].b=beige[2]*brightness;

            led[yellow0].r=yellow[0]*brightness;
            led[yellow0].g=yellow[1]*brightness;
            led[yellow0].b=yellow[2]*brightness;

            led[yellow2].r=yellow[0]*brightness;
            led[yellow2].g=yellow[1]*brightness;
            led[yellow2].b=yellow[2]*brightness;

            led[cyan0].r=cyan[0]*brightness;
            led[cyan0].g=cyan[1]*brightness;
            led[cyan0].b=cyan[2]*brightness;

            led[cyan2].r=cyan[0]*brightness;
            led[cyan2].g=cyan[1]*brightness;
            led[cyan2].b=cyan[2]*brightness;
        }

        else{
            led[violet0].r=violet[0]*brightness;
            led[violet0].g=violet[1]*brightness;
            led[violet0].b=violet[2]*brightness;

            led[violet1].r=violet[0]*brightness;
            led[violet1].g=violet[1]*brightness;
            led[violet1].b=violet[2]*brightness;

            led[violet2].r=violet[0]*brightness;
            led[violet2].g=violet[1]*brightness;
            led[violet2].b=violet[2]*brightness;

            led[beige0].r=beige[0]*brightness;
            led[beige0].g=beige[1]*brightness;
            led[beige0].b=beige[2]*brightness;

            led[beige1].r=beige[0]*brightness;
            led[beige1].g=beige[1]*brightness;
            led[beige1].b=beige[2]*brightness;

            led[beige2].r=beige[0]*brightness;
            led[beige2].g=beige[1]*brightness;
            led[beige2].b=beige[2]*brightness;

            led[yellow0].r=yellow[0]*brightness;
            led[yellow0].g=yellow[1]*brightness;
            led[yellow0].b=yellow[2]*brightness;

            led[yellow1].r=yellow[0]*brightness;
            led[yellow1].g=yellow[1]*brightness;
            led[yellow1].b=yellow[2]*brightness;

            led[yellow2].r=yellow[0]*brightness;
            led[yellow2].g=yellow[1]*brightness;
            led[yellow2].b=yellow[2]*brightness;

            led[cyan0].r=cyan[0]*brightness;
            led[cyan0].g=cyan[1]*brightness;
            led[cyan0].b=cyan[2]*brightness;

            led[cyan1].r=cyan[0]*brightness;
            led[cyan1].g=cyan[1]*brightness;
            led[cyan1].b=cyan[2]*brightness;

            led[cyan2].r=cyan[0]*brightness;
            led[cyan2].g=cyan[1]*brightness;
            led[cyan2].b=cyan[2]*brightness;
        }
    }

    ws2812_setleds(led,_MAX_LED);
}
// sparkle pattern
// Randomly select LEDs
// to activate
void run_sparkle(){


    if((istep % DELAY) == 0){
        n_sparkle++;
        for( int il = 0 ; il < _MAX_LED; il++ ) {
            led[il].r=0;
            led[il].g=0;
            led[il].b=0;
        }

        uint8_t new_seed = fill_random( led, brightness, (uint8_t)istep );
        for(int i = 0; i < sparkle_count; ++i) {
           new_seed = fill_random( led, brightness, new_seed );
        }

    }
    if(n_sparkle >= _MAX_SPARKLE && disable_auto_update == 0 ){
        n_sparkle = 0;
        update_pattern();
    }
    ws2812_setleds(led,_MAX_LED);
}

// Select random-looking value by 
// applying an xor and bit shift
uint8_t random( uint8_t seed ) { 

    seed ^= seed << 4;
    seed ^= seed >> 7;
    seed ^= seed << 1;

    return seed;

}

// Set colors based on random values
uint8_t fill_random(struct cRGB *led, int brightness, uint8_t seed )
{

    uint8_t rand_violet = random( seed );
    uint8_t rand_beige = random( rand_violet );
    uint8_t rand_yellow = random( rand_beige );
    uint8_t rand_cyan = random( rand_yellow );

    if( rand_violet >= _N_LED_VIOLET ) {
        rand_violet -= _N_LED_VIOLET;
    }

    // only need 7 bits (128) for beige/yellow
    rand_beige = rand_beige >> 1;
    if( rand_beige > _N_LED_BEIGE ) { 
        rand_beige -= _N_LED_BEIGE;
    }

    rand_yellow = rand_yellow >> 1;
    if( rand_yellow > _N_LED_YELLOW) { 
        rand_yellow -= _N_LED_YELLOW;
    }

    if( rand_cyan >= _N_LED_CYAN ) { 
        rand_cyan -= _N_LED_CYAN;
    }

    uint16_t val_beige = rand_beige + _START_BEIGE;

    uint16_t val_yellow = rand_yellow + _START_YELLOW - 1;

    uint16_t val_cyan = rand_cyan + _START_CYAN;

    led[rand_violet].r = violet[0]*brightness;
    led[rand_violet].g = violet[1]*brightness;
    led[rand_violet].b = violet[2]*brightness;

    led[val_beige].r  = beige[0]*brightness;
    led[val_beige].g  = beige[1]*brightness;
    led[val_beige].b  = beige[2]*brightness;

    led[val_yellow].r  = yellow[0]*brightness;
    led[val_yellow].g  = yellow[1]*brightness;
    led[val_yellow].b  = yellow[2]*brightness;

    led[val_cyan].r  = cyan[0]*brightness;
    led[val_cyan].g  = cyan[1]*brightness;
    led[val_cyan].b  = cyan[2]*brightness;

    return rand_cyan;
}

// Initialize USART for bluetooth
void USART_Init( uint16_t ubrr)
{
    UCSR0A |= (1 << U2X0);
    /*Set baud rate */
    UBRR0H = (uint8_t)(ubrr>>8);
    UBRR0L = (uint8_t)ubrr;
    //Enable receiver and transmitter, recieve interrupt */
    UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
    /* Set frame format: 8data, 1stop bit */
    UCSR0C = (3<<UCSZ00);
}

// Receive a byte from bluetooth
uint8_t USART_Receive( void )
{
    // wait for data to be recieved
    while ( !(UCSR0A & (1<<RXC0)) );
    return UDR0;
}

// send a btye by bluetooth
void USART_Transmit( unsigned char data )
{
    //Wait for empty transmit buffer
    while ( !( UCSR0A & (1<<UDRE0)) );

    UDR0 = data;
}


// update color values
void change_color(uint8_t index, uint8_t col1, uint8_t col2, uint8_t col3){

    if(index == 1){
        violet[0] = col1;
        violet[1] = col2;
        violet[2] = col3;
    }
    if(index == 2){
        cyan[0] = col1;
        cyan[1] = col2;
        cyan[2] = col3;
    }
    if(index == 3){
        yellow[0] = col1;
        yellow[1] = col2;
        yellow[2] = col3;
    }
    if(index == 4){
        beige[0] = col1;
        beige[1] = col2;
        beige[2] = col3;
    }
}
