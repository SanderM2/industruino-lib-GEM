/*
  GEM (a.k.a. Good Enough Menu) - Arduino library for creation of graphic multi-level menu with
  editable menu items, such as variables (supports int, byte, float, double, bool, char[17] data types)
  and option selects. User-defined callback function can be specified to invoke when menu item is saved.
  
  Supports buttons that can invoke user-defined actions and create action-specific
  context, which can have its own enter (setup) and exit callbacks as well as loop function.

  Supports:
  - AltSerialGraphicLCD library by Jon Green (http://www.jasspa.com/serialGLCD.html);
  - U8g2 library by olikraus (https://github.com/olikraus/U8g2_Arduino);
  - Adafruit GFX library by Adafruit (https://github.com/adafruit/Adafruit-GFX-Library).

  For documentation visit:
  https://github.com/Spirik/GEM

  Copyright (c) 2018-2026 Alexander 'Spirik' Spiridonov

  This file is part of GEM library.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public License
  along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include "GEM_u8g2.h"

#ifdef GEM_ENABLE_U8G2_VERSION

// AVR-based Arduinos have suppoort for dtostrf, some others may require manual inclusion (e.g. SAMD),
// see https://github.com/plotly/arduino-api/issues/38#issuecomment-108987647
#if defined(GEM_SUPPORT_FLOAT_EDIT) && (defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_NRF52840))
#include <avr/dtostrf.h>
#endif

// Macro constants (aliases) for some of the ASCII character codes
#define GEM_CHAR_CODE_9 57
#define GEM_CHAR_CODE_0 48
#define GEM_CHAR_CODE_MINUS 45
#define GEM_CHAR_CODE_DOT 46
#define GEM_CHAR_CODE_SPACE 32
#define GEM_CHAR_CODE_UNDERSCORE 95
#define GEM_CHAR_CODE_TILDA 126
#define GEM_CHAR_CODE_BANG 33
#define GEM_CHAR_CODE_a 97
#define GEM_CHAR_CODE_ACCENT 96
/*
// WIP for Cyrillic values support
#define GEM_CHAR_CODE_CYR_YO 1025
#define GEM_CHAR_CODE_CYR_A 1040
#define GEM_CHAR_CODE_CYR_E 1045
#define GEM_CHAR_CODE_CYR_E_SM 1077
#define GEM_CHAR_CODE_CYR_YA_SM 1103
#define GEM_CHAR_CODE_CYR_YO_SM 1105
*/

// XBM image of the default GEM _splash screen (GEM logo v1)
/*
#define logo_width  12
#define logo_height 12
static const unsigned char logo_bits [] U8X8_PROGMEM = {
  0xc0,0xf7,0x60,0xfe,0x30,0xfe,0x18,0xff,0x8c,0xff,0xc6,0xff,
  0xe3,0xff,0xf1,0xff,0x19,0xff,0x7f,0xfc,0xff,0xfd,0xfe,0xf7
};
*/

// XBM image of the default GEM _splash screen (GEM logo v2)
#define logo_width  20
#define logo_height 8
static const unsigned char logo_bits [] U8X8_PROGMEM = {
  0x8f,0x4f,0xf4,0x00,0x40,0xf4,0x00,0x40,0xf5,0x98,0x47,0xf5,
  0x00,0x40,0xf4,0x00,0x40,0xf4,0x9f,0x4f,0xf4,0x00,0x00,0xf0
};

const GEMSprite logo = {logo_width, logo_height, logo_bits};

// Sprites of the UI elements used to draw menu

#define sprite_height  8

#define arrowRight_width  6
#define arrowRight_height 8
static const unsigned char arrowRight_bits [] U8X8_PROGMEM = {
   0xc0,0xc4,0xcc,0xdc,0xcc,0xc4,0xc0,0xc0
};

#define arrowLeft_width  6
#define arrowLeft_height 8
static const unsigned char arrowLeft_bits [] U8X8_PROGMEM = {
   0xc0,0xc4,0xc6,0xc7,0xc6,0xc4,0xc0,0xc0
};

#define arrowBtn_width  6
#define arrowBtn_height 8
static const unsigned char arrowBtn_bits [] U8X8_PROGMEM = {
  0xc0,0xc3,0xc5,0xc9,0xc5,0xc3,0xc0,0xc0
};

#define checkboxUnchecked_width  7
#define checkboxUnchecked_height 8
static const unsigned char checkboxUnchecked_bits [] U8X8_PROGMEM = {
   0x80,0xbf,0xa1,0xa1,0xa1,0xa1,0xbf,0x80
};

#define checkboxChecked_width  7
#define checkboxChecked_height 8
static const unsigned char checkboxChecked_bits [] U8X8_PROGMEM = {
   0xc0,0xaf,0xb1,0xab,0xa5,0xa1,0xbf,0x80
};

#define selectArrows_width  6
#define selectArrows_height 8
static const unsigned char selectArrows_bits [] U8X8_PROGMEM = {
   0xc0,0xc4,0xce,0xc0,0xce,0xc4,0xc0,0xc0
};

const GEMSprite arrowRight = {arrowRight_width, arrowRight_height, arrowRight_bits};

const GEMSprite arrowLeft = {arrowLeft_width, arrowLeft_height, arrowLeft_bits};

const GEMSprite arrowBtn = {arrowBtn_width, arrowBtn_height, arrowBtn_bits};

const GEMSprite checkboxUnchecked = {checkboxUnchecked_width, checkboxUnchecked_height, checkboxUnchecked_bits};

const GEMSprite checkboxChecked = {checkboxChecked_width, checkboxChecked_height, checkboxChecked_bits};

const GEMSprite selectArrows = {selectArrows_width, selectArrows_height, selectArrows_bits};

// Custom 3-button edit sprites (6x8 XBM, LSB = leftmost pixel)
// editConfirm: checkmark ✓  (UP past end of range → confirm/save)
//   .....X   row1: col5  (long arm tip, upper-right)
//   ....X.   row2: col4
//   .X.X..   row3: col1 (short arm) + col3 (long arm)  ← short arm 2px higher
//   .XX...   row4: col1+col2 (knee, where both arms meet)
//   ......   row5-6: blank
#define editConfirm_width  6
#define editConfirm_height 8
static const unsigned char editConfirm_bits [] U8X8_PROGMEM = {
  0xC0, 0xE0, 0xD2, 0xCA, 0xC6, 0xC0, 0xC0, 0xC0
};

// editDelete: X cross ✗  (DOWN past start of range → backspace/delete)
//   X....X   0xE1
//   .X..X.   0xD2
//   ..XX..   0xCC
//   ..XX..   0xCC
//   .X..X.   0xD2
//   X....X   0xE1
#define editDelete_width  6
#define editDelete_height 8
static const unsigned char editDelete_bits [] U8X8_PROGMEM = {
  0xC0, 0xE1, 0xD2, 0xCC, 0xCC, 0xD2, 0xE1, 0xC0
};

const GEMSprite editConfirmSprite = {editConfirm_width, editConfirm_height, editConfirm_bits};
const GEMSprite editDeleteSprite  = {editDelete_width,  editDelete_height,  editDelete_bits};

// Build zero-padded IP string "XXX.XXX.XXX.XXX\0" (16 bytes) from uint8_t[4]
static void buildIpString(char* buf, const uint8_t* ip) {
  for (int i = 0; i < 4; i++) {
    uint8_t v = ip[i];
    buf[i*4 + 0] = '0' + v / 100;
    buf[i*4 + 1] = '0' + (v / 10) % 10;
    buf[i*4 + 2] = '0' + v % 10;
    if (i < 3) buf[i*4 + 3] = '.';
  }
  buf[15] = '\0';
}

GEM_u8g2::GEM_u8g2(U8G2& u8g2_, byte menuPointerType_, byte menuItemsPerScreen_, byte menuItemHeight_, byte menuPageScreenTopOffset_, byte menuValuesLeftOffset_)
  : _u8g2(u8g2_)
{
  _appearance.menuPointerType = menuPointerType_;
  _appearance.menuItemsPerScreen = menuItemsPerScreen_;
  _appearance.menuItemHeight = menuItemHeight_;
  _appearance.menuPageScreenTopOffset = menuPageScreenTopOffset_;
  _appearance.menuValuesLeftOffset = menuValuesLeftOffset_;
  _appearanceCurrent = &_appearance;
  _splash = logo;
  clearContext();
  _editValueMode = false;
  _editSpecialAction = 0;
  _editSpecialSavedChar = '\0';
  _editValueCursorPosition = 0;
  memset(_valueString, '\0', GEM_STR_LEN - 1);
  _valueSelectNum = -1;
}

