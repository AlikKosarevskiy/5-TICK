#include <FastLED.h>

#define LED_PIN     18
#define BTN_P1      4
#define BTN_P2      6

#define NUM_LEDS    108
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#define SPEED_DELAY 40          // скорость "мяча"
#define HIT_ZONE    4           // зона попадания у края
#define MAX_SCORE   10

CRGB leds[NUM_LEDS];

int ballPos = NUM_LEDS / 2;
int direction = 1;

int scoreP1 = 0;
int scoreP2 = 0;

bool gameOver = false;

unsigned long lastMove = 0;

void setup() {
  pinMode(BTN_P1, INPUT_PULLUP);
  pinMode(BTN_P2, INPUT_PULLUP);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(40);   // <<< ЯРКОСТЬ ЛЕНТЫ
  FastLED.clear();
  FastLED.show();
}

void drawScores() {
  // Игрок 1 — слева (зелёный)
  for (int i = 0; i < scoreP1; i++) {
    leds[i] = CRGB::Green;
  }

  // Игрок 2 — справа (красный)
  for (int i = 0; i < scoreP2; i++) {
    leds[NUM_LEDS - 1 - i] = CRGB::Red;
  }
}

void resetRound(bool toRight) {
  ballPos = NUM_LEDS / 2;
  direction = toRight ? 1 : -1;
}

void checkButtons() {
  if (!digitalRead(BTN_P1)) {
    if (ballPos <= HIT_ZONE) {
      direction = 1;   // отбил
    } else {
      scoreP2++;
      resetRound(true);
    }
    delay(200);
  }

  if (!digitalRead(BTN_P2)) {
    if (ballPos >= NUM_LEDS - 1 - HIT_ZONE) {
      direction = -1;  // отбил
    } else {
      scoreP1++;
      resetRound(false);
    }
    delay(200);
  }
}

void loop() {
  if (gameOver) return;

  if (millis() - lastMove > SPEED_DELAY) {
    lastMove = millis();

    FastLED.clear();
    drawScores();

    ballPos += direction;

    if (ballPos <= 0) {
      scoreP2++;
      resetRound(true);
    }

    if (ballPos >= NUM_LEDS - 1) {
      scoreP1++;
      resetRound(false);
    }

    leds[ballPos] = CRGB::White;
    FastLED.show();
  }

  checkButtons();

  if (scoreP1 >= MAX_SCORE || scoreP2 >= MAX_SCORE) {
    gameOver = true;
    FastLED.clear();

    CRGB winColor = (scoreP1 > scoreP2) ? CRGB::Green : CRGB::Red;
    for (int i = 0; i < NUM_LEDS; i++) leds[i] = winColor;

    FastLED.show();
  }
}
