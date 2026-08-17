#include <FastLED.h>

#define LED_PIN_LEFT    D2
#define LED_PIN_RIGHT   D4

#define NUM_LEDS        45
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB

#define BRIGHTNESS      120

CRGB leftLEDs[NUM_LEDS];
CRGB rightLEDs[NUM_LEDS];


// ====================================================
// OCEAN BACKGROUND PALETTE
// ====================================================

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


// ====================================================
// BACKGROUND FLOW
// ====================================================

float leftPosition  = 0;
float rightPosition = 0;

float leftSpeed  = 1.35;
float rightSpeed = 1.65;


// ====================================================
// MOVING EDDIES
// ====================================================

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


// ====================================================
// SIMULATED TIDE
//
// 60 seconds LOW -> HIGH
// 60 seconds HIGH -> LOW
// ====================================================

#define HALF_TIDE_MS 60000UL
#define FULL_TIDE_MS 120000UL


// ====================================================
// START EDDY
// ====================================================

void startEddy(Eddy &e) {

  e.active = true;

  e.startCenter =
      random(5, NUM_LEDS - 5);

  e.center =
      e.startCenter;

  e.radius =
      random(6, 13);

  e.strength =
      random(30, 75);

  e.duration =
      random(2500, 5500);

  e.travelDirection =
      (random(100) < 65) ? 1 : -1;

  e.travelSpeed =
      random(120, 350) / 100.0;

  e.startTime =
      millis();
}


// ====================================================
// UPDATE EDDY
// ====================================================

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


// ====================================================
// EDDY DISTORTION
// ====================================================

float eddyEffect(
  Eddy &e,
  int ledNumber
) {

  if (!e.active)
    return 0;

  unsigned long elapsed =
      millis() - e.startTime;

  float life =
      (float)elapsed /
      (float)e.duration;

  if (life >= 1.0)
    return 0;


  // Grow and fade

  float envelope =
      sin(life * PI);


  // Distance from moving center

  float distance =
      abs(
        (float)ledNumber -
        e.center
      );

  if (distance > e.radius)
    return 0;


  float spatial =
      1.0 -
      distance / e.radius;

  spatial *= spatial;


  // Opposite displacement above/below center

  float side =
      (ledNumber < e.center)
      ? -1.0
      : 1.0;


  // Internal curl

  float curl =
      sin(
        life * PI * 4.0 +
        ledNumber * 0.32
      );


  float distortion =
      side * e.strength +
      curl *
      e.strength *
      0.40;


  return
      distortion *
      spatial *
      envelope;
}


// ====================================================
// SIMULATED TIDE LEVEL
//
// LED 0  = LOW
// LED 44 = HIGH
// ====================================================

float getTideLevel(
  bool &rising
) {

  unsigned long phase =
      millis() %
      FULL_TIDE_MS;

  float fraction;


  if (phase < HALF_TIDE_MS) {

    // LOW -> HIGH

    rising = true;

    fraction =
        (float)phase /
        (float)HALF_TIDE_MS;

  } else {

    // HIGH -> LOW

    rising = false;

    fraction =
        (float)(
          phase -
          HALF_TIDE_MS
        ) /
        (float)HALF_TIDE_MS;
  }


  // Sinusoidal movement
  // Slow near high and low,
  // fastest through mid-tide.

  float smooth =
      (
        1.0 -
        cos(
          fraction * PI
        )
      ) *
      0.5;


  if (rising) {

    return
        smooth *
        (NUM_LEDS - 1);

  } else {

    return
        (1.0 - smooth) *
        (NUM_LEDS - 1);
  }
}


// ====================================================
// RED CURRENT TIDE LEVEL
// ====================================================

void addTideMarker(
  CRGB leds[],
  float tideLevel
) {

  for (int i = 0;
       i < NUM_LEDS;
       i++) {

    float distance =
        (float)i -
        tideLevel;

    float absDistance =
        abs(distance);


    // Bright red center

    if (absDistance < 0.75) {

      leds[i] =
          CRGB(
            255,
            25,
            0
          );
    }


    // Narrow red edge

    else if (
      absDistance < 1.55
    ) {

      leds[i] =
          CRGB(
            190,
            5,
            0
          );
    }
  }
}


