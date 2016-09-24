#include "analysis/keyboardmap.h"

#include <QtGlobal>
#include <QtMath>
#include <QSettings>
#include <QGraphicsScene>
#include <QStringList>
#include <QPointF>
#include <QGraphicsTextItem>
#include <QHash>
#include <QBrush>
#include <QColor>
#include <QPen>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>

#include <limits>

#include <QsLog.h>

#include "database/db.h"


KeyboardMap::KeyboardMap(QWidget *parent) : QGraphicsView(parent),
  keySpacing(4),
  keySize(35),
  dataToMap("frequency") {

  QSettings s;

  this->setKeyboard(
    static_cast<KeyboardLayout>(s.value("keyboard_layout", 0).toInt()),
    static_cast<KeyboardStandard>(s.value("keyboard_standard", 0).toInt()));
  this->keyboardScene = new QGraphicsScene;
  this->setScene(this->keyboardScene);
  connect(this, &KeyboardMap::dataChanged, this, &KeyboardMap::addKeys);
  this->setRenderHints(QPainter::Antialiasing);
}

KeyboardMap::~KeyboardMap() {
  delete this->keyboardScene;
}

void KeyboardMap::setKeyboard(KeyboardLayout layout, KeyboardStandard standard) {
  this->keyboardLayout = layout;
  this->keyboardStandard = standard;
  emit dataChanged();
}

void KeyboardMap::setLayout(KeyboardLayout layout) {
  this->keyboardLayout = layout;
  emit dataChanged();
}

void KeyboardMap::setStandard(KeyboardStandard standard) {
  this->keyboardStandard = standard;
  emit dataChanged();
}

void KeyboardMap::setData(const QString & data) {
  this->dataToMap = data;
  emit dataChanged();
}

void KeyboardMap::addKeys() {
  this->keyboardScene->clear();

  double max = 0;
  double min = std::numeric_limits<double>::max();
  QHash<QChar, QHash<QString, QVariant>> frequencyData = DB::getKeyFrequency();

  // find the min and max of the data
  foreach (auto const & value, frequencyData) {
    if (value[this->dataToMap] > max)
      max = value[this->dataToMap].toDouble();
    if (value[this->dataToMap] > 0 && value[this->dataToMap] < min)
      min = value[this->dataToMap].toDouble();
  }

  const char (*keys)[5][14];
  switch(this->keyboardLayout) {
    case KeyboardLayout::QWERTY:
      keys = &(Layouts::qwerty);
      break;
    case KeyboardLayout::QWERTZ:
      keys = &(Layouts::qwertz);
      break;
    case KeyboardLayout::AZERTY:
      keys = &(Layouts::azerty);
      break;
    case KeyboardLayout::WORKMAN:
      keys = &(Layouts::workman);
      break;
    case KeyboardLayout::COLEMAK:
      keys = &(Layouts::colemak);
      break;
    case KeyboardLayout::DVORAK:
      keys = &(Layouts::dvorak);
      break;
  }

  for (int row = 0; row < 5; row++) {
    double sum = 0;
    for (int column = 0; column < 15; column++) {
      qreal key_size;

      int column_offset;
      switch(this->keyboardStandard) {
        case KeyboardStandard::ANSI:
          column_offset = Standards::ansi_offset[row];
          key_size = Standards::ansi_keys[row][column];
          break;
        case KeyboardStandard::ISO:
          column_offset = Standards::iso_offset[row];
          key_size = Standards::iso_keys[row][column];
          break;
      }
      if (key_size == -1)
        continue;

      QChar key = (*keys)[row][column - column_offset];
      bool letter_key = !key.isNonCharacter();

      QBrush brush;
      QColor color;
      // QChar key;
      if (letter_key) {
        QLOG_DEBUG() << "letter_key" << key << "@" << row << column;
        brush.setStyle(Qt::SolidPattern);
        // key = (*keys)[row][column - column_offset];
        qreal value = frequencyData[key][this->dataToMap].toDouble();

        // set the brush alpha based on the data
        if (value > 0) {
          color.setRgb(255, 0, 0);
          qreal alpha;
          if (this->dataToMap == "accuracy") {
            alpha = -1.0 * this->scaleToRange2(value, 255.0, min, max, 100) + 255.0;
          } else if (this->dataToMap == "viscosity") {
            alpha = qMin(255.0, 10 * this->scaleToRange(value, 255.0, min, max));
          } else {
            alpha = this->scaleToRange(value, 255.0, min, max);
          }
          // QLOG_DEBUG() << key << this->dataToMap << value << alpha;
          color.setAlpha(alpha);
          // if (key == QChar::Space)
          //   color.setAlpha(0);
        } else {
          color.setRgb(255, 255, 255);
        }
        brush.setColor(color);
      } else {
        brush.setColor(QColor("white"));
        brush.setStyle(Qt::SolidPattern);
      }

      // adjust the key width for spacing
      qreal widthOffset = (key_size > 2) ? qFloor(key_size) : 0;
      if (widthOffset && (column == 0 || sum + key_size == 15))
        widthOffset -= 1;

      // add the key
      QPainterPath path;
      path.addRoundedRect(
        QRectF(
          (this->keySize * sum) + (qFloor(sum) * this->keySpacing),
          row * this->keySize + (row * this->keySpacing),
          this->keySize * key_size + widthOffset * this->keySpacing,
          this->keySize
        ),
        this->keySize / 4, this->keySize / 4);

      auto keyRect = this->keyboardScene->addPath(path, QPen(), brush);

      if (letter_key) {
        // add the text on the key
        QString display;
        if (key == QChar::Space)
          display = "space";
        else
          display = key;
        QGraphicsTextItem* textItem = this->keyboardScene->addText(
          display);
        textItem->setPos(
          keyRect->boundingRect().center() - QPointF(
            textItem->boundingRect().width() / 2,
            textItem->boundingRect().height() / 2));
      }

      sum += key_size;
    }
  }
}

qreal KeyboardMap::scaleToRange(qreal x, qreal range, qreal min, qreal max) {
  return range - range * (1.0 - qAbs(x - min) / qAbs(max - min));
}

qreal KeyboardMap::scaleToRange2(qreal x, qreal range, qreal min, qreal max, qreal factor) {
  return range * ((qPow(factor, qAbs(x-min) / qAbs(max-min)) - 1) / (factor - 1));
}