GEM_u8g2::GEM_u8g2(U8G2& u8g2_, GEMAppearance appearance_)
  : _u8g2(u8g2_)
  , _appearance(appearance_)
{
  _appearanceCurrent = &_appearance;
  _splash = logo;
  clearContext();
  _editValueMode = false;
  _editSpecialAction = 0;
  _editSpecialSavedChar = '\0';
  _editValueCursorPosition = 0;
  memset(_valueString, '\0', GEM_STR_LEN - 1);
  _valueSelectNum = -1;
}

//====================== APPEARANCE OPERATIONS

GEM_u8g2& GEM_u8g2::setAppearance(GEMAppearance appearance) {
  _appearance = appearance;
  return *this;
}

GEMAppearance* GEM_u8g2::getCurrentAppearance() {
  return (_menuPageCurrent != nullptr && _menuPageCurrent->_appearance != nullptr) ? _menuPageCurrent->_appearance : &_appearance;
}

byte GEM_u8g2::getMenuItemsPerScreen() {
  return getCurrentAppearance()->menuItemsPerScreen == GEM_ITEMS_COUNT_AUTO ? (_u8g2.getDisplayHeight() - getCurrentAppearance()->menuPageScreenTopOffset) / getCurrentAppearance()->menuItemHeight : getCurrentAppearance()->menuItemsPerScreen;
}

byte GEM_u8g2::getMenuItemFontSize() {
  return getCurrentAppearance()->menuItemHeight >= _menuItemFont[0].height ? 0 : 1;
}

byte GEM_u8g2::getMenuItemTitleLength() {
  return (getCurrentAppearance()->menuValuesLeftOffset - 5) / _menuItemFont[getMenuItemFontSize()].width;
}

byte GEM_u8g2::getMenuItemValueLength() {
  return (_u8g2.getDisplayWidth() - getCurrentAppearance()->menuValuesLeftOffset - 6) / _menuItemFont[getMenuItemFontSize()].width;
}

//====================== INIT OPERATIONS

GEM_u8g2& GEM_u8g2::setSplash(byte width, byte height, const unsigned char *image) {
  _splash = {width, height, image};
  return *this;
}

GEM_u8g2&  GEM_u8g2::setSplashDelay(uint16_t value) {
  _splashDelay = value;
  return *this;
}

GEM_u8g2& GEM_u8g2::hideVersion(bool flag) {
  _enableVersion = !flag;
  return *this;
}

GEM_u8g2& GEM_u8g2::enableUTF8(bool flag) {
  _UTF8Enabled = flag;
  if (_UTF8Enabled) {
    _u8g2.enableUTF8Print();
  } else {
    _u8g2.disableUTF8Print();
  }
  return *this;
}

GEM_u8g2& GEM_u8g2::enableCyrillic(bool flag) {
  enableUTF8(flag);
  if (_UTF8Enabled) {
    _fontFamilies = {(uint8_t *)GEM_FONT_BIG_CYR, (uint8_t *)GEM_FONT_SMALL_CYR};
  } else {
    _fontFamilies = {(uint8_t *)GEM_FONT_BIG, (uint8_t *)GEM_FONT_SMALL};
  }
  _menuItemFont[0] = {6, 8};
  _menuItemFont[1] = {4, 6};
  return *this;
}

GEM_u8g2& GEM_u8g2::setFontBig(const uint8_t* font, uint8_t width, uint8_t height) {
  _fontFamilies.big = font;
  _menuItemFont[0] = {width, height};
  return *this;
}

GEM_u8g2& GEM_u8g2::setFontBig() {
  _fontFamilies.big = _UTF8Enabled ? GEM_FONT_BIG_CYR : GEM_FONT_BIG;
  _menuItemFont[0] = {6, 8};
  return *this;
}

GEM_u8g2& GEM_u8g2::setFontSmall(const uint8_t* font, uint8_t width, uint8_t height) {
  _fontFamilies.small = font;
  _menuItemFont[1] = {width, height};
  return *this;
}

GEM_u8g2& GEM_u8g2::setFontSmall() {
  _fontFamilies.small = _UTF8Enabled ? GEM_FONT_SMALL_CYR : GEM_FONT_SMALL;
  _menuItemFont[1] = {4, 6};
  return *this;
}

GEM_u8g2& GEM_u8g2::invertKeysDuringEdit(bool invert) {
  _invertKeysDuringEdit = invert;
  return *this;
}

GEM_u8g2& GEM_u8g2::setTwoLineItems(bool mode) {
  _twoLineItems = mode;
  return *this;
}

byte GEM_u8g2::getEffectiveItemHeight() {
  return getCurrentAppearance()->menuItemHeight;  // kept for compat; use getItemHeight(item) for per-item logic
}

bool GEM_u8g2::itemNeedsTwoLine(GEMItem* item) {
  if (!_twoLineItems) return false;
  if (item->type != GEM_ITEM_VAL) return false;
  return item->linkedType == GEM_VAL_CHAR || item->linkedType == GEM_VAL_IP;
}

byte GEM_u8g2::getItemHeight(GEMItem* item) {
  return getCurrentAppearance()->menuItemHeight * (itemNeedsTwoLine(item) ? 2 : 1);
}

// First item index visible on the screen that contains currentItemNum.
byte GEM_u8g2::getFirstVisibleItemIndex() {
  byte screenAvail = _u8g2.getDisplayHeight() - getCurrentAppearance()->menuPageScreenTopOffset;
  int  current     = _menuPageCurrent->currentItemNum;
  byte firstOnScreen = 0;
  byte heightAccum   = 0;
  GEMItem* item = _menuPageCurrent->getMenuItem(0);
  for (int i = 0; i <= current && item != nullptr; i++, item = item->getMenuItemNext()) {
    byte h = getItemHeight(item);
    if (i > firstOnScreen && heightAccum + h > screenAvail) {
      firstOnScreen = i;
      heightAccum   = h;
    } else {
      heightAccum += h;
    }
  }
  return firstOnScreen;
}

GEM_u8g2& GEM_u8g2::init() {
  _u8g2.clear();
  _u8g2.setDrawColor(1);
  _u8g2.setFontPosTop();

  if (_splashDelay > 0) {

    _u8g2.firstPage();
    do {
      _u8g2.drawXBMP((_u8g2.getDisplayWidth() - _splash.width) / 2, (_u8g2.getDisplayHeight() - _splash.height) / 2, _splash.width, _splash.height, _splash.image);
    } while (_u8g2.nextPage());

    if (_enableVersion) {
      delay(_splashDelay / 2);
      _u8g2.firstPage();
      do {
        _u8g2.drawXBMP((_u8g2.getDisplayWidth() - _splash.width) / 2, (_u8g2.getDisplayHeight() - _splash.height) / 2, _splash.width, _splash.height, _splash.image);
        _u8g2.setFont(_fontFamilies.small);
        byte x = _u8g2.getDisplayWidth() - strlen(GEM_VER)*4;
        byte y = _u8g2.getDisplayHeight() - 7;
        if (_splash.image != logo_bits) {
          _u8g2.setCursor(x - 12, y);
          _u8g2.print("GEM");
        } else {
          _u8g2.setCursor(x, y);
        }
        _u8g2.print(GEM_VER);
      } while (_u8g2.nextPage());
      delay(_splashDelay / 2);
    } else {
      delay(_splashDelay);
    }

    _u8g2.clear();

  }

  return *this;
}

GEM_u8g2& GEM_u8g2::reInit() {
  _u8g2.initDisplay();
  _u8g2.setPowerSave(0);
  _u8g2.clear();
  _u8g2.setDrawColor(1);
  _u8g2.setFontPosTop();
  if (_UTF8Enabled) {
    _u8g2.enableUTF8Print();
  } else {
    _u8g2.disableUTF8Print();
  }
  return *this;
}

GEM_u8g2& GEM_u8g2::setMenuPageCurrent(GEMPage& menuPageCurrent) {
  _menuPageCurrent = &menuPageCurrent;
  return *this;
}

GEMPage* GEM_u8g2::getCurrentMenuPage() {
  return _menuPageCurrent;
}

//====================== CONTEXT OPERATIONS

GEM_u8g2& GEM_u8g2::clearContext() {
  context.loop = nullptr;
  context.enter = nullptr;
  context.exit = nullptr;
  context.allowExit = true;
  return *this;
}

//====================== DRAW OPERATIONS

