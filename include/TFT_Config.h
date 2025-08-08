#define ST7796_DRIVER

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// Your confirmed working pin mapping
#define TFT_MOSI 11        // SDI(MOSI)
#define TFT_SCLK 12        // SCK
#define TFT_CS   10        // LCD_CS
#define TFT_DC   9         // LCD_RS
#define TFT_RST  8         // LCD_RST

#define TFT_SPI_HOST FSPI_HOST  // Critical for ESP32-S3 + PSRAM

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY  40000000