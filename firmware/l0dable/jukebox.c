#include "basic/basic.h"
#include "basic/config.h"
#include "lcd/render.h"
#include "lcd/print.h"
#include "core/timer32/timer32.h"
#include "usetable.h"

/***********************************************************************
 * jukebox.c
 * 
 * This l0dable plays songs if you connect a speaker to SS0(6) and
 * GND(5) on m0dul bus A.
 * 
 *                 |-----|
 *                 | o o |
 *                 | o o |
 *                 | o o |
 * SS0 (Pin 6) --> | X X | <-- GND (Pin 5)
 *                 | o o |
 *                 | o o |
 *                 |-----|
 *               M0dul Bus A
 * 
 * Available songs are:
 *  - Nyan Cat (without intro)
 *  - Tetris (first part; see https://en.wikipedia.org/wiki/Korobeiniki)
 *  - Rick Astley - Never Gonna Give You Up (only refrain)
 * 
 * 
 * (c) 2013 by Ikarus
 * 
 ***********************************************************************
 * 
 * TODO:
 * - Recode everything to be much smaller. Implementing
 *      My Little Pony: Friendship is Magic" (Theme song) is not
 *      possible yet, because the l0dable will be to big.
 * - Add song "My Little Pony: Friendship is Magic" (Theme song)
 * 
 **********************************************************************/

// Output GPIO Pin. (Default is SS0/Pin 6 of m0dul bus A.)
#define FREQ_PIN RB_SPI_SS0

// All notes in 4 (C4, CIS4, ...).
// Exp. B4 = B, C5 = C*2 or Ais3 = AIS/2, A6 = C*2*2
// (Function beep makes of it: NOTE(C*2) or NOTE(AIS/2).)
#define C 262
#define CIS 277
#define D 294
#define DIS 311
#define E 330
#define F 349
#define FIS 370
#define G 392
#define GIS 415
#define A 440
#define AIS 466
#define B 494
#define NOTE(X) (TIMER32_CCLK_1S/(X))

/**********************************************************************/

int8_t exit;

void song_chooser(void);
void beep(uint32_t note, uint16_t duration);
void rest(uint16_t duration);
void handler(void);

// Songs.
void song_tetris(uint16_t speed);
void song_nyan_cat(uint16_t speed, uint8_t repeat);
void song_rickroll(uint16_t speed);
void song_rickroll_sub(uint8_t speed);

/**********************************************************************/

void ram(void) {
    timer32Callback0 = (uint32_t)handler;

    // Init GPIO.
    gpioSetDir(FREQ_PIN, gpioDirection_Output);
    gpioSetValue(FREQ_PIN, 1);

    // Main loop.
    song_chooser();

    // End
    timer32Disable(0);
}

// Main loop.
void song_chooser(void) {
    uint8_t key = 0;
    delayms(200);
    do {
        exit = 0;
        switch(key) {
            case BTN_UP:
                song_nyan_cat(65, 1);
                break;
            case BTN_DOWN:
                song_tetris(110);
                break;
            case BTN_ENTER:
                song_rickroll(90);
                break;
        }
        timer32Disable(0);
        
        lcdClear();
        lcdPrintln(" .:JukeBox:.");
        lcdPrintln("");
        lcdPrintln("UP Nyan Cat");
        lcdPrintln("DOWN Tetris");
        lcdPrintln("PRESS Rick A.");
        // lcdPrintln("RIGHT MLP:FiM");
        lcdPrintln("LEFT Exit");
        lcdPrintln(""); // Comment out if MLP:FiM is implemented.
        lcdPrintln("  by ikarus");
        lcdRefresh();
        
        if (key != 0)
            delayms(200);
    } while((key=getInputWait()) != BTN_LEFT);
}

// Note in kHz and duration in ms.
void beep(uint32_t note, uint16_t duration) {
    if (exit == 0) {
        // Check for cancel song.
        if (getInputRaw() == BTN_LEFT) {
            exit = 1;
            return;
        }
        // Display.
        lcdPrintln("Beep:");
        lcdPrint("  ");
        lcdPrint(IntToStr(note, 5, 0));
        lcdPrintln("Hz");
        lcdPrint("  ");
        lcdPrint(IntToStr(duration, 5, 0));
        lcdPrintln("ms");
        lcdRefresh();
        // Set timer.
        timer32Reset(0);
        timer32Init(0, NOTE(note));
        // Enable timer.
        timer32Enable(0);
        // Wait duration
        delayms(duration);
        // Disable pins and timer.
        gpioSetValue(FREQ_PIN, 0);
        timer32Disable(0);
    }
}

