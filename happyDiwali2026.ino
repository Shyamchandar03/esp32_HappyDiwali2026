/*
  ESP32 Diwali Show: countdown -> rockets & cracker bursts -> "Happy Diwali!" with flickering diyas
  Works with: SSD1306 / SH1106 OLED (128x64) or ST7789 / ILI9341 colour TFT.

  Libraries (Arduino IDE > Library Manager):
    - Adafruit GFX Library
    - Adafruit SSD1306        (for 0.96" OLED)
    - Adafruit SH110X         (for 1.3" OLED)
    - Adafruit ST7735 and ST7789 Library  (for ST7789 TFT)
    - Adafruit ILI9341        (for ILI9341 TFT)
*/

// ================== 1. PICK YOUR DISPLAY (uncomment ONE) ==================
//#define DISPLAY_SSD1306    // 0.96" OLED 128x64, I2C
#define DISPLAY_SH1106       // 1.3" OLED 128x64, I2C  <-- your display (pins: VCC GND SCK SDA)
//#define DISPLAY_ST7789     // 1.3"/1.54"/1.69"/2.0" colour TFT, SPI
//#define DISPLAY_ILI9341    // 2.4"/2.8" colour TFT, SPI

// ================== 2. YOUR MESSAGE ==================
const char* MSG1 = "Happy";
const char* MSG2 = "Diwali!";
const char* MSG3 = "From Shyam";     // change to your name / family
const int   COUNTDOWN_FROM = 10;
const uint32_t MESSAGE_MS = 12000;   // how long the greeting stays before repeating

// ================== 3. PINS ==================
// OLED (I2C): SDA -> GPIO21, SCL -> GPIO22
// TFT  (SPI): SDA/MOSI -> GPIO23, SCL/SCK -> GPIO18, and:
#define TFT_CS   5     // set to -1 if your module has no CS pin
#define TFT_DC   2
#define TFT_RST  4
#define ST7789_W 240   // 240x240 (1.3"/1.54"), 240x280 (1.69"), 240x320 (2.0")
#define ST7789_H 240

// ==========================================================================
#include <Adafruit_GFX.h>

#if defined(DISPLAY_SSD1306)
  #include <Wire.h>
  #include <Adafruit_SSD1306.h>
  Adafruit_SSD1306 tft(128, 64, &Wire, -1);
  #define IS_MONO 1
#elif defined(DISPLAY_SH1106)
  #include <Wire.h>
  #include <Adafruit_SH110X.h>
  Adafruit_SH1106G tft(128, 64, &Wire, -1);
  #define IS_MONO 1
#elif defined(DISPLAY_ST7789)
  #include <SPI.h>
  #include <Adafruit_ST7789.h>
  Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);
  #define IS_MONO 0
#elif defined(DISPLAY_ILI9341)
  #include <SPI.h>
  #include <Adafruit_ILI9341.h>
  Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
  #define IS_MONO 0
#else
  #error "Uncomment one DISPLAY_ line at the top of the sketch"
#endif

// ---------- screen globals ----------
int W, H;          // screen size
float S;           // scale relative to a 128px-wide screen
int PSIZE;            // particle pixel size
float GRAV;        // gravity per frame
const uint16_t BLACK_C = 0;
uint16_t WHITE_C;
const int FRAME_MS = IS_MONO ? 5 : 16;

// festive palette: gold, orange, red-pink, purple, green, sky blue, white
const uint8_t PAL[][3] = {
  {255, 200, 40}, {255, 100, 20}, {255, 40, 90}, {200, 70, 255},
  {60, 255, 120}, {40, 190, 255}, {255, 255, 255}
};
const int NPAL = 7;

uint16_t rgb(int r, int g, int b) {
#if IS_MONO
  return (r + g + b) > 60 ? 1 : 0;
#else
  r = constrain(r, 0, 255); g = constrain(g, 0, 255); b = constrain(b, 0, 255);
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
#endif
}

void show() {
#if IS_MONO
  tft.display();      // OLEDs draw from a buffer; TFTs draw instantly
#endif
}

void centerText(const char* s, int y, int size, uint16_t col) {
  tft.setTextSize(size);
  tft.setTextColor(col);
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((W - (int)w) / 2, y);
  tft.print(s);
}

// ---------- particles (sparks) ----------
struct Particle {
  float x, y, vx, vy;
  uint8_t r, g, b;
  int16_t life, maxLife, px, py;
  bool active;
};
const int MAXP = 180;
Particle P[MAXP];

void spawn(float x, float y, float vx, float vy, uint8_t r, uint8_t g, uint8_t b, int life) {
  for (int i = 0; i < MAXP; i++) {
    if (!P[i].active) {
      Particle &p = P[i];
      p.x = x; p.y = y; p.vx = vx; p.vy = vy;
      p.r = r; p.g = g; p.b = b;
      p.life = life; p.maxLife = life;
      p.px = -100; p.py = -100;
      p.active = true;
      return;
    }
  }
}

