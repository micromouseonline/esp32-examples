#pragma once

#include <Arduino.h>

#include "display.h"

inline void basic_setup(LGFX& lcd, LGFX_Sprite& sprite) {
  // Set screen rotation to one of 4 directions: 0 to 3. (Using values 4 to 7 flips the image upside down.)
  lcd.setRotation(3);

  // Set the backlight brightness level within a range of 0 to 255.
  // lcd.setBrightness(128);

  // Configure the color mode as required. (The default color depth is 16)
  // 16-bit depth uses less SPI bandwidth and runs faster, but splits Red/Blue channels into 5-bit color gradations.
  // 24-bit depth uses more SPI bandwidth, but yields much smoother color gradations.
  // lcd.setColorDepth(16);  // Set to RGB565 16-bit color depth
  // Set to RGB888 24-bit color depth (Actual viewable colors may drop to 18-bit RGB666 depending on your panel's physical specifications)
  lcd.setColorDepth(24);

  // The basic shape drawing functions are listed below:
  /*
    fillScreen    (                color);  // Fills the entire screen
    drawPixel     ( x, y         , color);  // Single pixel/point
    drawFastVLine ( x, y   , h   , color);  // Vertical line
    drawFastHLine ( x, y, w      , color);  // Horizontal line
    drawRect      ( x, y, w, h   , color);  // Outline of a rectangle
    fillRect      ( x, y, w, h   , color);  // Filled rectangle
    drawRoundRect ( x, y, w, h, r, color);  // Outline of a rounded rectangle
    fillRoundRect ( x, y, w, h, r, color);  // Filled rounded rectangle
    drawCircle    ( x, y      , r, color);  // Outline of a circle
    fillCircle    ( x, y      , r, color);  // Filled circle
    drawEllipse   ( x, y, rx, ry , color);  // Outline of an ellipse
    fillEllipse   ( x, y, rx, ry , color);  // Filled ellipse
    drawLine      ( x0, y0, x1, y1        , color); // Line between two coordinate points
    drawTriangle  ( x0, y0, x1, y1, x2, y2, color); // Outline of a triangle across three coordinates
    fillTriangle  ( x0, y0, x1, y1, x2, y2, color); // Filled triangle across three coordinates
    drawBezier    ( x0, y0, x1, y1, x2, y2, color); // Quadratic Bezier curve across three control points
    drawBezier    ( x0, y0, x1, y1, x2, y2, x3, y3, color); // Cubic Bezier curve across four control points
    drawArc       ( x, y, r0, r1, angle0, angle1, color);   // Outline of a circular arc
    fillArc       ( x, y, r0, r1, angle0, angle1, color);   // Filled circular arc
  */

  lcd.setBaseColor(0x000000u);  // Sets background tracking to Black
  lcd.clear();                  // Clears the screen to Black

  // Built-in converter functions are available to generate color codes for your drawing commands.
  // Parameters expect values for Red, Green, and Blue from 0 to 255.
  // Using color888 is highly recommended to avoid losing color fidelity during processing.
  lcd.drawFastHLine(0, 10, 100, lcd.color888(255, 0, 0));  // Draws a vertical line in Red
  lcd.drawFastHLine(0, 20, 100, lcd.color565(0, 255, 0));  // Draws a vertical line in Green
  lcd.drawFastHLine(0, 30, 100, lcd.color332(0, 0, 255));  // Draws a vertical line in Blue

  // The allocation and release of the SPI bus hardware happen automatically behind individual shape functions.
  // If optimizing heavily for execution speed, use startWrite and endWrite around groups of rendering instructions.
  // This wraps multiple draw calls into a single transaction, bypassing bus overhead for a massive speed boost.
  // Note: On Electronic Paper Displays (EPD), pixels drawn after calling startWrite() will remain cached until endWrite() forces a panel refresh.

  lcd.drawLine(0, 1, 39, 40, TFT_AZURE);   // Allocates SPI bus, draws line, releases SPI bus
  lcd.drawLine(1, 0, 40, 39, TFT_MAROON);  // Allocates SPI bus, draws line, releases SPI bus

  lcd.startWrite();                        // Allocates and holds the SPI bus transaction open
  lcd.drawLine(38, 0, 0, 60, 0xFFFF00U);   // Draws line
  lcd.drawLine(39, 5, 1, 70, 0xFF00FFU);   // Draws line
  lcd.drawLine(40, 10, 2, 80, 0x00FFFFU);  // Draws line
  lcd.endWrite();                          // Finalizes transaction and releases the SPI bus

  // A specialized writePixel variant exists alongside standard drawPixel logic.
  // While drawPixel checks and locks transaction scopes automatically as needed,
  // writePixel skips internal safety checks and transmits data directly to the SPI registers.
  lcd.startWrite();  // Manually lock and claim the SPI bus first
  for (uint32_t x = 0; x < 128; ++x) {
    for (uint32_t y = 0; y < 128; ++y) {
      lcd.writePixel(x, y, lcd.color888(x * 2, x + y, y * 2));
    }
  }
  lcd.endWrite();  // Release the SPI bus safely when finished

  delay(1000);

  // Rendering onto RAM-backed Sprites (Offscreen canvas buffers) uses identical syntax structures.
  // Define the desired color depth of your offscreen memory canvas using setColorDepth. (Defaults to 16-bit if omitted.)
  // sprite.setColorDepth(1);   // 1-bit per pixel (2-color) palette mode configuration
  // sprite.setColorDepth(2);   // 2-bits per pixel (4-color) palette mode configuration
  // sprite.setColorDepth(4);   // 4-bits per pixel (16-color) palette mode configuration
  // sprite.setColorDepth(8);   // 8-bit per pixel RGB332 configuration
  // sprite.setColorDepth(16);  // 16-bit per pixel RGB565 configuration
  sprite.setColorDepth(24);  // 24-bit per pixel RGB888 configuration

  sprite.createSprite(65, 65);  // Allocates a canvas buffer 65px wide by 65px high.

  for (uint32_t x = 0; x < 64; ++x) {
    for (uint32_t y = 0; y < 64; ++y) {
      sprite.drawPixel(x, y, lcd.color888(3 + x * 4, (x + y) * 2, 3 + y * 4));  // Render lines directly inside offscreen memory
    }
  }
  sprite.drawRect(0, 0, 65, 65, TFT_BLACK);

  // Output compiled offscreen layouts onto physical hardware configurations via pushSprite.
  // The data transfers directly to the target LGFX instance passed to the Sprite object during initialization.
  sprite.pushSprite(64, 0);  // Blits the sprite buffer onto the main LCD at position 64:0

  // return;
  // If no pointer was bound during constructor declarations or you need to direct data onto alternate displays,
  // pass the target destination reference as the leading parameter inside pushSprite.
  sprite.pushSprite(&lcd, 0, 64);  // Blits the sprite buffer onto the specific LCD at position 0:64
  sprite.setPivot(0, 0);
}

inline void basic_loop(LGFX& lcd, LGFX_Sprite& sprite) {
  static int count = 0;
  static int a = 0;
  static int x = lcd.width() / 2;
  static int y = lcd.height() / 2;
  static float zoom = 1;
  count;
  if ((a += 1) >= 360) {
    a -= 360;
  }
  sprite.pushRotateZoom(x, y, a, zoom, zoom, 0);
}