// Rest (diration in ms).
void rest(uint16_t duration) {
    if (exit == 0) {
        if (getInputRaw() == BTN_LEFT) {
            exit = 1;
            return;
        }
        lcdPrintln("Rest:");
        lcdPrint("  ");
        lcdPrint(IntToStr(duration, 5, 0));
        lcdPrintln("ms");
        lcdRefresh();
        delayms(duration);
    }
}

// Interrupt handler.
void handler(void) {
    static uint8_t x=0;
    x=(x==0) ? 1 : 0; // Toggle x.
    gpioSetValue(FREQ_PIN, x);
}

/***********************************************************************
 * Songs
 **********************************************************************/

// Tetris song (Korobeiniki). Increased by one octave.
void song_tetris(uint16_t speed) {
    // Frist part.
    beep(G*2*2, 2*speed);
    beep(D*2*2, 1*speed);
    beep(DIS*2*2, 1*speed);
    beep(F*2*2, 2*speed);
    beep(DIS*2*2, 1*speed);
    beep(D*2*2, 1*speed);
    beep(C*2*2, 2*speed);
    beep(C*2*2, 1*speed);
    beep(DIS*2*2, 1*speed);
    beep(G*2*2, 2*speed);
    beep(F*2*2, 1*speed);
    beep(DIS*2*2, 1*speed);
    beep(D*2*2, 2*speed);
    beep(D*2*2, 1*speed);
    beep(DIS*2*2, 1*speed);
    beep(F*2*2, 2*speed);
    beep(G*2*2, 2*speed);
    beep(DIS*2*2, 2*speed);
    beep(C*2*2, 2*speed);
    beep(C*2*2, 2*speed);
    rest(3*speed);
    // Second part.
    beep(F*2*2, 2*speed);
    beep(GIS*2*2, 1*speed);
    beep(C*2*2*2, 2*speed);
    beep(AIS*2*2, 1*speed);
    beep(GIS*2*2, 1*speed);
    beep(G*2*2, 3*speed);
    beep(DIS*2*2, 1*speed);
    beep(G*2*2, 2*speed);
    beep(F*2*2, 1*speed);
    beep(DIS*2*2, 1*speed);
    beep(D*2*2, 2*speed);
    beep(D*2*2, 1*speed);
    beep(DIS*2*2, 1*speed);
    beep(F*2*2, 2*speed);
    beep(G*2*2, 2*speed);
    beep(DIS*2*2, 2*speed);
    beep(C*2*2, 2*speed);
    beep(C*2*2, 2*speed);
    rest(2*speed);
}