void updateParticles() {
  for (int i = 0; i < MAXP; i++) {
    Particle &p = P[i];
    if (!p.active) continue;
    tft.fillRect(p.px, p.py, PSIZE, PSIZE, BLACK_C);          // erase old spot
    p.vx *= 0.97f;
    p.vy = p.vy * 0.97f + GRAV;
    p.x += p.vx;
    p.y += p.vy;
    if (--p.life <= 0 || p.x < 0 || p.x >= W || p.y >= H) { p.active = false; continue; }
    p.px = (int)p.x; p.py = (int)p.y;
    float f = (float)p.life / p.maxLife;
#if IS_MONO
    if (f < 0.4f && random(3) == 0) continue;            // twinkle while dying out
    tft.fillRect(p.px, p.py, PSIZE, PSIZE, 1);
#else
    float k = f < 0.5f ? f * 2.0f : 1.0f;                // full colour, then fade
    if (random(25) == 0) tft.fillRect(p.px, p.py, PSIZE, PSIZE, WHITE_C);   // crackle
    else tft.fillRect(p.px, p.py, PSIZE, PSIZE, rgb(p.r * k, p.g * k, p.b * k));
#endif
  }
}

// ---------- cracker burst ----------
// type 0 = ring, 1 = random peony, 2 = two-colour double ring
void burst(float x, float y, int ci, int type, float speedScale = 1.0f) {
  const uint8_t* c1 = PAL[ci];
  const uint8_t* c2 = PAL[(ci + 3) % NPAL];
  int n = IS_MONO ? 26 : 44;
  float base = 1.5f * S * speedScale;

  // bright flash at the burst point
  tft.fillCircle((int)x, (int)y, 3 * PSIZE, WHITE_C); show(); delay(20);
  tft.fillCircle((int)x, (int)y, 3 * PSIZE, BLACK_C);

  for (int i = 0; i < n; i++) {
    float a  = (type == 1) ? random(0, 628) / 100.0f : i * 6.2832f / n;
    float sp = (type == 1) ? base * random(30, 100) / 100.0f : base;
    const uint8_t* cc = c1;
    if (type == 2 && (i & 1)) { sp *= 0.55f; cc = c2; }
    spawn(x, y, cosf(a) * sp, sinf(a) * sp, cc[0], cc[1], cc[2], random(28, 46));
  }
}

// ---------- rockets ----------
struct Rocket {
  float x, y, vy, ty;
  int c, type;
  int16_t px, py;
  bool active;
};
Rocket R[6];

void launchRocket() {
  for (int i = 0; i < 6; i++) {
    Rocket &r = R[i];
    if (r.active) continue;
    r.x = random(W * 15 / 100, W * 85 / 100);
    r.y = H - 1;
    r.vy = -random(28, 38) / 10.0f * S;
    r.ty = random(H * 10 / 100, H * 40 / 100);
    r.c = random(NPAL);
    r.type = random(3);
    r.px = -100; r.py = -100;
    r.active = true;
    return;
  }
}

void updateRockets() {
  for (int i = 0; i < 6; i++) {
    Rocket &r = R[i];
    if (!r.active) continue;
    tft.fillRect(r.px, r.py, PSIZE + 1, PSIZE + 1, BLACK_C);
    r.y += r.vy;
    r.vy *= 0.985f;
    // golden spark trail
    spawn(r.x + random(-1, 2) * 0.5f, r.y + PSIZE + 1, random(-15, 16) / 100.0f * S, 0.3f * S,
          255, 160, 50, random(6, 12));
    if (r.y <= r.ty || r.vy > -0.7f * S) {
      r.active = false;
      burst(r.x, r.y, r.c, r.type);
      continue;
    }
    r.px = (int)r.x; r.py = (int)r.y;
    tft.fillRect(r.px, r.py, PSIZE + 1, PSIZE + 1, WHITE_C);
  }
}

bool busy() {
  for (int i = 0; i < 6; i++) if (R[i].active) return true;
  for (int i = 0; i < MAXP; i++) if (P[i].active) return true;
  return false;
}

void clearAll() {
  for (int i = 0; i < 6; i++) R[i].active = false;
  for (int i = 0; i < MAXP; i++) P[i].active = false;
  tft.fillScreen(BLACK_C);
}

void stepFrame() {
  updateParticles();
  updateRockets();
  show();
  delay(FRAME_MS);
}

