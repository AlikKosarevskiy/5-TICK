#include <FastLED.h>

/* ================= CONFIG ================= */

#define NUM_STRIPS   5
#define NUM_LEDS     108

#define LED_TYPE     WS2812
#define COLOR_ORDER  GRB

#define DIR_RIGHT  1
#define DIR_LEFT  -1

#define DEMO_DELAY 25   // мс для демо-анимации
#define GAME_DELAY 30   // мс для движения шарика в игре
#define SPEED_DELAY  30
#define HIT_ZONE     3
#define SCORE_STEP 10
#define MAX_SCORE  50

#define BRIGHTNESS 40

#define COLOR_LEFT   CRGB(0, 100, 0)
#define COLOR_RIGHT  CRGB(0, 0, 100)
#define COLOR_BALL   CRGB(255, 255, 255)

const uint8_t LED_PINS[NUM_STRIPS] = {18, 17, 16, 15, 14};
const uint8_t BTN_L[NUM_STRIPS]    = {4, 5, 6, 7, 8};
const uint8_t BTN_R[NUM_STRIPS]    = {9, 10, 11, 12, 13};

/* ================= LED BUFFER ================= */

CRGB leds[NUM_STRIPS][NUM_LEDS];

/* ================= STATES ================= */

enum GlobalState {
  G_DEMO,
  G_START_FILL,
  G_PLAYING,
  G_GAME_OVER_ANIM
};

enum GameState {
  PLAYING,
  GAME_OVER
};

GlobalState globalState = G_DEMO;

/* ================= GAME STRUCT ================= */

struct StripGame {
  GameState state;
  int ballPos;
  int direction;
  int scoreL;
  int scoreR;
  unsigned long lastMove;
  unsigned long lastButton;
};

StripGame game[NUM_STRIPS];

/* ================= GLOBAL ANIM ================= */

int fillPos = 0;
CRGB gameOverColor = CRGB::Black;
int blinkCount = 0;
bool blinkState = false;
unsigned long lastBlink = 0;

/* ================= SETUP ================= */

void setup() {
  FastLED.setBrightness(BRIGHTNESS);

  FastLED.addLeds<WS2812, 18, GRB>(leds[0], NUM_LEDS);
  FastLED.addLeds<WS2812, 17, GRB>(leds[1], NUM_LEDS);
  FastLED.addLeds<WS2812, 16, GRB>(leds[2], NUM_LEDS);
  FastLED.addLeds<WS2812, 15, GRB>(leds[3], NUM_LEDS);
  FastLED.addLeds<WS2812, 14, GRB>(leds[4], NUM_LEDS);

  for (int s = 0; s < NUM_STRIPS; s++) {
    pinMode(BTN_L[s], INPUT_PULLUP);
    pinMode(BTN_R[s], INPUT_PULLUP);
  }

  FastLED.clear();
  FastLED.show();
}

/* ================= DEMO ================= */

void demoAnimation() {
  // Проверка кнопок
  for (int s = 0; s < NUM_STRIPS; s++) {
    if (!digitalRead(BTN_L[s]) || !digitalRead(BTN_R[s])) {
      globalState = G_START_FILL;
      fillPos = 0;
      FastLED.clear();
      return;
    }
  }

  // ===== Плавное дыхание =====
  static uint8_t step = 2;
  static int dir = 1;
  const int minB = 15;
  const int maxB = 50;
  const int stepsCount = 20;

  float brightness = minB + (maxB - minB) * step / float(stepsCount);

  for (int s = 0; s < NUM_STRIPS; s++) {
    fill_solid(leds[s], NUM_LEDS, CRGB(brightness, brightness, brightness));
  }

  step += dir;
  if (step >= stepsCount) dir = -1;
  if (step <= 0) dir = 1;
}

/* ================= START FILL ================= */

void startFillAnimation() {
  static unsigned long lastFillUpdate = 0;
  const unsigned long fillDelay = 20;

  unsigned long now = millis();
  if (now - lastFillUpdate < fillDelay) return;
  lastFillUpdate = now;

  for (int s = 0; s < NUM_STRIPS; s++) {
    for (int i = 0; i <= fillPos; i++) {
      leds[s][i] = COLOR_LEFT;
      leds[s][NUM_LEDS - 1 - i] = COLOR_RIGHT;
    }
  }

  fillPos++;

  if (fillPos >= NUM_LEDS / 2) {
    for (int s = 0; s < NUM_STRIPS; s++) {
      game[s].state      = PLAYING;
      game[s].ballPos    = NUM_LEDS / 2;
      game[s].direction  = (s % 2 == 0) ? 1 : -1;
      game[s].scoreL     = 0;
      game[s].scoreR     = 0;
      game[s].lastMove   = millis();
      game[s].lastButton = 0;
    }
    globalState = G_PLAYING;
  }
}

/* ================= DRAW SCORE ================= */

void drawScores(int s) {
  for (int i = 0; i < game[s].scoreL; i++)
    leds[s][i] = COLOR_LEFT;

  for (int i = 0; i < game[s].scoreR; i++)
    leds[s][NUM_LEDS - 1 - i] = COLOR_RIGHT;
}

