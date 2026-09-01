#include <FastLED.h>

#define LED_PIN_LEFT    D2
#define LED_PIN_RIGHT   D4

#define NUM_LEDS        45
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB

#define BRIGHTNESS      235

CRGB leftLEDs[NUM_LEDS];
CRGB rightLEDs[NUM_LEDS];


// ====================================================
// NORMAL COLOR DISPLAY
// ====================================================

struct GradientSet {

  CRGB leftA;
  CRGB leftB;

  CRGB rightA;
  CRGB rightB;
};


GradientSet gradientSets[] = {

  {
    CRGB(255,   0,   0),
    CRGB(255, 110,   0),

    CRGB(  0,   0, 255),
    CRGB(  0, 220, 255)
  },

  {
    CRGB(255,   0, 180),
    CRGB(255,   0,  20),

    CRGB(  0, 230, 255),
    CRGB(  0, 255,  80)
  },

  {
    CRGB(  0,  30, 255),
    CRGB(180,   0, 255),

    CRGB(255, 180,   0),
    CRGB(255,  20,   0)
  },

  {
    CRGB(  0, 255,  50),
    CRGB(  0, 160, 255),

    CRGB(255,   0,   0),
    CRGB(255,   0, 180)
  },

  {
    CRGB(255,  80,   0),
    CRGB(255,   0, 170),

    CRGB(  0,  50, 255),
    CRGB(  0, 255, 120)
  },

  {
    CRGB(255,  30, 120),
    CRGB(140,   0, 255),

    CRGB(  0, 255, 190),
    CRGB(  0,  60, 255)
  }
};


#define NUM_GRADIENT_SETS \
  (sizeof(gradientSets) / sizeof(gradientSets[0]))


GradientSet currentSet;
GradientSet previousSet;

bool transitioning = false;

unsigned long transitionStart = 0;
unsigned long lastSetChange   = 0;
unsigned long setInterval     = 10000;

#define TRANSITION_TIME 2500UL


// ====================================================
// NORMAL GRADIENT MOTION
// ====================================================

float leftGradientPhase  = 0;
float rightGradientPhase = 0;

float leftGradientSpeed  = 0.85;
float rightGradientSpeed = 1.07;


// ====================================================
// CALM LIGHT PULSES
// ====================================================

struct Pulse {

  bool active;

  float center;
  float velocity;
  float acceleration;
  float width;

  int direction;

  uint8_t strength;
};


#define MAX_PULSES 4

Pulse leftPulses[MAX_PULSES];
Pulse rightPulses[MAX_PULSES];

unsigned long nextLeftPulse  = 0;
unsigned long nextRightPulse = 0;


// ====================================================
// GHOST MEMORY
// ====================================================

uint8_t leftGhost[NUM_LEDS];
uint8_t rightGhost[NUM_LEDS];

CRGB leftGhostColor[NUM_LEDS];
CRGB rightGhostColor[NUM_LEDS];


// ====================================================
// SIMULATED TIDE
// ====================================================

#define HALF_TIDE_MS 60000UL
#define FULL_TIDE_MS 120000UL


// ====================================================
// DISPLAY TIMING
// ====================================================

#define NORMAL_DISPLAY_TIME   30000UL
#define TIDE_FILL_TIME        15000UL
#define TIDE_HOLD_TIME        10000UL
#define BLUE_ONLY_TIME         2000UL


// ====================================================
// DISPLAY STATES
// ====================================================

enum DisplayState {

  NORMAL_DISPLAY,
  TIDE_FILL,
  TIDE_HOLD,
  TIDE_DRAIN,
  BLUE_ONLY

};


DisplayState displayState =
    NORMAL_DISPLAY;

unsigned long stateStart = 0;


// ====================================================
// TIDE VARIABLES
// ====================================================

float displayedTideLevel = 20.0;
float tideSurface = 0.0;


// ====================================================
// FALLING LIGHT BLOBS
// ====================================================

#define MAX_BLOBS 7

struct Blob {

  bool active;

  float position;
  float velocity;

  float radius;

