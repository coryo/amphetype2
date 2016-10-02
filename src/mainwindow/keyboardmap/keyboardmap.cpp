// Copyright (C) 2016  Cory Parsons
//
// This file is part of amphetype2.
//
// amphetype2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// amphetype2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with amphetype2.  If not, see <http://www.gnu.org/licenses/>.
//

#include "mainwindow/keyboardmap/keyboardmap.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSettings>
#include <QStringList>
#include <QtGlobal>
#include <QtMath>

#include <limits>

#include <QsLog.h>

#include "database/db.h"

KeyboardMap::KeyboardMap(QWidget* parent)
    : QGraphicsView(parent),
      keySpacing(3),
      keySize(24),
      dataToMap("frequency"),
      keyboardScene(new QGraphicsScene) {
  QSettings s;
  this->setKeyboard(
      static_cast<Amphetype::Layout>(s.value("keyboard_layout", 0).toInt()),
      static_cast<Amphetype::Standard>(
          s.value("keyboard_standard", 0).toInt()));
  this->setScene(this->keyboardScene);
  connect(this, &KeyboardMap::dataChanged, this, &KeyboardMap::addKeys);
  this->setRenderHints(QPainter::Antialiasing);
}

KeyboardMap::~KeyboardMap() { delete this->keyboardScene; }

void KeyboardMap::resizeEvent(QResizeEvent* event) {
  this->fitInView(this->scene()->sceneRect(), Qt::KeepAspectRatio);
}

void KeyboardMap::setKeyboard(Amphetype::Layout layout,
                              Amphetype::Standard standard) {
  this->keyboardLayout = layout;
  this->keyboardStandard = standard;
  this->loadLayout(layout);
  emit dataChanged();
}

void KeyboardMap::setLayout(Amphetype::Layout layout) {
  this->keyboardLayout = layout;
  this->loadLayout(layout);
  emit dataChanged();
}

void KeyboardMap::setStandard(Amphetype::Standard standard) {
  this->keyboardStandard = standard;
  emit dataChanged();
}

void KeyboardMap::setData(const QString& data) {
  this->dataToMap = data;
  emit dataChanged();
}

void KeyboardMap::updateData() {
  Database db;
  this->statsData = db.getKeyFrequency();
  emit dataChanged();
}

void KeyboardMap::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton)
    this->drawKeyboard(Amphetype::Modifier::Shift);
  else
    this->drawKeyboard(Amphetype::Modifier::AltGr);
}

void KeyboardMap::mouseReleaseEvent(QMouseEvent* event) {
  this->drawKeyboard(Amphetype::Modifier::None);
}

void KeyboardMap::addKeys() { this->drawKeyboard(Amphetype::Modifier::None); }

void KeyboardMap::drawKeyboard(Amphetype::Modifier modifier) {
  this->keyboardScene->clear();

  double max = 0;
  double min = std::numeric_limits<double>::max();

  // find the min and max of the data
  foreach (auto const& value, this->statsData) {
    if (value[this->dataToMap] > max) max = value[this->dataToMap].toDouble();
    if (value[this->dataToMap] > 0 && value[this->dataToMap] < min)
      min = value[this->dataToMap].toDouble();
  }
  this->drawKeyboard(this->statsData, modifier, min, max, 0, 0);
}

