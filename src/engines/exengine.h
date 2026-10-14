#ifndef EXENGINE_H
#define EXENGINE_H

#include <QColor>
#include <QObject>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>
#include <QWidget>

// Общие помощники упражнений (аналог pclassSel / sysImages / exn).
namespace ExAssets {
QString assetsDir();                         // .../assets
QString sysImage(const QString &name);       // .../assets/sysImages/name
QString exnPath(const QString &relative);    // .../assets/exn/relative
QString resourceImage(const QString &name);  // .../assets/resources/name
QPixmap pixmap(const QString &path);         // кэш
QStringList readLines(const QString &path);  // UTF-8, без пустых хвостов
QString readAll(const QString &path);
void playCorrect();
void playIncorrect();
void playMetronome(bool start, int bpm);
}

// Спрайт сцены пациента (1920x1080). Карта pclassSel из оригинала.
struct ExSprite {
    QString name;
    QString word;
    QString path;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    QColor color = Qt::black;
    bool movable = false;
    bool visible = true;
    bool border = false;       // выбор пациента — синяя рамка (видна на обоих экранах)
    bool dborder = false;      // подсказка врача — видна только врачу
    bool green = false;
    bool red = false;
    bool doctorOnly = false;   // видно только врачу (otv, hint)
    bool patientOnly = false;
    bool isText = false;
    QPixmap pix;
    QPixmap doctorPix;         // если задана — врач видит её вместо pix (у пациента в оригинале своя картинка)

    QRect rect() const { return QRect(x, y, w, h); }
    bool hit(int px, int py) const {
        return visible && px > x && px < x + w && py > y && py < y + h;
    }
    void setImage(const QString &file, int scalePercent = 100);
    void setText(const QString &text, int fontSize = 26, const QColor &c = Qt::black, int scalePercent = 100, bool italic = false);
    // Многострочная метка с рамкой (Label BorderStyle=FixedSingle, AutoSize=false).
    void setBox(const QString &text, int width, int height, int pointSize, bool bordered = true, bool bold = false);
    void setCenterX(int totalWidth) { x = (totalWidth - w) / 2; }
};

class ExEngine : public QObject {
    Q_OBJECT
public:
    explicit ExEngine(QObject *parent = nullptr) : QObject(parent) {}

    QVector<ExSprite> sprites;
    bool block = false;
    QColor background = QColor(249, 249, 249);
    QSize sceneSize = QSize(1920, 1080);

    int addSprite();
    ExSprite &last() { return sprites.last(); }
    int indexByName(const QString &name) const;
    int topAt(const QPoint &p, bool patientView) const;
    void clear() { sprites.clear(); m_moving = -1; }

    // Ввод пациента (координаты сцены).
    virtual void patientPress(const QPoint &p);
    virtual void patientMove(const QPoint &p);
    virtual void patientRelease(const QPoint &p);
    // Клик врача по зеркалу (координаты сцены).
    virtual void doctorClick(const QPoint &p);

    // Результат кнопки done: спрайт done превращается в correct/incorrect.
    void finishDone(bool wrong);
    void addDoneButton(int x, int y);
    void clearMarks();

signals:
    void changed();
    void resultRecorded(bool wrong);

protected:
    int m_moving = -1;
    QPoint m_off;
    QPoint m_start;
    bool m_dragging = false;
};

// Виджет отрисовки сцены движка: врач (зеркало) или пациент.
class ExCanvas : public QWidget {
    Q_OBJECT
public:
    explicit ExCanvas(bool patient, QWidget *parent = nullptr);
    void setEngine(ExEngine *engine);
    ExEngine *engine() const { return m_engine; }
    void setPatient(bool p) { m_patient = p; update(); }
    QPointF toScene(const QPoint &p) const;

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    double scaleFactor() const;
    QPointF originOffset() const;
    ExEngine *m_engine = nullptr;
    bool m_patient = false;
};

#endif
