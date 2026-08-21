#include <FastLED.h>

#define LED_PIN_LEFT    D2
#define LED_PIN_RIGHT   D4

#define NUM_LEDS        45
#define NUM_DROPS       8

#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB

#define BRIGHTNESS      255

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
//
// Later replace this with real tide data.
// ====================================================

#define HALF_TIDE_MS 60000UL
#define FULL_TIDE_MS 120000UL


// ====================================================
// RED DROP STRUCTURE
// ====================================================

struct Drop {

  bool active;

  float position;
  float velocity;
};

Drop leftDrops[NUM_DROPS];
Drop rightDrops[NUM_DROPS];


// ====================================================
// RED WATER VARIABLES
// ====================================================

float redFillLevel = 0;

float targetTideLevel = 20;


// ====================================================
// DISPLAY STATES
// ====================================================

enum DisplayState {

  BLUE_PAUSE,
  RED_FILL,
  RED_WIGGLE

};

DisplayState displayState = BLUE_PAUSE;


// ====================================================
// TIMING
// ====================================================

unsigned long stateStart = 0;

unsigned long lastDropLaunch = 0;


// Blue background alone before next fill

#define BLUE_PAUSE_TIME  2500UL


// Red reaches tide level in exactly 15 seconds

#define RED_FILL_TIME    15000UL


// Red remains animated for 30 seconds

#define RED_HOLD_TIME    30000UL


// Drop release interval

#define DROP_INTERVAL    300UL


// ====================================================
// START EDDY
// ====================================================

void startEddy(Eddy &e) {

  e.active = true;

  e.startCenter =
      random(
        5,
        NUM_LEDS - 5
      );

  e.center =
      e.startCenter;

  e.radius =
      random(
        6,
        13
      );

  e.strength =
      random(
        30,
        75
      );

  e.duration =
      random(
        2500,
        5500
      );

  e.travelDirection =
      (random(100) < 65)
      ? 1
      : -1;

  e.travelSpeed =
      random(
        120,
        350
      ) /
      100.0;

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
      millis() -
      e.startTime;


  if (elapsed >= e.duration) {

    e.active = false;

    return;
  }


  float seconds =
      elapsed /
      1000.0;


  e.center =
      e.startCenter +
      e.travelDirection *
      e.travelSpeed *
      seconds;
}


// ====================================================
// EDDY EFFECT
// ====================================================

float eddyEffect(
  Eddy &e,
  int ledNumber
) {

  if (!e.active)
    return 0;


  unsigned long elapsed =
      millis() -
      e.startTime;


  float life =
      (float)elapsed /
      (float)e.duration;


  if (life >= 1.0)
    return 0;


  float envelope =
      sin(
        life *
        PI
      );


  float distance =
      abs(
        (float)ledNumber -
        e.center
      );


  if (distance > e.radius)
    return 0;


  float spatial =
      1.0 -
      distance /
      e.radius;


  spatial *=
      spatial;


  float side =
      (ledNumber < e.center)
      ? -1.0
      : 1.0;


  float curl =
      sin(
        life *
        PI *
        4.0 +
        ledNumber *
        0.32
      );


  float distortion =
      side *
      e.strength +
      curl *
      e.strength *
      0.40;


  return
      distortion *
      spatial *
      envelope;
}


// ====================================================
// MANAGE EDDIES
// ====================================================

void manageEddies() {

  unsigned long now =
      millis();


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
}


// ====================================================
// DRAW MOVING BLUE/GREEN BACKGROUND
// ====================================================

