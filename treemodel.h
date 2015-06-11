#ifndef AMPHMODEL_H
#define AMPHMODEL_H

#include <QAbstractItemModel>
class QStringList;

class TreeItem
{
public:
        explicit TreeItem(const QList<QVariant>& data,
                          TreeItem* parentItem = 0);
        ~TreeItem();

        void appendChild(TreeItem* child);

        TreeItem* child(int row);
        int childCount() const;
        int columnCount() const;
        QVariant data(int column) const;
        int row() const;
        TreeItem* parentItem();

private:
        QList<TreeItem*> m_childItems;
        QList<QVariant> m_itemData;
        TreeItem* m_parentItem;
};

class TreeModel : public QAbstractItemModel
{
        Q_OBJECT
public:
        explicit TreeModel(const QString& data, const QList<QVariant>& cols,
                           QObject* parent = 0);
        ~TreeModel();

        QVariant data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        QVariant getId(const QModelIndex& index, int role) const;
        Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
        QModelIndex
        index(int row, int column,
              const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        QModelIndex parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        int rowCount(const QModelIndex& parent = QModelIndex()) const
                Q_DECL_OVERRIDE;
        int columnCount(const QModelIndex& parent = QModelIndex()) const
                Q_DECL_OVERRIDE;
        void populateData(const QModelIndex&, const QList<QStringList>&);

private:
        void setupModelData(const QStringList& lines, TreeItem* parent);

        TreeItem* rootItem;
};

#endif // AMPHMODEL_H