GEM_u8g2& GEM_u8g2::drawMenu() {
  // _u8g2.clear(); // Not clearing for better performance
  _u8g2.firstPage();
  do {
    drawTitleBar();
    printMenuItems();
    drawMenuPointer();
    drawScrollbar();
    if (drawMenuCallback != nullptr) {
      drawMenuCallback();
    }
  } while (_u8g2.nextPage());
  return *this;
}

GEM_u8g2& GEM_u8g2::setDrawMenuCallback(void (*drawMenuCallback_)()) {
  drawMenuCallback = drawMenuCallback_;
  return *this;
}

GEM_u8g2& GEM_u8g2::removeDrawMenuCallback() {
  drawMenuCallback = nullptr;
  return *this;
}

void GEM_u8g2::drawTitleBar() {
 _u8g2.setFont(_fontFamilies.small);
 _u8g2.setCursor(5, 0);
 _u8g2.print(_menuPageCurrent->title);
 _u8g2.setFont(getMenuItemFontSize() ? _fontFamilies.small : _fontFamilies.big);
}

void GEM_u8g2::drawSprite(u8g2_uint_t x, u8g2_uint_t y, const GEMSprite sprite) {
  _u8g2.drawXBMP(x, y, sprite.width, sprite.height, sprite.image);
}

void GEM_u8g2::printMenuItemString(const char* str, byte num, byte startPos) {
  if (_UTF8Enabled) {

    byte j = 0;
    byte p = 0;
    while ((j < startPos || ((byte)str[p] >= 128 && (byte)str[p] <= 191)) && str[p] != '\0') {
      if ((byte)str[p] <= 127 || (byte)str[p] >= 194) {
        j++;
      }
      p++;
    }
    byte startPosReal = p;

    byte i = j;
    byte k = startPosReal;
    while ((i < num + j || ((byte)str[k] >= 128 && (byte)str[k] <= 191)) && str[k] != '\0') {
      _u8g2.print(str[k]);
      if ((byte)str[k] <= 127 || (byte)str[k] >= 194) {
        i++;
      }
      k++;
    }

  } else {

    byte i = startPos;
    while (i < num + startPos && str[i] != '\0') {
      _u8g2.print(str[i]);
      i++;
    }

  }
}

void GEM_u8g2::printMenuItemTitle(const char* str, int offset) {
  printMenuItemString(str, getMenuItemTitleLength() + offset);
}

void GEM_u8g2::printMenuItemValue(const char* str, int offset, byte startPos) {
  printMenuItemString(str, getMenuItemValueLength() + offset, startPos);
}

void GEM_u8g2::printMenuItemFull(const char* str, int offset) {
  printMenuItemString(str, getMenuItemTitleLength() + getMenuItemValueLength() + offset);
}

byte GEM_u8g2::getMenuItemInsetOffset(bool forSprite) {
  byte menuItemFontSize = getMenuItemFontSize();
  byte menuItemInsetOffset = (getCurrentAppearance()->menuItemHeight - _menuItemFont[menuItemFontSize].height) / 2;
  return menuItemInsetOffset + (forSprite ? (_menuItemFont[menuItemFontSize].height - sprite_height) / 2 : -1); // With additional offset for sprites and text for better visual alignment
}

byte GEM_u8g2::getCurrentItemTopOffset(bool withInsetOffset, bool forSprite) {
  byte topOffset    = getCurrentAppearance()->menuPageScreenTopOffset;
  byte firstVisible = getFirstVisibleItemIndex();
  byte y = 0;
  GEMItem* item = _menuPageCurrent->getMenuItem(firstVisible);
  int current = _menuPageCurrent->currentItemNum;
  for (int i = firstVisible; i < current && item != nullptr; i++, item = item->getMenuItemNext()) {
    y += getItemHeight(item);
  }
  return topOffset + y + (withInsetOffset ? getMenuItemInsetOffset(forSprite) : 0);
}

