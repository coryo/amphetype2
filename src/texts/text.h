#ifndef SRC_TEXTS_TEXT_H_
#define SRC_TEXTS_TEXT_H_

#include <QByteArray>
#include <QString>

class Text {
 public:
  Text();
  explicit Text(Text*);
  Text(const QByteArray&, int, const QString&, int type = 0);
  Text(const QByteArray&, int, const QString&, const QString&, int,
       int type = 0);
  ~Text();

  const QByteArray& getId() const;
  int getSource() const;
  const QString& getText() const;
  const QString& getSourceName() const;
  int getTextNumber() const;
  int getType() const;

 private:
  QByteArray id;
  int source;
  QString text;
  QString sourceName;
  int textNumber;
  int type;
};

#endif  // SRC_TEXTS_TEXT_H_
