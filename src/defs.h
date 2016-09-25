#ifndef SRC_DEFS_H_
#define SRC_DEFS_H_

enum class KeyboardLayout {
  QWERTY = 0,
  QWERTZ,
  AZERTY,
  WORKMAN,
  COLEMAK,
  DVORAK,
  CUSTOM
};

enum class KeyboardStandard {
  NONE = -1,
  ANSI = 0,
  ISO
};


enum class KeyboardRow {
  UPPER,
  MIDDLE,
  LOWER
};

enum class Finger {
  INDEX_INNER,
  INDEX,
  MIDDLE,
  RING,
  PINKY,
  PINKY_EXTRA
};

Q_DECLARE_METATYPE(KeyboardLayout);
Q_DECLARE_METATYPE(KeyboardRow);
Q_DECLARE_METATYPE(KeyboardStandard);
Q_DECLARE_METATYPE(Finger);

// The sizes of the key caps in the standard 5x15 area on most keyboards.
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
  static constexpr const int iso_offset[5] = {0, 1, 1, 1, 3};
};

#endif  // SRC_DEFS_H_