// Nyan Cat song. Increased by one octave.
// Repeat 255 times max.
void song_nyan_cat(uint16_t speed, uint8_t repeat) {
    uint8_t a, b;
    // Hmm Intro?!?
    for (b=0; b<=repeat; b++) {
        // First part (repeat).
        for (a=0; a<2; a++) {
            // Bar 3 (due to missing intro).
            beep(FIS*2*2, 2*speed);
            beep(GIS*2*2, 2*speed);
            beep(DIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            rest(1*speed);
            beep(B*2, 1*speed);
            beep(D*2*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            rest(1*speed);
            beep(B*2, 2*speed);
            beep(CIS*2*2, 2*speed);
            // Bar 4
            beep(D*2*2, 2*speed);
            beep(DIS*2*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(FIS*2*2, 1*speed);
            beep(GIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(FIS*2*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            // Bar 5
            beep(DIS*2*2, 2*speed);
            beep(FIS*2*2, 2*speed);
            beep(GIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(FIS*2*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            beep(D*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(D*2*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            // Bar 6
            beep(D*2*2, 2*speed);
            beep(B*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(FIS*2*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            beep(CIS*2*2, 2*speed);
            beep(B*2, 2*speed);
            beep(CIS*2*2, 2*speed);
        }

        // Second part (repeat, different endings).
        for (a=0; a<2; a++) {
            // Common part
            // Bar 7
            beep(B*2, 2*speed);
            beep(FIS*2, 1*speed);
            beep(GIS*2, 1*speed);
            beep(B*2, 2*speed);
            beep(FIS*2, 1*speed);
            beep(GIS*2, 1*speed);
            beep(B*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            beep(E*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(E*2*2, 1*speed);
            beep(FIS*2*2, 1*speed);
            // Bar 8
            beep(B*2, 2*speed);
            beep(B*2, 2*speed);
            beep(FIS*2, 1*speed);
            beep(GIS*2, 1*speed);
            beep(B*2, 1*speed);
            beep(FIS*2, 1*speed);
            beep(E*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            beep(FIS*2, 1*speed);
            beep(DIS*2, 1*speed);
            beep(E*2, 1*speed);
            beep(FIS*2, 1*speed);
            // Bar 9
            beep(B*2, 2*speed);
            beep(FIS*2, 1*speed);
            beep(GIS*2, 1*speed);
            beep(B*2, 2*speed);
            beep(FIS*2, 1*speed);
            beep(GIS*2, 1*speed);
            beep(B*2, 1*speed);
            beep(B*2, 1*speed);
            beep(CIS*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(B*2, 1*speed);
            beep(FIS*2, 1*speed);
            beep(GIS*2, 1*speed);
            beep(FIS*2, 1*speed);
            // Bar 10 (different endings)
            beep(B*2, 2*speed);
            beep(B*2, 1*speed);
            beep(AIS*2, 1*speed);
            beep(B*2, 1*speed);
            beep(FIS*2, 1*speed);
            beep(GIS*2, 1*speed);
            beep(B*2, 1*speed);
            beep(E*2*2, 1*speed);
            beep(DIS*2*2, 1*speed);
            beep(E*2*2, 1*speed);
            beep(FIS*2*2, 1*speed);
            beep(B*2, 2*speed);
            if (a == 0) {
                // Ending 1
                beep(AIS*2, 2*speed);
            } else {
                // Ending 2
                beep(CIS*2*2, 2*speed);
            }
        }
    }
}

// Rick Astley - Never Gonna Give You Up. Increased by two octave.
void song_rickroll(uint16_t speed) {
    // Frist part.
    song_rickroll_sub(speed);
    beep(D*2*2*2, 3*speed);
    beep(D*2*2*2, 2*speed);
    beep(C*2*2*2, 6*speed);
    song_rickroll_sub(speed);
    beep(C*2*2*2, 3*speed);
    beep(C*2*2*2, 2*speed);
    beep(AIS*2*2, 3*speed);
    beep(A*2*2, 1*speed);
    beep(G*2*2, 4*speed);
    song_rickroll_sub(speed);
    beep(AIS*2*2, 4*speed);
    beep(C*2*2*2, 2*speed);
    beep(A*2*2, 3*speed);
    beep(G*2*2, 1*speed);
    beep(F*2*2, 4*speed);
    beep(F*2*2, 2*speed);
    beep(C*2*2*2, 4*speed);
    beep(AIS*2*2, 8*speed);
    // Second part.
    song_rickroll_sub(speed);
    beep(D*2*2*2, 3*speed);
    beep(D*2*2*2, 2*speed);
    beep(C*2*2*2, 6*speed);
    song_rickroll_sub(speed);
    beep(F*2*2*2, 4*speed);
    beep(A*2*2, 2*speed);
    beep(AIS*2*2, 3*speed);
    beep(A*2*2, 1*speed);
    beep(G*2*2, 4*speed);
    song_rickroll_sub(speed);
    beep(AIS*2*2, 4*speed);
    beep(C*2*2*2, 2*speed);
    beep(A*2*2, 3*speed);
    beep(G*2*2, 1*speed);
    beep(F*2*2, 4*speed);
    beep(F*2*2, 2*speed);
    beep(C*2*2*2, 4*speed);
    beep(AIS*2*2, 8*speed);
}

// Save space...
void song_rickroll_sub(uint8_t speed) {
    beep(F*2*2, 1*speed);
    beep(G*2*2, 1*speed);
    beep(AIS*2*2, 1*speed);
    beep(G*2*2, 1*speed);
}

/***********************************************************************
 * Timer stuff from core/timer32/timer32.c.
 * Unfortunately the timer32.o will not be linked to this l0dable
 * via the default make file. This is why the needed code ist here.
 * Removed some unused code. (Only timer 0 is needed.)
 **********************************************************************/

void timer32Disable(uint8_t timerNum) {
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_DISABLED;
}

void timer32Enable(uint8_t timerNum) {
    TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_ENABLED;
}

void timer32Reset(uint8_t timerNum) {
    uint32_t regVal;
    regVal = TMR_TMR32B0TCR;
    regVal |= TMR_TMR32B0TCR_COUNTERRESET_ENABLED;
    TMR_TMR32B0TCR = regVal;
}

void timer32Init(uint8_t timerNum, uint32_t timerInterval) {
    // If timerInterval is invalid, use the default value
    if (timerInterval < 1)
        timerInterval = TIMER32_DEFAULTINTERVAL;

    /* Enable the clock for CT32B0 */
    SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT32B0);

    // timer32_0_counter = 0;
    TMR_TMR32B0MR0 = timerInterval;

    /* Configure match control register to raise an interrupt and reset on MR0 */
    TMR_TMR32B0MCR = (TMR_TMR32B0MCR_MR0_INT_ENABLED | TMR_TMR32B0MCR_MR0_RESET_ENABLED);
    
    /* Enable the TIMER0 interrupt */
    NVIC_EnableIRQ(TIMER_32_0_IRQn);
}