// ---------- diya (oil lamp) with flickering flame ----------
void drawDiya(int cx, int baseY, float s, int dir) {
  int bw = (int)(10 * s), bh = (int)(5 * s), top = baseY - bh;
  int sx = cx + dir * bw;
  int fx = sx + dir * (int)(2 * s), fy = top - (int)(3 * s);
  int fw = (int)(2.5f * s) + 1;
  int fh = (int)((7 + random(0, 4)) * s);

  // clear the old flame
  tft.fillRect(fx - fw - 1, fy - (int)(11 * s) - 2, 2 * fw + 3, (int)(11 * s) + 2, BLACK_C);

  // clay bowl + spout
  uint16_t bowl = rgb(210, 95, 25);
  tft.fillTriangle(cx - bw, top, cx + bw, top, cx - bw / 2, baseY, bowl);
  tft.fillTriangle(cx + bw, top, cx - bw / 2, baseY, cx + bw / 2, baseY, bowl);
  tft.fillTriangle(sx, top, sx - dir * (int)(4 * s), top, sx + dir * (int)(3 * s), top - (int)(3 * s), bowl);

  // flame
  tft.fillCircle(fx, fy - fw, fw, rgb(255, 120, 20));
  tft.fillTriangle(fx - fw, fy - fw, fx + fw, fy - fw, fx + random(-1, 2) * (int)(s + 0.5f), fy - fh,
                   rgb(255, 220, 60));
#if !IS_MONO
  tft.fillCircle(fx, fy - fw, max(1, fw / 2), rgb(255, 255, 200));
#endif
}

// ---------- scenes ----------
void countdown(int from) {
  clearAll();
  int lblSz = max(1, (int)(S + 0.5f));
  int ly = H * 6 / 100;
  int lblBottom = ly + 8 * lblSz;
  int ns = max(3, min(W / 24, H / 16));            // big number size
  int cy = (H + lblBottom) / 2;
  int ny = cy - 4 * ns;

  for (int n = from; n >= 1; n--) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", n);
    tft.fillRect(0, ny - 2, W, 8 * ns + 4, BLACK_C);
    burst(W / 2, cy, random(NPAL), n % 3, 0.8f);   // sparks behind each number
    uint32_t t0 = millis();
    while (millis() - t0 < 1000) {
      updateParticles();
      centerText("", ly, lblSz, rgb(255, 200, 40));
      centerText(buf, ny, ns, n == 1 ? rgb(255, 60, 40) : WHITE_C);
      show();
      delay(FRAME_MS);
    }
  }
  clearAll();
}

void fireworksShow(int rockets) {
  int launched = 0;
  uint32_t next = 0;
  while (launched < rockets || busy()) {
    if (launched < rockets && millis() >= next) {
      launchRocket();
      launched++;
      next = millis() + random(300, 800);
    }
    stepFrame();
  }
}

void grandFinale() {
  for (int i = 0; i < 5; i++) launchRocket();     // everything at once!
  while (busy()) stepFrame();
}

void messageScene(uint32_t ms) {
  clearAll();
  int ts  = max(2, W / 48);
  int ts3 = max(1, ts / 2);
  int y1 = H * 18 / 100;
  int y2 = y1 + 8 * ts + 3;
  int y3 = y2 + 8 * ts + 4;
  float ds = S * 0.9f;

  uint32_t t0 = millis(), next = 0;
  while (millis() - t0 < ms) {
    if (millis() >= next) {
      burst(random(W / 8, W * 7 / 8), random(H / 12, H / 2), random(NPAL), random(3), 0.9f);
      next = millis() + random(500, 1200);
    }
    updateParticles();
    bool alt = (millis() / 350) % 2;
    centerText(MSG1, y1, ts, rgb(255, 200, 40));
    centerText(MSG2, y2, ts, alt ? rgb(255, 120, 20) : rgb(255, 60, 90));
    centerText(MSG3, y3, ts3, WHITE_C);
    drawDiya(W * 10 / 100, H - 1, ds, 1);
    drawDiya(W * 90 / 100, H - 1, ds, -1);
    show();
    delay(FRAME_MS + 15);
  }
}

// ---------- Arduino ----------
void setup() {
  randomSeed(esp_random());

#if defined(DISPLAY_SSD1306)
  Wire.begin();
  if (!tft.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {   // blank screen? try 0x3D
    for (;;) delay(100);
  }
#elif defined(DISPLAY_SH1106)
  Wire.begin();
  tft.begin(0x3C, true);
#elif defined(DISPLAY_ST7789)
  tft.init(ST7789_W, ST7789_H, TFT_CS < 0 ? SPI_MODE3 : SPI_MODE0);
  tft.setRotation(0);
#elif defined(DISPLAY_ILI9341)
  tft.begin();
  tft.setRotation(1);
#endif

  WHITE_C = IS_MONO ? 1 : 0xFFFF;
  W = tft.width();
  H = tft.height();
  S = W / 128.0f;
  PSIZE = (W >= 200) ? 2 : 1;
  GRAV = 0.045f * S;
  tft.setTextWrap(false);
  tft.fillScreen(BLACK_C);
  show();
}

void loop() {
  countdown(COUNTDOWN_FROM);
  fireworksShow(7);
  grandFinale();
  messageScene(MESSAGE_MS);
}
