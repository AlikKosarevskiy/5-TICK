#include <FastLED.h>

/* ================= CONFIG ================= */
#define NUM_STRIPS   5
#define NUM_LEDS     108

#define LED_TYPE     WS2812
#define COLOR_ORDER  GRB
#define BRIGHTNESS   40

#define SPEED_DELAY  40
#define HIT_ZONE     2
#define MAX_SCORE    10

#define COLOR_LEFT   CRGB::Green
#define COLOR_RIGHT  CRGB::Blue
#define COLOR_BALL   CRGB::White

const uint8_t LED_PINS[NUM_STRIPS] = {18, 17, 16, 15, 14};
const uint8_t BTN_L[NUM_STRIPS]    = {4, 5, 6, 7, 8};
const uint8_t BTN_R[NUM_STRIPS]    = {9, 10, 11, 12, 13};

/* ================= LED BUFFER ================= */
CRGB leds[NUM_STRIPS][NUM_LEDS];

/* ================= STATES ================= */
enum GlobalState { G_DEMO, G_START_FILL, G_PLAYING };
enum GameState   { PLAYING, GAME_OVER };

GlobalState globalState = G_DEMO;

/* ================= GAME STRUCT ================= */
struct StripGame {
  GameState state;
  int ballPos;
  int direction;       // текущее направление шарика
  int baseDirection;   // стартовое направление для каждой ленты
  int scoreL;
  int scoreR;
  unsigned long lastMove;
  unsigned long lastButton;
};

StripGame game[NUM_STRIPS];

/* ================= GLOBAL ANIM ================= */
int fillPos = 0;

/* ================= SETUP ================= */
void setup() { FastLED.setBrightness(BRIGHTNESS);
              FastLED.addLeds<WS2812, 18, GRB>(leds[0], NUM_LEDS); 
              FastLED.addLeds<WS2812, 17, GRB>(leds[1], NUM_LEDS); 
              FastLED.addLeds<WS2812, 16, GRB>(leds[2], NUM_LEDS); 
              FastLED.addLeds<WS2812, 15, GRB>(leds[3], NUM_LEDS); 
              FastLED.addLeds<WS2812, 14, GRB>(leds[4], NUM_LEDS); 
          for (int s = 0; s < NUM_STRIPS; s++) 
      { pinMode(BTN_L[s], INPUT_PULLUP); 
        pinMode(BTN_R[s], INPUT_PULLUP); } 
        FastLED.clear(); 
        FastLED.show(); 
      }

/* ================= DEMO ================= */
void demoAnimation(unsigned long now) {
  static int pos = 0;
  static int dir = 1;

  for (int s = 0; s < NUM_STRIPS; s++) {
    if (!digitalRead(BTN_L[s]) || !digitalRead(BTN_R[s])) {
      globalState = G_START_FILL;
      fillPos = 0;
      FastLED.clear();
      return;
    }
  }

  for (int s = 0; s < NUM_STRIPS; s++) {
    fadeToBlackBy(leds[s], NUM_LEDS, 40);
    leds[s][pos] = COLOR_BALL;
  }

  pos += dir;
  if (pos <= 0 || pos >= NUM_LEDS - 1) dir = -dir;
}

/* ================= START FILL ================= */
void startFillAnimation() {
  for (int s = 0; s < NUM_STRIPS; s++) {
    for (int i = 0; i <= fillPos; i++) {
      leds[s][i] = COLOR_LEFT;
      leds[s][NUM_LEDS - 1 - i] = COLOR_RIGHT;
    }
  }

  fillPos++;

  if (fillPos >= NUM_LEDS / 2) {
    for (int s = 0; s < NUM_STRIPS; s++) {
      game[s].state         = PLAYING;
      game[s].ballPos       = NUM_LEDS / 2;
      game[s].baseDirection = (s % 2 == 0) ? 1 : -1;
      game[s].direction     = game[s].baseDirection;
      game[s].scoreL        = 0;
      game[s].scoreR        = 0;
      game[s].lastMove      = millis();
      game[s].lastButton    = 0;
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

  if (!digitalRead(BTN_L[s])) {
    game[s].lastButton = now;
    if (game[s].ballPos <= HIT_ZONE)
      game[s].direction = abs(game[s].direction); // отражение вправо
    else {
      game[s].scoreR++;
      game[s].ballPos = NUM_LEDS / 2;
      game[s].direction = game[s].baseDirection; // стартовая фаза
    }
  }

  if (!digitalRead(BTN_R[s])) {
    game[s].lastButton = now;
    if (game[s].ballPos >= NUM_LEDS - 1 - HIT_ZONE)
      game[s].direction = -abs(game[s].direction); // отражение влево
    else {
      game[s].scoreL++;
      game[s].ballPos = NUM_LEDS / 2;
      game[s].direction = -game[s].baseDirection; // стартовая фаза
    }
  }
}

/* ================= PLAY GAME ================= */
void updateStrip(int s, unsigned long now) {
  StripGame &g = game[s];

  if (g.state == GAME_OVER) {
    CRGB winColor = (g.scoreL > g.scoreR) ? COLOR_LEFT : COLOR_RIGHT;
    fill_solid(leds[s], NUM_LEDS, winColor);
    return;
  }

  drawScores(s);
  handleButtons(s, now);

  if (now - g.lastMove >= SPEED_DELAY) {
    g.lastMove = now;
    g.ballPos += g.direction;

    if (g.ballPos < 0) {
      g.scoreR++;
      g.ballPos = NUM_LEDS / 2;
      g.direction = g.baseDirection;  // сохраняем стартовую фазу
    }

    if (g.ballPos >= NUM_LEDS) {
      g.scoreL++;
      g.ballPos = NUM_LEDS / 2;
      g.direction = -g.baseDirection; // сохраняем стартовую фазу
    }
  }

  leds[s][g.ballPos] = COLOR_BALL;

  if (g.scoreL >= MAX_SCORE || g.scoreR >= MAX_SCORE)
    g.state = GAME_OVER;
}

/* ================= LOOP ================= */
void loop() {
  static unsigned long lastFrame = 0;
  unsigned long now = millis();

  if (now - lastFrame < SPEED_DELAY) return;
  lastFrame = now;

  FastLED.clear();

  switch (globalState) {
    case G_DEMO:
      demoAnimation(now);
      break;

    case G_START_FILL:
      startFillAnimation();
      break;

    case G_PLAYING:
      for (int s = 0; s < NUM_STRIPS; s++)
        updateStrip(s, now);
      break;
  }

  FastLED.show();
}
