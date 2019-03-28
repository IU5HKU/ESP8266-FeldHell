//
// Simple Feld Hell beacon for Arduino, with the Etherkit Si5351a breakout
// board, by Jason Milldrum NT7S.
// 
// Adapted for ESP8266 by Marco Campinoti IU5HKU  - 28/03/2019
// This release uses the timer1 hardware interrupt for the required precise timing
// For the moment i've powered off the WiFi, maybe in a future release i'll use
// it for something like an interactive webpage to change frequency ad text 
// of the beacon.
//
// Original Feld Hell code for Arduino by Mark Vandewettering K6HX, adapted
// for the Si5351A by Robert Liesenfeld AK6L <ak6l@ak6l.org>.  Timer setup
// code by Thomas Knutsen LA3PNA.
// 
// 
// Copyright (c) 2015 Robert Liesenfeld AK6L <ak6l@ak6l.org>
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject
// to the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <si5351.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Ticker.h>

// Define some variables.
// Si5351A is connected via I2C on pin D1 (SCL) and D2 (SDA) as marked on Wemos D1 mini Lite
Si5351 si5351;
long freq = 14063000;
//long freq = 144400000;
#define CORRECTION              58000            // Change this for your ref osc -12000 

// This variable must be defined 'volatile' to avoid some compiler
// optimizations, which would render the interrupt timing useless.
volatile bool proceed = false;

// Define the structure of a glyph
typedef struct glyph {
    char ch ;
    word col[7] ;
} Glyph ;
 
