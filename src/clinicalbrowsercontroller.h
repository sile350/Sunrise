#ifndef CLINICALBROWSERCONTROLLER_H
#define CLINICALBROWSERCONTROLLER_H

#include "imagebutton.h"
#include "summarypanel.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <functional>
#include <memory>

class QAxObject;
class QAxWidget;
class QTimer;
class QWidget;
struct IDispatch;

// pagemanager: один WebBrowser (IE) для вкладок оценки риска, коррекции, речи, гимнастики и терапии.
class ClinicalBrowserController final : public QObject {
    Q_OBJECT
public:
    enum class Section { Risk, Correction, Speech, Breathing, Medication };

    explicit ClinicalBrowserController(QWidget *panel, QObject *parent = nullptr);
    ~ClinicalBrowserController() override;

    void setSummary(SummaryPanel *summary);
    void setHtmlRoot(const QString &htmlsDir);
    void setImageLoader(const std::function<QString(const QString &)> &loader);

    void openSection(Section section);
    void saveTemplates();
    QString helpIndex() const { return m_helpIndex; }
    QString pageName() const { return m_pageName; }

signals:
    void helpIndexChanged(const QString &helpFile);
    void programChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onBeforeNavigate(IDispatch *frame, QVariant &url, QVariant &flags, QVariant &target,
                          QVariant &postData, QVariant &headers, bool &cancel);
    void onNavigateComplete(IDispatch *frame, QVariant &url);
    void onDocumentComplete(IDispatch *frame, QVariant &url);
    void onBodyMouseDown(IDispatch *event);
    void onDocumentClick(IDispatch *event);
    void onDocumentEdited(IDispatch *event);

private:
    struct TemplateSpec {
        QString fileName;
        int count = 0;
        QString idPrefix;
        QStringList placeholders;
        bool asHtml = false;
    };

    void openPage(const QString &path);
    void setHelpIndex(const QString &help);
    void setBackImage(const QString &name);
    void setBrowserHeight(int height);
    void showButton(ImageButton *button, bool visible);
    void placeResultButtons();
    void runItog();
    void runPlus();
    void handleSpeechClick(const QString &id, const QString &src, QAxObject *element);
    void handleClick(const QString &id, QAxObject *element);
    void loadTemplates();
    bool templateSpec(TemplateSpec *spec) const;
    QString templatePath(const QString &fileName) const;
    void appendTo(SummaryPanel::Field field, const QString &chunk);
    void appendTemplate(SummaryPanel::Field field, const QString &head, const QString &bodyId);
    void printRtf(const QString &fileName);
    void showError(const QString &text);
    void installPageAlert();
    void flushPageAlerts();
    bool browserHasFocus() const;

    std::unique_ptr<QAxObject> element(const QString &id) const;
    QString innerText(const QString &id) const;
    QString innerHtml(const QString &id) const;
    void setInnerHtml(const QString &id, const QString &html);
    void setInnerText(const QString &id, const QString &text);
    QString valueOf(const QString &id) const;
    bool isChecked(const QString &id) const;
    void setStyle(const QString &id, const QString &css);

    QWidget *m_panel = nullptr;
    QAxWidget *m_web = nullptr;
    QAxObject *m_document = nullptr;
    SummaryPanel *m_summary = nullptr;
    ImageButton *m_backBtn = nullptr;
    ImageButton *m_itogBtn = nullptr;
    ImageButton *m_plusBtn = nullptr;
    ImageButton *m_recBtn = nullptr;
    QTimer *m_templateSaveTimer = nullptr;
    QString m_htmlsRoot;
    std::function<QString(const QString &)> m_imageLoader;
    Section m_section = Section::Risk;
    QString m_pageName;
    QString m_helpIndex;
    QString m_address;
    QString m_previousAddress;
    int m_fagerstromScore = -1;
    QString m_fagerstromVerdict;
    int m_prochaskaScore = -1;
    QString m_prochaskaVerdict;
};

#endif