void GEM_u8g2::printMenuItems() {
  byte firstVisible = getFirstVisibleItemIndex();
  GEMItem* menuItemTmp = _menuPageCurrent->getMenuItem(firstVisible);
  byte y = getCurrentAppearance()->menuPageScreenTopOffset;
  byte screenBottom = _u8g2.getDisplayHeight();
  char valueStringTmp[GEM_STR_LEN];
  while (menuItemTmp != nullptr && y < screenBottom) {
    byte yText = y + getMenuItemInsetOffset();
    byte yDraw = y + getMenuItemInsetOffset(true);
    switch (menuItemTmp->type) {
      case GEM_ITEM_VAL:
        {
          _u8g2.setCursor(5, yText);
          if (menuItemTmp->readonly) {
            printMenuItemTitle(menuItemTmp->title, -1);
            _u8g2.print("^");
          } else {
            printMenuItemTitle(menuItemTmp->title);
          }

          bool isTwoLine = itemNeedsTwoLine(menuItemTmp);
          byte menuItemHeight = getCurrentAppearance()->menuItemHeight;
          byte menuValuesLeftOffset = isTwoLine ? 5 : getCurrentAppearance()->menuValuesLeftOffset;
          byte yTextValue = isTwoLine ? (byte)(yText + menuItemHeight) : yText;
          byte yDrawValue = isTwoLine ? (byte)(yDraw + menuItemHeight) : yDraw;
          _u8g2.setCursor(menuValuesLeftOffset, yTextValue);
          switch (menuItemTmp->linkedType) {
            case GEM_VAL_INTEGER:
              if (_editValueMode && menuItemTmp == _menuPageCurrent->getCurrentMenuItem()) {
                printMenuItemValue(_valueString, 0, _editValueVirtualCursorPosition - _editValueCursorPosition);
                drawEditValueCursor();
                if (_editSpecialAction != 0) {
                  byte menuItemFontSize = getMenuItemFontSize();
                  byte xSprite = menuValuesLeftOffset + _editValueCursorPosition * _menuItemFont[menuItemFontSize].width - 1;
                  drawEditValueCursor();  // second call undoes XOR → cursor area becomes black
                  _u8g2.setDrawColor(1);  // draw white-on-black
                  drawSprite(xSprite, yDrawValue, (_editSpecialAction == 1) ? editDeleteSprite : editConfirmSprite);
                }
              } else {
                itoa(*(int*)menuItemTmp->linkedVariable, valueStringTmp, 10);
                printMenuItemValue(valueStringTmp);
              }
              break;
            case GEM_VAL_BYTE:
              if (_editValueMode && menuItemTmp == _menuPageCurrent->getCurrentMenuItem()) {
                printMenuItemValue(_valueString, 0, _editValueVirtualCursorPosition - _editValueCursorPosition);
                drawEditValueCursor();
                if (_editSpecialAction != 0) {
                  byte menuItemFontSize = getMenuItemFontSize();
                  byte xSprite = menuValuesLeftOffset + _editValueCursorPosition * _menuItemFont[menuItemFontSize].width - 1;
                  drawEditValueCursor();  // second call undoes XOR → cursor area becomes black
                  _u8g2.setDrawColor(1);  // draw white-on-black
                  drawSprite(xSprite, yDrawValue, (_editSpecialAction == 1) ? editDeleteSprite : editConfirmSprite);
                }
              } else {
                itoa(*(byte*)menuItemTmp->linkedVariable, valueStringTmp, 10);
                printMenuItemValue(valueStringTmp);
              }
              break;
            case GEM_VAL_CHAR:
              if (_editValueMode && menuItemTmp == _menuPageCurrent->getCurrentMenuItem()) {
                if (isTwoLine) {
                  byte menuItemFontSize = getMenuItemFontSize();
                  byte strW = _u8g2.getStrWidth(_valueString);
                  byte startX = (strW + 10 <= _u8g2.getDisplayWidth()) ? (_u8g2.getDisplayWidth() - strW - 5) : 5;
                  _u8g2.setCursor(startX, yTextValue);
                  _u8g2.print(_valueString);
                  drawEditValueCursor();
                  if (_editSpecialAction != 0) {
                    byte xSprite = startX + _editValueCursorPosition * _menuItemFont[menuItemFontSize].width - 1;
                    drawEditValueCursor();
                    _u8g2.setDrawColor(1);
                    drawSprite(xSprite, yDrawValue, (_editSpecialAction == 1) ? editDeleteSprite : editConfirmSprite);
                  }
                } else {
                  printMenuItemValue(_valueString, 0, _editValueVirtualCursorPosition - _editValueCursorPosition);
                  drawEditValueCursor();
                  if (_editSpecialAction != 0) {
                    byte menuItemFontSize = getMenuItemFontSize();
                    byte xSprite = menuValuesLeftOffset + _editValueCursorPosition * _menuItemFont[menuItemFontSize].width - 1;
                    drawEditValueCursor();
                    _u8g2.setDrawColor(1);
                    drawSprite(xSprite, yDrawValue, (_editSpecialAction == 1) ? editDeleteSprite : editConfirmSprite);
                  }
                }
              } else {
                if (isTwoLine) {
                  const char* charVal = (char*)menuItemTmp->linkedVariable;
                  byte strW = _u8g2.getStrWidth(charVal);
                  byte xRight = (strW + 10 < _u8g2.getDisplayWidth()) ? (_u8g2.getDisplayWidth() - strW - 5) : 5;
                  _u8g2.setCursor(xRight, yTextValue);
                  _u8g2.print(charVal);
                } else {
                  printMenuItemValue((char*)menuItemTmp->linkedVariable);
                }
              }
              break;
            case GEM_VAL_BOOL:
              if (*(bool*)menuItemTmp->linkedVariable) {
                drawSprite(menuValuesLeftOffset, yDrawValue, checkboxChecked);
              } else {
                drawSprite(menuValuesLeftOffset, yDrawValue, checkboxUnchecked);
              }
              break;
            case GEM_VAL_SELECT:
              {
                GEMSelect* select = menuItemTmp->select;
                if (_editValueMode && menuItemTmp == _menuPageCurrent->getCurrentMenuItem()) {
                  printMenuItemValue(select->getOptionNameByIndex(_valueSelectNum));
                  drawSprite(_u8g2.getDisplayWidth() - 7, yDrawValue, selectArrows);
                  drawEditValueCursor();
                } else {
                  printMenuItemValue(select->getSelectedOptionName(menuItemTmp->linkedVariable));
                  drawSprite(_u8g2.getDisplayWidth() - 7, yDrawValue, selectArrows);
                }
              }
              break;
            #ifdef GEM_SUPPORT_SPINNER
            case GEM_VAL_SPINNER:
              {
                GEMSpinner* spinner = menuItemTmp->spinner;
                if (_editValueMode && menuItemTmp == _menuPageCurrent->getCurrentMenuItem()) {
                  GEMSpinnerValue valueTmp = spinner->getOptionNameByIndex(menuItemTmp->linkedVariable, _valueSelectNum);
                  switch (spinner->getType()) {
                    case GEM_VAL_BYTE:
                      itoa(valueTmp.valByte, valueStringTmp, 10);
                      break;
                    case GEM_VAL_INTEGER:
                      itoa(valueTmp.valInt, valueStringTmp, 10);
                      break;
                    #ifdef GEM_SUPPORT_FLOAT_EDIT
                    case GEM_VAL_FLOAT:
                      dtostrf(valueTmp.valFloat, menuItemTmp->precision + 1, menuItemTmp->precision, valueStringTmp);
                      break;
                    case GEM_VAL_DOUBLE:
                      dtostrf(valueTmp.valDouble, menuItemTmp->precision + 1, menuItemTmp->precision, valueStringTmp);
                      break;
                    #endif
                  }
                  printMenuItemValue(valueStringTmp);
                  drawSprite(_u8g2.getDisplayWidth() - 7, yDrawValue, selectArrows);
                  drawEditValueCursor();
                } else {
                  switch (spinner->getType()) {
                    case GEM_VAL_BYTE:
                      itoa(*(byte*)menuItemTmp->linkedVariable, valueStringTmp, 10);
                      break;
                    case GEM_VAL_INTEGER:
                      itoa(*(int*)menuItemTmp->linkedVariable, valueStringTmp, 10);
                      break;
                    #ifdef GEM_SUPPORT_FLOAT_EDIT
                    case GEM_VAL_FLOAT:
                      dtostrf(*(float*)menuItemTmp->linkedVariable, menuItemTmp->precision + 1, menuItemTmp->precision, valueStringTmp);
                      break;
                    case GEM_VAL_DOUBLE:
                      dtostrf(*(double*)menuItemTmp->linkedVariable, menuItemTmp->precision + 1, menuItemTmp->precision, valueStringTmp);
                      break;
                    #endif
                  }
                  printMenuItemValue(valueStringTmp);
                  drawSprite(_u8g2.getDisplayWidth() - 7, yDrawValue, selectArrows);
                }
              }
              break;
            #endif
            #ifdef GEM_SUPPORT_FLOAT_EDIT
            case GEM_VAL_FLOAT:
              if (_editValueMode && menuItemTmp == _menuPageCurrent->getCurrentMenuItem()) {
                printMenuItemValue(_valueString, 0, _editValueVirtualCursorPosition - _editValueCursorPosition);
                drawEditValueCursor();
              } else {
                // sprintf(valueStringTmp,"%.6f", *(float*)menuItemTmp->linkedVariable); // May work for non-AVR boards
                dtostrf(*(float*)menuItemTmp->linkedVariable, menuItemTmp->precision + 1, menuItemTmp->precision, valueStringTmp);
                printMenuItemValue(valueStringTmp);
              }
              break;
            case GEM_VAL_DOUBLE:
              if (_editValueMode && menuItemTmp == _menuPageCurrent->getCurrentMenuItem()) {
                printMenuItemValue(_valueString, 0, _editValueVirtualCursorPosition - _editValueCursorPosition);
                drawEditValueCursor();
              } else {
                // sprintf(valueStringTmp,"%.6f", *(double*)menuItemTmp->linkedVariable); // May work for non-AVR boards
                dtostrf(*(double*)menuItemTmp->linkedVariable, menuItemTmp->precision + 1, menuItemTmp->precision, valueStringTmp);
                printMenuItemValue(valueStringTmp);
              }
              break;
            #endif
            case GEM_VAL_IP:
              if (_editValueMode && menuItemTmp == _menuPageCurrent->getCurrentMenuItem()) {
                buildIpString(valueStringTmp, _editIpOctets);
                {
                  byte strW = _u8g2.getStrWidth(valueStringTmp);
                  byte startX = (strW + 10 <= _u8g2.getDisplayWidth()) ? (_u8g2.getDisplayWidth() - strW - 5) : 5;
                  _u8g2.setCursor(startX, yTextValue);
                }
                _u8g2.print(valueStringTmp);
                drawEditValueCursor();
              } else {
                buildIpString(valueStringTmp, (uint8_t*)menuItemTmp->linkedVariable);
                byte strW = _u8g2.getStrWidth(valueStringTmp);
                byte startX = (strW + 10 <= _u8g2.getDisplayWidth()) ? (_u8g2.getDisplayWidth() - strW - 5) : 5;
                _u8g2.setCursor(startX, yTextValue);
                _u8g2.print(valueStringTmp);
              }
              break;
          }
          break;
        }
      case GEM_ITEM_LINK:
        _u8g2.setCursor(5, yText);
        if (menuItemTmp->readonly) {
          printMenuItemFull(menuItemTmp->title, -1);
          _u8g2.print("^");
        } else {
          printMenuItemFull(menuItemTmp->title);
        }
        drawSprite(_u8g2.getDisplayWidth() - 8, yDraw, arrowRight);
        break;
      case GEM_ITEM_BACK:
        drawSprite(5, yDraw, arrowLeft);
        break;
      case GEM_ITEM_BUTTON:
        _u8g2.setCursor(11, yText);
        if (menuItemTmp->readonly) {
          printMenuItemFull(menuItemTmp->title, -1);
          _u8g2.print("^");
        } else {
          printMenuItemFull(menuItemTmp->title);
        }
        drawSprite(5, yDraw, arrowBtn);
        break;
      case GEM_ITEM_LABEL:
        _u8g2.setCursor(5, yText);
        printMenuItemFull(menuItemTmp->title);
        break;
    }
    byte prevItemHeight = getItemHeight(menuItemTmp);
    menuItemTmp = menuItemTmp->getMenuItemNext();
    y += prevItemHeight;
  }
  memset(valueStringTmp, '\0', GEM_STR_LEN - 1);
}

void GEM_u8g2::drawMenuPointer() {
  if (_menuPageCurrent->itemsCount > 0) {
    GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
    int pointerPosition = getCurrentItemTopOffset(false);
    byte itemH = getItemHeight(menuItemTmp);
    if (getCurrentAppearance()->menuPointerType == GEM_POINTER_DASH) {
      if (menuItemTmp->readonly || menuItemTmp->type == GEM_ITEM_LABEL) {
        for (byte i = 0; i < (itemH - 1) / 2; i++) {
          _u8g2.drawPixel(0, pointerPosition + i * 2);
          _u8g2.drawPixel(1, pointerPosition + i * 2 + 1);
        }
      } else {
        _u8g2.drawBox(0, pointerPosition, 2, itemH - 1);
      }
    } else if (!_editValueMode) {
      _u8g2.setDrawColor(2);
      _u8g2.drawBox(0, pointerPosition - 1, _u8g2.getDisplayWidth() - 2, itemH + 1);
      _u8g2.setDrawColor(1);
      if (menuItemTmp->readonly || menuItemTmp->type == GEM_ITEM_LABEL) {
        _u8g2.setDrawColor(0);
        for (byte i = 0; i < (itemH + 2) / 2; i++) {
          _u8g2.drawPixel(0, pointerPosition + i * 2);
          _u8g2.drawPixel(1, pointerPosition + i * 2 - 1);
        }
        _u8g2.setDrawColor(1);
      }
    }
  }
}

