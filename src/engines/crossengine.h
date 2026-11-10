#ifndef CROSSENGINE_H
#define CROSSENGINE_H

#include "exengine.h"

#include <QStringList>

// Порт Engines\cross.cs: кроссворд 4.3.6 / 5.2.5.
// Сцена — раскладка пациента (uprSpace: cword 10,200 + pictureBox 19,14,
// вопросы справа). Зеркало врача показывает ту же сцену.
class CrossEngine : public ExEngine {
    Q_OBJECT
public:
    explicit CrossEngine(QObject *parent = nullptr) : ExEngine(parent) {}

    int currentEx = 1;          // 1..10, rebuild приводит номер к этому диапазону
    QString userNowQuest;       // "g1".."g4" / "v1".."v4" — слово, выбранное пациентом
    int wordOpen = 0;           // сколько слов врач открыл целиком (ПКМ)
    int oneTry = 0;             // 1 после открытия одной буквы (ЛКМ)
    QStringList gQuest;         // ответы по горизонтали
    QStringList vQuest;         // ответы по вертикали
    QStringList gClues;
    QStringList vClues;

    // exercise вне 1..10 заворачивается (11 → 1, 0 → 10).
    void rebuild(int exercise);

    // code: "g1" / "v2" — подсветка клеток слова (клик пациента по вопросу).
    void selectQuestion(const QString &code);

    void patientRelease(const QPoint &p) override;
    void doctorClick(const QPoint &p, Qt::MouseButton button) override;

private:
    void loadPuzzle(int exercise);
    void buildGrid();
    void placeHorizontal(const QStringList &words, const QString &prefix);
    void placeVertical(const QStringList &words, const QString &prefix);
    void addClues();
    void addClue(const QString &name, const QString &text, int localX, int localY);
    void setClueSelected(int index, bool on);
    void unselectAll();
    void paintWord(const QString &code, bool on);
    void revealCell(int cell);
    void openWord(const QString &quest);
    QString answer(const QString &code) const;
    int cellAt(int px, int py) const;
};

#endif
