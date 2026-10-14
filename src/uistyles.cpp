#include "uistyles.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QStandardPaths>

namespace {

struct CheckBoxIndicatorPaths {
    QString unchecked;
    QString checked;
    QString disabledUnchecked;
    QString disabledChecked;
};

struct RadioIndicatorPaths {
    QString unchecked;
    QString checked;
    QString disabledUnchecked;
    QString disabledChecked;
};

CheckBoxIndicatorPaths ensureCheckBoxIndicatorImages() {
    static CheckBoxIndicatorPaths paths;
    if (!paths.unchecked.isEmpty()) {
        return paths;
    }
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const auto makeIndicator = [&](bool checked, bool disabled, const QString &fileName) {
        const QString filePath = QDir(tempDir).filePath(fileName);
        if (!QFile::exists(filePath)) {
            QPixmap pixmap(13, 13);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing, false);
            const QColor borderColor = disabled ? QColor(0xa0, 0xa0, 0xa0) : QColor(0x80, 0x80, 0x80);
            const QColor fillColor = disabled ? QColor(0xf4, 0xf4, 0xf4) : Qt::white;
            painter.fillRect(QRect(1, 1, 11, 11), fillColor);
            painter.setPen(QPen(borderColor, 1));
            painter.drawRect(QRect(1, 1, 11, 11));
            if (checked) {
                painter.setPen(QPen(disabled ? QColor(0x90, 0x90, 0x90) : QColor(0x22, 0x22, 0x22), 2));
                painter.drawLine(3, 7, 5, 9);
                painter.drawLine(5, 9, 10, 3);
            }
            pixmap.save(filePath, "PNG");
        }
        return QDir::fromNativeSeparators(filePath);
    };
    paths.unchecked = makeIndicator(false, false, QStringLiteral("sunrise_cb_unchecked.png"));
    paths.checked = makeIndicator(true, false, QStringLiteral("sunrise_cb_checked.png"));
    paths.disabledUnchecked = makeIndicator(false, true, QStringLiteral("sunrise_cb_disabled_unchecked.png"));
    paths.disabledChecked = makeIndicator(true, true, QStringLiteral("sunrise_cb_disabled_checked.png"));
    return paths;
}

RadioIndicatorPaths ensureRadioIndicatorImages() {
    static RadioIndicatorPaths paths;
    if (!paths.unchecked.isEmpty()) {
        return paths;
    }
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const auto makeIndicator = [&](bool checked, bool disabled, const QString &fileName) {
        const QString filePath = QDir(tempDir).filePath(fileName);
        if (!QFile::exists(filePath)) {
            QPixmap pixmap(13, 13);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing, true);
            const QColor borderColor = disabled ? QColor(0xa0, 0xa0, 0xa0) : QColor(0x80, 0x80, 0x80);
            const QColor fillColor = disabled ? QColor(0xf4, 0xf4, 0xf4) : Qt::white;
            painter.setPen(QPen(borderColor, 1));
            painter.setBrush(fillColor);
            painter.drawEllipse(QRectF(1.5, 1.5, 10, 10));
            if (checked) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(disabled ? QColor(0x90, 0x90, 0x90) : QColor(0x22, 0x22, 0x22));
                painter.drawEllipse(QRectF(4.5, 4.5, 4, 4));
            }
            pixmap.save(filePath, "PNG");
        }
        return QDir::fromNativeSeparators(filePath);
    };
    paths.unchecked = makeIndicator(false, false, QStringLiteral("sunrise_rb_unchecked.png"));
    paths.checked = makeIndicator(true, false, QStringLiteral("sunrise_rb_checked.png"));
    paths.disabledUnchecked = makeIndicator(false, true, QStringLiteral("sunrise_rb_disabled_unchecked.png"));
    paths.disabledChecked = makeIndicator(true, true, QStringLiteral("sunrise_rb_disabled_checked.png"));
    return paths;
}

} // namespace

QString checkBoxIndicatorCss() {
    const CheckBoxIndicatorPaths paths = ensureCheckBoxIndicatorImages();
    return QStringLiteral(
        "QCheckBox::indicator { width: 13px; height: 13px; }"
        "QCheckBox::indicator:unchecked { image: url(\"%1\"); }"
        "QCheckBox::indicator:checked { image: url(\"%2\"); }"
        "QCheckBox::indicator:unchecked:disabled { image: url(\"%3\"); }"
        "QCheckBox::indicator:checked:disabled { image: url(\"%4\"); }"
    ).arg(paths.unchecked, paths.checked, paths.disabledUnchecked, paths.disabledChecked);
}

QString radioIndicatorCss() {
    const RadioIndicatorPaths paths = ensureRadioIndicatorImages();
    return QStringLiteral(
        "QRadioButton::indicator { width: 13px; height: 13px; }"
        "QRadioButton::indicator:unchecked { image: url(\"%1\"); }"
        "QRadioButton::indicator:checked { image: url(\"%2\"); }"
        "QRadioButton::indicator:unchecked:disabled { image: url(\"%3\"); }"
        "QRadioButton::indicator:checked:disabled { image: url(\"%4\"); }"
    ).arg(paths.unchecked, paths.checked, paths.disabledUnchecked, paths.disabledChecked);
}

QString panelCheckBoxStyleSheet() {
    return QStringLiteral(
        "QCheckBox, QRadioButton {"
        "  background: transparent;"
        "  color: #000000;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 8.25pt;"
        "  spacing: 4px;"
        "}"
        "QCheckBox:disabled, QRadioButton:disabled { color: rgba(0, 0, 0, 128); }"
    ) + checkBoxIndicatorCss() + radioIndicatorCss();
}
