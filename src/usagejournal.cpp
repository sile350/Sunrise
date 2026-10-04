#include "usagejournal.h"

#include "approotpaths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSaveFile>

namespace {

QByteArray emptyJournalRtf() {
    return QByteArrayLiteral(
        "{\\rtf1\\ansi\\ansicpg1251\\deff0\\deflang1049{\\fonttbl{\\f0\\fnil\\fcharset204 Calibri;}}\r\n"
        "\\viewkind4\\uc1\\pard\\sa200\\sl276\\slmult1\\f0\\fs22\\lang1049\r\n"
        "}\r\n");
}

QByteArray escapeRtf(const QString &text) {
    QByteArray out;
    for (const QChar ch : text) {
        const ushort code = ch.unicode();
        if (code == '\\' || code == '{' || code == '}') {
            out += '\\';
            out += static_cast<char>(code);
        } else if (code >= 0x20 && code < 0x80) {
            out += static_cast<char>(code);
        } else if (code >= 0x80) {
            out += "\\u" + QByteArray::number(static_cast<short>(code)) + '?';
        }
    }
    return out;
}

} // namespace

namespace UsageJournal {

QString filePath() {
    return QDir(AppRootPaths::applicationRoot()).filePath(QStringLiteral("aJournal.rtf"));
}

void append(const QString &event) {
    const QString path = filePath();
    QByteArray content;
    {
        QFile in(path);
        if (in.open(QIODevice::ReadOnly)) {
            content = in.readAll();
        }
    }
    int closing = content.lastIndexOf('}');
    if (!content.trimmed().startsWith("{\\rtf") || closing < 0) {
        content = emptyJournalRtf();
        closing = content.lastIndexOf('}');
    }

    const QString line = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy HH:mm")) + QLatin1Char(' ') + event;
    content.insert(closing, escapeRtf(line) + "\\par\r\n");

    QSaveFile out(path);
    if (out.open(QIODevice::WriteOnly)) {
        out.write(content);
        out.commit();
    }
}

} // namespace UsageJournal
