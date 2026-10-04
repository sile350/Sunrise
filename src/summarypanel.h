#ifndef SUMMARYPANEL_H
#define SUMMARYPANEL_H

#include <QScrollArea>
#include <QString>
#include <QStringList>
#include <QVector>

class QLabel;
class QTextEdit;
class QTimer;

class SummaryPanel final : public QScrollArea {
    Q_OBJECT
public:
    enum Field {
        Risk = 0,
        Corr = 1,
        Speech = 2,
        Dih = 3,
        Mp = 4,
        ExtraBody = 5,
        ExtraTitle = 6,
        FieldCount = 7
    };

    explicit SummaryPanel(QWidget *parent = nullptr);

    void setMarkerImage(const QString &path);

    void resetDefaults();
    void loadSerialized(const QString &program);
    QString serialize() const;

    void setPatientHeader(const QString &fio, const QString &birthDate, const QString &diag);
    void setHeaderVisible(bool visible);
    QString patientFio() const;
    QString patientBirthDate() const;
    QString patientDiag() const;
    QString sectionTitle(Field field) const;

    void appendToField(Field field, const QString &text, bool prependDate = true);
    QString fieldText(Field field) const;
    bool isPlaceholder(Field field) const;
    void setFieldText(Field field, const QString &text, bool asUserContent);

    QString defaultText(Field field) const;

signals:
    void programChanged();

private:
    void buildUi();
    void applyPlaceholderStyle(QTextEdit *edit, bool placeholder);
    void clearPlaceholderOnFocus(Field field);
    void restorePlaceholderIfEmpty(Field field);
    void scheduleEmitChanged();
    void updateAllSize();
    int fittedHeight(QTextEdit *edit, int padding) const;

    QWidget *m_content = nullptr;
    QLabel *m_title1 = nullptr;
    QLabel *m_title2 = nullptr;
    QLabel *m_pfio = nullptr;
    QLabel *m_lfio = nullptr;
    QLabel *m_pdr = nullptr;
    QLabel *m_ldr = nullptr;
    QLabel *m_pdiag = nullptr;
    QLabel *m_ldiag = nullptr;
    QVector<QLabel *> m_sectionLabels;
    QVector<QLabel *> m_markers;
    QVector<QTextEdit *> m_fields;
    QTimer *m_changeTimer = nullptr;
};

#endif
