#include "rtfcoverengine.h"

#include "../rtfconverter.h"

#include <QFile>
#include <QTextCharFormat>
#include <QTextEdit>

namespace RtfCover {

bool loadInto(QTextEdit *edit, const QString &rtfFile) {
    if (!edit) {
        return false;
    }
    QFile f(rtfFile);
    if (!f.open(QIODevice::ReadOnly)) {
        edit->setPlainText(QStringLiteral("Не удалось открыть файл: ") + rtfFile);
        return false;
    }
    const QByteArray data = f.readAll();
    const QString html = rtfToHtml(data);
    if (html.trimmed().isEmpty()) {
        edit->setPlainText(QString::fromLocal8Bit(data));
        return true;
    }
    edit->setHtml(html);
    return true;
}

void toggleBold(QTextEdit *edit) {
    if (!edit) {
        return;
    }
    QTextCharFormat fmt;
    fmt.setFontWeight(edit->fontWeight() >= QFont::Bold ? QFont::Normal : QFont::Bold);
    edit->mergeCurrentCharFormat(fmt);
}

void toggleUnderline(QTextEdit *edit) {
    if (!edit) {
        return;
    }
    QTextCharFormat fmt;
    fmt.setFontUnderline(!edit->fontUnderline());
    edit->mergeCurrentCharFormat(fmt);
}

} // namespace RtfCover