  uint8_t brightness;

  float phase;
};


Blob leftBlobs[MAX_BLOBS];
Blob rightBlobs[MAX_BLOBS];

unsigned long nextLeftBlob  = 0;
unsigned long nextRightBlob = 0;


// ====================================================
// IMPACT ENERGY
// ====================================================

float leftImpactEnergy  = 0.0;
float rightImpactEnergy = 0.0;


// ====================================================
// CHOOSE COLOR SET
// ====================================================

void chooseNewGradientSet() {

  previousSet =
      currentSet;


  currentSet =
      gradientSets[
        random(
          NUM_GRADIENT_SETS
        )
      ];


  leftGradientSpeed =
      random(65, 125) /
      100.0;


  rightGradientSpeed =
      random(75, 140) /
      100.0;


  setInterval =
      random(
        8000,
        14000
      );


  transitionStart =
      millis();


  lastSetChange =
      millis();


  transitioning =
      true;
}


// ====================================================
// ACTIVE GRADIENT SET
// ====================================================

GradientSet getActiveGradientSet() {

  if (!transitioning)
    return currentSet;


  unsigned long elapsed =
      millis() -
      transitionStart;


  if (elapsed >= TRANSITION_TIME) {

    transitioning =
        false;

    return currentSet;
  }


  uint8_t amount =
      map(
        elapsed,
        0,
        TRANSITION_TIME,
        0,
        255
      );


  GradientSet result;


  result.leftA =
      blend(
        previousSet.leftA,
        currentSet.leftA,
        amount
      );


  result.leftB =
      blend(
        previousSet.leftB,
        currentSet.leftB,
        amount
      );


  result.rightA =
      blend(
        previousSet.rightA,
        currentSet.rightA,
        amount
      );


  result.rightB =
      blend(
        previousSet.rightB,
        currentSet.rightB,
        amount
      );


  return result;
}


// ====================================================
// CLEAR NORMAL PULSES
// ====================================================

void clearPulses() {

  for (int i = 0; i < MAX_PULSES; i++) {

    leftPulses[i].active =
        false;

    rightPulses[i].active =
        false;
  }
}


// ====================================================
// LAUNCH NORMAL CALM PULSE
// ====================================================

void launchPulse(
  Pulse pulses[],
  bool fromBottom
) {

  for (int i = 0; i < MAX_PULSES; i++) {

    if (!pulses[i].active) {

      pulses[i].active =
          true;


      pulses[i].width =
          random(
            700,
            1300
          ) /
          100.0;


      pulses[i].velocity =
          random(
            10,
            24
          ) /
          100.0;


      pulses[i].acceleration =
          random(
            2,
            7
          ) /
          10000.0;


      pulses[i].strength =
          random(
            175,
            235
          );


      if (fromBottom) {

        pulses[i].center =
            -pulses[i].width;

        pulses[i].direction =
            1;

      } else {

        pulses[i].center =
            NUM_LEDS +
            pulses[i].width;

        pulses[i].direction =
            -1;
      }


      return;
    }
  }
}


// ====================================================
// UPDATE NORMAL PULSES
// ====================================================

void updatePulses(
  Pulse pulses[]
) {

  for (int i = 0; i < MAX_PULSES; i++) {

    if (!pulses[i].active)
      continue;


    pulses[i].velocity +=
        pulses[i].acceleration;


    pulses[i].velocity *=
        0.9992;


    pulses[i].velocity =
        constrain(
          pulses[i].velocity,
          0.08,
          0.32
        );


    pulses[i].center +=
        pulses[i].velocity *
        pulses[i].direction;


    float drift =
        sin(
          millis() * 0.00035 +
          i * 1.7
        ) *
        0.015;


    pulses[i].center +=
        drift;


    if (
      pulses[i].center <
      -pulses[i].width - 3
    ) {

      pulses[i].active =
          false;
    }


    if (
      pulses[i].center >
      NUM_LEDS +
      pulses[i].width + 3
    ) {

      pulses[i].active =
          false;
    }
  }
}


// ====================================================
// NORMAL PULSE STRENGTH
// ====================================================