// Build the Feld Hell "font".  This is declared PROGMEM to save
// memory on the Arduino; this data will be stored in program
// flash instead of loaded in memory.  This makes accesses slower
// and more complicated, but this is a small price to pay for the
// memory savings.
const Glyph glyphtab[] PROGMEM = {
  {' ', {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
  {'A', {0x07fc, 0x0e60, 0x0c60, 0x0e60, 0x07fc, 0x0000, 0x0000}},
  {'B', {0x0c0c, 0x0ffc, 0x0ccc, 0x0ccc, 0x0738, 0x0000, 0x0000}},
  {'C', {0x0ffc, 0x0c0c, 0x0c0c, 0x0c0c, 0x0c0c, 0x0000, 0x0000}},
  {'D', {0x0c0c, 0x0ffc, 0x0c0c, 0x0c0c, 0x07f8, 0x0000, 0x0000}},
  {'E', {0x0ffc, 0x0ccc, 0x0ccc, 0x0c0c, 0x0c0c, 0x0000, 0x0000}},
  {'F', {0x0ffc, 0x0cc0, 0x0cc0, 0x0c00, 0x0c00, 0x0000, 0x0000}},
  {'G', {0x0ffc, 0x0c0c, 0x0c0c, 0x0ccc, 0x0cfc, 0x0000, 0x0000}},
  {'H', {0x0ffc, 0x00c0, 0x00c0, 0x00c0, 0x0ffc, 0x0000, 0x0000}},
  {'I', {0x0ffc, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
  {'J', {0x003c, 0x000c, 0x000c, 0x000c, 0x0ffc, 0x0000, 0x0000}},
  {'K', {0x0ffc, 0x00c0, 0x00e0, 0x0330, 0x0e1c, 0x0000, 0x0000}},
  {'L', {0x0ffc, 0x000c, 0x000c, 0x000c, 0x000c, 0x0000, 0x0000}},
  {'M', {0x0ffc, 0x0600, 0x0300, 0x0600, 0x0ffc, 0x0000, 0x0000}},
  {'N', {0x0ffc, 0x0700, 0x01c0, 0x0070, 0x0ffc, 0x0000, 0x0000}},
  {'O', {0x0ffc, 0x0c0c, 0x0c0c, 0x0c0c, 0x0ffc, 0x0000, 0x0000}},
  {'P', {0x0c0c, 0x0ffc, 0x0ccc, 0x0cc0, 0x0780, 0x0000, 0x0000}},
  {'Q', {0x0ffc, 0x0c0c, 0x0c3c, 0x0ffc, 0x000f, 0x0000, 0x0000}},
  {'R', {0x0ffc, 0x0cc0, 0x0cc0, 0x0cf0, 0x079c, 0x0000, 0x0000}},
  {'S', {0x078c, 0x0ccc, 0x0ccc, 0x0ccc, 0x0c78, 0x0000, 0x0000}},
  {'T', {0x0c00, 0x0c00, 0x0ffc, 0x0c00, 0x0c00, 0x0000, 0x0000}},
  {'U', {0x0ff8, 0x000c, 0x000c, 0x000c, 0x0ff8, 0x0000, 0x0000}},
  {'V', {0x0ffc, 0x0038, 0x00e0, 0x0380, 0x0e00, 0x0000, 0x0000}},
  {'W', {0x0ff8, 0x000c, 0x00f8, 0x000c, 0x0ff8, 0x0000, 0x0000}},
  {'X', {0x0e1c, 0x0330, 0x01e0, 0x0330, 0x0e1c, 0x0000, 0x0000}},
  {'Y', {0x0e00, 0x0380, 0x00fc, 0x0380, 0x0e00, 0x0000, 0x0000}},
  {'Z', {0x0c1c, 0x0c7c, 0x0ccc, 0x0f8c, 0x0e0c, 0x0000, 0x0000}},
  {'0', {0x07f8, 0x0c0c, 0x0c0c, 0x0c0c, 0x07f8, 0x0000, 0x0000}},
  {'1', {0x0300, 0x0600, 0x0ffc, 0x0000, 0x0000, 0x0000, 0x0000}},
  {'2', {0x061c, 0x0c3c, 0x0ccc, 0x078c, 0x000c, 0x0000, 0x0000}},
  {'3', {0x0006, 0x1806, 0x198c, 0x1f98, 0x00f0, 0x0000, 0x0000}},
  {'4', {0x1fe0, 0x0060, 0x0060, 0x0ffc, 0x0060, 0x0000, 0x0000}},
  {'5', {0x000c, 0x000c, 0x1f8c, 0x1998, 0x18f0, 0x0000, 0x0000}},
  {'6', {0x07fc, 0x0c66, 0x18c6, 0x00c6, 0x007c, 0x0000, 0x0000}},
  {'7', {0x181c, 0x1870, 0x19c0, 0x1f00, 0x1c00, 0x0000, 0x0000}},
  {'8', {0x0f3c, 0x19e6, 0x18c6, 0x19e6, 0x0f3c, 0x0000, 0x0000}},
  {'9', {0x0f80, 0x18c6, 0x18cc, 0x1818, 0x0ff0, 0x0000, 0x0000}},
  {'*', {0x018c, 0x0198, 0x0ff0, 0x0198, 0x018c, 0x0000, 0x0000}},
  {'.', {0x001c, 0x001c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
  {'?', {0x1800, 0x1800, 0x19ce, 0x1f00, 0x0000, 0x0000, 0x0000}},
  {'!', {0x1f9c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
  {'(', {0x01e0, 0x0738, 0x1c0e, 0x0000, 0x0000, 0x0000, 0x0000}},
  {')', {0x1c0e, 0x0738, 0x01e0, 0x0000, 0x0000, 0x0000, 0x0000}},
  {'#', {0x0330, 0x0ffc, 0x0330, 0x0ffc, 0x0330, 0x0000, 0x0000}},
  {'$', {0x078c, 0x0ccc, 0x1ffe, 0x0ccc, 0x0c78, 0x0000, 0x0000}},
  {'/', {0x001c, 0x0070, 0x01c0, 0x0700, 0x1c00, 0x0000, 0x0000}},
} ;
 
// Define an upper bound on the number of glyphs.  Defining it this
// way allows adding characters without having to update a hard-coded
// upper bound.
#define NGLYPHS         (sizeof(glyphtab)/sizeof(glyphtab[0]))

// Timer interrupt vector.  This toggles the variable we use to gate
// each column of output to ensure accurate timing.  Called whenever
// Timer1 hits the count set below in setup().
void ICACHE_RAM_ATTR onTimerISR(){
    proceed = true;
}

// This is the heart of the beacon.  Given a character, it finds the
// appropriate glyph and toggles output from the Si5351A to key the
// Feld Hell signal.
void encodechar(int ch) {
    int i, x, y, fch;
    word fbits;
 
    for (i=0; i<NGLYPHS; i++) {
        // Check each element of the glyphtab to see if we've found the
        // character we're trying to send.
        fch = pgm_read_byte(&glyphtab[i].ch);
        if (fch == ch) {
            // Found the character, now fetch the pattern to be transmitted,
            // one column at a time.
            for (x=0; x<7; x++) {
                fbits = pgm_read_word(&(glyphtab[i].col[x]));
                // Transmit (or do not tx) one 'pixel' at a time; characters
                // are 7 cols by 14 rows.
                for (y=0; y<14; y++) {
                    if (fbits & (1<<y)) {
                      si5351.output_enable(SI5351_CLK0, 1);
                      digitalWrite(D4, LOW);
                    } else {
                      si5351.output_enable(SI5351_CLK0, 0);
                      digitalWrite(D4, HIGH);
                    }
                         
                    while(!proceed)   // Wait for the timer interrupt to fire
                      ;               // before continuing.
                    noInterrupts();   // Turn off interrupts; just good practice...
                    proceed = false;  // reset the flag for the next character...
                    interrupts();     // and re-enable interrupts.
                }
            }
            break; // We've found and transmitted the char,
                   // so exit the for loop
        }
    }
}
 
// Loop through the string, transmitting one character at a time.
void encode(char *str) {
    while (*str != '\0') 
        encodechar(*str++) ;
}
 
void setup() {
    // Use the ESP8266's on-board LED as a keying indicator.
    pinMode(D4, OUTPUT);
    digitalWrite(D4, LOW);

    // Shutdown the WiFi, don't needed in this release
    WiFi.mode( WIFI_OFF );
    WiFi.forceSleepBegin();
        
    // Initialize the Si5351
    // Change the 2nd parameter in init if using a ref osc other
    // than 25 MHz
    si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, CORRECTION);
  
    // Set CLK0 output to 14.063000 MHz with no correction factor;
    // see _____ for how to determine your board's correction factor.
    // Setting the correction factor appropriately will reduce slant
    // in the Feld Hell signal.
    //si5351.set_correction(0);
    //si5351.set_correction(0, SI5351_PLL_INPUT_XO);
    si5351.set_freq((uint64_t)freq * 100ULL, SI5351_CLK0);
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA); // Set for max power if desired

    // carrier output test
    si5351.output_enable(SI5351_CLK0, 1); // Enable the clock
    delay(5000);
    si5351.output_enable(SI5351_CLK0, 0); // Disable the clock initially
    
    // Set up Timer1 for interrupts at 245 Hz.
    noInterrupts();          // Turn off interrupts.

    //Init interrupt
    timer1_attachInterrupt(onTimerISR);
    timer1_write(20408);  //(maximum ticks 8388607)
    // 245Hz = 4081.6326530612 us
    
    interrupts();            // Re-enable interrupts.
    
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
    
    // timer dividers
    //  TIM_DIV1 = 0    //80MHz (80 ticks/us - 104857.588 us max)
    //  TIM_DIV16 = 1   //5MHz (5 ticks/us - 1677721.4 us max)
    //  TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
}
 
void loop() {

    // Beacon a call sign and a locator.
    encode("...IU5HKU JN53HB...");
  
    // Wait 5 seconds between beacons.
    delay(5000);
}
