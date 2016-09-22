#include "generators/traininggenerator.h"

#include <QStringList>


TrainingGenerator::TrainingGenerator(KeyboardLayout layout,
                                     KeyboardStandard standard, QObject* parent)
    : QObject(parent), standard(standard), layout(layout) {
    switch (layout) {
        case KeyboardLayout::QWERTY:
            rows.append("qwertyuiop");
            rows.append("asdfghjkl;");
            rows.append("zxcvbnm,./");
            break;
        case KeyboardLayout::QWERTZ:
            rows.append("qwertzuiopü");
            rows.append("asdfghjklöä");
            rows.append(">yxcvbnm,.-");
            this->standard = KeyboardStandard::ISO;
            break;
        case KeyboardLayout::AZERTY:
            rows.append("azertyuiop");
            rows.append("qsdfghjklm");
            rows.append(">wxcvbn,;:!");
            this->standard = KeyboardStandard::ISO;
            break;
        case KeyboardLayout::COLEMAK:
            rows.append("qwfpgjluy;");
            rows.append("arstdhneio'");
            rows.append("zxcvbkm,./");
            break;
        case KeyboardLayout::DVORAK:
            rows.append("',.pyfgcrl");
            rows.append("aoeuidhtns");
            rows.append(";qjkxbmwvz");
        case KeyboardLayout::WORKMAN:
            rows.append("qdrwbjfup;");
            rows.append("ashtgyneoi");
            rows.append("zxmcvkl,./");
        default:
            rows.append("qwertyuiop");
            rows.append("asdfghjkl;");
            rows.append("zxcvbnm,./");
    }
}
TrainingGenerator::TrainingGenerator(QString& u, QString& m, QString& l,
                                     KeyboardStandard standard, QObject* parent)
    : QObject(parent), standard(standard), layout(KeyboardLayout::QWERTY) {
    rows.append(u);
    rows.append(m);
    rows.append(l);

    if (l.size() == 10) {
        this->standard = KeyboardStandard::ANSI;
    } else if (l.size() == 11) {
        this->standard = KeyboardStandard::ISO;
    }
}

QList<QStringList>* TrainingGenerator::generate(int lessonsPerFingerGroup,
                                                int maxLength, KeyboardRow r) {
    QString& homeRow = rows[1];

    int rowLength  = homeRow.size();
    int startIndex = 0;

    // ISO has an extra key on bottom row before the main typing area
    if (standard == KeyboardStandard::ISO && r == KeyboardRow::LOWER)
        startIndex = 1;

    // innerindex, index, m, a, c
    QStringList keys_by_finger;
    for (int i = 4; i >= 0; --i) {
        QString finger_keys(homeRow[startIndex + i]);
        finger_keys += homeRow[startIndex + (9-i)];
        keys_by_finger.append(finger_keys);
    }

    QStringList stages;
    stages.append(keys_by_finger.value(1));
    stages.append(keys_by_finger.value(2));
    stages.append(stages.value(0) + stages.value(1));
    stages.append(keys_by_finger.value(3));
    stages.append(stages.value(2) + stages.value(3));
    stages.append(keys_by_finger.value(4));
    stages.append(stages.value(4) + stages.value(5));
    stages.append(keys_by_finger.value(0));
    stages.append(stages.value(5) + stages.value(6));
    // extra keys on the right
    if (rowLength > 9) {
        int extraKeys = rowLength - 9;
        keys_by_finger.append(homeRow.right(extraKeys));
        stages.append(keys_by_finger.value(4) + keys_by_finger.value(5));
    }

    stages.append(charactersPerFinger(Finger::INDEX).left(4));
    stages.append(charactersPerFinger(Finger::MIDDLE).left(4));
    stages.append(stages.value(stages.size()-1) +
                  stages.value(stages.size()-2));
    stages.append(charactersPerFinger(Finger::RING).left(4));
    stages.append(stages.value(stages.size()-1) +
                  stages.value(stages.size()-2));
    stages.append(charactersPerFinger(Finger::PINKY).left(4));
    stages.append(stages.value(stages.size()-1) +
                  stages.value(stages.size()-2));
    // stages.append(charactersPerFinger(Finger::PINKY_EXTRA).left(4));

    stages.append(charactersPerFinger(Finger::INDEX).right(4));
    stages.append(charactersPerFinger(Finger::MIDDLE).right(4));
    stages.append(stages.value(stages.size()-1) +
                  stages.value(stages.size()-2));
    stages.append(charactersPerFinger(Finger::RING).right(4));
    stages.append(stages.value(stages.size()-1) +
                  stages.value(stages.size()-2));
    stages.append(charactersPerFinger(Finger::PINKY).right(4));
    stages.append(stages.value(stages.size()-1) +
                  stages.value(stages.size()-2));
    // stages.append(charactersPerFinger(Finger::PINKY_EXTRA).right(4));

    stages.append(charactersPerFinger(Finger::INDEX));
    stages.append(charactersPerFinger(Finger::MIDDLE));
    stages.append(stages.value(stages.size()-1) +
                  stages.value(stages.size()-2));
    stages.append(charactersPerFinger(Finger::RING));
    stages.append(stages.value(stages.size()-1) +
                  stages.value(stages.size()-2));
    stages.append(charactersPerFinger(Finger::PINKY));
    stages.append(stages.value(stages.size()-1) +
                  stages.value(stages.size()-2));
    // stages.append(charactersPerFinger(Finger::PINKY_EXTRA));

    QList<QStringList>* rowLessons = new QList<QStringList>;
    for (QString stage : stages) {
        // Make lessonsPerFinger lessons for this finger
        QStringList lessons;
        for (int i = 0; i < lessonsPerFingerGroup; ++i) {
            QString lesson = generateLesson(stage, 5, maxLength);
            lessons.append(lesson);
        }
        rowLessons->append(lessons);
    }

    return rowLessons;
}

// return a QString of the characters assigned to a given finger
QString TrainingGenerator::charactersPerFinger(Finger f) {
    int colLeft, colRight;

    switch (f) {
        case Finger::INDEX_INNER:
            colLeft  = 4;
            colRight = 5;
            break;
        case Finger::INDEX:
            colLeft  = 3;
            colRight = 6;
            break;
        case Finger::MIDDLE:
            colLeft  = 2;
            colRight = 7;
            break;
        case Finger::RING:
            colLeft  = 1;
            colRight = 8;
            break;
        case Finger::PINKY:
            colLeft  = 0;
            colRight = 9;
            break;
        case Finger::PINKY_EXTRA:
            colLeft  = 0;
            colRight = 10;
            break;
    }

    QString keys;
    for (int i = 0; i < 3; ++i) {
        keys += rows[i][colLeft];
        keys += rows[i][colRight];
    }

    return keys;
}

QString TrainingGenerator::generateLesson(QString& characters,
                                          int wordLength, int maxLength) {
    QString lesson;
    while (lesson.length() < maxLength) {
        // word of 1 to wordLength characters
        int x = (qrand() % wordLength) + 1;
        // random pick from characters in given string
        for (int i = 0; i < x; ++i) {
            int rand_index = (qrand() % characters.size());
            lesson.append(characters.at(rand_index));
        }
        lesson.append(" ");
    }
    return lesson.trimmed();
}
