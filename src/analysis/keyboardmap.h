#ifndef SRC_ANALYSIS_KEYBOARDMAP_H_
#define SRC_ANALYSIS_KEYBOARDMAP_H_

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QList>
#include <QStringList>

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
  void updateData();

 private:
  QHash<QChar, QHash<QString, QVariant>> statsData;
  QGraphicsScene* keyboardScene;
  KeyboardLayout keyboardLayout;
  KeyboardStandard keyboardStandard;
  QList<QStringList> keyboardKeys;
  int keySpacing;
  int keySize;
  QString dataToMap;

  qreal scaleToRange(qreal, qreal, qreal min = 0, qreal max = 1);
  qreal scaleToRange2(qreal, qreal, qreal min = 0, qreal max = 1, qreal factor = 10);

  void drawKeyboard(
    const QHash<QChar, QHash<QString, QVariant>>&,
    bool shift,
    qreal min = 0.0,
    qreal max = 100.0,
    qreal x = 0,
    qreal y = 0);

  void loadLayout(KeyboardLayout);

 signals:
  void dataChanged();

};

#endif  // SRC_ANALYSIS_KEYBOARDMAP_H_