void GEM_u8g2::drawScrollbar() {
  byte menuItemsPerScreen = getMenuItemsPerScreen();
  byte screensCount = (_menuPageCurrent->itemsCount % menuItemsPerScreen == 0) ? _menuPageCurrent->itemsCount / menuItemsPerScreen : _menuPageCurrent->itemsCount / menuItemsPerScreen + 1;
  if (screensCount > 1) {
    byte currentScreenNum = _menuPageCurrent->currentItemNum / menuItemsPerScreen;
    byte menuPageScreenTopOffset = getCurrentAppearance()->menuPageScreenTopOffset;
    byte scrollbarHeight = (_u8g2.getDisplayHeight() - menuPageScreenTopOffset + 1) / screensCount;
    byte scrollbarPosition = currentScreenNum * scrollbarHeight + menuPageScreenTopOffset - 1;
    _u8g2.drawLine(_u8g2.getDisplayWidth() - 1, scrollbarPosition, _u8g2.getDisplayWidth() - 1, scrollbarPosition + scrollbarHeight);
  }
}

//====================== MENU ITEMS NAVIGATION

void GEM_u8g2::nextMenuItem() {
  if (_menuPageCurrent->itemsCount > 0) {
    if (_menuPageCurrent->currentItemNum == _menuPageCurrent->itemsCount-1) {
      _menuPageCurrent->currentItemNum = 0;
    } else {
      _menuPageCurrent->currentItemNum++;
    }
    drawMenu();
  }
}

void GEM_u8g2::prevMenuItem() {
  if (_menuPageCurrent->itemsCount > 0) {
    if (_menuPageCurrent->currentItemNum == 0) {
      _menuPageCurrent->currentItemNum = _menuPageCurrent->itemsCount-1;
    } else {
      _menuPageCurrent->currentItemNum--;
    }
    drawMenu();
  }
}

void GEM_u8g2::menuItemSelect() {
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  if (menuItemTmp != nullptr) {
    switch (menuItemTmp->type) {
      case GEM_ITEM_VAL:
        if (!menuItemTmp->readonly) {
          enterEditValueMode();
        }
        break;
      case GEM_ITEM_LINK:
        if (!menuItemTmp->readonly) {
          _menuPageCurrent = menuItemTmp->linkedPage;
          drawMenu();
        }
        break;
      case GEM_ITEM_BACK:
        _menuPageCurrent->currentItemNum = (_menuPageCurrent->itemsCount > 1) ? 1 : 0;
        _menuPageCurrent = menuItemTmp->linkedPage;
        drawMenu();
        break;
      case GEM_ITEM_BUTTON:
        if (!menuItemTmp->readonly) {
          if (menuItemTmp->callbackWithArgs) {
            menuItemTmp->callbackActionArg(menuItemTmp->callbackData);
          } else {
            menuItemTmp->callbackAction();
          }
        }
        break;
    }
  }
}

//====================== VALUE EDIT

void GEM_u8g2::enterEditValueMode() {
  _editValueMode = true;
  
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  _editValueType = menuItemTmp->linkedType;
  switch (_editValueType) {
    case GEM_VAL_INTEGER:
      itoa(*(int*)menuItemTmp->linkedVariable, _valueString, 10);
      _editValueLength = 6;
      initEditValueCursor();
      break;
    case GEM_VAL_BYTE:
      itoa(*(byte*)menuItemTmp->linkedVariable, _valueString, 10);
      _editValueLength = 3;
      initEditValueCursor();
      break;
    case GEM_VAL_CHAR:
      strcpy(_valueString, (char*)menuItemTmp->linkedVariable);
      _editValueLength = GEM_STR_LEN - 1;
      initEditValueCursor();
      break;
    case GEM_VAL_BOOL:
      checkboxToggle();
      drawMenu();
      break;
    case GEM_VAL_SELECT:
      {
        GEMSelect* select = menuItemTmp->select;
        _valueSelectNum = select->getSelectedOptionNum(menuItemTmp->linkedVariable);
        initEditValueCursor();
      }
      break;
    #ifdef GEM_SUPPORT_SPINNER
    case GEM_VAL_SPINNER:
      {
        GEMSpinner* spinner = menuItemTmp->spinner;
        _valueSelectNum = spinner->getSelectedOptionNum(menuItemTmp->linkedVariable);
        initEditValueCursor();
      }
      break;
    #endif
    #ifdef GEM_SUPPORT_FLOAT_EDIT
    case GEM_VAL_FLOAT:
      // sprintf(_valueString,"%.6f", *(float*)menuItemTmp->linkedVariable); // May work for non-AVR boards
      dtostrf(*(float*)menuItemTmp->linkedVariable, menuItemTmp->precision + 1, menuItemTmp->precision, _valueString);
      _editValueLength = GEM_STR_LEN - 1;
      initEditValueCursor();
      break;
    case GEM_VAL_DOUBLE:
      // sprintf(_valueString,"%.6f", *(double*)menuItemTmp->linkedVariable); // May work for non-AVR boards
      dtostrf(*(double*)menuItemTmp->linkedVariable, menuItemTmp->precision + 1, menuItemTmp->precision, _valueString);
      _editValueLength = GEM_STR_LEN - 1;
      initEditValueCursor();
      break;
    #endif
    case GEM_VAL_IP:
      memcpy(_editIpOctets, (uint8_t*)menuItemTmp->linkedVariable, 4);
      _editValueLength = 4;  // octet index 0-3
      initEditValueCursor();
      break;
  }
}

void GEM_u8g2::checkboxToggle() {
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  bool checkboxValue = *(bool*)menuItemTmp->linkedVariable;
  *(bool*)menuItemTmp->linkedVariable = !checkboxValue;
  if (menuItemTmp->callbackAction != nullptr) {
    resetEditValueState(); // Explicitly reset edit value state to be more predictable before user-defined callback is called
    if (menuItemTmp->callbackWithArgs) {
      menuItemTmp->callbackActionArg(menuItemTmp->callbackData);
    } else {
      menuItemTmp->callbackAction();
    }
    drawEditValueCursor();
    drawMenu();
  } else {
    _editValueMode = false;
  }
}

void GEM_u8g2::initEditValueCursor() {
  _editValueCursorPosition = 0;
  _editValueVirtualCursorPosition = 0;
  _editSpecialAction = 0;
  drawMenu();
}

void GEM_u8g2::nextEditValueCursorPosition() {
  if (_editValueType == GEM_VAL_IP) {
    if (_editValueCursorPosition < 3) { _editValueCursorPosition++; }
    drawMenu();
    return;
  }
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  bool curIsTwoLine = _twoLineItems && menuItemTmp && itemNeedsTwoLine(menuItemTmp);
  byte maxVisPos = curIsTwoLine ? (_editValueLength - 1) : (getMenuItemValueLength() - 1);
  if ((_editValueCursorPosition != maxVisPos) && (_editValueCursorPosition != _editValueLength - 1) && (_valueString[_editValueCursorPosition] != '\0')) {
    _editValueCursorPosition++;
  }
  if ((_editValueVirtualCursorPosition != _editValueLength - 1) && (_valueString[_editValueVirtualCursorPosition] != '\0')) {
    _editValueVirtualCursorPosition++;
  }
  drawMenu();
}

void GEM_u8g2::prevEditValueCursorPosition() {
  if (_editValueType == GEM_VAL_IP) {
    if (_editValueCursorPosition > 0) { _editValueCursorPosition--; }
    drawMenu();
    return;
  }
  if (_editValueCursorPosition != 0) {
    _editValueCursorPosition--;
  }
  if (_editValueVirtualCursorPosition != 0) {
    _editValueVirtualCursorPosition--;
  }
  drawMenu();
}

