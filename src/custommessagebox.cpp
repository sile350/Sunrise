#include "custommessagebox.h"

#include "imagebutton.h"

#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>

namespace {

QString locateAsset(const QString &name) {
    const QString app = QCoreApplication::applicationDirPath();
    const QStringList roots = {
        app + "/assets/sysImages",
        app + "/../assets/sysImages",
        app + "/../../assets/sysImages",
        app + "/../../../assets/sysImages",
        QDir::currentPath() + "/assets/sysImages",
        QDir::currentPath() + "/../assets/sysImages",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/sysImages"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }
    return {};
}

QString cssUrlPath(QString path) {
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return path;
}

QWidget *makeActionButton(QWidget *parent, int x, int y, int w, int h, const QStringList &imageNames,
                          const QString &fallbackText) {
    QString path;
    for (const QString &name : imageNames) {
        path = locateAsset(name);
        if (!path.isEmpty()) {
            break;
        }
    }
    if (!path.isEmpty()) {
        auto *btn = new ImageButton(parent);
        btn->setGeometry(x, y, w, h);
        btn->setImagePath(path);
        btn->setCursor(Qt::PointingHandCursor);
        const QPixmap pm(path);
        if (!pm.isNull()) {
            btn->setFixedSize(pm.size());
            btn->move(x, y);
        }
        btn->raise();
        btn->show();
        return btn;
    }
    auto *btn = new QPushButton(fallbackText, parent);
    btn->setGeometry(x, y, w, h);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #5a7a5a; color: white; border: 1px solid #1f3f2f;"
        " border-radius: 3px; font-family: 'Microsoft Sans Serif'; font-size: 10pt; padding: 4px 12px; }"
        "QPushButton:hover { background: #6b8b6b; }"));
    btn->raise();
    btn->show();
    return btn;
}

void connectAccept(QWidget *btn, QDialog *dialog) {
    if (auto *img = qobject_cast<ImageButton *>(btn)) {
        QObject::connect(img, &ImageButton::clicked, dialog, &QDialog::accept);
    } else if (auto *push = qobject_cast<QPushButton *>(btn)) {
        QObject::connect(push, &QPushButton::clicked, dialog, &QDialog::accept);
    }
}

void centerOnParent(QDialog *dialog, QWidget *parent) {
    QWidget *anchor = parent ? parent->window() : nullptr;
    QRect host;
    if (anchor && anchor->isVisible()) {
        host = anchor->frameGeometry();
    } else if (QScreen *screen = QGuiApplication::primaryScreen()) {
        host = screen->availableGeometry();
    } else {
        return;
    }
    dialog->move(host.center() - dialog->rect().center());
}

class MessageDialog final : public QDialog {
public:
    enum class Mode { Error, Warning, Info, Confirm };

