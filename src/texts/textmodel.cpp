#include "texts/textmodel.h"

#include <QsLog.h>

#include "database/db.h"

TextItem::TextItem(int text_id, const QString& text, int length, int results,
                   double wpm, int dis)
    : text_id(text_id),
      text(text),
      length(length),
      results(results),
      wpm(wpm),
      dis(dis) {}

TextItem::~TextItem() {}

TextModel::TextModel(QObject* parent) : QAbstractTableModel(parent) {
  page_size = 100;
  current_page = 0;
  source = -1;
}

TextModel::~TextModel() {
  for (TextItem* item : this->items) {
    delete item;
  }
}

QVariant TextModel::headerData(int section, Qt::Orientation orientation,
                               int role) const {
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal) {
      switch (section) {
        case 0:
          return QString("text");
        case 1:
          return QString("length");
        case 2:
          return QString("results");
        case 3:
          return QString("wpm");
        case 4:
          return QString("dis");
      }
    }
  }
  return QVariant();
}

int TextModel::rowCount(const QModelIndex& parent) const {
  return this->items.size();
}

int TextModel::columnCount(const QModelIndex& parent) const { return 4; }

QVariant TextModel::data(const QModelIndex& index, int role) const {
  int row = index.row();
  int column = index.column();
  TextItem* item = this->items[row];

  if (role == Qt::DisplayRole) {
    switch (column) {
      case 0:
        return item->text;
        break;
      case 1:
        return item->length;
        break;
      case 2:
        return item->results;
        break;
      case 3:
        return item->wpm;
        break;
      case 4:
        return item->dis;
        break;
    }
  } else if (role == Qt::UserRole) {
    return item->text_id;
  }
  return QVariant();
}

bool TextModel::canFetchMore(const QModelIndex& parent) const {
  return (this->source >= 0 && this->items.size() < this->total_size);
}

void TextModel::fetchMore(const QModelIndex& parent) {
  Database db;
  QList<QVariantList> rows =
      db.getTextsData(this->source, this->current_page, this->page_size);
  this->beginInsertRows(QModelIndex(), this->rowCount(),
                        this->rowCount() + rows.size() - 1);
  for (const auto& row : rows) {
    TextItem* item =
        new TextItem(row[0].toInt(), row[1].toString().simplified(), row[2].toInt(),
                     row[3].toInt(), row[4].toDouble(), row[5].toInt());

    this->items.append(item);
  }
  this->current_page++;
  this->endInsertRows();
}

int TextModel::getSource() { return this->source; }

void TextModel::setSource(int source) {
  this->beginResetModel();

  this->source = source;
  this->current_page = 0;
  Database db;
  this->total_size = db.getTextsCount(this->source);

  for (TextItem* item : this->items) delete item;
  this->items.clear();

  this->endResetModel();
}

void TextModel::removeIndexes(const QModelIndexList& indexes) {
  if (indexes.isEmpty()) return;

  QList<int> rows;

  for (const auto& index : indexes) rows << index.row();
  qSort(rows.begin(), rows.end());

  for (int i = rows.size() - 1; i >= 0; i--) {
    int row = rows[i];
    this->beginRemoveRows(QModelIndex(), row, row);
    TextItem* item = this->items.takeAt(row);
    if (item) delete item;
    this->total_size--;
    this->endRemoveRows();
  }
}

void TextModel::clear() {
  this->beginResetModel();
  this->source = -1;
  this->current_page = 0;
  for (TextItem* item : this->items) delete item;
  this->items.clear();
  this->endResetModel();
}