void GEM_u8g2::drawEditValueCursor() {
  int pointerPosition = getCurrentItemTopOffset(false);
  byte menuItemFontSize = getMenuItemFontSize();
  byte fontWidth = _menuItemFont[menuItemFontSize].width;
  byte menuItemHeight = getCurrentAppearance()->menuItemHeight;
  // In two-line mode the value is on the second line
  GEMItem* editItemTmp = _menuPageCurrent->getCurrentMenuItem();
  bool curIsTwoLine = _twoLineItems && editItemTmp && itemNeedsTwoLine(editItemTmp);
  if (curIsTwoLine) pointerPosition += menuItemHeight;
  byte valX;
  if (curIsTwoLine) {
    byte strW;
    if (_editValueType == GEM_VAL_IP) {
      char ipBuf[16];
      buildIpString(ipBuf, _editIpOctets);
      strW = _u8g2.getStrWidth(ipBuf);
    } else {
      strW = _u8g2.getStrWidth(_valueString);
    }
    valX = (strW + 10 <= _u8g2.getDisplayWidth()) ? (_u8g2.getDisplayWidth() - strW - 5) : 5;
  } else {
    valX = getCurrentAppearance()->menuValuesLeftOffset;
  }
  _u8g2.setDrawColor(2);
  if (_editValueType == GEM_VAL_SELECT || _editValueType == GEM_VAL_SPINNER) {
    byte cursorLeftOffset = valX + _editValueCursorPosition * fontWidth;
    _u8g2.drawBox(cursorLeftOffset - 1, pointerPosition - 1, _u8g2.getDisplayWidth() - cursorLeftOffset - 1, menuItemHeight + 1);
  } else if (_editValueType == GEM_VAL_IP) {
    // Highlight the current octet (3 chars wide); each octet+dot = 4 char widths
    byte cursorLeftOffset = valX + _editValueCursorPosition * 4 * fontWidth;
    _u8g2.drawBox(cursorLeftOffset - 1, pointerPosition - 1, 3 * fontWidth + 1, menuItemHeight + 1);
  } else {
    byte cursorLeftOffset = valX + _editValueCursorPosition * fontWidth;
    _u8g2.drawBox(cursorLeftOffset - 1, pointerPosition - 1, fontWidth + 1, menuItemHeight + 1);
  }
  _u8g2.setDrawColor(1);
}

void GEM_u8g2::nextEditValueDigit() {
  if (_editSpecialAction == 2) {
    // Already at confirm – UP keeps us here (icon stays visible)
    drawMenu();
    return;
  }
  if (_editSpecialAction == 1) {
    // In backspace state – UP exits it, char in _valueString is unchanged
    _editSpecialAction = 0;
    drawMenu();
    return;
  }
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  char chr = _valueString[_editValueVirtualCursorPosition];
  byte code = (byte)chr;
  // At end of string (null terminator): UP goes directly to CONFIRM
  if (code == 0 && (_editValueType == GEM_VAL_INTEGER || _editValueType == GEM_VAL_BYTE || _editValueType == GEM_VAL_CHAR)) {
    _editSpecialAction = 2;
    drawMenu();
    return;
  }
  if (_editValueType == GEM_VAL_CHAR) {
    if (menuItemTmp->adjustedAsciiOrder) {
      switch (code) {
        case 0:
          code = GEM_CHAR_CODE_a;
          break;
        case GEM_CHAR_CODE_SPACE:
          code = GEM_CHAR_CODE_a;
          break;
        case GEM_CHAR_CODE_ACCENT:
          code = GEM_CHAR_CODE_SPACE;
          break;
        case GEM_CHAR_CODE_TILDA:
          code = GEM_CHAR_CODE_BANG;
          break;
        default:
          code++;
          break;
      }
    } else {
      switch (code) {
        case 0:
          code = GEM_CHAR_CODE_SPACE;
          break;
        case GEM_CHAR_CODE_TILDA:
          // Enter CONFIRM state – truncate string at cursor position and show icon
          _valueString[_editValueVirtualCursorPosition] = '\0';
          _editSpecialAction = 2;
          drawMenu();
          return;
          break;
        /*
        // WIP for Cyrillic values support
        case GEM_CHAR_CODE_TILDA:
          code = _cyrillicEnabled ? GEM_CHAR_CODE_CYR_A : GEM_CHAR_CODE_SPACE;
          break;
        case GEM_CHAR_CODE_CYR_YA_SM:
          code = GEM_CHAR_CODE_SPACE;
          break;
        case GEM_CHAR_CODE_CYR_E:
          code = GEM_CHAR_CODE_CYR_YO;
          break;
        case GEM_CHAR_CODE_CYR_YO:
          code = GEM_CHAR_CODE_CYR_E + 1;
          break;
        case GEM_CHAR_CODE_CYR_E_SM:
          code = GEM_CHAR_CODE_CYR_YO_SM;
          break;
        case GEM_CHAR_CODE_CYR_YO_SM:
          code = GEM_CHAR_CODE_CYR_E_SM + 1;
          break;
        */
        default:
          code++;
          break;
      }
    }
  } else {
    switch (code) {
      case 0:
        code = GEM_CHAR_CODE_0;
        break;
      case GEM_CHAR_CODE_9:
        if (_editValueCursorPosition == 0 && (_editValueType == GEM_VAL_INTEGER || _editValueType == GEM_VAL_FLOAT || _editValueType == GEM_VAL_DOUBLE)) {
          code = GEM_CHAR_CODE_MINUS;  // pos-0: go to minus first
        } else {
          // Enter CONFIRM state – leave _valueString unchanged, just show icon
          _editSpecialAction = 2;
          drawMenu();
          return;
        }
        break;
      case GEM_CHAR_CODE_MINUS:
        // Enter CONFIRM state – leave _valueString unchanged, just show icon
        _editSpecialAction = 2;
        drawMenu();
        return;
        break;
      case GEM_CHAR_CODE_SPACE:
        code = (_editValueCursorPosition != 0 && (_editValueType == GEM_VAL_FLOAT || _editValueType == GEM_VAL_DOUBLE)) ? GEM_CHAR_CODE_DOT : GEM_CHAR_CODE_0;
        break;
      case GEM_CHAR_CODE_DOT:
        code = GEM_CHAR_CODE_0;
        break;
      default:
        code++;
        break;
    }
  }
  drawEditValueDigit(code);
}

void GEM_u8g2::prevEditValueDigit() {
  if (_editSpecialAction == 1) {
    // Already at backspace – DOWN keeps us here (icon stays visible)
    drawMenu();
    return;
  }
  if (_editSpecialAction == 2) {
    // In confirm state – DOWN exits it, char in _valueString is unchanged
    _editSpecialAction = 0;
    drawMenu();
    return;
  }
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  char chr = _valueString[_editValueVirtualCursorPosition];
  byte code = (byte)chr;
  if (_editValueType == GEM_VAL_CHAR) {
    if (menuItemTmp->adjustedAsciiOrder) {
      switch (code) {
        case 0:
          code = GEM_CHAR_CODE_ACCENT;
          break;
        case GEM_CHAR_CODE_BANG:
          code = GEM_CHAR_CODE_TILDA;
          break;
        case GEM_CHAR_CODE_a:
          code = GEM_CHAR_CODE_SPACE;
          break;
        case GEM_CHAR_CODE_SPACE:
          code = GEM_CHAR_CODE_ACCENT;
          break;
        default:
          code--;
          break;
      }
    } else {
      switch (code) {
        case 0:
          code = GEM_CHAR_CODE_TILDA;
          break;
        case GEM_CHAR_CODE_SPACE:
          // Enter BACKSPACE state – leave _valueString unchanged, just show icon
          _editSpecialAction = 1;
          drawMenu();
          return;
          break;
        /*
        // WIP for Cyrillic values support
        case 0:
          code = _cyrillicEnabled ? GEM_CHAR_CODE_CYR_YA_SM : GEM_CHAR_CODE_TILDA;
          break;
        case GEM_CHAR_CODE_SPACE:
          code = _cyrillicEnabled ? GEM_CHAR_CODE_CYR_YA_SM : GEM_CHAR_CODE_TILDA;
          break;
        case GEM_CHAR_CODE_CYR_A:
          code = GEM_CHAR_CODE_TILDA;
          break;
        case GEM_CHAR_CODE_CYR_E + 1:
          code = GEM_CHAR_CODE_CYR_YO;
          break;
        case GEM_CHAR_CODE_CYR_YO:
          code = GEM_CHAR_CODE_CYR_E;
          break;
        case GEM_CHAR_CODE_CYR_E_SM + 1:
          code = GEM_CHAR_CODE_CYR_YO_SM;
          break;
        case GEM_CHAR_CODE_CYR_YO_SM:
          code = GEM_CHAR_CODE_CYR_E_SM;
          break;
        */
        default:
          code--;
          break;
      }
    }
  } else {
    switch (code) {
      case 0:
        code = (_editValueCursorPosition == 0 && (_editValueType == GEM_VAL_INTEGER || _editValueType == GEM_VAL_FLOAT || _editValueType == GEM_VAL_DOUBLE)) ? GEM_CHAR_CODE_MINUS : GEM_CHAR_CODE_9;
        break;
      case GEM_CHAR_CODE_MINUS:
        code = GEM_CHAR_CODE_9;
        break;
      case GEM_CHAR_CODE_0:
        if (_editValueCursorPosition != 0 && (_editValueType == GEM_VAL_FLOAT || _editValueType == GEM_VAL_DOUBLE)) {
          code = GEM_CHAR_CODE_DOT;
        } else {
          // Skip SPACE intermediate – go directly to BACKSPACE state
          _editSpecialAction = 1;
          drawMenu();
          return;
        }
        break;
      case GEM_CHAR_CODE_SPACE:
        // Enter BACKSPACE state – leave _valueString unchanged, just show icon
        _editSpecialAction = 1;
        drawMenu();
        return;
        break;
      case GEM_CHAR_CODE_DOT:
        code = GEM_CHAR_CODE_SPACE;
        break;
      default:
        code--;
        break;
    }
  }
  drawEditValueDigit(code);
}

