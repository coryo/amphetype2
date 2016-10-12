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

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSize>
#include <QStringList>
#include <QColor>

#include "defs.h"

class KeyboardMap : public QGraphicsView {
  Q_OBJECT
  Q_PROPERTY(QColor foregroundColor MEMBER foregroundColor NOTIFY colorChanged)
  Q_PROPERTY(QColor deadKeyColor MEMBER deadKeyColor NOTIFY colorChanged)
  Q_PROPERTY(QColor mapColor MEMBER mapColor NOTIFY colorChanged)

 public:
  explicit KeyboardMap(QWidget* parent = Q_NULLPTR);
  ~KeyboardMap();
  void setKeyboard(Amphetype::Layout, Amphetype::Standard);
  void setLayout(Amphetype::Layout);
  void setStandard(Amphetype::Standard);
  void setData(const QString&);
  void addKeys();
  void drawKeyboard(Amphetype::Modifier modifier = Amphetype::Modifier::None);
  void updateData();

 protected:
  void resizeEvent(QResizeEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);

 private:
  qreal scaleToRange(qreal, qreal, qreal min = 0, qreal max = 1);
  qreal scaleToRange2(qreal, qreal, qreal min = 0, qreal max = 1,
                      qreal factor = 10);
  void drawKeyboard(const QHash<QChar, QHash<QString, QVariant>>&,
                    Amphetype::Modifier modifier, qreal min = 0.0,
                    qreal max = 100.0, qreal x = 0, qreal y = 0);
  void loadLayout(Amphetype::Layout);

 private:
  QHash<QChar, QHash<QString, QVariant>> statsData;
  QGraphicsScene* keyboardScene;
  Amphetype::Layout keyboardLayout;
  Amphetype::Standard keyboardStandard;
  QList<QStringList> keyboardKeys;
  int keySpacing;
  int keySize;
  QString dataToMap;
  QColor foregroundColor;
  QColor deadKeyColor;
  QColor mapColor;

 signals:
  void dataChanged();
  void colorChanged();
};

#endif  // SRC_ANALYSIS_KEYBOARDMAP_H_
