#ifndef SRC_ANALYSIS_KEYBOARDMAP_H_
#define SRC_ANALYSIS_KEYBOARDMAP_H_

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QList>
#include <QStringList>
#include <QSize>
#include <QMouseEvent>

#include "defs.h"

class KeyboardMap : public QGraphicsView {
  Q_OBJECT

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
  QSize sizeHint();
  QSize minimumSizeHint();

 private:
  QHash<QChar, QHash<QString, QVariant>> statsData;
  QGraphicsScene* keyboardScene;
  Amphetype::Layout keyboardLayout;
  Amphetype::Standard keyboardStandard;
  QList<QStringList> keyboardKeys;
  int keySpacing;
  int keySize;
  QString dataToMap;

  void mousePressEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);

  qreal scaleToRange(qreal, qreal, qreal min = 0, qreal max = 1);
  qreal scaleToRange2(qreal, qreal, qreal min = 0, qreal max = 1,
                      qreal factor = 10);

  void drawKeyboard(const QHash<QChar, QHash<QString, QVariant>>&,
                    Amphetype::Modifier modifier, qreal min = 0.0,
                    qreal max = 100.0, qreal x = 0, qreal y = 0);

  void loadLayout(Amphetype::Layout);

 signals:
  void dataChanged();
};

#endif  // SRC_ANALYSIS_KEYBOARDMAP_H_
