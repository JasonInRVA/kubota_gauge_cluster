#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// ---------- Tunables ----------
static const uint8_t  TEXT_SIZE      = 2;      // 1=6x8, 2=12x16, etc.
static const uint8_t  MIN_TAIL       = 6;      // min trail length (rows)
static const uint8_t  MAX_TAIL       = 18;     // max trail length (rows)
static const uint8_t  MIN_SPEED      = 1;      // min rows per tick
static const uint8_t  MAX_SPEED      = 3;      // max rows per tick
static const uint16_t TICK_MS        = 30;     // frame delay
static const bool     USE_KATAKANA   = true;   // otherwise ASCII gibberish
// ------------------------------

uint16_t W, H, charW, charH, COLS, ROWS;

struct Drop {
  int16_t row;      // current head row (can move past ROWS)
  uint8_t speed;    // rows per frame
  uint8_t tail;     // trail length in rows
};

Drop* drops = nullptr;

// A few shades of green for the trail (dark -> bright)
uint16_t palette[6]; // will be filled in setup()

// Simple mix of ASCII + Katakana-esque glyphs
char randomGlyph() {
  if (USE_KATAKANA) {
    // Half-width Katakana range often renders okay in built-ins on many TFT fonts
    // Fallback: sprinkle ASCII if code page doesn’t render
    uint8_t r = random(0, 10);
    if (r < 7) return (char)random(0xB0, 0xDF); // Katakana-ish
  }
  // ASCII fallback (digits, uppercase, symbols)
  const char ascii[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "!@#$%^&*()[]{}<>+-=*/\\|";
  return ascii[random(0, sizeof(ascii) - 1)];
}

void printPinConfig() {
  Serial.println("🧩 TFT_eSPI Pin Configuration:");
#ifdef TFT_MOSI
  Serial.printf("  TFT_MOSI: %d\n", TFT_MOSI);
#endif
#ifdef TFT_SCLK
  Serial.printf("  TFT_SCLK: %d\n", TFT_SCLK);
#endif
#ifdef TFT_CS
  Serial.printf("  TFT_CS:   %d\n", TFT_CS);
#endif
#ifdef TFT_DC
  Serial.printf("  TFT_DC:   %d\n", TFT_DC);
#endif
#ifdef TFT_RST
  Serial.printf("  TFT_RST:  %d\n", TFT_RST);
#endif
#ifdef TFT_MISO
  Serial.printf("  TFT_MISO: %d\n", TFT_MISO);
#endif
}

// Draw one glyph at (col,row) grid coords with a chosen color
inline void drawGlyph(uint16_t col, int16_t row, uint16_t color, bool invertBg = false) {
  if (row < 0 || row >= (int16_t)ROWS) return;
  int16_t x = col * charW;
  int16_t y = row * charH;
  if (invertBg) {
    // brighter head with black background wipe for crispness
    tft.fillRect(x, y, charW, charH, TFT_BLACK);
    tft.setTextColor(color, TFT_BLACK);
  } else {
    tft.setTextColor(color, TFT_BLACK);
  }
  tft.setCursor(x, y);
  tft.write(randomGlyph());
}

// Compute a trail shade index (0..palette_len-1) given offset from head
inline uint16_t trailColor(uint8_t offset, uint8_t tail) {
  // Map offset (0=head) -> palette index (bright to dark)
  // Ensure head is brightest, tail fades out
  uint8_t idx = (uint8_t)map(offset, 0, tail, (int) (sizeof(palette)/sizeof(palette[0]) - 1), 0);
  if (idx >= sizeof(palette)/sizeof(palette[0])) idx = sizeof(palette)/sizeof(palette[0]) - 1;
  return palette[idx];
}

void setup() {
  Serial.begin(115200);
  delay(300);
  printPinConfig();

  // Seed RNG
  esp_fill_random(&W, sizeof(W)); // just grabbing some entropy
  randomSeed(micros() ^ W);

  tft.init();
  tft.setRotation(1); // landscape for 480x320 ST7796S; change if you like
  tft.fillScreen(TFT_BLACK);

  // Character cell size from classic 6x8 font scaled by TEXT_SIZE
  charW = 6 * TEXT_SIZE;
  charH = 8 * TEXT_SIZE;

  W = tft.width();
  H = tft.height();
  COLS = W / charW;
  ROWS = H / charH;

  // Build a green palette (dark -> bright)
  // You can tweak these for your panel’s gamma; values are 565 RGB
  palette[0] = tft.color565(0,   20, 0);
  palette[1] = tft.color565(0,   40, 0);
  palette[2] = tft.color565(0,   80, 0);
  palette[3] = tft.color565(0,  140, 0);
  palette[4] = tft.color565(40, 220, 40);
  palette[5] = tft.color565(180, 255, 180); // head highlight

  // Allocate and randomize drops
  drops = (Drop*)malloc(sizeof(Drop) * COLS);
  for (uint16_t c = 0; c < COLS; c++) {
    drops[c].row   = random(-ROWS, ROWS); // start off-screen for staggered entries
    drops[c].speed = random(MIN_SPEED, MAX_SPEED + 1);
    drops[c].tail  = random(MIN_TAIL, MAX_TAIL + 1);
  }

  // Title blip (because flair)
  tft.setTextSize(TEXT_SIZE);
  tft.setTextColor(palette[5], TFT_BLACK);
  tft.setCursor(8, 8);
  tft.print("Hello, Kubota… loading rain");
  delay(700);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < TICK_MS) return;
  last = now;

  // For each column, advance the head and draw the trail
  for (uint16_t c = 0; c < COLS; c++) {
    Drop& d = drops[c];
    // Head moves down by d.speed rows
    int16_t prevHead = d.row;
    d.row += d.speed;

    // Erase behind tail end (clean black cell after the trail)
    int16_t eraseRow = d.row - (int16_t)d.tail - 1;
    if (eraseRow >= 0 && eraseRow < (int16_t)ROWS) {
      tft.fillRect(c * charW, eraseRow * charH, charW, charH, TFT_BLACK);
    }

    // Draw trail from tail to head
    // Draw darker first, then brighter, so head ends on top
    for (int16_t offset = d.tail; offset >= 0; offset--) {
      int16_t r = d.row - offset;
      if (r < 0 || r >= (int16_t)ROWS) continue;

      uint16_t col = (offset == 0) ? palette[5] : trailColor(offset, d.tail);
      bool head = (offset == 0);

      // Occasionally “refresh” characters in the trail so it shimmers
      if (head || (random(0, 100) < 25)) {
        drawGlyph(c, r, col, head);
      } else {
        // slight dimming by overdraw (optional)
        tft.drawFastHLine(c * charW, r * charH + charH - 1, charW, TFT_BLACK);
      }
    }

    // When past bottom, respawn with new params for variety
    if (d.row - d.tail > (int16_t)ROWS + 2) {
      d.row   = random(-ROWS / 2, 0);
      d.speed = random(MIN_SPEED, MAX_SPEED + 1);
      d.tail  = random(MIN_TAIL, MAX_TAIL + 1);
    }
  }
}