#ifdef GEM_SUPPORT_PREVIEW_CALLBACKS
void GEM_u8g2::callPreviewCallback(bool reset) {
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  if (menuItemTmp->previewCallbackAction != nullptr) {
    GEMPreviewCallbackData previewCallbackData;
    previewCallbackData.callbackData = menuItemTmp->callbackData;
    if (!reset) {
      previewCallbackData.type = menuItemTmp->linkedType;
      switch (menuItemTmp->linkedType) {
        case GEM_VAL_INTEGER:
          previewCallbackData.previewString = _valueString;
          previewCallbackData.previewValInt = atoi(_valueString);
          break;
        case GEM_VAL_BYTE:
          previewCallbackData.previewString = _valueString;
          previewCallbackData.previewValByte = atoi(_valueString);
          break;
        case GEM_VAL_CHAR:
          previewCallbackData.previewString = _valueString;
          previewCallbackData.previewValChar = _valueString;
          break;
        case GEM_VAL_SELECT:
          {
            previewCallbackData.previewSelectNum = _valueSelectNum;
            GEMSelect* select = menuItemTmp->select;
            // Members of an anonymous union share the same memory location, so we can take pointer to any one of them
            select->setValue(&previewCallbackData.previewValByte, _valueSelectNum);
            previewCallbackData.type = select->getType();
          }
          break;
        #ifdef GEM_SUPPORT_SPINNER
        case GEM_VAL_SPINNER:
          {
            previewCallbackData.previewSelectNum = _valueSelectNum;
            GEMSpinner* spinner = menuItemTmp->spinner;
            void* linkedVariable = menuItemTmp->getLinkedVariablePointer();
            // Members of an anonymous union share the same memory location, so we can take pointer to any one of them
            spinner->setValue(&previewCallbackData.previewValByte, _valueSelectNum, linkedVariable);
            previewCallbackData.type = spinner->getType();
          }
          break;
        #endif
        #ifdef GEM_SUPPORT_FLOAT_EDIT
        case GEM_VAL_FLOAT:
          previewCallbackData.previewString = _valueString;
          previewCallbackData.previewValFloat = atof(_valueString);
          break;
        case GEM_VAL_DOUBLE:
          previewCallbackData.previewString = _valueString;
          previewCallbackData.previewValDouble = atof(_valueString);
          break;
        #endif
      }
    }
    menuItemTmp->previewCallbackAction(previewCallbackData);
  }
}
#endif

void GEM_u8g2::drawEditValueDigit(byte code) {
  char chrNew = (char)code;
  _valueString[_editValueVirtualCursorPosition] = chrNew;
  #ifdef GEM_SUPPORT_PREVIEW_CALLBACKS
  callPreviewCallback();
  #endif
  drawMenu();
}

void GEM_u8g2::nextEditValueSelect() {
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  GEMSelect* select = menuItemTmp->select;
  if (_valueSelectNum+1 < select->getLength()) {
    _valueSelectNum++;
  } else if (select->getLoop()) {
    _valueSelectNum = 0;
  }
  #ifdef GEM_SUPPORT_PREVIEW_CALLBACKS
  callPreviewCallback();
  #endif
  drawMenu();
}

void GEM_u8g2::prevEditValueSelect() {
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  GEMSelect* select = menuItemTmp->select;
  if (_valueSelectNum > 0) {
    _valueSelectNum--;
  } else if (select->getLoop()) {
    _valueSelectNum = select->getLength() - 1;
  }
  #ifdef GEM_SUPPORT_PREVIEW_CALLBACKS
  callPreviewCallback();
  #endif
  drawMenu();
}

#ifdef GEM_SUPPORT_SPINNER
void GEM_u8g2::nextEditValueSpinner() {
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  GEMSpinner* spinner = menuItemTmp->spinner;
  if (_valueSelectNum+1 < spinner->getLength()) {
    _valueSelectNum++;
  } else if (spinner->getLoop()) {
    _valueSelectNum = 0;
  }
  #ifdef GEM_SUPPORT_PREVIEW_CALLBACKS
  callPreviewCallback();
  #endif
  drawMenu();
}

void GEM_u8g2::prevEditValueSpinner() {
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  GEMSpinner* spinner = menuItemTmp->spinner;
  if (_valueSelectNum > 0) {
    _valueSelectNum--;
  } else if (spinner->getLoop()) {
    _valueSelectNum = spinner->getLength() - 1;
  }
  #ifdef GEM_SUPPORT_PREVIEW_CALLBACKS
  callPreviewCallback();
  #endif
  drawMenu();
}
#endif

void GEM_u8g2::nextEditValueIpOctet() {
  _editIpOctets[_editValueCursorPosition] = (_editIpOctets[_editValueCursorPosition] + 1) & 0xFF;
  drawMenu();
}

void GEM_u8g2::prevEditValueIpOctet() {
  _editIpOctets[_editValueCursorPosition] = (_editIpOctets[_editValueCursorPosition] - 1) & 0xFF;
  drawMenu();
}

void GEM_u8g2::saveEditValue() {
  GEMItem* menuItemTmp = _menuPageCurrent->getCurrentMenuItem();
  switch (menuItemTmp->linkedType) {
    case GEM_VAL_INTEGER:
      *(int*)menuItemTmp->linkedVariable = atoi(_valueString);
      break;
    case GEM_VAL_BYTE:
      *(byte*)menuItemTmp->linkedVariable = atoi(_valueString);
      break;
    case GEM_VAL_CHAR:
      strcpy((char*)menuItemTmp->linkedVariable, trimString(_valueString)); // Potential overflow if string length is not defined
      break;
    case GEM_VAL_SELECT:
      {
        GEMSelect* select = menuItemTmp->select;
        select->setValue(menuItemTmp->linkedVariable, _valueSelectNum);
      }
      break;
    #ifdef GEM_SUPPORT_SPINNER
    case GEM_VAL_SPINNER:
      {
        GEMSpinner* spinner = menuItemTmp->spinner;
        spinner->setValue(menuItemTmp->linkedVariable, _valueSelectNum);
      }
      break;
    #endif
    #ifdef GEM_SUPPORT_FLOAT_EDIT
    case GEM_VAL_FLOAT:
      *(float*)menuItemTmp->linkedVariable = atof(_valueString);
      break;
    case GEM_VAL_DOUBLE:
      *(double*)menuItemTmp->linkedVariable = atof(_valueString);
      break;
    #endif
    case GEM_VAL_IP:
      memcpy((uint8_t*)menuItemTmp->linkedVariable, _editIpOctets, 4);
      break;
  }
  if (menuItemTmp->callbackAction != nullptr) {
    resetEditValueState(); // Explicitly reset edit value state to be more predictable before user-defined callback is called
    if (menuItemTmp->callbackWithArgs) {
      menuItemTmp->callbackActionArg(menuItemTmp->callbackData);
    } else {
      menuItemTmp->callbackAction();
    }
    drawEditValueCursor();
    drawMenu();
  } else {
    exitEditValue();
  }
}

void GEM_u8g2::cancelEditValue() {
  #ifdef GEM_SUPPORT_PREVIEW_CALLBACKS
  callPreviewCallback(true);
  #endif
  exitEditValue();
}

