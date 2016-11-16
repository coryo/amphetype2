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
      key_spacing_(3),
      key_size_(24),
      display_("accuracy") {
  QSettings s;
  setKeyboard(
      static_cast<amphetype::Layout>(s.value("keyboard_layout", 0).toInt()),
      static_cast<amphetype::Standard>(
          s.value("keyboard_standard", 0).toInt()));
  setScene(&keyboard_scene_);
  connect(this, &KeyboardMap::colorChanged, this, &KeyboardMap::addKeys);
  setRenderHints(QPainter::Antialiasing);
}

KeyboardMap::~KeyboardMap() {}

void KeyboardMap::onProfileChange() {
  db_.reset(new Database);
  updateData();
}

void KeyboardMap::resizeEvent(QResizeEvent* event) {
  fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void KeyboardMap::setKeyboard(amphetype::Layout layout,
                              amphetype::Standard standard) {
  keyboard_layout_ = layout;
  keyboard_standard_ = standard;
  loadLayout(layout);
  addKeys();
}

void KeyboardMap::setLayout(amphetype::Layout layout) {
  keyboard_layout_ = layout;
  loadLayout(layout);
  addKeys();
}

void KeyboardMap::setStandard(amphetype::Standard standard) {
  keyboard_standard_ = standard;
  addKeys();
}

void KeyboardMap::setData(const QString& data) {
  display_ = data;
  addKeys();
}

void KeyboardMap::updateData() {
  data_ = db_->getKeyFrequency();
  addKeys();
}

void KeyboardMap::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton)
    drawKeyboard(amphetype::Modifier::Shift);
  else
    drawKeyboard(amphetype::Modifier::AltGr);
}

void KeyboardMap::mouseReleaseEvent(QMouseEvent* event) {
  drawKeyboard(amphetype::Modifier::None);
}

void KeyboardMap::addKeys() { drawKeyboard(amphetype::Modifier::None); }

void KeyboardMap::drawKeyboard(amphetype::Modifier modifier) {
  keyboard_scene_.clear();

  double max = 0;
  double min = std::numeric_limits<double>::max();

  // find the min and max of the data
  for (const auto& pair : data_) {
    if (pair.second.at(display_) > max)
      max = pair.second.at(display_).toDouble();
    if (pair.second.at(display_) > 0 && pair.second.at(display_) < min)
      min = pair.second.at(display_).toDouble();
  }
  draw_keyboard(modifier, min, max, 0, 0);
}

void KeyboardMap::draw_keyboard(amphetype::Modifier modifier, qreal min,
                                qreal max, qreal x, qreal y) {
  QStringList* keys;
  switch (modifier) {
    case amphetype::Modifier::Shift:
      keys = &(keyboard_keys_[1]);
      break;
    case amphetype::Modifier::AltGr:
      keys = &(keyboard_keys_[2]);
      break;
    case amphetype::Modifier::None:
    default:
      keys = &(keyboard_keys_[0]);
      break;
  }

  for (int row = 0; row < 5; row++) {
    double sum = 0;
    for (int column = 0; column < 15; column++) {
      qreal key_size;

      int column_offset;
      switch (keyboard_standard_) {
        case amphetype::Standard::ANSI:
          column_offset = amphetype::standards::ansi_offset[row];
          key_size = amphetype::standards::ansi_keys[row][column];
          break;
        case amphetype::Standard::ISO:
          column_offset = amphetype::standards::iso_offset[row];
          key_size = amphetype::standards::iso_keys[row][column];
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
      QColor color(mapColor);
      if (display_key_label) {
        brush.setStyle(Qt::SolidPattern);
        qreal value = data_.empty() ? -1 : data_[key][display_].toDouble();

        // set the brush alpha based on the data
        if (value > 0) {
          qreal alpha;
          if (display_ == "accuracy") {
            alpha = -1.0 * scaleToRange2(value, 255.0, min, max, 100) + 255.0;
          } else if (display_ == "viscosity") {
            alpha = qMin(255.0, 2 * scaleToRange(value, 255.0, min, max));
          } else {
            alpha = scaleToRange(value, 255.0, min, max);
          }
          color.setAlpha(alpha);
        } else {
          color.setAlpha(0);
        }
        brush.setColor(color);
      } else {
        pen = QPen(QBrush(), 0);
        brush.setColor(deadKeyColor);
        brush.setStyle(Qt::SolidPattern);
      }

      // adjust the key width for spacing
      qreal widthOffset = (key_size > 2) ? qFloor(key_size) : 0;
      if (widthOffset && (column == 0 || sum + key_size == 15))
        widthOffset -= 1;

      // add the key
      QPainterPath path;
      path.addRoundedRect(
          QRectF(x + (key_size_ * sum) + (qFloor(sum) * key_spacing_),
                 y + row * key_size_ + (row * key_spacing_),
                 key_size_ * key_size + widthOffset * key_spacing_, key_size_),
          key_size_ / 4, key_size_ / 4);

      auto keyRect = keyboard_scene_.addPath(path, pen, brush);

      if (display_key_label) {
        // add the text on the key
        QString display = (key == QChar::Space) ? "space" : QString(key);
        auto* textItem = keyboard_scene_.addText(display);
        textItem->setDefaultTextColor(foregroundColor);
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

void KeyboardMap::loadLayout(amphetype::Layout layout) {
  QString filename;
  switch (keyboard_layout_) {
    case amphetype::Layout::QWERTY:
      filename = ":/keyboard_layouts/qwerty.layout";
      break;
    case amphetype::Layout::QWERTZ:
      filename = ":/keyboard_layouts/qwertz.layout";
      break;
    case amphetype::Layout::AZERTY:
      filename = ":/keyboard_layouts/azerty.layout";
      break;
    case amphetype::Layout::WORKMAN:
      filename = ":/keyboard_layouts/workman.layout";
      break;
    case amphetype::Layout::COLEMAK:
      filename = ":/keyboard_layouts/colemak.layout";
      break;
    case amphetype::Layout::DVORAK:
      filename = ":/keyboard_layouts/dvorak.layout";
      break;
  }

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    keyboard_keys_ = QList<QStringList>();

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
  keyboard_keys_ = output;
}
