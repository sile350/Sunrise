#ifndef MANBODYENGINE_H
#define MANBODYENGINE_H

#include "exengine.h"

// Порт Engines\man.cs + подписи из mpictures.cs (5.1.4).
// Обе проекции и колонка подписей живут в одной сцене пациента:
//   спереди mn (310,50), сзади mn2 (1140,50), ratio 100, pictureBox (3,3);
//   подписи movingPictures Left=-80 Top=50, pictureBox (48,14).
// Индекс части — порядковый номер в barray этой проекции (0..), не индекс спрайта WinForms.
// -1 — ничего не выбрано.
class ManBodyEngine : public ExEngine {
    Q_OBJECT
public:
    explicit ManBodyEngine(QObject *parent = nullptr) : ExEngine(parent) {}

    // "" — показать, "sub" — подписать, "3" — на себе. Задать до rebuild().
    QString mode;

    void rebuild();

    int doctorFront() const { return m_doctorFront; }
    int doctorBack() const { return m_doctorBack; }
    int patientFront() const { return m_patientFront; }
    int patientBack() const { return m_patientBack; }
    QString selectedLabel() const { return m_selectedLabel; }
    int partCount(bool front) const { return (front ? m_front : m_back).size(); }

    // name2: ключ barray без хвостовой цифры («Голова1» → «Голова»).
    QString partName2(bool front, int part) const;

    void doctorChoose(bool front, int part);
    void patientChoose(bool front, int part);
    void chooseLabel(const QString &name);
    void resetAllSelections();

    // Кнопка «готово» пациента: режимы "" и "sub". Пишет resultRecorded через finishDone.
    void submit();
    // Кнопки «верно / неверно» обложки в режиме "3". Не блокирует сцену.
    void markManual(bool wrong);

    // Картинки кнопок обложки (button1 — задание, button2 — ответ пациента).
    QString coverTaskImage() const;
    QString coverChipImage() const;
    bool graded() const { return m_graded; }
    bool lastCorrect() const { return m_lastCorrect; }

    void patientPress(const QPoint &p) override;
    void patientRelease(const QPoint &p) override;
    void doctorClick(const QPoint &p) override;

signals:
    void doctorSelectionChanged();
    void labelSelectionChanged(const QString &name);

private:
    struct Part {
        QString key;
        QString name2;
        int blue = -1;
        int red = -1;
        int green = -1;
        bool bD = false, bP = false;
        bool rD = false, rP = false;
        bool gD = false, gP = false;
    };

    void buildFigure(bool front, int hostX, int hostY);
    void buildLabels();
    void applyFlag(int sprite, bool doctor, bool patient);
    void applyAll();
    void clearFlags(Part &part);
    void clearDoctorSide(bool front);
    void clearPatientColors();
    void showColor(bool front, int part, QChar color);
    int partAt(const QPoint &p, bool *front) const;
    int labelIndex(const QString &name) const;
    int labelAt(const QPoint &p) const;
    void paintLabel(const QString &name, const QColor &tint, bool green, bool red);

    QVector<Part> m_front;
    QVector<Part> m_back;
    int m_doctorFront = -1;
    int m_doctorBack = -1;
    int m_patientFront = -1;
    int m_patientBack = -1;
    QString m_selectedLabel;
    QString m_chipFile;
    bool m_graded = false;
    bool m_lastCorrect = false;
};

#endif