    MessageDialog(Mode mode, const QString &message, QWidget *parent)
        : QDialog(parent ? parent->window() : nullptr, Qt::FramelessWindowHint | Qt::Dialog), m_mode(mode) {
        setModal(true);
        setWindowModality(Qt::ApplicationModal);
        constexpr int kDialogW = 393;
        constexpr int kDialogH = 224;
        setFixedSize(kDialogW, kDialogH);

        QString background = locateAsset("popup.jpg");
        if (background.isEmpty()) {
            background = locateAsset("popup.png");
        }
        if (!background.isEmpty()) {
            setStyleSheet(QStringLiteral(
                "QDialog { border: 2px solid #1f3f2f; background-color: #d7e3d2;"
                " background-image: url('%1'); background-repeat: no-repeat; }")
                              .arg(cssUrlPath(background)));
        } else {
            setStyleSheet(QStringLiteral(
                "QDialog { border: 2px solid #1f3f2f; background-color: #d7e3d2; }"));
        }

        auto *title = new QLabel(this);
        title->setGeometry(0, 8, kDialogW, 20);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet(
            "color: white; font-family: 'Microsoft Sans Serif'; font-size: 10pt; font-style: italic; background: transparent;");

        auto *icon = new QLabel(this);
        icon->setGeometry(17, 40, 45, 41);
        icon->setScaledContents(true);

        auto *heading = new QLabel(this);
        heading->setGeometry(0, 48, kDialogW, 24);
        heading->setAlignment(Qt::AlignCenter);
        heading->setStyleSheet(
            "color: black; font-family: 'Microsoft Sans Serif'; font-size: 12pt; font-weight: bold; background: transparent;");

        auto *body = new QLabel(message, this);
        body->setGeometry(20, 86, kDialogW - 40, 78);
        body->setAlignment(Qt::AlignCenter);
        body->setWordWrap(true);
        const QString bodyStyle = mode == Mode::Confirm
            ? "color: black; font-family: 'Microsoft Sans Serif'; font-size: 12pt; font-style: italic; background: transparent;"
            : "color: black; font-family: 'Microsoft Sans Serif'; font-size: 12pt; background: transparent;";
        body->setStyleSheet(bodyStyle);

        if (mode == Mode::Confirm) {
            title->setText("Внимание");
            heading->setText("Внимание!");
            const QString alertPath = locateAsset("alert.png");
            if (!alertPath.isEmpty()) {
                icon->setPixmap(QPixmap(alertPath));
            }

            auto *continueBtn = makeActionButton(this, 52, 171, 129, 34,
                                                 {QStringLiteral("Продолжить.png"), QStringLiteral("continue.png")},
                                                 QStringLiteral("Продолжить"));
            auto *cancelBtn = makeActionButton(this, 210, 171, 120, 29, {QStringLiteral("cancel.png")},
                                               QStringLiteral("Отмена"));

            if (auto *img = qobject_cast<ImageButton *>(continueBtn)) {
                connect(img, &ImageButton::clicked, this, [this]() {
                    m_confirmed = true;
                    accept();
                });
            } else if (auto *push = qobject_cast<QPushButton *>(continueBtn)) {
                connect(push, &QPushButton::clicked, this, [this]() {
                    m_confirmed = true;
                    accept();
                });
            }
            if (auto *img = qobject_cast<ImageButton *>(cancelBtn)) {
                connect(img, &ImageButton::clicked, this, [this]() {
                    m_confirmed = false;
                    reject();
                });
            } else if (auto *push = qobject_cast<QPushButton *>(cancelBtn)) {
                connect(push, &QPushButton::clicked, this, [this]() {
                    m_confirmed = false;
                    reject();
                });
            }
            return;
        }

        title->setText(mode == Mode::Info ? "Сообщение" : "Сообщение об ошибке!");
        heading->setText(mode == Mode::Info ? "Сообщение" : "Ошибка!");
        const QString iconPath = mode == Mode::Info
            ? QString()
            : locateAsset(mode == Mode::Warning ? "alert.png" : "error.png");
        if (!iconPath.isEmpty()) {
            icon->setPixmap(QPixmap(iconPath));
        }

        connectAccept(makeActionButton(this, 160, 175, 66, 29, {QStringLiteral("ok.png")}, QStringLiteral("OK")), this);
    }

    bool confirmed() const {
        return m_confirmed;
    }

private:
    Mode m_mode;
    bool m_confirmed = false;
};

void execDialog(MessageDialog::Mode mode, QWidget *parent, const QString &message) {
    MessageDialog dialog(mode, message, parent);
    centerOnParent(&dialog, parent);
    dialog.exec();
}

} // namespace

QString CustomMessageBox::assetPath(const QString &name) {
    return locateAsset(name);
}

void CustomMessageBox::showError(QWidget *parent, const QString &message) {
    execDialog(MessageDialog::Mode::Error, parent, message);
}

void CustomMessageBox::showWarning(QWidget *parent, const QString &message) {
    execDialog(MessageDialog::Mode::Warning, parent, message);
}

void CustomMessageBox::showInfo(QWidget *parent, const QString &message) {
    execDialog(MessageDialog::Mode::Info, parent, message);
}

bool CustomMessageBox::askConfirm(QWidget *parent, const QString &message) {
    MessageDialog dialog(MessageDialog::Mode::Confirm, message, parent);
    centerOnParent(&dialog, parent);
    dialog.exec();
    return dialog.confirmed();
}
