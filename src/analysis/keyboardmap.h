#ifndef SRC_ANALYSIS_KEYBOARDMAP_H_
#define SRC_ANALYSIS_KEYBOARDMAP_H_

#include <QGraphicsView>
#include <QGraphicsScene>

#include "defs.h"

class KeyboardMap : public QGraphicsView {
  Q_OBJECT

 public:
  explicit KeyboardMap(QWidget *parent = Q_NULLPTR);
  ~KeyboardMap();
  void setKeyboard(KeyboardLayout, KeyboardStandard);
  void setLayout(KeyboardLayout);
  void setStandard(KeyboardStandard);
  void setData(const QString&);
  void addKeys();

 private:
  QGraphicsScene* keyboardScene;
  KeyboardLayout keyboardLayout;
  KeyboardStandard keyboardStandard;
  int keySpacing;
  int keySize;
  QString dataToMap;

  qreal scaleToRange(qreal, qreal, qreal min = 0, qreal max = 1);
  qreal scaleToRange2(qreal, qreal, qreal min = 0, qreal max = 1, qreal factor = 10);

 signals:
  void dataChanged();

};

#endif  // SRC_ANALYSIS_KEYBOARDMAP_H_
