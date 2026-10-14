#ifndef COVERPANEL_H
#define COVERPANEL_H

#include "exerciseids.h"

#include <QWidget>

class ExCanvas;
class ImageButton;
class QComboBox;
class QLabel;
class QLineEdit;
class QSlider;
class QTextEdit;

// Правая панель упражнения: аналог Windows Forms «обложки» (_1, _112, ... ).
// Содержит только интерфейс; логику ведёт ExerciseHost.
class CoverPanel : public QWidget {
    Q_OBJECT
public:
    explicit CoverPanel(ExCoverKind kind, QWidget *parent = nullptr);

    ExCoverKind kind() const { return m_kind; }

    // Кнопки
    ImageButton *beginu = nullptr;
    ImageButton *stopu = nullptr;
    ImageButton *nextz = nullptr;
    ImageButton *bagain = nullptr;
    ImageButton *showp = nullptr;     // показать (подсказки/варианты/пример)
    ImageButton *modePic = nullptr;   // 1.1.2: ppod/skrp
    ImageButton *updateR = nullptr;   // 1.2.1
    ImageButton *dcorrect = nullptr;  // right.png
    ImageButton *dincorrect = nullptr;// unright.png
    ImageButton *explainBtn = nullptr;// 2.2.6 (225): объяснение
    ImageButton *showStory = nullptr; // 2.2.2: показать/скрыть на экране пациента
    ImageButton *metroBtn = nullptr;  // 1.2.1
    ImageButton *conj[5] = {nullptr, nullptr, nullptr, nullptr, nullptr}; // 2.1.4
    ImageButton *boldBtn = nullptr;
    ImageButton *underlineBtn = nullptr;
    ImageButton *showQuestion = nullptr; // 1.1.4

    // Списки
    QComboBox *combo1 = nullptr;
    QComboBox *combo2 = nullptr;
    QComboBox *combo3 = nullptr;
    QComboBox *combo4 = nullptr;

    // Подписи
    QLabel *label1 = nullptr;
    QLabel *label2 = nullptr;
    QLabel *label3 = nullptr;
    QLabel *label4 = nullptr;
    QLabel *lcounter = nullptr;
    QLabel *lcorrect = nullptr;
    QLabel *modeLabel = nullptr;  // 1.1.2 label2 «Покажите сюжетные картинки»
    QLabel *hintLabel = nullptr;  // подсказка врачу (1.1.4, 1.2.2...)
    QLabel *taskLabel = nullptr;  // 2.2.6 ltext
    QLabel *explainLabel = nullptr; // 2.2.6 lob
    QLabel *explainCaption = nullptr;
    QLabel *fioLabel = nullptr;   // 1.2.3
    QLabel *raz1 = nullptr;       // 2.1.4: «Причинные союзы» (r/ron.png)
    QLabel *raz2 = nullptr;       // 2.1.4: «Следственные союзы» (rr/rron.png)

    // Ввод
    QTextEdit *rtf = nullptr;     // 1.1.1, 1.1.4 (режим 2), 2.2.6 (режим 3: about.rtf)
    QLineEdit *question = nullptr;// 1.1.4
    QLineEdit *address = nullptr; // 1.2.3
    QLineEdit *phone = nullptr;   // 1.2.3
    QWidget *dataBox = nullptr;   // 1.2.3
    QWidget *showCheck = nullptr; // 1.2.3 checkBox (QCheckBox)
    QSlider *metroSlider = nullptr;
    QWidget *metroPanel = nullptr;

    // Зеркало экрана пациента
    ExCanvas *mirror = nullptr;

    void setMirrorGeometry(const QRect &r);
    void setCounters(int done, int correct);
    // 2.1.4 f1..f5_Click: подсветка союза и группы (raz1 — союзы 1–3, raz2 — 4–5)
    void selectConjunction(int index);

private:
    ImageButton *makeButton(const QString &sysImage, int x, int y);
    ImageButton *makeExnButton(const QString &path, int x, int y, int w, int h);
    // Шрифты как в *.Designer.cs: по умолчанию Microsoft Sans Serif 8.25pt
    QComboBox *makeCombo(const QStringList &items, int x, int y, int w, int h = 24, double pt = 8.25);
    QLabel *makeLabel(const QString &text, int x, int y, bool bold = false, double pt = 8.25);
    void addCounters(int x1, int y1, int x2, int y2, bool withColon);
    void buildFor(ExCoverKind kind);

    ExCoverKind m_kind;
};

#endif
