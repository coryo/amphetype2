#ifndef SRC_DEFS_H_
#define SRC_DEFS_H_

enum class KeyboardLayout {
  QWERTY = 0, QWERTZ, AZERTY, WORKMAN, COLEMAK, DVORAK, CUSTOM };
Q_DECLARE_METATYPE(KeyboardLayout);

enum class KeyboardRow { UPPER, MIDDLE, LOWER };
Q_DECLARE_METATYPE(KeyboardRow);

enum class KeyboardStandard { NONE = -1, ANSI = 0, ISO };
Q_DECLARE_METATYPE(KeyboardStandard);

enum class Finger { INDEX_INNER, INDEX, MIDDLE, RING, PINKY, PINKY_EXTRA};
Q_DECLARE_METATYPE(Finger);

namespace Standards {
  static constexpr const double ansi_keys[][15] = {
    {1,    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    1,    2,   -1},
    {1.5,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    1,    1.5, -1},
    {1.75, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    2.25, -1,  -1},
    {2.25, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.75, -1,   -1,  -1},
    {1.25, 1.25, 1.25, 6.25, 1.25, 1.25, 1.25, 1.25, -1, -1, -1, -1, -1, -1, -1}};
  static constexpr const int ansi_offset[5] = {0, 1, 1, 1, 3};

  static constexpr const double iso_keys[][15] = {
    {1,    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    2,    -1},
    {1.5,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    1.5,  -1},
    {1.75, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    1.25, -1},
    {1.25, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.75, -1,   -1},
    {1.25, 1.25, 1.25, 6.25, 1.25, 1.25, 1.25, 1.25, -1, -1, -1, -1, -1, -1, -1}};
  static constexpr const int iso_offset[5] = {0, 1, 1, 2, 3};
};

namespace Layouts {
  static constexpr const char qwerty[][14] = {
    {"`1234567890-="},
    {"qwertyuiop[]\\"},
    {"asdfghjkl;'"},
    {"zxcvbnm,./"},
    {" "}};
  static constexpr const char qwertz[][14] = {
    {"`1234567890-="},
    {"qwertzuiopü"},
    {"asdfghjklöä"},
    {"yxcvbnm,.-"},
    {" "}};
  static constexpr const char azerty[][14] = {
    {"`1234567890-="},
    {"azertyuiop"},
    {"qsdfghjklm"},
    {"wxcvbn,;:!"},
    {" "}};
  static constexpr const char colemak[][14] = {
    {"`1234567890-="},
    {"qwfpgjluy;[]\\"},
    {"arstdhneio'"},
    {"zxcvbkm,./"},
    {" "}};
  static constexpr const char dvorak[][14] = {
    {"`1234567890[]"},
    {"',.pyfgcrl/=\\"},
    {"aoeuidhtns-"},
    {";qjkxbmwvz"},
    {" "}};
  static constexpr const char workman[][14] = {
    {"`1234567890-="},
    {"qdrwbjfup;"},
    {"ashtgyneoi"},
    {"zxmcvkl,./"},
    {" "}};
};

#endif  // SRC_DEFS_H_
