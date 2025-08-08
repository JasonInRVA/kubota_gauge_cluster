#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // TFT constructor

void printPinConfig() {
  Serial.println("🧩 TFT_eSPI Pin Configuration:");
  Serial.printf("  TFT_MOSI: %d\n", TFT_MOSI);
  Serial.printf("  TFT_SCLK: %d\n", TFT_SCLK);
  Serial.printf("  TFT_CS:   %d\n", TFT_CS);
  Serial.printf("  TFT_DC:   %d\n", TFT_DC);
  Serial.printf("  TFT_RST:  %d\n", TFT_RST);
  Serial.printf("  TFT_MISO: %d\n", TFT_MISO);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  printPinConfig();

  Serial.println("About to call tft.init()...");
  tft.init();
  Serial.println("TFT_eSPI Initialized");

  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 100);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.println("Hello, Kubota!");
}

void loop() {}