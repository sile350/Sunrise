#ifndef MPICTURESENGINE_H
#define MPICTURESENGINE_H

#include "exengine.h"

#include <QPair>
#include <QStringList>

// Порт Engines\mpictures.cs для упражнений 1.1.2/1.1.3, 1.1.4, 1.2.1, 1.2.3, 2.2.1.
class MPicturesEngine : public ExEngine {
    Q_OBJECT
public:
    explicit MPicturesEngine(QObject *parent = nullptr) : ExEngine(parent) {}

    // Конфигурация (выставляется обложкой перед rebuild()).
    QString runtime;      // "1.1.2" | "1.1.4" | "1.2.1" | "1.2.3" | "2.2.1"
    QString folder;       // папка exn (1.1.2 или 1.1.3 для картинок)
    QString p1, p2, p3;   // см. rebuild*
    int currentEx = 1;
    QString fio, address, phone; // 1.2.3, режим 1
    QString questionText; // 1.1.4, письменный вопрос
    int metronomeBpm = 60;

    QStringList dSay;     // выбор врача (индексы спрайтов)

    void rebuild(bool newPicks = true);
    QString doctorHint() const { return m_hint; }
    void setHintVisible(bool on); // rcolors для "Цвета радуги"
    bool hintVisible() const { return m_hintVisible; }
    bool seriesFinished() const { return m_seriesDone; }

    void patientPress(const QPoint &p) override;
    void patientRelease(const QPoint &p) override;
    void doctorClick(const QPoint &p) override;

    static int storyCount(const QString &folder); // сколько N.png есть в папке

private:
    void build112();
    void build114();
    void build121();
    void build123();
    void build221();
    void pressDone112();
    void releaseSeries();
    void resetDoneState();
    int label112Offset() const;
    bool selectableOnClick(const ExSprite &s) const;

    struct SeriesInfo {
        QString dir;
        int count = 0;
        QString voidImg;
        QString bvoidImg;
        int docStep = 0;   // шаг x на докторской панели
        int cellDocY = 0;  // y ячеек (doctor)
        int poolDocY = 0;  // y пула (doctor)
        int left = 0;      // uprSpace.movingPictures.Left
        QVector<int> levels;
        int docX0 = 0;     // x первой ячейки (doctor)
        int scale = 60;    // 100 / scale при переносе на экран пациента
    };
    SeriesInfo seriesInfo(const QString &material) const;

    QVector<int> m_pics;       // 1.1.2 выбранные номера
    QStringList m_captionHome; // не используется
    QVector<QPoint> m_home;    // исходные позиции перетаскиваемых спрайтов
    QVector<int> m_assign;     // не используется (оставлено для совместимости)
    QVector<QPair<int, bool>> m_ans112; // main.ans: (индекс картинки, верно) для режима «подписать»

    // ряды 1.2.1
    int m_pointLine = 1;
    int m_dir = 1;
    int m_seriesCount = 0;
    bool m_seriesDone = false;
    SeriesInfo m_series;
    QString m_hint;
    bool m_hintVisible = false;
};

#endif
