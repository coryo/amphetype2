#ifndef SRC_TEXTS_TEXT_H_
#define SRC_TEXTS_TEXT_H_

#include <QByteArray>
#include <QString>

class Text {
 public:
  Text();
  explicit Text(Text*);
  Text(int, int, const QString&, int type = 0);
  Text(int, int, const QString&, const QString&, int, int type = 0);
  ~Text();

  int getId() const;
  int getSource() const;
  const QString& getText() const;
  const QString& getSourceName() const;
  int getTextNumber() const;
  int getType() const;

 private:
  int id;
  int source;
  QString text;
  QString sourceName;
  int textNumber;
  int type;
};

#endif  // SRC_TEXTS_TEXT_H_
