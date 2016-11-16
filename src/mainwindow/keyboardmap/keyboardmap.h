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

#ifndef SRC_ANALYSIS_KEYBOARDMAP_H_
#define SRC_ANALYSIS_KEYBOARDMAP_H_

#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSize>
#include <QStringList>

#include <map>
#include <memory>

#include "database/db.h"
#include "defs.h"

class KeyboardMap : public QGraphicsView {
  Q_OBJECT
  Q_PROPERTY(QColor foregroundColor MEMBER foregroundColor NOTIFY colorChanged)
  Q_PROPERTY(QColor deadKeyColor MEMBER deadKeyColor NOTIFY colorChanged)
  Q_PROPERTY(QColor mapColor MEMBER mapColor NOTIFY colorChanged)

 public:
  explicit KeyboardMap(QWidget* parent = Q_NULLPTR);
  ~KeyboardMap();

 public slots:
  void onProfileChange();
  void setKeyboard(amphetype::Layout, amphetype::Standard);
  void setLayout(amphetype::Layout);
  void setStandard(amphetype::Standard);
  void setData(const QString&);
  void addKeys();
  void drawKeyboard(amphetype::Modifier modifier = amphetype::Modifier::None);
  void updateData();

 protected:
  void resizeEvent(QResizeEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void mouseReleaseEvent(QMouseEvent*) override;

 private:
  qreal scaleToRange(qreal, qreal, qreal min = 0, qreal max = 1);
  qreal scaleToRange2(qreal, qreal, qreal min = 0, qreal max = 1,
                      qreal factor = 10);
  void draw_keyboard(amphetype::Modifier modifier, qreal min = 0.0,
                     qreal max = 100.0, qreal x = 0, qreal y = 0);
  void loadLayout(amphetype::Layout);

 private:
  std::map<QChar, std::map<QString, QVariant>> data_;
  std::unique_ptr<Database> db_;
  QGraphicsScene keyboard_scene_;
  amphetype::Layout keyboard_layout_;
  amphetype::Standard keyboard_standard_;
  QList<QStringList> keyboard_keys_;
  int key_spacing_;
  int key_size_;
  QString display_;
  QColor foregroundColor;
  QColor deadKeyColor;
  QColor mapColor;

 signals:
  void dataChanged();
  void colorChanged();
};

#endif  // SRC_ANALYSIS_KEYBOARDMAP_H_
