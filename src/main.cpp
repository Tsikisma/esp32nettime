#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>

#define CLOCK_PIN         18
#define DATA_PIN          23
#define CHIP_SELECT_PIN   17
#define RESET_PIN         4
#define DATA_COMMAND_PIN  16

// Color definitions
#define YELLOW           0xFFE0
#define BITMAP_WIDTH     32
#define BITMAP_HEIGHT    32

const unsigned char epd_bitmap_Bitmap [] PROGMEM = {
	// 'waning_crescent2, 32x32px
	0x00, 0xf8, 0x1f, 0x00, 0x00, 0xfe, 0x7f, 0x00, 0x80, 0xff, 0xff, 0x01, 0xc0, 0xff, 0x3f, 0x03, 
	0xe0, 0xff, 0x1f, 0x06, 0xf0, 0xff, 0x0f, 0x0c, 0xf8, 0xff, 0x07, 0x18, 0xfc, 0xff, 0x07, 0x30, 
	0xfc, 0xff, 0x03, 0x20, 0xfe, 0xff, 0x01, 0x60, 0xfe, 0xff, 0x01, 0x40, 0xff, 0xff, 0x01, 0xc0, 
	0xff, 0xff, 0x00, 0xc0, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 
	0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0xc0, 
	0xff, 0xff, 0x01, 0xc0, 0xfe, 0xff, 0x01, 0x40, 0xfe, 0xff, 0x01, 0x60, 0xfc, 0xff, 0x03, 0x20, 
	0xfc, 0xff, 0x03, 0x30, 0xf8, 0xff, 0x07, 0x18, 0xf0, 0xff, 0x0f, 0x08, 0xe0, 0xff, 0x1f, 0x06, 
	0xc0, 0xff, 0x3f, 0x03, 0x80, 0xff, 0xff, 0x01, 0x00, 0xfe, 0x7f, 0x00, 0x00, 0xf8, 0x1f, 0x00
};


Adafruit_SSD1331 display = Adafruit_SSD1331(CHIP_SELECT_PIN, DATA_COMMAND_PIN, DATA_PIN, CLOCK_PIN, RESET_PIN);

void initDisplay() {
  //Initialize display settings.
  display.begin();
  display.fillScreen(0);
  display.setTextSize(1);
  display.setTextWrap(false);
}

void displayBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
  int16_t byteWidth = (w + 7) / 8; // Width of bitmap in bytes before padding
  uint8_t byte = 0;

  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++ ) {
      if (i & 7) {
        byte >>= 1;
      }
      else {
        byte = pgm_read_byte(bitmap + j * byteWidth + i / 8);
      }
      // Draw the pixel when we get to the end of a byte
      if (byte & 0x01) {
        display.drawPixel(x+i, y+j, color);
      }
    }
  }
}


void setup() {
  initDisplay();

  int16_t xPos = (display.width() - BITMAP_WIDTH) / 2;
  int16_t yPos = (display.height() - BITMAP_HEIGHT) / 2;

  displayBitmap(xPos, yPos, epd_bitmap_Bitmap, BITMAP_WIDTH, BITMAP_HEIGHT, YELLOW);

}

void loop() {
  // Nothing to do in the loop
}
