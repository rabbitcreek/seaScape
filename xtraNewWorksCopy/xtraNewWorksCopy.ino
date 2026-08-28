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
// FOUR-COLOR GRADIENT SETS
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


// ====================================================
// CURRENT GRADIENT SET
// ====================================================

GradientSet currentSet;
GradientSet previousSet;

bool transitioning = false;

unsigned long transitionStart = 0;
unsigned long lastSetChange   = 0;
unsigned long setInterval     = 10000;

#define TRANSITION_TIME 2500UL


// ====================================================
// GRADIENT MOTION
// ====================================================

float leftGradientPhase  = 0;
float rightGradientPhase = 0;

float leftGradientSpeed  = 0.85;
float rightGradientSpeed = 1.07;


// ====================================================
// SLOW DRIFTING LIGHT PULSES
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
//
// 60 seconds LOW -> HIGH
// 60 seconds HIGH -> LOW
// ====================================================

#define HALF_TIDE_MS 60000UL
#define FULL_TIDE_MS 120000UL


// ====================================================
// CHOOSE NEW GRADIENT SET
// ====================================================

void chooseNewGradientSet() {

  previousSet =
      currentSet;


  int n =
      random(
        NUM_GRADIENT_SETS
      );


  currentSet =
      gradientSets[n];


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
// GET SMOOTHLY TRANSITIONED GRADIENT SET
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
// CLEAR PULSES
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
// LAUNCH A SLOW PULSE
// ====================================================

void launchPulse(
  Pulse pulses[],
  bool fromBottom
) {

  for (int i = 0; i < MAX_PULSES; i++) {

    if (!pulses[i].active) {

      pulses[i].active =
          true;


      // Broad and soft

      pulses[i].width =
          random(
            700,
            1300
          ) /
          100.0;


      // Very gentle initial velocity

      pulses[i].velocity =
          random(
            10,
            24
          ) /
          100.0;


      // Tiny acceleration

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
// UPDATE SLOW PULSES
// ====================================================

void updatePulses(
  Pulse pulses[]
) {

  for (int i = 0; i < MAX_PULSES; i++) {

    if (!pulses[i].active)
      continue;


    // ------------------------------------------------
    // VERY GENTLE PHYSICS
    // ------------------------------------------------

    pulses[i].velocity +=
        pulses[i].acceleration;


    // Fluid resistance

    pulses[i].velocity *=
        0.9992;


    // Keep everything calm

    pulses[i].velocity =
        constrain(
          pulses[i].velocity,
          0.08,
          0.32
        );


    // Main movement

    pulses[i].center +=
        pulses[i].velocity *
        pulses[i].direction;


    // ------------------------------------------------
    // SUBTLE FLOATING DRIFT
    // ------------------------------------------------

    float drift =
        sin(
          millis() *
          0.00035 +
          i *
          1.7
        ) *
        0.015;


    pulses[i].center +=
        drift;


    // ------------------------------------------------
    // REMOVE WHEN OFF DISPLAY
    // ------------------------------------------------

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
// CALCULATE PULSE STRENGTH AT ONE LED
// ====================================================

uint8_t pulseAmountAtLED(
  Pulse pulses[],
  int led
) {

  uint16_t total =
      0;


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


    // Smooth cosine-shaped packet

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
// FADE GHOST COLORS
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
// DRAW BLENDED GRADIENT / SLOW-PULSE FIELD
// ====================================================

void drawLightField() {

  GradientSet active =
      getActiveGradientSet();


  // Underlying gradients move slowly.

  leftGradientPhase +=
      leftGradientSpeed;


  rightGradientPhase +=
      rightGradientSpeed;


  fadeGhosts();


  for (int i = 0; i < NUM_LEDS; i++) {


    // =================================================
    // LEFT GRADIENT
    // =================================================

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


    // =================================================
    // RIGHT GRADIENT
    // =================================================

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


    // =================================================
    // SLOW MOVING BRIGHTNESS FIELDS
    // =================================================

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


    // Always retain some illumination.
    //
    // Pulses enhance the gradient rather than
    // switching it completely on/off.

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


    // =================================================
    // GHOST MEMORY
    // =================================================

    if (
      leftPulse >
      leftGhost[i]
    ) {

      leftGhost[i] =
          leftPulse /
          4;


      leftGhostColor[i] =
          leftColor;
    }


    if (
      rightPulse >
      rightGhost[i]
    ) {

      rightGhost[i] =
          rightPulse /
          4;


      rightGhostColor[i] =
          rightColor;
    }


    // -------------------------------------------------
    // ADD GHOST ILLUMINATION
    // -------------------------------------------------

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
// BRIGHT TIDE LEVEL
//
// Two bright warm yellow/white LEDs.
//
// Surrounding pixels are reduced slightly so the
// tide level remains readable against any palette.
// ====================================================

void drawTideLevel(
  float tideLevel
) {

  int p =
      round(
        tideLevel
      );


  p =
      constrain(
        p,
        0,
        NUM_LEDS - 1
      );


  // --------------------------------------------------
  // DARKEN NEARBY LEDs FOR CONTRAST
  // --------------------------------------------------

  if (p > 1) {

    leftLEDs[p - 2].nscale8(
      100
    );

    rightLEDs[p - 2].nscale8(
      100
    );
  }


  if (p > 0) {

    leftLEDs[p - 1].nscale8(
      60
    );

    rightLEDs[p - 1].nscale8(
      60
    );
  }


  if (
    p + 1 <
    NUM_LEDS
  ) {

    leftLEDs[p + 1].nscale8(
      60
    );

    rightLEDs[p + 1].nscale8(
      60
    );
  }


  if (
    p + 2 <
    NUM_LEDS
  ) {

    leftLEDs[p + 2].nscale8(
      100
    );

    rightLEDs[p + 2].nscale8(
      100
    );
  }


  // --------------------------------------------------
  // MAIN LEVEL MARKER
  // --------------------------------------------------

  leftLEDs[p] =
      CRGB(
        255,
        255,
        80
      );


  rightLEDs[p] =
      CRGB(
        255,
        255,
        80
      );


  // Second LED gives a slightly broader horizontal
  // optical plane through the reflector.

  if (
    p + 1 <
    NUM_LEDS
  ) {

    leftLEDs[p + 1] =
        CRGB(
          255,
          210,
          30
        );


    rightLEDs[p + 1] =
        CRGB(
          255,
          210,
          30
        );
  }
}


// ====================================================
// GREEN TIDE DIRECTION MARKER
//
// TOP    = high tide coming
// BOTTOM = low tide coming
// ====================================================

void drawDirectionMarker(
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


  currentSet =
      gradientSets[0];


  previousSet =
      currentSet;


  unsigned long now =
      millis();


  lastSetChange =
      now;


  nextLeftPulse =
      now + 1000;


  nextRightPulse =
      now + 1700;
}


// ====================================================
// MAIN LOOP
// ====================================================

void loop() {

  unsigned long now =
      millis();


  // ==================================================
  // CHANGE COLOR COMBINATION
  // ==================================================

  if (
    now -
    lastSetChange >=
    setInterval
  ) {

    chooseNewGradientSet();
  }


  // ==================================================
  // CREATE SLOW LEFT PULSE
  // ==================================================

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


  // ==================================================
  // CREATE SLOW RIGHT PULSE
  // ==================================================

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


  // ==================================================
  // UPDATE SLOW MOTION
  // ==================================================

  updatePulses(
    leftPulses
  );


  updatePulses(
    rightPulses
  );


  // ==================================================
  // DRAW BLENDED LIGHT FIELD
  // ==================================================

  drawLightField();


  // ==================================================
  // CURRENT TIDE
  // ==================================================

  bool rising;


  float tideLevel =
      getTideLevel(
        rising
      );


  // ==================================================
  // TIDE HEIGHT
  // ==================================================

  drawTideLevel(
    tideLevel
  );


  // ==================================================
  // TIDE DIRECTION
  // ==================================================

  drawDirectionMarker(
    rising
  );


  // ==================================================
  // DISPLAY
  // ==================================================

  FastLED.show();


  delay(20);
}