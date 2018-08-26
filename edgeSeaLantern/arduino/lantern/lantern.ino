#include "FastLED.h"

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few
// of the kinds of animation patterns you can quickly and easily
// compose using FastLED.
//
// This example also shows one easy way to define multiple
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif


const unsigned char COUNT_PETALS = 8 ; 
const unsigned char COUNT_LEDS_PER_PETALS = 8 ; 


#define DATA_PIN D6
//#define CLK_PIN   4
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS (COUNT_PETALS * COUNT_LEDS_PER_PETALS + (COUNT_LEDS_PER_PETALS))
CRGB leds[NUM_LEDS];

#define BRIGHTNESS 96
#define FRAMES_PER_SECOND 120



void setup()
{
    delay(10 *1000); // 3 second delay for recovery

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { lantern, lantern, lantern, /*,fadeInAndOut,*/ rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm  };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void loop()
{
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();

    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    // insert a delay to keep the framerate modest
    FastLED.delay(1000 / FRAMES_PER_SECOND);

    // do some periodic updates
    EVERY_N_MILLISECONDS(20) { gHue++; } // slowly cycle the "base color" through the rainbow
    EVERY_N_SECONDS(30) { nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void SetPetalColor( unsigned char petalOffset, CHSV color )
{
    if(petalOffset > COUNT_PETALS ) {
        // out of range. Do nothing 
        return ;
    }

    for( unsigned char ledOffset = 0 ; ledOffset < COUNT_LEDS_PER_PETALS ; ledOffset++ ) {
        leds[(petalOffset * COUNT_LEDS_PER_PETALS) + ledOffset] = color ; 
    }
}

void lantern()
{
    fadeToBlackBy(leds, NUM_LEDS, 1);
    const unsigned long TIMER_NEXT_PETAL_TIMEOUT = 100 ; 

    static unsigned char petal = 0;
    static unsigned long timeNextPetal = millis() + TIMER_NEXT_PETAL_TIMEOUT;
    if( millis() > timeNextPetal ) {
        timeNextPetal = millis() + TIMER_NEXT_PETAL_TIMEOUT;
        petal++;
        if( petal > COUNT_PETALS) {
            petal = 0 ; 
        }
        SetPetalColor( petal, CHSV(gHue, 200, 255) );
    }
}
#include <math.h>
void fadeInAndOut()
{
    static unsigned char petal = 0;

    const unsigned int TIME_RAMP_UP = 3000;
    const unsigned int TIME_GLOW_DURATION = 10*1000;

    const unsigned char STAGE_RAMP_UP = 0;
    const unsigned char STAGE_GLOW = 1;

    static unsigned char currentStage = STAGE_RAMP_UP;
    static unsigned char currentHue = 0;
    static unsigned long nextStageChangeTime = millis() + TIME_RAMP_UP;
    
    unsigned long currentTime = millis() ; 

    // We need to stage change 
    if( nextStageChangeTime < currentTime) {
        currentStage++; 
        if( currentStage > STAGE_GLOW ) {
            currentStage = STAGE_RAMP_UP ; 
            petal++; 
            currentHue += 32 ; // Change the color of the next petal 
            if( petal > COUNT_PETALS) {
                petal = 0 ; 
            }
        }
        switch (currentStage) {
            case STAGE_RAMP_UP: {
                nextStageChangeTime = currentTime + TIME_RAMP_UP ; 
                break;
            }
            default:
            case STAGE_GLOW: {
                nextStageChangeTime = currentTime + TIME_GLOW_DURATION ; 
                break;
            }
        } // switch         
    }
    
    // Fade everything down 
    fadeToBlackBy(leds, NUM_LEDS, 1);

    // Depending on the stage we will do different things. 
    if( currentStage == STAGE_RAMP_UP) {
        // We have TIME_RAMP_UP to get to full brightness... 
        // use the difference between current time and step time to tell what precentage of brightness to set. 
        unsigned long timeLeftInThisStage = (nextStageChangeTime - currentTime) ; 
        float precentageOfBrightness = (TIME_RAMP_UP - timeLeftInThisStage ) / TIME_RAMP_UP ; 

        unsigned char currentBrightness = (unsigned char) round(precentageOfBrightness * 255 ) ; 
        SetPetalColor( petal, CHSV(currentHue, 200, currentBrightness) );
    } else if( currentStage == STAGE_GLOW) {
        SetPetalColor( petal, CHSV(currentHue, 200, 255) );
    }
}

void rainbow()
{
    // FastLED's built-in rainbow generator
    fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter()
{
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter(80);
}

void addGlitter(fract8 chanceOfGlitter)
{
    if (random8() < chanceOfGlitter) {
        leds[random16(NUM_LEDS)] += CRGB::White;
    }
}

void confetti()
{
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy(leds, NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon()
{
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy(leds, NUM_LEDS, 20);
    int pos = beatsin16(13, 0, NUM_LEDS - 1);
    leds[pos] += CHSV(gHue, 255, 192);
}

void bpm()
{
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < NUM_LEDS; i++) { //9948
        leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
}

void juggle()
{
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy(leds, NUM_LEDS, 20);
    byte dothue = 0;
    for (int i = 0; i < 8; i++) {
        leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
        dothue += 32;
    }
}

