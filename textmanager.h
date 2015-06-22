#ifndef TEXTMANAGER_H
#define TEXTMANAGER_H

#include <QWidget>

class QModelIndex;
class LessonMinerController;
class Text;
class QStandardItemModel;

namespace Ui
{
class TextManager;
}

class TextManager : public QWidget
{
        Q_OBJECT

public:
        explicit TextManager(QWidget* parent = 0);
        ~TextManager();

private:
        Ui::TextManager* ui;
        QStringList files;
        LessonMinerController* lmc;
        bool refreshed;
        QStandardItemModel* sourcesModel;
        QStandardItemModel* textsModel;

signals:
        void setText(Text*);
        void progress(int);
        void gotoTab(int);

private slots:
        void nextText(Text*);
        void addFiles();
        void refreshSources();
        void doubleClicked(const QModelIndex&);
        void tabActive(int);
        void processNextFile();
        void changeSelectMethod(int);
        void populateTexts(const QModelIndex&);
        void addSource();
        void deleteSource();
};

#endif // TEXTMANAGER_H
