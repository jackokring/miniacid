#pragma once
#include <cstdint>

#if defined(ARDUINO)
  #if __has_include(<M5GFX.h>)
    #include <M5GFX.h>
  #elif __has_include(<Adafruit_GFX.h>)
    #include <Adafruit_GFX.h>
  #endif
  #include <pgmspace.h>
#else
  #ifndef PROGMEM
    #define PROGMEM
  #endif
  #ifndef pgm_read_byte
    #define pgm_read_byte(addr) (*(reinterpret_cast<const uint8_t*>(addr)))
  #endif
  #ifndef pgm_read_word
    #define pgm_read_word(addr) (*(reinterpret_cast<const uint16_t*>(addr)))
  #endif
#endif

// Only define the font structs if they aren't provided by platform headers.
#if !defined(LGFX_FONTS_HPP_) && !defined(_GFXFONT_H_)

// Minimal Adafruit GFX font structures for local rendering.
// https://github.com/adafruit/Adafruit-GFX-Library/blob/master/gfxfont.h
typedef struct {
  uint16_t bitmapOffset;
  uint8_t width;
  uint8_t height;
  uint8_t xAdvance;
  int8_t xOffset;
  int8_t yOffset;
} GFXglyph;

typedef struct {
  const uint8_t* bitmap;
  const GFXglyph* glyph;
  uint16_t first;
  uint16_t last;
  uint8_t yAdvance;
} GFXfont;

#endif // !LGFX_FONTS_HPP_ && !_GFXFONT_H_