// ====================================================
// GREEN DIRECTION INDICATOR
//
// HIGH tide coming:
//     green at TOP
//
// LOW tide coming:
//     green at BOTTOM
// ====================================================

void addDirectionIndicator(
  CRGB leds[],
  bool rising
) {

  if (rising) {

    // HIGH tide coming

    leds[NUM_LEDS - 1] =
        CRGB(
          0,
          255,
          0
        );

  } else {

    // LOW tide coming

    leds[0] =
        CRGB(
          0,
          255,
          0
        );
  }
}


// ====================================================
// SETUP
// ====================================================

void setup() {

  delay(1000);


  FastLED.addLeds<
      LED_TYPE,
      LED_PIN_LEFT,
      COLOR_ORDER
    >(
      leftLEDs,
      NUM_LEDS
    );


  FastLED.addLeds<
      LED_TYPE,
      LED_PIN_RIGHT,
      COLOR_ORDER
    >(
      rightLEDs,
      NUM_LEDS
    );


  FastLED.setBrightness(
      BRIGHTNESS
  );


  FastLED.clear();
  FastLED.show();


  randomSeed(
      micros()
  );


  leftEddy.active =
      false;

  rightEddy.active =
      false;


  nextLeftEddy =
      millis() +
      random(
        1500,
        5000
      );


  nextRightEddy =
      millis() +
      random(
        2500,
        6500
      );
}


// ====================================================
// MAIN LOOP
// ====================================================

void loop() {

  unsigned long now =
      millis();


  // ==================================================
  // CREATE RANDOM MOVING EDDIES
  // ==================================================

  if (
    !leftEddy.active &&
    now >= nextLeftEddy
  ) {

    startEddy(
      leftEddy
    );


    nextLeftEddy =
        now +
        random(
          3000,
          8500
        );
  }


  if (
    !rightEddy.active &&
    now >= nextRightEddy
  ) {

    startEddy(
      rightEddy
    );


    nextRightEddy =
        now +
        random(
          3000,
          8500
        );
  }


  updateEddy(
    leftEddy
  );

  updateEddy(
    rightEddy
  );


  // ==================================================
  // MOVE BACKGROUND CURRENT
  // ==================================================

  leftPosition +=
      leftSpeed;

  rightPosition +=
      rightSpeed;


  // ==================================================
  // DRAW MOVING OCEAN BACKGROUND
  // ==================================================

  for (int i = 0;
       i < NUM_LEDS;
       i++) {

    float leftDisturbance =
        eddyEffect(
          leftEddy,
          i
        );

    float rightDisturbance =
        eddyEffect(
          rightEddy,
          i
        );


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


  // ==================================================
  // CALCULATE CURRENT SIMULATED TIDE
  // ==================================================

  bool tideRising;

  float tideLevel =
      getTideLevel(
        tideRising
      );


  // ==================================================
  // RED CURRENT TIDE LEVEL
  // ==================================================

  addTideMarker(
    leftLEDs,
    tideLevel
  );

  addTideMarker(
    rightLEDs,
    tideLevel
  );


  // ==================================================
  // GREEN DIRECTION MARKER
  //
  // TOP    = HIGH tide coming
  // BOTTOM = LOW tide coming
  // ==================================================

  addDirectionIndicator(
    leftLEDs,
    tideRising
  );

  addDirectionIndicator(
    rightLEDs,
    tideRising
  );


  // ==================================================
  // GENTLE WHOLE-DISPLAY SWELL
  // ==================================================

  uint8_t overall =
      beatsin8(
        4,
        190,
        255
      );


  FastLED.setBrightness(
      scale8(
        BRIGHTNESS,
        overall
      )
  );


  // ==================================================
  // DISPLAY FRAME
  // ==================================================

  FastLED.show();

  delay(20);
}