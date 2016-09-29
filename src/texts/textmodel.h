#ifndef SRC_TEXTS_TEXTMODEL_H_
#define SRC_TEXTS_TEXTMODEL_H_

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QObject>
#include <QVariant>
#include <QVector>

#include "database/db.h"

class TextItem {
  friend class TextModel;

 public:
  TextItem(Database *, int text_id, const QString &text, int length,
           int results, double wpm, int dis);
  ~TextItem();
  void refresh();
  void deleteFromDb();
  void enable();
  void disable();
  QString getFullText();

 private:
  Database *db_;
  int id_;
  QString text_;
  int length_;
  int results_;
  double wpm_;
  int dis_;
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

  void refresh();
  void refreshText(const QModelIndex &index);

 private:
  Database db_;
  QVector<TextItem *> items;
  int source;
  int page_size;
  int total_size;
  int current_page;
};

#endif  // SRC_TEXTS_TEXTMODEL_H_
