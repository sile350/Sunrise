#ifndef WOLFENGINE_H
#define WOLFENGINE_H

#include "exengine.h"

#include <QStringList>

class QTimer;

// Порт Engines\wolf.cs: головоломка «Волк, коза и капуста» (2.2.6 меню, режим «Головоломка»).
class WolfEngine : public ExEngine {
    Q_OBJECT
public:
    explicit WolfEngine(QObject *parent = nullptr);

    void rebuild();

    void patientPress(const QPoint &p) override;
    void patientRelease(const QPoint &p) override;

private:
    void arrangeLeft();
    void arrangeRight();
    void startCrossing();
    void tick();
    bool checkDie(const QStringList &bank);
    void checkCorrect();
    void showMessage(const QString &text);
    int idx(const QString &name) const { return indexByName(name); }

    QStringList m_left;   // имена животных на левом берегу (порядок как в оригинале)
    QStringList m_right;
    QString m_load;       // кто в лодке
    QString m_shipSide = QStringLiteral("left");
    QTimer *m_timer = nullptr;
    QPoint m_sPos;        // позиция до перетаскивания
    bool m_animating = false;
};

#endif
