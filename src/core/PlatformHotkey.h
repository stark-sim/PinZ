#ifndef PLATFORMHOTKEY_H
#define PLATFORMHOTKEY_H

#include <QtGlobal>

#if defined(Q_OS_WIN)
#include <windows.h>
#else
using UINT = unsigned int;

static constexpr UINT MOD_ALT = 0x0001;
static constexpr UINT MOD_CONTROL = 0x0002;
static constexpr UINT MOD_SHIFT = 0x0004;
static constexpr UINT MOD_WIN = 0x0008;

static constexpr UINT VK_BACK = 0x08;
static constexpr UINT VK_TAB = 0x09;
static constexpr UINT VK_RETURN = 0x0D;
static constexpr UINT VK_PAUSE = 0x13;
static constexpr UINT VK_CAPITAL = 0x14;
static constexpr UINT VK_ESCAPE = 0x1B;
static constexpr UINT VK_SPACE = 0x20;
static constexpr UINT VK_PRIOR = 0x21;
static constexpr UINT VK_NEXT = 0x22;
static constexpr UINT VK_END = 0x23;
static constexpr UINT VK_HOME = 0x24;
static constexpr UINT VK_LEFT = 0x25;
static constexpr UINT VK_UP = 0x26;
static constexpr UINT VK_RIGHT = 0x27;
static constexpr UINT VK_DOWN = 0x28;
static constexpr UINT VK_SNAPSHOT = 0x2C;
static constexpr UINT VK_INSERT = 0x2D;
static constexpr UINT VK_DELETE = 0x2E;
static constexpr UINT VK_APPS = 0x5D;
static constexpr UINT VK_NUMPAD0 = 0x60;
static constexpr UINT VK_NUMPAD9 = 0x69;
static constexpr UINT VK_MULTIPLY = 0x6A;
static constexpr UINT VK_ADD = 0x6B;
static constexpr UINT VK_SUBTRACT = 0x6D;
static constexpr UINT VK_DECIMAL = 0x6E;
static constexpr UINT VK_DIVIDE = 0x6F;
static constexpr UINT VK_F1 = 0x70;
static constexpr UINT VK_F2 = 0x71;
static constexpr UINT VK_F3 = 0x72;
static constexpr UINT VK_F4 = 0x73;
static constexpr UINT VK_F5 = 0x74;
static constexpr UINT VK_F6 = 0x75;
static constexpr UINT VK_F7 = 0x76;
static constexpr UINT VK_F8 = 0x77;
static constexpr UINT VK_F9 = 0x78;
static constexpr UINT VK_F10 = 0x79;
static constexpr UINT VK_F11 = 0x7A;
static constexpr UINT VK_F12 = 0x7B;
static constexpr UINT VK_F13 = 0x7C;
static constexpr UINT VK_F14 = 0x7D;
static constexpr UINT VK_F15 = 0x7E;
static constexpr UINT VK_F16 = 0x7F;
static constexpr UINT VK_F17 = 0x80;
static constexpr UINT VK_F18 = 0x81;
static constexpr UINT VK_F19 = 0x82;
static constexpr UINT VK_F20 = 0x83;
static constexpr UINT VK_F21 = 0x84;
static constexpr UINT VK_F22 = 0x85;
static constexpr UINT VK_F23 = 0x86;
static constexpr UINT VK_F24 = 0x87;
static constexpr UINT VK_NUMLOCK = 0x90;
static constexpr UINT VK_SCROLL = 0x91;
#endif

#endif
