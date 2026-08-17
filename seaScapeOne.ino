#include <FastLED.h>

#define LED_PIN_LEFT    D2
#define LED_PIN_RIGHT   D4

#define NUM_LEDS        45
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB

#define BRIGHTNESS      120

CRGB leftLEDs[NUM_LEDS];
CRGB rightLEDs[NUM_LEDS];

CRGBPalette16 tidePalette = CRGBPalette16(
  CRGB(0, 0, 8),
  CRGB(0, 10, 35),
  CRGB(0, 30, 70),
  CRGB(0, 70, 105),

  CRGB(0, 120, 125),
  CRGB(10, 150, 125),
  CRGB(30, 130, 105),
  CRGB(0, 95, 115),

  CRGB(0, 65, 100),
  CRGB(0, 40, 80),
  CRGB(0, 20, 55),
  CRGB(5, 10, 35),

  CRGB(0, 5, 20),
  CRGB(0, 15, 40),
  CRGB(0, 50, 80),
  CRGB(0, 0, 8)
);


// ----------------------------------------------------
// NORMAL FLOW
// ----------------------------------------------------

float leftPosition  = 0;
float rightPosition = 0;

float leftSpeed  = 1.25;
float rightSpeed = 1.55;


// ----------------------------------------------------
// EDDY STRUCTURE
// ----------------------------------------------------

struct Eddy {
  bool active;

  float center;          // LED number where disturbance lives
  float radius;          // affected distance from center
  float strength;        // maximum displacement

  unsigned long startTime;
  unsigned long duration;
};


Eddy leftEddy;
Eddy rightEddy;


// Times for deciding whether to create new eddies
unsigned long nextLeftEddy  = 0;
unsigned long nextRightEddy = 0;


// ----------------------------------------------------
// START A NEW EDDY
// ----------------------------------------------------

void startEddy(Eddy &e) {

  e.active = true;

  // Don't put the center right at the ends
  e.center = random(6, NUM_LEDS - 6);

  // Region approximately 12-24 LEDs across
  e.radius = random(6, 13);

  // How strongly the color field gets bent backward
  e.strength = random(25, 65);

  // Eddy lasts roughly 2-5 seconds
  e.duration = random(2000, 5000);

  e.startTime = millis();
}


// ----------------------------------------------------
// CALCULATE DISTURBANCE AT ONE LED
// ----------------------------------------------------

float eddyEffect(Eddy &e, int ledNumber) {

  if (!e.active)
    return 0;

  unsigned long elapsed = millis() - e.startTime;

  if (elapsed >= e.duration) {
    e.active = false;
    return 0;
  }


  // --------------------------------------------------
  // TIME ENVELOPE
  //
  // Eddy smoothly grows, peaks, then disappears.
  // sin(0..PI) gives exactly that shape.
  // --------------------------------------------------

  float timePhase =
    (float)elapsed / (float)e.duration;

  float timeEnvelope =
    sin(timePhase * PI);


  // --------------------------------------------------
  // DISTANCE FROM EDDY CENTER
  // --------------------------------------------------

  float distance =
    abs((float)ledNumber - e.center);

  if (distance > e.radius)
    return 0;


  // Smoothly reduce effect toward edges
  float spatialEnvelope =
    1.0 - (distance / e.radius);

  // Curve it slightly so center is stronger
  spatialEnvelope *= spatialEnvelope;


  // --------------------------------------------------
  // ROTATIONAL EFFECT
  //
  // LEDs below the center are pushed one way;
  // LEDs above center are pushed the opposite way.
  //
  // This creates something more like an eddy than
  // simply shifting an entire block backward.
  // --------------------------------------------------

  float direction;

  if (ledNumber < e.center)
    direction = -1.0;
  else
    direction = 1.0;


  return direction *
         e.strength *
         spatialEnvelope *
         timeEnvelope;
}


// ----------------------------------------------------
// SETUP
// ----------------------------------------------------

void setup() {

  delay(1000);

  FastLED.addLeds<LED_TYPE, LED_PIN_LEFT, COLOR_ORDER>
         (leftLEDs, NUM_LEDS);

  FastLED.addLeds<LED_TYPE, LED_PIN_RIGHT, COLOR_ORDER>
         (rightLEDs, NUM_LEDS);

  FastLED.setBrightness(BRIGHTNESS);

  FastLED.clear();
  FastLED.show();

  randomSeed(micros());

  leftEddy.active  = false;
  rightEddy.active = false;

  nextLeftEddy  = millis() + random(3000, 7000);
  nextRightEddy = millis() + random(4000, 9000);
}


// ----------------------------------------------------
// LOOP
// ----------------------------------------------------

void loop() {

  unsigned long now = millis();


  // --------------------------------------------------
  // CREATE RANDOM EDDIES
  // --------------------------------------------------

  if (!leftEddy.active && now >= nextLeftEddy) {

    startEddy(leftEddy);

    nextLeftEddy =
      now + random(4000, 11000);
  }


  if (!rightEddy.active && now >= nextRightEddy) {

    startEddy(rightEddy);

    nextRightEddy =
      now + random(4000, 11000);
  }


  // --------------------------------------------------
  // BASIC BACKGROUND FLOW
  // --------------------------------------------------

  leftPosition  += leftSpeed;
  rightPosition += rightSpeed;


  // --------------------------------------------------
  // BUILD THE LIGHT FIELD
  // --------------------------------------------------

  for (int i = 0; i < NUM_LEDS; i++) {

    // Get local disturbance
    float leftDisturbance =
      eddyEffect(leftEddy, i);

    float rightDisturbance =
      eddyEffect(rightEddy, i);


    // ------------------------------------------------
    // LEFT STRIP
    // ------------------------------------------------

    uint8_t leftIndex =
      (uint8_t)(
        i * 11 +
        leftPosition +
        leftDisturbance
      );


    uint8_t leftBrightness =
      beatsin8(
        9,
        65,
        255,
        0,
        i * 8
      );


    leftLEDs[i] =
      ColorFromPalette(
        tidePalette,
        leftIndex,
        leftBrightness,
        LINEARBLEND
      );


    // ------------------------------------------------
    // RIGHT STRIP
    // ------------------------------------------------

    uint8_t rightIndex =
      (uint8_t)(
        i * 15 +
        rightPosition +
        rightDisturbance
      );


    uint8_t rightBrightness =
      beatsin8(
        11,
        55,
        255,
        0,
        i * 13
      );


    rightLEDs[i] =
      ColorFromPalette(
        tidePalette,
        rightIndex,
        rightBrightness,
        LINEARBLEND
      );
  }


  // --------------------------------------------------
  // VERY GENTLE GLOBAL SWELL
  // --------------------------------------------------

  uint8_t overall =
    beatsin8(4, 180, 255);

  FastLED.setBrightness(
    scale8(BRIGHTNESS, overall)
  );


  FastLED.show();

  delay(20);
}