#ifndef MOVEWORDSENGINE_H
#define MOVEWORDSENGINE_H

#include "exengine.h"

#include <QStringList>
#include <QVector>

// Порт Engines\movewords.cs: упражнения 1.2.2, 2.1.1-2.1.4, 2.2.3-2.2.6, 3.1.1, 3.1.2, 3.2.3, 3.2.4.
class MovewordsEngine : public ExEngine {
    Q_OBJECT
public:
    explicit MovewordsEngine(QObject *parent = nullptr) : ExEngine(parent) {}

    QString runtime;      // имя движка: "1.2.2", "2.1.1", ...
    QString p1, p2;       // параметры обложки (см. rebuild)
    int currentEx = 1;
    int dataNumber = 1;   // 3.1.2: data1/data2
    bool showHints = false; // 3.2.3/3.2.4: показать варианты ("any")
    bool showExamples = false; // 2.1.4

    // Для обложек врача
    QString taskText;        // 2.2.5: формулировка задания
    QString explanationText; // 2.2.5: объяснение
    QString doctorHint;      // подсказка врачу (ответ и т.п.)

    bool explainOn = false;  // 2.2.5: показ объяснения / подсвеченных картинок пациенту (bob в _225.cs)

    void rebuild(bool resetState = true);
    int recordCount() const;
    void setExplain(bool on);

    void patientPress(const QPoint &p) override;
    void patientRelease(const QPoint &p) override;
    void doctorClick(const QPoint &p) override;

private:
    struct Group {
        QStringList words;
        QString order;
    };

    // построители
    void build122();
    void build211();
    void build212();
    void build213();
    void build214();
    void build223();
    void build224();
    void build225();
    void build226();
    void build311();
    void build312();
    void build323();
    void build324();
    // помощники
    int addText(const QString &text, int x, int y, int fontSize = 26, const QColor &c = Qt::black,
                const QString &name = QString(), bool movable = false, int scale = 100);
    int addImage(const QString &path, int x, int y, const QString &name = QString(), int scale = 100);
    void snapshotHome();
    void sortWordsInLine(int lineMax, int left, int y, int gap);
    void layout213();
    void layout324();
    void pressDone();
    bool releaseClick(int mv);
    void toggleExclusive(int idx, const QStringList &names);
    QStringList wordList() const;
    QString dataPath(const QString &file) const;

    // Смещение хоста: movingWords.Left/Top в uprSpace.initEx + внутренний pictureBox (3,21 либо 3,0).
    void applyHost();
    void addAbsolute();
    int m_hx = 0;
    int m_hy = 0;
    bool m_hostApplied = false;
    // Центрирование как в doctorInit: x = ((docWidth - w*scale/100)/2) * 100/scale (+ extra)
    void centerDoc(int idx, int scale, int docWidth = 900, int extra = 0);

    QVector<QPoint> m_home;
    int m_mem = -1;         // 1.2.2/3.1.1/3.2.3: спрайт, занявший пропуск
    QPoint m_memPos;        // его прежнее место
    int m_lineMax = 0;
    int m_sortLeft = 0;
    int m_sortY = 0;
    int m_sortGap = 10;
    bool m_sortAlt = false;
    QStringList m_cache;    // кэш смешанных списков
    QString m_cacheKey;
    // 2.1.3
    QStringList m_sentence;
    QStringList m_replacements; // "Nслово"
    // 2.2.6
    QVector<qreal> m_slotsY;
    QString m_order;
    // 3.2.4
    int m_slotLetter[3] = {-1, -1, -1};
    QStringList m_partNames;
};

#endif