void KeyboardMap::drawKeyboard(
    const QHash<QChar, QHash<QString, QVariant>>& data,
    Amphetype::Modifier modifier, qreal min, qreal max, qreal x, qreal y) {
  QStringList* keys;
  switch (modifier) {
    case Amphetype::Modifier::Shift:
      keys = &(this->keyboardKeys[1]);
      break;
    case Amphetype::Modifier::AltGr:
      keys = &(this->keyboardKeys[2]);
      break;
    case Amphetype::Modifier::None:
    default:
      keys = &(this->keyboardKeys[0]);
      break;
  }

  for (int row = 0; row < 5; row++) {
    double sum = 0;
    for (int column = 0; column < 15; column++) {
      qreal key_size;

      int column_offset;
      switch (this->keyboardStandard) {
        case Amphetype::Standard::ANSI:
          column_offset = Amphetype::Standards::ansi_offset[row];
          key_size = Amphetype::Standards::ansi_keys[row][column];
          break;
        case Amphetype::Standard::ISO:
          column_offset = Amphetype::Standards::iso_offset[row];
          key_size = Amphetype::Standards::iso_keys[row][column];
          break;
      }
      if (key_size <= 0) continue;

      QChar key;
      if (column - column_offset >= 0 &&
          column - column_offset < (*keys)[row].size())
        key = (*keys)[row][column - column_offset];

      bool display_key_label = (!key.isNull() && key != QChar::Tabulation);

      QBrush brush;
      QPen pen(brush, 0);
      QColor color;
      if (display_key_label) {
        brush.setStyle(Qt::SolidPattern);
        qreal value =
            data.isEmpty() ? -1 : data[key][this->dataToMap].toDouble();

        // set the brush alpha based on the data
        if (value > 0) {
          color.setRgb(174, 86, 238);
          qreal alpha;
          if (this->dataToMap == "accuracy") {
            alpha =
                -1.0 * this->scaleToRange2(value, 255.0, min, max, 100) + 255.0;
          } else if (this->dataToMap == "viscosity") {
            alpha = qMin(255.0, 2 * this->scaleToRange(value, 255.0, min, max));
          } else {
            alpha = this->scaleToRange(value, 255.0, min, max);
          }
          color.setAlpha(alpha);
        } else {
          color.setRgb(255, 255, 255);
        }
        brush.setColor(color);
      } else {
        pen = QPen(QBrush(), 0);
        brush.setColor(QColor(166, 208, 162));
        brush.setStyle(Qt::SolidPattern);
      }

      // adjust the key width for spacing
      qreal widthOffset = (key_size > 2) ? qFloor(key_size) : 0;
      if (widthOffset && (column == 0 || sum + key_size == 15))
        widthOffset -= 1;

      // add the key
      QPainterPath path;
      path.addRoundedRect(
          QRectF(x + (this->keySize * sum) + (qFloor(sum) * this->keySpacing),
                 y + row * this->keySize + (row * this->keySpacing),
                 this->keySize * key_size + widthOffset * this->keySpacing,
                 this->keySize),
          this->keySize / 4, this->keySize / 4);

      auto keyRect = this->keyboardScene->addPath(path, pen, brush);

      if (display_key_label) {
        // add the text on the key
        QString display = (key == QChar::Space) ? "space" : QString(key);
        QGraphicsTextItem* textItem = this->keyboardScene->addText(display);
        textItem->setPos(keyRect->boundingRect().center() -
                         QPointF(textItem->boundingRect().width() / 2,
                                 textItem->boundingRect().height() / 2));
      }

      sum += key_size;
    }
  }
}

qreal KeyboardMap::scaleToRange(qreal x, qreal range, qreal min, qreal max) {
  return range - range * (1.0 - qAbs(x - min) / qAbs(max - min));
}

qreal KeyboardMap::scaleToRange2(qreal x, qreal range, qreal min, qreal max,
                                 qreal factor) {
  return range *
         ((qPow(factor, qAbs(x - min) / qAbs(max - min)) - 1) / (factor - 1));
}

void KeyboardMap::loadLayout(Amphetype::Layout layout) {
  QString filename;
  switch (this->keyboardLayout) {
    case Amphetype::Layout::QWERTY:
      filename = ":/keyboard_layouts/qwerty.layout";
      break;
    case Amphetype::Layout::QWERTZ:
      filename = ":/keyboard_layouts/qwertz.layout";
      break;
    case Amphetype::Layout::AZERTY:
      filename = ":/keyboard_layouts/azerty.layout";
      break;
    case Amphetype::Layout::WORKMAN:
      filename = ":/keyboard_layouts/workman.layout";
      break;
    case Amphetype::Layout::COLEMAK:
      filename = ":/keyboard_layouts/colemak.layout";
      break;
    case Amphetype::Layout::DVORAK:
      filename = ":/keyboard_layouts/dvorak.layout";
      break;
  }

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    this->keyboardKeys = QList<QStringList>();

  QList<QStringList> output;
  QStringList keys_standard;
  QStringList keys_shift;
  QStringList keys_altgr;

  int line_number = 0;
  while (!file.atEnd()) {
    int row = line_number % 3;
    QByteArray line = file.readLine();
    line = line.replace('\n', "");
    if (row == 0)
      keys_standard << line;
    else if (row == 1)
      keys_shift << line;
    else if (row == 2)
      keys_altgr << line;
    line_number++;
  }
  output << keys_standard << keys_shift << keys_altgr;
  this->keyboardKeys = output;
}
