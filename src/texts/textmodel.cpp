#include "texts/textmodel.h"

#include <QsLog.h>

#include "database/db.h"

TextItem::TextItem(Database* db, int text_id, const QString& text, int length,
                   int results, double wpm, int dis)
    : db_(db),
      id_(text_id),
      text_(text),
      length_(length),
      results_(results),
      wpm_(wpm),
      dis_(dis) {}

TextItem::~TextItem() {}

void TextItem::refresh() {
  auto data = db_->getTextData(id_);
  id_ = data[0].toInt();
  text_ = data[1].toString().simplified();
  length_ = data[2].toInt();
  results_ = data[3].toInt();
  wpm_ = data[4].toDouble();
  dis_ = data[5].toInt();
}

void TextItem::deleteFromDb() {
  QList<int> ids;
  ids << id_;
  db_->deleteText(ids);
}

void TextItem::enable() {}

void TextItem::disable() {}

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
        return item->text_;
      case 1:
        return item->length_;
      case 2:
        return item->results_;
      case 3:
        return item->wpm_;
      case 4:
        return item->dis_;
    }
  } else if (role == Qt::UserRole) {
    return item->id_;
  }
  return QVariant();
}

bool TextModel::canFetchMore(const QModelIndex& parent) const {
  return (this->source >= 0 && this->items.size() < this->total_size);
}

void TextModel::fetchMore(const QModelIndex& parent) {
  QList<QVariantList> rows =
      db_.getTextsData(this->source, this->current_page, this->page_size);
  this->beginInsertRows(QModelIndex(), this->rowCount(),
                        this->rowCount() + rows.size() - 1);
  for (const auto& row : rows) {
    TextItem* item = new TextItem(
        &db_, row[0].toInt(), row[1].toString().simplified(), row[2].toInt(),
        row[3].toInt(), row[4].toDouble(), row[5].toInt());

    this->items.append(item);
  }
  this->current_page++;
  this->endInsertRows();
}

int TextModel::getSource() { return this->source; }

void TextModel::refresh() { this->setSource(this->source); }

void TextModel::refreshText(const QModelIndex& index) {
  if (!index.isValid()) return;

  if (index.row() >= this->rowCount()) return;

  TextItem* item = this->items[index.row()];
  item->refresh();
  emit dataChanged(index, this->index(index.row(), this->columnCount()));
}

void TextModel::setSource(int source) {
  this->beginResetModel();

  this->source = source;
  this->current_page = 0;
  this->total_size = db_.getTextsCount(this->source);

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