void GEM_u8g2::resetEditValueState() {
  memset(_valueString, '\0', GEM_STR_LEN - 1);
  _valueSelectNum = -1;
  _editValueMode = false;
  _editSpecialAction = 0;
}

void GEM_u8g2::exitEditValue() {
  resetEditValueState();
  drawEditValueCursor();
  drawMenu();
}

bool GEM_u8g2::isEditMode() {
  return _editValueMode;
}

// Trim leading/trailing whitespaces
// Author: Adam Rosenfield, https://stackoverflow.com/a/122721
char* GEM_u8g2::trimString(char* str) {
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;
  
  return str;
}

//====================== KEY DETECTION

bool GEM_u8g2::readyForKey() {
  if ( (context.loop == nullptr) ||
      ((context.loop != nullptr) && (context.allowExit)) ) {

    // Suppress all input while OK is still physically held after a long-press.
    // On release: drain the pending u8g2 event so menu_loop never sees the OK.
    if (_okLongFired) {
      if (_repeatOkPin != 255 && digitalRead(_repeatOkPin) == LOW) {
        return false;  // still held – block
      } else {
        _u8g2.getMenuEvent();  // drain the release event from u8g2's queue
        _okLongFired = false;
        _okHeldSince = 0;
        return false;  // skip this cycle too, so menu_loop sees nothing
      }
    }

    // Key-repeat for UP/DOWN, and OK long-press to save – only in edit mode
    if (_editValueMode) {
      // Lazy-read pin numbers from u8g2's internal state (set via u8g2.begin())
      if (_repeatUpPin == 255) {
        _repeatUpPin   = _u8g2.getU8x8()->pins[U8X8_PIN_MENU_UP];
        _repeatDownPin = _u8g2.getU8x8()->pins[U8X8_PIN_MENU_DOWN];
        _repeatOkPin   = _u8g2.getU8x8()->pins[U8X8_PIN_MENU_SELECT];
      }
      uint32_t now = millis();

      // UP/DOWN repeat
      bool upHeld   = (_repeatUpPin   != 255) && (digitalRead(_repeatUpPin)   == LOW);
      bool downHeld = (_repeatDownPin != 255) && (digitalRead(_repeatDownPin) == LOW);
      byte heldKey  = upHeld ? GEM_KEY_UP : (downHeld ? GEM_KEY_DOWN : 0);

      if (heldKey != 0) {
        if (heldKey != _repeatLastKey) {
          _repeatLastKey   = heldKey;
          _repeatHeldSince = now;
          _repeatLastFired = now;
        } else if ((now - _repeatHeldSince  >= REPEAT_DELAY_MS) &&
                   (now - _repeatLastFired  >= REPEAT_INTERVAL_MS)) {
          _repeatLastFired = now;
          registerKeyPress(heldKey);
        }
      } else {
        _repeatLastKey = 0;
      }

      // OK long-press (2s) → save and exit edit mode immediately
      bool okHeld = (_repeatOkPin != 255) && (digitalRead(_repeatOkPin) == LOW);
      if (okHeld) {
        if (_okHeldSince == 0) {
          _okHeldSince = now;
        } else if (!_okLongFired && (now - _okHeldSince >= OK_LONGPRESS_MS)) {
          _okLongFired     = true;   // blocks input until button is released
          _editSpecialAction = 2;    // force confirm state → OK saves immediately
          registerKeyPress(GEM_KEY_OK);
        }
      } else {
        _okHeldSince = 0;
      }
    }

    return true;
  } else {
    registerKeyPress(GEM_KEY_NONE);
    return false;
  }

}

GEM_u8g2& GEM_u8g2::registerKeyPress(byte keyCode) {
  _currentKey = keyCode;
  dispatchKeyPress();
  return *this;
}

void GEM_u8g2::dispatchKeyPress() {

  if (context.loop != nullptr) {
    if ((context.allowExit) && (_currentKey == GEM_KEY_CANCEL)) {
      if (context.exit != nullptr) {
        context.exit();
      } else {
        reInit();
        drawMenu();
        clearContext();
      }
    } else {
      context.loop();
    }
  } else {
  
    if (_editValueMode) {
      switch (_currentKey) {
        case GEM_KEY_UP:
          if (_editValueType == GEM_VAL_SELECT) {
            prevEditValueSelect();
          #ifdef GEM_SUPPORT_SPINNER
          } else if (_editValueType == GEM_VAL_SPINNER) {
            if (_invertKeysDuringEdit) {
              prevEditValueSpinner();
            } else {
              nextEditValueSpinner();
            }
          #endif
          } else if (_editValueType == GEM_VAL_IP) {
            if (_invertKeysDuringEdit) { prevEditValueIpOctet(); } else { nextEditValueIpOctet(); }
          } else if (_invertKeysDuringEdit) {
            prevEditValueDigit();
          } else {
            nextEditValueDigit();
          }
          break;
        case GEM_KEY_RIGHT:
          if (_editValueType != GEM_VAL_SELECT && _editValueType != GEM_VAL_SPINNER) {
            nextEditValueCursorPosition();
          }
          break;
        case GEM_KEY_DOWN:
          if (_editValueType == GEM_VAL_SELECT) {
            nextEditValueSelect();
          #ifdef GEM_SUPPORT_SPINNER
          } else if (_editValueType == GEM_VAL_SPINNER) {
            if (_invertKeysDuringEdit) {
              nextEditValueSpinner();
            } else {
              prevEditValueSpinner();
            }
          #endif
          } else if (_editValueType == GEM_VAL_IP) {
            if (_invertKeysDuringEdit) { nextEditValueIpOctet(); } else { prevEditValueIpOctet(); }
          } else if (_invertKeysDuringEdit) {
            nextEditValueDigit();
          } else {
            prevEditValueDigit();
          }
          break;
        case GEM_KEY_LEFT:
          if (_editValueType != GEM_VAL_SELECT && _editValueType != GEM_VAL_SPINNER) {
            prevEditValueCursorPosition();
          }
          break;
        case GEM_KEY_CANCEL:
          cancelEditValue();
          break;
        case GEM_KEY_OK:
          if (_editSpecialAction == 2) {
            // CONFIRM icon: save the value
            _editSpecialAction = 0;
            saveEditValue();
          } else if (_editSpecialAction == 1) {
            // BACKSPACE icon: truncate string at cursor and go back one position
            _editSpecialAction = 0;
            _valueString[_editValueVirtualCursorPosition] = '\0';
            if (_editValueVirtualCursorPosition > 0) {
              prevEditValueCursorPosition();
            } else {
              drawMenu();
            }
          } else if (_editValueType == GEM_VAL_SELECT || _editValueType == GEM_VAL_SPINNER) {
            // SELECT / SPINNER: OK means save directly
            saveEditValue();
          } else if (_editValueType == GEM_VAL_IP) {
            // IP: OK advances to next octet; on last octet saves
            if (_editValueCursorPosition < 3) {
              _editValueCursorPosition++;
              drawMenu();
            } else {
              saveEditValue();
            }
          } else {
            // Normal char/digit: advance cursor; save if at end of string
            if (_valueString[_editValueVirtualCursorPosition] == '\0') {
              saveEditValue();
            } else {
              nextEditValueCursorPosition();
            }
          }
          break;
      }
    } else {
      switch (_currentKey) {
        case GEM_KEY_UP:
          prevMenuItem();
          break;
        case GEM_KEY_RIGHT:
          if (_menuPageCurrent->getCurrentMenuItem() != nullptr && (
              _menuPageCurrent->getCurrentMenuItem()->type == GEM_ITEM_LINK ||
              _menuPageCurrent->getCurrentMenuItem()->type == GEM_ITEM_BUTTON)) {
            menuItemSelect();
          }
          break;
        case GEM_KEY_DOWN:
          nextMenuItem();
          break;
        case GEM_KEY_LEFT:
          if (_menuPageCurrent->getCurrentMenuItem() != nullptr &&
              _menuPageCurrent->getCurrentMenuItem()->type == GEM_ITEM_BACK) {
            menuItemSelect();
          }
          break;
        case GEM_KEY_CANCEL:
          if (_menuPageCurrent->getMenuItem(0) != nullptr &&
              _menuPageCurrent->getMenuItem(0)->type == GEM_ITEM_BACK) {
            _menuPageCurrent->currentItemNum = 0;
            menuItemSelect();
          } else if (_menuPageCurrent->exitAction != nullptr) {
            _menuPageCurrent->currentItemNum = 0;
            _menuPageCurrent->exitAction();
          }
          break;
        case GEM_KEY_OK:
          menuItemSelect();
          break;
      }
    }

  }
}

#endif
