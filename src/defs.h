#ifndef SRC_DEFS_H_
#define SRC_DEFS_H_

namespace Amphetype {
enum class SelectionMethod { None = -1, Random = 0, InOrder, Repeat, Ask };

enum class Layout {
  QWERTY = 0,
  QWERTZ,
  AZERTY,
  WORKMAN,
  COLEMAK,
  DVORAK,
  CUSTOM
};

enum class Standard { NONE = -1, ANSI = 0, ISO };

enum class Modifier { None, Shift, AltGr };

// The sizes of the key caps in the standard 5x15 area on most keyboards.
namespace Standards {
static constexpr const double ansi_keys[][15] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, -1},
    {1.5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5, -1},
    {1.75, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.25, -1, -1},
    {2.25, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.75, -1, -1, -1},
    {1.25, 1.25, 1.25, 6.25, 1.25, 1.25, 1.25, 1.25, -1, -1, -1, -1, -1, -1,
     -1}};
static constexpr const int ansi_offset[5] = {0, 1, 1, 1, 3};

static constexpr const double iso_keys[][15] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, -1},
    {1.5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5, -1},
    {1.75, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.25, -1},
    {1.25, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.75, -1, -1},
    {1.25, 1.25, 1.25, 6.25, 1.25, 1.25, 1.25, 1.25, -1, -1, -1, -1, -1, -1,
     -1}};
static constexpr const int iso_offset[5] = {0, 1, 1, 1, 3};
};

enum class KeyboardRow { UPPER, MIDDLE, LOWER };

enum class Finger { INDEX_INNER, INDEX, MIDDLE, RING, PINKY, PINKY_EXTRA };
};

Q_DECLARE_METATYPE(Amphetype::SelectionMethod);
Q_DECLARE_METATYPE(Amphetype::Layout);
Q_DECLARE_METATYPE(Amphetype::Standard);
Q_DECLARE_METATYPE(Amphetype::Modifier);

Q_DECLARE_METATYPE(Amphetype::KeyboardRow);
Q_DECLARE_METATYPE(Amphetype::Finger);

#endif  // SRC_DEFS_H_
