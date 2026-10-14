#ifndef SCORINGFORM_H
#define SCORINGFORM_H

#include <QHash>
#include <QWidget>

class QAbstractButton;
class QButtonGroup;
class QVBoxLayout;

// Блок «Оценка результатов» из or.html: QTextBrowser не рисует <input>, поэтому радиокнопки idch*,
// флажки idd* (характер деятельности) и idp* (виды помощи) строятся нативными виджетами
// с подписями из разметки. Блок <div id='idvz'> обложки заполняют или очищают по режиму.
class ScoringForm : public QWidget {
    Q_OBJECT
public:
    explicit ScoringForm(QWidget *parent = nullptr);

    void load(const QString &formHtml);
    bool hasScoreBlock() const { return m_vz != nullptr; }
    // _122/_226/_324: idvz.InnerHtml = стандартные «Количество баллов» или "".
    void setScoreBlock(bool filled);

    int checkedScore(bool *ok) const;
    QString activityHtml() const;
    QString helpHtml() const;
    void clear();

    QStringList inputIds() const;
    QStringList inputLabels() const;
    QAbstractButton *input(const QString &id) const;

private:
    struct Input {
        QString id;
        QString value;
        QString label;
        QAbstractButton *button = nullptr;
        bool inScoreBlock = false;
    };

    void buildInto(QVBoxLayout *layout, const QString &html, bool scoreBlock);
    void addHeading(QVBoxLayout *layout, const QString &html, bool centered);
    void addInput(QVBoxLayout *layout, const QString &tag, const QString &labelHtml, bool scoreBlock);
    QString joinChecked(const QString &prefix) const;
    bool inputActive(const Input &in) const;

    QVBoxLayout *m_layout = nullptr;
    QWidget *m_vz = nullptr;
    QVBoxLayout *m_vzLayout = nullptr;
    QList<Input> m_inputs;
    QHash<QString, QButtonGroup *> m_groups;
};

#endif