uint8_t pulseAmountAtLED(
  Pulse pulses[],
  int led
) {

  uint16_t total = 0;


  for (int p = 0; p < MAX_PULSES; p++) {

    if (!pulses[p].active)
      continue;


    float distance =
        abs(
          (float)led -
          pulses[p].center
        );


    if (
      distance >
      pulses[p].width
    )
      continue;


    float x =
        distance /
        pulses[p].width;


    float envelope =
        0.5 *
        (
          1.0 +
          cos(
            x *
            PI
          )
        );


    total +=
        pulses[p].strength *
        envelope;
  }


  return
      constrain(
        total,
        0,
        255
      );
}


// ====================================================
// FADE GHOSTS
// ====================================================

void fadeGhosts() {

  for (int i = 0; i < NUM_LEDS; i++) {

    leftGhost[i] =
        scale8(
          leftGhost[i],
          246
        );


    rightGhost[i] =
        scale8(
          rightGhost[i],
          246
        );
  }
}


// ====================================================
// NORMAL COLOR ARTWORK
// ====================================================

void drawNormalDisplay() {

  GradientSet active =
      getActiveGradientSet();


  leftGradientPhase +=
      leftGradientSpeed;


  rightGradientPhase +=
      rightGradientSpeed;


  fadeGhosts();


  for (int i = 0; i < NUM_LEDS; i++) {


    uint8_t leftMix =
        sin8(
          i * 11 -
          leftGradientPhase
        );


    CRGB leftColor =
        blend(
          active.leftA,
          active.leftB,
          leftMix
        );


    uint8_t rightMix =
        sin8(
          i * 9 +
          rightGradientPhase
        );


    CRGB rightColor =
        blend(
          active.rightA,
          active.rightB,
          rightMix
        );


    uint8_t leftPulse =
        pulseAmountAtLED(
          leftPulses,
          i
        );


    uint8_t rightPulse =
        pulseAmountAtLED(
          rightPulses,
          i
        );


    uint8_t leftBrightness =
        65 +
        scale8(
          leftPulse,
          190
        );


    uint8_t rightBrightness =
        65 +
        scale8(
          rightPulse,
          190
        );


    leftColor.nscale8(
      leftBrightness
    );


    rightColor.nscale8(
      rightBrightness
    );


    if (
      leftPulse >
      leftGhost[i]
    ) {

      leftGhost[i] =
          leftPulse / 4;


      leftGhostColor[i] =
          leftColor;
    }


    if (
      rightPulse >
      rightGhost[i]
    ) {

      rightGhost[i] =
          rightPulse / 4;


      rightGhostColor[i] =
          rightColor;
    }


    CRGB leftAfter =
        leftGhostColor[i];


    leftAfter.nscale8(
      leftGhost[i]
    );


    CRGB rightAfter =
        rightGhostColor[i];


    rightAfter.nscale8(
      rightGhost[i]
    );


    leftColor +=
        leftAfter;


    rightColor +=
        rightAfter;


    leftLEDs[i] =
        leftColor;


    rightLEDs[i] =
        rightColor;
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


  if (
    phase <
    HALF_TIDE_MS
  ) {

    rising =
        true;


    fraction =
        (float)phase /
        (float)HALF_TIDE_MS;

  } else {

    rising =
        false;


    fraction =
        (float)(
          phase -
          HALF_TIDE_MS
        ) /
        (float)HALF_TIDE_MS;
  }


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
// DARK GREEN-BLUE TIDE BACKGROUND
// ====================================================

void drawTideBackground() {

  for (int i = 0; i < NUM_LEDS; i++) {

    float f =
        (float)i /
        (float)(NUM_LEDS - 1);


    float curve =
        f * f;


    uint8_t green =
        10 +
        72 *
        curve;


    uint8_t blue =
        18 +
        48 *
        curve;


    leftLEDs[i] =
        CRGB(
          0,
          green,
          blue
        );


    rightLEDs[i] =
        CRGB(
          0,
          min(
            255,
            green + 8
          ),
          max(
            0,
            blue - 4
          )
        );
  }
}


// ====================================================
// ANIMATED RED TIDE BODY
//
// Dark crimson at bottom.
//
// Bright red near surface.
//
// Two broad dark-red bands slowly move upward
// and back downward through the tide body.
// ====================================================

void drawTideBody(
  float surface
) {

  surface =
      constrain(
        surface,
        0.0,
        NUM_LEDS - 1
      );


  int top =
      floor(
        surface
      );


  if (top < 0)
    return;


  float t =
      millis() /
      1000.0;


  // --------------------------------------------------
  // MOVING DARK BANDS
  // --------------------------------------------------

  float band1Normalized =
      0.5 +
      0.5 *
      sin(
        t *
        0.55
      );


  float band2Normalized =
      0.5 +
      0.5 *
      sin(
        t *
        0.34 +
        2.1
      );


  float band1Center =
      surface *
      (
        0.12 +
        0.76 *
        band1Normalized
      );


  float band2Center =
      surface *
      (
        0.18 +
        0.66 *
        band2Normalized
      );


  float band1Width =
      2.3;


  float band2Width =
      3.5;


  // ==================================================
  // DRAW RED VOLUME
  // ==================================================

  for (
    int i = 0;
    i <= top &&
    i < NUM_LEDS;
    i++
  ) {

    float f;


    if (surface > 0.2) {

      f =
          (float)i /
          surface;

    } else {

      f =
          1.0;
    }


    f =
        constrain(
          f,
          0.0,
          1.0
        );


    // Vertical red gradient

    float gradient =
        f *
        f;


    float baseRed =
        48 +
        207 *
        gradient;


    float baseGreen =
        1 +
        25 *
        gradient *
        gradient;


    // ------------------------------------------------
    // DARK BAND 1
    // ------------------------------------------------

    float d1 =
        (
          i -
          band1Center
        ) /
        band1Width;


    float band1 =
        exp(
          -d1 *
          d1
        );


    // ------------------------------------------------
    // DARK BAND 2
    // ------------------------------------------------

    float d2 =
        (
          i -
          band2Center
        ) /
        band2Width;


    float band2 =
        exp(
          -d2 *
          d2
        );


    // ------------------------------------------------
    // COMBINED DARKNESS
    // ------------------------------------------------

    float darkness =
        1.0 -
        (
          0.55 *
          band1
        ) -
        (
          0.32 *
          band2
        );


    darkness =
        constrain(
          darkness,
          0.30,
          1.0
        );


    // ------------------------------------------------
    // SMALL INTERNAL MOTION
    // ------------------------------------------------

    float leftMotion =
        sin(
          t *
          1.15 +
          i *
          0.43
        );


    float rightMotion =
        sin(
          t *
          0.97 +
          i *
          0.38 +
          1.6
        );


    int leftRed =
        constrain(
          (
            baseRed *
            darkness
          ) +
          leftMotion *
          10,
          0,
          255
        );


    int rightRed =
        constrain(
          (
            baseRed *
            darkness
          ) +
          rightMotion *
          10,
          0,
          255
        );


    int green =
        constrain(
          baseGreen *
          darkness,
          0,
          255
        );


    leftLEDs[i] =
        CRGB(
          leftRed,
          green,
          0
        );


    rightLEDs[i] =
        CRGB(
          rightRed,
          min(
            255,
            green + 3
          ),
          0
        );
  }


  // ==================================================
  // BRIGHT LIVING SURFACE
  // ==================================================

  int surfaceLED =
      constrain(
        round(
          surface
        ),
        0,
        NUM_LEDS - 1
      );


  uint8_t surfacePulse =
      beatsin8(
        9,
        210,
        255
      );


  leftLEDs[surfaceLED] =
      CRGB(
        surfacePulse,
        40,
        0
      );


  rightLEDs[surfaceLED] =
      CRGB(
        surfacePulse,
        40,
        0
      );
}


// ====================================================
// CLEAR BLOBS
// ====================================================

void clearBlobs() {

  for (int i = 0; i < MAX_BLOBS; i++) {

    leftBlobs[i].active =
        false;

    rightBlobs[i].active =
        false;
  }
}


// ====================================================
// LAUNCH LIGHT BLOB
// ====================================================

void launchBlob(
  Blob blobs[]
) {

  for (int i = 0; i < MAX_BLOBS; i++) {

    if (!blobs[i].active) {

      blobs[i].active =
          true;


      blobs[i].position =
          NUM_LEDS + 2;


      blobs[i].velocity =
          random(
            12,
            23
          ) /
          100.0;


      blobs[i].radius =
          random(
            140,
            290
          ) /
          100.0;


      blobs[i].brightness =
          random(
            190,
            256
          );


      blobs[i].phase =
          random(
            0,
            628
          ) /
          100.0;


      return;
    }
  }
}


// ====================================================
// UPDATE FALLING BLOBS
//
// Blobs accelerate gently and then slow as they
// approach the rising tide surface.
// ====================================================

void updateBlobs(
  Blob blobs[],
  float surface,
  float &impactEnergy
) {

  for (int i = 0; i < MAX_BLOBS; i++) {

    if (!blobs[i].active)
      continue;


    float fallDistance =
        blobs[i].position -
        surface;


    float totalDistance =
        NUM_LEDS -
        surface;


    if (totalDistance < 1.0)
      totalDistance = 1.0;


    float normalized =
        fallDistance /
        totalDistance;


    normalized =
        constrain(
          normalized,
          0.0,
          1.0
        );


    float acceleration;


    if (normalized > 0.40) {

      acceleration =
          0.0045 *
          normalized;

    } else {

      float braking =
          (
            0.40 -
            normalized
          ) /
          0.40;


      acceleration =
          -0.009 *
          braking;
    }


    blobs[i].velocity +=
        acceleration;


    blobs[i].velocity =
        constrain(
          blobs[i].velocity,
          0.055,
          0.30
        );


    blobs[i].position -=
        blobs[i].velocity;


    blobs[i].position +=
        sin(
          millis() * 0.0015 +
          blobs[i].phase
        ) *
        0.003;


    // Impact with tide surface

    if (
      blobs[i].position -
      blobs[i].radius <=
      surface
    ) {

      blobs[i].active =
          false;


      impactEnergy +=
          blobs[i].radius *
          0.18;


      impactEnergy =
          constrain(
            impactEnergy,
            0.0,
            2.0
          );
    }
  }
}


// ====================================================
// DRAW TWO-COLOR BLOBS
//
// Main body:
//     red / orange
//
// Offset shadow:
//     green
//
// Left and right shadows are shifted in opposite
// directions to help the reflector create depth.
// ====================================================

void drawBlobs(
  CRGB leds[],
  Blob blobs[],
  int shadowDirection
) {

  for (int b = 0; b < MAX_BLOBS; b++) {

    if (!blobs[b].active)
      continue;


    // ------------------------------------------------
    // GREEN SHADOW
    // ------------------------------------------------

    float shadowCenter =
        blobs[b].position +
        shadowDirection *
        1.25;


    float shadowRadius =
        blobs[b].radius *
        1.15;


    for (int i = 0; i < NUM_LEDS; i++) {

      float distance =
          abs(
            (float)i -
            shadowCenter
          );


      if (
        distance >
        shadowRadius
      )
        continue;


      float x =
          distance /
          shadowRadius;


      float envelope =
          0.5 *
          (
            1.0 +
            cos(
              x *
              PI
            )
          );


      uint8_t green =
          150 *
          envelope;


      uint8_t blue =
          18 *
          envelope;


      leds[i] +=
          CRGB(
            0,
            green,
            blue
          );
    }


    // ------------------------------------------------
    // RED / ORANGE CORE
    // ------------------------------------------------

    for (int i = 0; i < NUM_LEDS; i++) {

      float distance =
          abs(
            (float)i -
            blobs[b].position
          );


      if (
        distance >
        blobs[b].radius
      )
        continue;


      float x =
          distance /
          blobs[b].radius;


      float envelope =
          0.5 *
          (
            1.0 +
            cos(
              x *
              PI
            )
          );


      uint8_t red =
          blobs[b].brightness *
          envelope;


      uint8_t green =
          42 *
          envelope *
          envelope;


      leds[i] +=
          CRGB(
            red,
            green,
            0
          );
    }
  }
}


// ====================================================
// START TIDE DISPLAY
// ====================================================

void startTideDisplay() {

  bool rising;


  displayedTideLevel =
      getTideLevel(
        rising
      );


  displayedTideLevel =
      constrain(
        displayedTideLevel,
        1,
        NUM_LEDS - 2
      );


  tideSurface =
      0.0;


  leftImpactEnergy =
      0.0;


  rightImpactEnergy =
      0.0;


  clearBlobs();


  nextLeftBlob =
      millis() +
      150;


  nextRightBlob =
      millis() +
      350;


  displayState =
      TIDE_FILL;


  stateStart =
      millis();
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


  randomSeed(
    micros()
  );


  clearPulses();

  clearBlobs();


  currentSet =
      gradientSets[0];


  previousSet =
      currentSet;


  unsigned long now =
      millis();


  lastSetChange =
      now;


  nextLeftPulse =
      now +
      1000;


  nextRightPulse =
      now +
      1700;


  displayState =
      NORMAL_DISPLAY;


  stateStart =
      now;
}


// ====================================================
// MAIN LOOP
// ====================================================

void loop() {

  unsigned long now =
      millis();


  // ==================================================
  // NORMAL ARTWORK
  // ==================================================

  if (
    displayState ==
    NORMAL_DISPLAY
  ) {

    if (
      now -
      lastSetChange >=
      setInterval
    ) {

      chooseNewGradientSet();
    }


    if (
      now >=
      nextLeftPulse
    ) {

      launchPulse(
        leftPulses,
        random(100) < 50
      );


      nextLeftPulse =
          now +
          random(
            2200,
            5000
          );
    }


    if (
      now >=
      nextRightPulse
    ) {

      launchPulse(
        rightPulses,
        random(100) < 50
      );


      nextRightPulse =
          now +
          random(
            2600,
            5500
          );
    }


    updatePulses(
      leftPulses
    );


    updatePulses(
      rightPulses
    );


    drawNormalDisplay();


    if (
      now -
      stateStart >=
      NORMAL_DISPLAY_TIME
    ) {

      startTideDisplay();
    }
  }


  // ==================================================
  // TIDE BLOB FILL
  // ==================================================

  else if (
    displayState ==
    TIDE_FILL
  ) {

    drawTideBackground();


    unsigned long age =
        now -
        stateStart;


    float fraction =
        (float)age /
        (float)TIDE_FILL_TIME;


    fraction =
        constrain(
          fraction,
          0.0,
          1.0
        );


    // Smooth rising target

    float targetSurface =
        displayedTideLevel *
        (
          0.5 -
          0.5 *
          cos(
            fraction *
            PI
          )
        );


    // ------------------------------------------------
    // IMPACT ENERGY DECAY
    // ------------------------------------------------

    leftImpactEnergy *=
        0.965;


    rightImpactEnergy *=
        0.965;


    float leftImpactWave =
        sin(
          millis() *
          0.010
        ) *
        leftImpactEnergy;


    float rightImpactWave =
        sin(
          millis() *
          0.009 +
          1.2
        ) *
        rightImpactEnergy;


    tideSurface =
        targetSurface;


    // ------------------------------------------------
    // LAUNCH BLOBS
    // ------------------------------------------------

    if (
      now >=
      nextLeftBlob
    ) {

      launchBlob(
        leftBlobs
      );


      nextLeftBlob =
          now +
          random(
            500,
            1050
          );
    }


    if (
      now >=
      nextRightBlob
    ) {

      launchBlob(
        rightBlobs
      );


      nextRightBlob =
          now +
          random(
            550,
            1150
          );
    }


    // ------------------------------------------------
    // UPDATE BLOBS
    // ------------------------------------------------

    updateBlobs(
      leftBlobs,
      tideSurface,
      leftImpactEnergy
    );


    updateBlobs(
      rightBlobs,
      tideSurface,
      rightImpactEnergy
    );


    // ------------------------------------------------
    // DRAW TIDE BODY
    // ------------------------------------------------

    drawTideBody(
      tideSurface
    );


    // Slightly separate left/right surface disturbance

    float leftSurface =
        tideSurface +
        leftImpactWave;


    float rightSurface =
        tideSurface +
        rightImpactWave;


    if (
      leftSurface >
      0
    ) {

      int p =
          constrain(
            round(
              leftSurface
            ),
            0,
            NUM_LEDS - 1
          );


      leftLEDs[p] =
          CRGB(
            255,
            48,
            0
          );
    }


    if (
      rightSurface >
      0
    ) {

      int p =
          constrain(
            round(
              rightSurface
            ),
            0,
            NUM_LEDS - 1
          );


      rightLEDs[p] =
          CRGB(
            255,
            48,
            0
          );
    }


    // ------------------------------------------------
    // TWO-COLOR FALLING BLOBS
    // ------------------------------------------------

    drawBlobs(
      leftLEDs,
      leftBlobs,
      1
    );


    drawBlobs(
      rightLEDs,
      rightBlobs,
      -1
    );


    // ------------------------------------------------
    // COMPLETE FILL
    // ------------------------------------------------

    if (
      age >=
      TIDE_FILL_TIME
    ) {

      clearBlobs();


      tideSurface =
          displayedTideLevel;


      displayState =
          TIDE_HOLD;


      stateStart =
          now;
    }
  }


  // ==================================================
  // TIDE HOLD / SLOSH
  // ==================================================

  else if (
    displayState ==
    TIDE_HOLD
  ) {

    drawTideBackground();


    unsigned long age =
        now -
        stateStart;


    float seconds =
        age /
        1000.0;


    float decay =
        exp(
          -seconds *
          0.13
        );


    float wave =
        sin(
          seconds *
          1.7
        ) *
        1.25 *
        decay;


    wave +=
        sin(
          seconds *
          0.83 +
          1.4
        ) *
        0.55 *
        decay;


    tideSurface =
        displayedTideLevel +
        wave;


    drawTideBody(
      tideSurface
    );


    if (
      age >=
      TIDE_HOLD_TIME
    ) {

      displayState =
          TIDE_DRAIN;


      stateStart =
          now;


      tideSurface =
          displayedTideLevel;
    }
  }


  // ==================================================
  // TIDE DRAIN
  //
  // Fastest while deep,
  // progressively slower as it empties.
  // ==================================================

  else if (
    displayState ==
    TIDE_DRAIN
  ) {

    drawTideBackground();


    float reference =
        max(
          displayedTideLevel,
          1.0f
        );


    float depthFraction =
        tideSurface /
        reference;


    depthFraction =
        constrain(
          depthFraction,
          0.0,
          1.0
        );


    float drainSpeed =
        0.012 +
        0.080 *
        sqrt(
          depthFraction
        );


    float ripple =
        sin(
          millis() *
          0.0025
        ) *
        0.006;


    tideSurface -=
        drainSpeed +
        ripple;


    if (
      tideSurface >
      0.05
    ) {

      drawTideBody(
        tideSurface
      );

    } else {

      tideSurface =
          0;


      displayState =
          BLUE_ONLY;


      stateStart =
          now;
    }
  }


  // ==================================================
  // DARK GREEN-BLUE BACKGROUND ONLY
  // ==================================================

  else if (
    displayState ==
    BLUE_ONLY
  ) {

    drawTideBackground();


    if (
      now -
      stateStart >=
      BLUE_ONLY_TIME
    ) {

      displayState =
          NORMAL_DISPLAY;


      stateStart =
          now;


      nextLeftPulse =
          now +
          1000;


      nextRightPulse =
          now +
          1700;
    }
  }


  // ==================================================
  // DISPLAY FRAME
  // ==================================================

  FastLED.show();

  delay(20);
}