/* ================= BUTTONS ================= */

void handleButtons(int s, unsigned long now) {
  if (now - game[s].lastButton < 150) return;

  int leftZoneStart  = game[s].scoreL;
  int leftZoneEnd    = game[s].scoreL + HIT_ZONE;
  int rightZoneEnd   = NUM_LEDS - 1 - game[s].scoreR;
  int rightZoneStart = rightZoneEnd - HIT_ZONE;

  if (!digitalRead(BTN_L[s])) {
    game[s].lastButton = now;
    if (game[s].ballPos >= leftZoneStart && game[s].ballPos <= leftZoneEnd)
      game[s].direction = DIR_RIGHT;
    else {
      game[s].scoreR += SCORE_STEP;
      game[s].ballPos = NUM_LEDS / 2;
      game[s].direction = DIR_RIGHT;
    }
  }

  if (!digitalRead(BTN_R[s])) {
    game[s].lastButton = now;
    if (game[s].ballPos >= rightZoneStart && game[s].ballPos <= rightZoneEnd)
      game[s].direction = DIR_LEFT;
    else {
      game[s].scoreL += SCORE_STEP;
      game[s].ballPos = NUM_LEDS / 2;
      game[s].direction = DIR_LEFT;
    }
  }
}

/* ================= PLAY GAME ================= */

// ================= PLAY GAME =================
void updateStrip(int s, unsigned long now) {
  StripGame &g = game[s];

  if (g.state == GAME_OVER) {
    // Если левая сторона выиграла
    if (g.scoreL >= MAX_SCORE) fill_solid(leds[s], NUM_LEDS, COLOR_LEFT);
    // Если правая сторона выиграла
    else if (g.scoreR >= MAX_SCORE) fill_solid(leds[s], NUM_LEDS, COLOR_RIGHT);
    return;
  }

  drawScores(s);
  handleButtons(s, now);

  if (now - g.lastMove >= SPEED_DELAY) {
    g.lastMove = now;
    g.ballPos += g.direction;

    if (g.ballPos < 0) {
      g.scoreR += SCORE_STEP;
      g.ballPos = NUM_LEDS / 2;
      g.direction = DIR_RIGHT;
    }

    if (g.ballPos >= NUM_LEDS) {
      g.scoreL += SCORE_STEP;
      g.ballPos = NUM_LEDS / 2;
      g.direction = DIR_LEFT;
    }
  }

  leds[s][g.ballPos] = COLOR_BALL;

  if (g.scoreL >= MAX_SCORE || g.scoreR >= MAX_SCORE) {
    g.state = GAME_OVER;
  }
}


/* ================= CHECK GAME OVER BY COLOR ================= */

bool checkThreeStripsSameColor(CRGB &color) {
  int leftCount = 0;
  int rightCount = 0;

  for (int s = 0; s < NUM_STRIPS; s++) {
    if (game[s].scoreL >= MAX_SCORE) leftCount++;
    if (game[s].scoreR >= MAX_SCORE) rightCount++;
  }

  if (leftCount >= 3) {
    color = COLOR_LEFT;
    return true;
  }

  if (rightCount >= 3) {
    color = COLOR_RIGHT;
    return true;
  }

  return false;
}

/* ================= LOOP ================= */

void loop() {
  static unsigned long lastDemo = 0;
  static unsigned long lastGame = 0;
  unsigned long now = millis();

  FastLED.clear();

  switch (globalState) {
    case G_DEMO:
      if (now - lastDemo >= DEMO_DELAY) {
        demoAnimation();
        lastDemo = now;
        FastLED.show();
      }
      break;

    case G_START_FILL:
      startFillAnimation();
      FastLED.show();
      break;

    case G_PLAYING:
      if (now - lastGame >= GAME_DELAY) {
        for (int s = 0; s < NUM_STRIPS; s++)
          updateStrip(s, now);
        lastGame = now;

        // Проверяем три ленты одного цвета
        if (checkThreeStripsSameColor(gameOverColor)) {
          blinkCount = 0;
          blinkState = false;
          lastBlink = now;
          globalState = G_GAME_OVER_ANIM;
        }

        FastLED.show();
      }
      break;

    case G_GAME_OVER_ANIM: {
      const unsigned long blinkDelay = 300; // мс между миганиями
      if (now - lastBlink >= blinkDelay) {
        lastBlink = now;
        blinkState = !blinkState;
        blinkCount++;

        for (int s = 0; s < NUM_STRIPS; s++) {
          fill_solid(leds[s], NUM_LEDS, blinkState ? gameOverColor : CRGB::Black);
        }

        FastLED.show();

        if (blinkCount >= 10) { // 5 миганий (10 переключений)
          globalState = G_DEMO;
          fillPos = 0;
          for (int s = 0; s < NUM_STRIPS; s++) {
            game[s].state = PLAYING;
            game[s].scoreL = 0;
            game[s].scoreR = 0;
          }
        }
      }
    } break;
  }
}
