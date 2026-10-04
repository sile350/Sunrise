#include "src/approotpaths.h"
#include "src/custommessagebox.h"
#include "src/sunrisewindow.h"
#include "src/licenseservice.h"
#include "src/repository.h"
#include "src/singleinstance.h"
#include "src/usagejournal.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QProxyStyle>

namespace {

class AppStyle final : public QProxyStyle {
public:
    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const override {
        if (hint == QStyle::SH_ToolTip_WakeUpDelay) {
            return 500;
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

QString findAppIconPath() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList roots = {
        appDir + QStringLiteral("/assets"),
        appDir + QStringLiteral("/../assets"),
        appDir + QStringLiteral("/../../assets"),
        QDir::currentPath() + QStringLiteral("/assets"),
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(QStringLiteral("Санрайс.ico"));
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

} // namespace

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_Use96Dpi);
#endif
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QApplication a(argc, argv);
    a.setStyle(new AppStyle);
    a.setApplicationName(QStringLiteral("Санрайс"));
    a.setApplicationDisplayName(QStringLiteral("Санрайс"));
    a.setOrganizationName(QStringLiteral("DokitLab"));
    const QString iconPath = findAppIconPath();
    if (!iconPath.isEmpty()) {
        a.setWindowIcon(QIcon(iconPath));
    }

    const SingleInstance singleInstance;
    if (!singleInstance.isPrimary()) {
        return 0;
    }

    QString layoutError;
    if (!AppRootPaths::ensureWritableLayout(&layoutError)) {
        CustomMessageBox::showError(nullptr, layoutError);
        return 0;
    }

    UsageJournal::append(QStringLiteral("Запуск программы."));
    struct ClosingRecord {
        ~ClosingRecord() { UsageJournal::append(QStringLiteral("Закрытие программы.")); }
    } closingRecord;

    ApiClient api;
    Repository repository(&api);
    LicenseService licenseService(&repository);
    if (!licenseService.ensureActivated(nullptr)) {
        return 0;
    }

    SunriseWindow w(licenseService.key(), licenseService.freshActivation());
    w.show();
    return a.exec();
}
