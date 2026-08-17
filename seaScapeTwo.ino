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
// MAIN FLOW
// ----------------------------------------------------

float leftPosition  = 0;
float rightPosition = 0;

float leftSpeed  = 1.35;
float rightSpeed = 1.65;


// ----------------------------------------------------
// MOVING EDDY
// ----------------------------------------------------

struct Eddy {

  bool active;

  float startCenter;
  float center;

  float radius;
  float strength;

  float travelSpeed;

  int travelDirection;

  unsigned long startTime;
  unsigned long duration;
};


Eddy leftEddy;
Eddy rightEddy;

unsigned long nextLeftEddy  = 0;
unsigned long nextRightEddy = 0;


// ----------------------------------------------------
// START AN EDDY
// ----------------------------------------------------

void startEddy(Eddy &e) {

  e.active = true;

  // Begin somewhere away from extreme ends
  e.startCenter = random(5, NUM_LEDS - 5);
  e.center = e.startCenter;

  e.radius = random(6, 13);

  e.strength = random(30, 75);

  e.duration = random(2500, 5500);

  // Most disturbances travel upward,
  // but some travel downward.
  e.travelDirection =
      (random(100) < 65) ? 1 : -1;

  // LED positions per second
  e.travelSpeed =
      random(120, 350) / 100.0;

  e.startTime = millis();
}


// ----------------------------------------------------
// UPDATE EDDY POSITION
// ----------------------------------------------------

void updateEddy(Eddy &e) {

  if (!e.active)
    return;

  unsigned long elapsed =
      millis() - e.startTime;

  if (elapsed >= e.duration) {
    e.active = false;
    return;
  }

  float seconds =
      elapsed / 1000.0;

  e.center =
      e.startCenter +
      e.travelDirection *
      e.travelSpeed *
      seconds;
}


// ----------------------------------------------------
// EDDY DISTORTION
// ----------------------------------------------------

float eddyEffect(Eddy &e, int ledNumber) {

  if (!e.active)
    return 0;

  unsigned long elapsed =
      millis() - e.startTime;

  float life =
      (float)elapsed /
      (float)e.duration;

  if (life >= 1.0)
    return 0;


  // --------------------------------------------------
  // GROW AND FADE
  // --------------------------------------------------

  float envelope =
      sin(life * PI);


  // --------------------------------------------------
  // DISTANCE FROM MOVING CENTER
  // --------------------------------------------------

  float distance =
      abs((float)ledNumber - e.center);

  if (distance > e.radius)
    return 0;

  float spatial =
      1.0 - distance / e.radius;

  spatial *= spatial;


  // --------------------------------------------------
  // OPPOSITE DISPLACEMENT ABOVE / BELOW CENTER
  // --------------------------------------------------

  float side;

  if (ledNumber < e.center)
    side = -1.0;
  else
    side = 1.0;


  // --------------------------------------------------
  // INTERNAL ROTATION / CURL
  //
  // This oscillates during the life of the eddy.
  // --------------------------------------------------

  float curl =
      sin(
        life * PI * 4.0 +
        ledNumber * 0.32
      );


  // Basic distortion + smaller curling component
  float distortion =
      side * e.strength +
      curl * e.strength * 0.40;


  return distortion *
         spatial *
         envelope;
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

  nextLeftEddy =
      millis() + random(1500, 5000);

  nextRightEddy =
      millis() + random(2500, 6500);
}


// ----------------------------------------------------
// LOOP
// ----------------------------------------------------

void loop() {

  unsigned long now = millis();


  // --------------------------------------------------
  // CREATE NEW MOVING DISTURBANCES
  // --------------------------------------------------

  if (!leftEddy.active &&
      now >= nextLeftEddy) {

    startEddy(leftEddy);

    nextLeftEddy =
        now + random(3000, 8500);
  }


  if (!rightEddy.active &&
      now >= nextRightEddy) {

    startEddy(rightEddy);

    nextRightEddy =
        now + random(3000, 8500);
  }


  // Move existing eddies
  updateEddy(leftEddy);
  updateEddy(rightEddy);


  // --------------------------------------------------
  // NORMAL BACKGROUND CURRENT
  // --------------------------------------------------

  leftPosition  += leftSpeed;
  rightPosition += rightSpeed;


  // --------------------------------------------------
  // DRAW BOTH LIGHT FIELDS
  // --------------------------------------------------

  for (int i = 0; i < NUM_LEDS; i++) {

    float leftDisturbance =
        eddyEffect(leftEddy, i);

    float rightDisturbance =
        eddyEffect(rightEddy, i);


    // ------------------------------------------------
    // LEFT SIDE
    // ------------------------------------------------

    uint8_t leftIndex =
        (uint8_t)(
          i * 11 +
          leftPosition +
          leftDisturbance
        );


    uint8_t leftBrightness =
        beatsin8(
          10,
          70,
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
    // RIGHT SIDE
    // ------------------------------------------------

    uint8_t rightIndex =
        (uint8_t)(
          i * 15 +
          rightPosition +
          rightDisturbance
        );


    uint8_t rightBrightness =
        beatsin8(
          12,
          60,
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
  // SLOW WHOLE-SCULPTURE SWELL
  // --------------------------------------------------

  uint8_t overall =
      beatsin8(4, 180, 255);

  FastLED.setBrightness(
      scale8(BRIGHTNESS, overall)
  );


  FastLED.show();

  delay(20);
}