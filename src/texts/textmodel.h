#ifndef SRC_TEXTS_TEXTMODEL_H_
#define SRC_TEXTS_TEXTMODEL_H_

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QObject>
#include <QVector>
#include <QVariant>

class TextItem {
  friend class TextModel;

 public:
  TextItem(int text_id, const QString &text, int length, int results,
           double wpm, int dis);
  ~TextItem();

  QString getFullText();

 private:
  int text_id;
  QString text;
  int length;
  int results;
  double wpm;
  int dis;
};

class TextModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  explicit TextModel(QObject *parent = Q_NULLPTR);
  ~TextModel();

  void clear();

  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  bool canFetchMore(const QModelIndex &parent) const;
  void fetchMore(const QModelIndex &parent);

  void removeIndexes(const QModelIndexList &indexes);

  void setSource(int);
  int getSource();

 private:
  QVector<TextItem *> items;
  int source;
  int page_size;
  int total_size;
  int current_page;
};

#endif  // SRC_TEXTS_TEXTMODEL_H_