void drawOceanBackground() {

  leftPosition +=
      leftSpeed;

  rightPosition +=
      rightSpeed;


  for (int i = 0; i < NUM_LEDS; i++) {

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
    // LEFT
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
          190,
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
    // RIGHT
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
          190,
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
}


// ====================================================
// SIMULATED TIDE LEVEL
// ====================================================

float getTideLevel(
  bool &rising
) {

  unsigned long phase =
      millis() %
      FULL_TIDE_MS;


  float fraction;


  if (phase < HALF_TIDE_MS) {

    rising = true;


    fraction =
        (float)phase /
        (float)HALF_TIDE_MS;

  } else {

    rising = false;


    fraction =
        (float)(
          phase -
          HALF_TIDE_MS
        ) /
        (float)HALF_TIDE_MS;
  }


  // Smooth sinusoidal tide

  float smooth =
      (
        1.0 -
        cos(
          fraction *
          PI
        )
      ) *
      0.5;


  if (rising) {

    return
        smooth *
        (NUM_LEDS - 1);

  } else {

    return
        (
          1.0 -
          smooth
        ) *
        (NUM_LEDS - 1);
  }
}


// ====================================================
// GREEN DIRECTION INDICATOR
//
// HIGH tide coming:
//     Green LED at TOP
//
// LOW tide coming:
//     Green LED at BOTTOM
// ====================================================

void addDirectionIndicator(
  bool rising
) {

  if (rising) {

    leftLEDs[NUM_LEDS - 1] =
        CRGB(
          0,
          255,
          0
        );


    rightLEDs[NUM_LEDS - 1] =
        CRGB(
          0,
          255,
          0
        );

  } else {

    leftLEDs[0] =
        CRGB(
          0,
          255,
          0
        );


    rightLEDs[0] =
        CRGB(
          0,
          255,
          0
        );
  }
}


// ====================================================
// CLEAR DROPS
// ====================================================

void clearDrops() {

  for (int i = 0; i < NUM_DROPS; i++) {

    leftDrops[i].active =
        false;

    rightDrops[i].active =
        false;
  }
}


// ====================================================
// LAUNCH RED DROP
// ====================================================

void launchDrop(
  Drop &d
) {

  d.active =
      true;


  d.position =
      NUM_LEDS - 1;


  // Nearly motionless at release

  d.velocity =
      0.025;
}


// ====================================================
// UPDATE RED DROPS
//
// Drop accelerates initially and then smoothly
// decelerates as it approaches the rising red water.
// ====================================================

void updateDrops(
  Drop drops[]
) {

  for (int i = 0; i < NUM_DROPS; i++) {

    if (!drops[i].active)
      continue;


    // ------------------------------------------------
    // DISTANCE FROM CURRENT RED SURFACE
    // ------------------------------------------------

    float distance =
        drops[i].position -
        redFillLevel;


    float totalDistance =
        (NUM_LEDS - 1) -
        redFillLevel;


    if (totalDistance < 1.0)
      totalDistance = 1.0;


    float fraction =
        distance /
        totalDistance;


    fraction =
        constrain(
          fraction,
          0.0,
          1.0
        );


    // ------------------------------------------------
    // VARIABLE ACCELERATION
    // ------------------------------------------------

    float acceleration;


    if (fraction > 0.50) {

      acceleration =
          0.014 *
          (
            fraction -
            0.50
          );

    } else {

      float lower =
          (
            0.50 -
            fraction
          ) /
          0.50;


      acceleration =
          -0.028 *
          lower *
          lower;
    }


    drops[i].velocity +=
        acceleration;


    // ------------------------------------------------
    // SPEED LIMITS
    // ------------------------------------------------

    drops[i].velocity =
        constrain(
          drops[i].velocity,
          0.020,
          0.38
        );


    // ------------------------------------------------
    // MOVE DROP
    // ------------------------------------------------

    drops[i].position -=
        drops[i].velocity;


    // ------------------------------------------------
    // DROP DISAPPEARS WHEN IT HITS RED WATER
    // ------------------------------------------------

    if (
      drops[i].position <=
      redFillLevel
    ) {

      drops[i].active =
          false;
    }
  }
}


// ====================================================
// DRAW RED DROPS
//
// Single pure-red LEDs.
// ====================================================

void drawDrops(
  CRGB leds[],
  Drop drops[]
) {

  for (int i = 0; i < NUM_DROPS; i++) {

    if (!drops[i].active)
      continue;


    int p =
        round(
          drops[i].position
        );


    p =
        constrain(
          p,
          0,
          NUM_LEDS - 1
        );


    leds[p] =
        CRGB(
          255,
          0,
          0
        );
  }
}


// ====================================================
// DRAW SOLID RED FILL DURING RISING PHASE
// ====================================================

void drawSolidRedFill() {

  int top =
      constrain(
        round(
          redFillLevel
        ),
        0,
        NUM_LEDS - 1
      );


  for (
    int i = 0;
    i <= top;
    i++
  ) {

    leftLEDs[i] =
        CRGB(
          235,
          0,
          0
        );


    rightLEDs[i] =
        CRGB(
          235,
          0,
          0
        );
  }
}


// ====================================================
// DRAW FULLY ANIMATED RED WATER
//
// Broad traveling waves move through the entire
// red region.
//
// Left and right sides run differently to help
// produce the apparent 3-D effect.
// ====================================================

void drawWigglingRedWater() {

  unsigned long now =
      millis();


  // ==================================================
  // MOVING SURFACE
  // ==================================================

  float leftSurfaceWave =
      sin(
        now *
        0.0027
      ) *
      2.0;


  float rightSurfaceWave =
      sin(
        now *
        0.0022 +
        1.8
      ) *
      2.0;


  // Secondary slower wave

  leftSurfaceWave +=
      sin(
        now *
        0.0011 +
        1.0
      ) *
      0.8;


  rightSurfaceWave +=
      sin(
        now *
        0.0013 +
        3.0
      ) *
      0.8;


  // --------------------------------------------------
  // EXISTING EDDIES ALSO MOVE RED SURFACE
  // --------------------------------------------------

  float leftEddyMove =
      eddyEffect(
        leftEddy,
        round(
          targetTideLevel
        )
      ) *
      0.035;


  float rightEddyMove =
      eddyEffect(
        rightEddy,
        round(
          targetTideLevel
        )
      ) *
      0.035;


  float leftSurface =
      targetTideLevel +
      leftSurfaceWave +
      leftEddyMove;


  float rightSurface =
      targetTideLevel +
      rightSurfaceWave +
      rightEddyMove;


  leftSurface =
      constrain(
        leftSurface,
        0,
        NUM_LEDS - 1
      );


  rightSurface =
      constrain(
        rightSurface,
        0,
        NUM_LEDS - 1
      );


  // ==================================================
  // LEFT RED WATER
  // ==================================================

  for (
    int i = 0;
    i <= leftSurface &&
    i < NUM_LEDS;
    i++
  ) {

    // Broad upward-moving wave

    uint8_t wave1 =
        sin8(
          i * 13 -
          now / 9
        );


    // Opposite-direction wave

    uint8_t wave2 =
        sin8(
          i * 7 +
          now / 17
        );


    // Slow irregular disturbance

    uint8_t wave3 =
        sin8(
          i * 5 -
          now / 29
        );


    uint16_t total =
        wave1 +
        wave2 +
        wave3;


    uint8_t combined =
        total / 3;


    // Strong red contrast:
    // dark part = 70
    // bright part = 255

    uint8_t redBrightness =
        map(
          combined,
          0,
          255,
          70,
          255
        );


    // Slight warm highlight on bright crests

    uint8_t greenAmount =
        map(
          combined,
          0,
          255,
          0,
          22
        );


    leftLEDs[i] =
        CRGB(
          redBrightness,
          greenAmount,
          0
        );
  }


  // ==================================================
  // RIGHT RED WATER
  // ==================================================

  for (
    int i = 0;
    i <= rightSurface &&
    i < NUM_LEDS;
    i++
  ) {

    uint8_t wave1 =
        sin8(
          i * 11 +
          now / 11
        );


    uint8_t wave2 =
        sin8(
          i * 8 -
          now / 19
        );


    uint8_t wave3 =
        sin8(
          i * 5 +
          now / 31
        );


    uint16_t total =
        wave1 +
        wave2 +
        wave3;


    uint8_t combined =
        total / 3;


    uint8_t redBrightness =
        map(
          combined,
          0,
          255,
          70,
          255
        );


    uint8_t greenAmount =
        map(
          combined,
          0,
          255,
          0,
          22
        );


    rightLEDs[i] =
        CRGB(
          redBrightness,
          greenAmount,
          0
        );
  }
}


// ====================================================
// BEGIN NEW FILL CYCLE
// ====================================================

void startFillCycle() {

  bool rising;


  targetTideLevel =
      getTideLevel(
        rising
      );


  targetTideLevel =
      constrain(
        targetTideLevel,
        1,
        NUM_LEDS - 2
      );


  redFillLevel =
      0;


  clearDrops();


  displayState =
      RED_FILL;


  stateStart =
      millis();


  lastDropLaunch =
      0;
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


  clearDrops();


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


  // Begin by showing only the original
  // moving blue/green background.

  displayState =
      BLUE_PAUSE;


  stateStart =
      millis();
}


// ====================================================
// MAIN LOOP
// ====================================================

void loop() {

  unsigned long now =
      millis();


  // ==================================================
  // ORIGINAL BLUE/GREEN BACKGROUND ALWAYS RUNS
  // ==================================================

  manageEddies();


  drawOceanBackground();


  // --------------------------------------------------
  // CURRENT TIDE DIRECTION
  // --------------------------------------------------

  bool tideRising;


  getTideLevel(
    tideRising
  );


  // ==================================================
  // BLUE PAUSE
  //
  // Original palette alone.
  // ==================================================

  if (
    displayState ==
    BLUE_PAUSE
  ) {

    if (
      now -
      stateStart >=
      BLUE_PAUSE_TIME
    ) {

      startFillCycle();
    }
  }


  // ==================================================
  // RED FILL
  //
  // Red drops fall through the moving blue palette.
  //
  // Red water rises continuously and reaches the
  // sampled tide height in exactly 15 seconds.
  // ==================================================

  else if (
    displayState ==
    RED_FILL
  ) {

    unsigned long fillAge =
        now -
        stateStart;


    // ------------------------------------------------
    // FILL PROGRESS
    // ------------------------------------------------

    float fillFraction =
        (float)fillAge /
        (float)RED_FILL_TIME;


    fillFraction =
        constrain(
          fillFraction,
          0.0,
          1.0
        );


    // Smooth-start / smooth-stop rise

    float smoothFill =
        fillFraction *
        fillFraction *
        (
          3.0 -
          2.0 *
          fillFraction
        );


    redFillLevel =
        targetTideLevel *
        smoothFill;


    // ------------------------------------------------
    // DRAW RED ACCUMULATED WATER
    // ------------------------------------------------

    drawSolidRedFill();


    // ------------------------------------------------
    // RELEASE RED DROPS
    // ------------------------------------------------

    if (
      now -
      lastDropLaunch >=
      DROP_INTERVAL
    ) {

      lastDropLaunch =
          now;


      // LEFT

      for (
        int i = 0;
        i < NUM_DROPS;
        i++
      ) {

        if (!leftDrops[i].active) {

          launchDrop(
            leftDrops[i]
          );

          break;
        }
      }


      // RIGHT
      //
      // Slightly less regular than left.

      if (
        random(100) <
        85
      ) {

        for (
          int i = 0;
          i < NUM_DROPS;
          i++
        ) {

          if (!rightDrops[i].active) {

            launchDrop(
              rightDrops[i]
            );

            break;
          }
        }
      }
    }


    // ------------------------------------------------
    // UPDATE DROP MOTION
    // ------------------------------------------------

    updateDrops(
      leftDrops
    );


    updateDrops(
      rightDrops
    );


    // ------------------------------------------------
    // DRAW RED DROPS
    // ------------------------------------------------

    drawDrops(
      leftLEDs,
      leftDrops
    );


    drawDrops(
      rightLEDs,
      rightDrops
    );


    // ------------------------------------------------
    // EXACTLY 15 SECONDS:
    // SWITCH TO FULL RED WAVE ANIMATION
    // ------------------------------------------------

    if (
      fillAge >=
      RED_FILL_TIME
    ) {

      redFillLevel =
          targetTideLevel;


      clearDrops();


      displayState =
          RED_WIGGLE;


      stateStart =
          now;
    }
  }


  // ==================================================
  // RED WAVE PHASE
  //
  // Red remains from bottom to tide level.
  //
  // Entire red region has traveling internal waves
  // and a moving upper surface.
  // ==================================================

  else if (
    displayState ==
    RED_WIGGLE
  ) {

    drawWigglingRedWater();


    // ------------------------------------------------
    // AFTER 30 SECONDS:
    //
    // Red vanishes.
    // Original blue/green palette is exposed.
    // Then another fill begins.
    // ------------------------------------------------

    if (
      now -
      stateStart >=
      RED_HOLD_TIME
    ) {

      clearDrops();


      redFillLevel =
          0;


      displayState =
          BLUE_PAUSE;


      stateStart =
          now;
    }
  }


  // ==================================================
  // GREEN DIRECTION INDICATOR
  //
  // Drawn last so it always remains visible.
  // ==================================================

  addDirectionIndicator(
    tideRising
  );


  // ==================================================
  // DISPLAY FRAME
  // ==================================================

  FastLED.show();


  delay(20);
}