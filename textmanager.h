#ifndef TEXTMANAGER_H
#define TEXTMANAGER_H

#include <QWidget>

class QModelIndex;
class LessonMinerController;
class Text;
class TreeModel;

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
        TreeModel* model;
        QStringList files;
        LessonMinerController* lmc;
        bool refreshed;

signals:
        void setText(Text*);
        void progress(int);
        void gotoTab(int);

private slots:
        void nextText();
        void addFiles();
        void refreshSources();
        void doubleClicked(const QModelIndex&);
        void resizeColumns();
        void tabActive(int);
        void processNextFile();
        void changeSelectMethod(int);
        void populateTexts(const QModelIndex&);
};

#endif // TEXTMANAGER_H
