#include "treemodel.h"
#include <QAbstractItemModel>
#include <QStringList>

TreeItem::TreeItem(const QList<QVariant>& data, TreeItem* parent)
{
        m_parentItem = parent;
        m_itemData = data;
}
TreeItem::~TreeItem() { qDeleteAll(m_childItems); }
void TreeItem::appendChild(TreeItem* item) { m_childItems.append(item); }
TreeItem* TreeItem::child(int row) { return m_childItems.value(row); }
int TreeItem::childCount() const { return m_childItems.count(); }
int TreeItem::row() const
{
        if (m_parentItem)
                return m_parentItem->m_childItems.indexOf(
                        const_cast<TreeItem*>(this));

        return 0;
}
int TreeItem::columnCount() const { return m_itemData.count(); }
QVariant TreeItem::data(int column) const { return m_itemData.value(column); }
TreeItem* TreeItem::parentItem() { return m_parentItem; }

TreeModel::TreeModel(const QString& data, const QList<QVariant>& cols,
                     QObject* parent)
        : QAbstractItemModel(parent)
{
        // QList<QVariant> rootData;
        // rootData << "id" << "Source" << "Length" << "Results" << "WPM" <<
        // "Enabled";
        rootItem = new TreeItem(cols);
        setupModelData(data.split(QString("\n")), rootItem);
}
TreeModel::~TreeModel() { delete rootItem; }
QModelIndex TreeModel::index(int row, int column,
                             const QModelIndex& parent) const
{
        if (!hasIndex(row, column, parent))
                return QModelIndex();

        TreeItem* parentItem;

        if (!parent.isValid())
                parentItem = rootItem;
        else
                parentItem = static_cast<TreeItem*>(parent.internalPointer());

        TreeItem* childItem = parentItem->child(row);
        if (childItem)
                return createIndex(row, column, childItem);
        else
                return QModelIndex();
}
QModelIndex TreeModel::parent(const QModelIndex& index) const
{
        if (!index.isValid())
                return QModelIndex();

        TreeItem* childItem = static_cast<TreeItem*>(index.internalPointer());
        TreeItem* parentItem = childItem->parentItem();

        if (parentItem == rootItem)
                return QModelIndex();

        return createIndex(parentItem->row(), 0, parentItem);
}
int TreeModel::rowCount(const QModelIndex& parent) const
{
        TreeItem* parentItem;
        if (parent.column() > 0)
                return 0;

        if (!parent.isValid())
                parentItem = rootItem;
        else
                parentItem = static_cast<TreeItem*>(parent.internalPointer());

        return parentItem->childCount();
}
int TreeModel::columnCount(const QModelIndex& parent) const
{
        if (parent.isValid())
                return static_cast<TreeItem*>(parent.internalPointer())
                        ->columnCount();
        else
                return rootItem->columnCount();
}
QVariant TreeModel::data(const QModelIndex& index, int role) const
{
        if (!index.isValid())
                return QVariant();

        if (role != Qt::DisplayRole)
                return QVariant();

        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

        return item->data(index.column()); // index.column());
}
QVariant TreeModel::getId(const QModelIndex& index, int role) const
{
        if (!index.isValid())
                return QVariant();

        if (role != Qt::DisplayRole)
                return QVariant();

        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

        return item->data(0); // index.column());
}
Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
{
        if (!index.isValid())
                return 0;

        return QAbstractItemModel::flags(index);
}
QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
                return rootItem->data(section);

        return QVariant();
}

void TreeModel::setupModelData(const QStringList& lines, TreeItem* parent)
{
        QList<TreeItem*> parents;
        QList<int> indentations;
        parents << parent;
        indentations << 0;

        int number = 0;

        while (number < lines.count()) {
                int position = 0;
                while (position < lines[number].length()) {
                        if (lines[number].mid(position, 1) != " ")
                                break;
                        position++;
                }

                QString lineData = lines[number].mid(position).trimmed();

                if (!lineData.isEmpty()) {
                        // Read the column data from the rest of the line.
                        QStringList columnStrings =
                                lineData.split("\t", QString::SkipEmptyParts);
                        QList<QVariant> columnData;
                        for (int column = 0; column < columnStrings.count();
                             ++column)
                                columnData << columnStrings[column];

                        if (position > indentations.last()) {
                                // The last child of the current parent is now
                                // the new parent
                                // unless the current parent has no children.

                                if (parents.last()->childCount() > 0) {
                                        parents << parents.last()->child(
                                                parents.last()->childCount() -
                                                1);
                                        indentations << position;
                                }
                        } else {
                                while (position < indentations.last() &&
                                       parents.count() > 0) {
                                        parents.pop_back();
                                        indentations.pop_back();
                                }
                        }

                        // Append a new item to the current parent's list of
                        // children.
                        parents.last()->appendChild(
                                new TreeItem(columnData, parents.last()));
                }

                ++number;
        }
}

void TreeModel::populateData(const QModelIndex& index, const QList<QStringList>& data)
{
        TreeItem* parent = static_cast<TreeItem*>(index.internalPointer());

        for (QStringList row : data) {
                QList<QVariant> columnData;
                for (QString& item : row) {
                        columnData << item;
                }
                parent->appendChild(new TreeItem(columnData, parent));
        }
}