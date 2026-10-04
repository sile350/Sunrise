#include "rtfconverter.h"

#include <QFont>
#include <QSet>
#include <QStack>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCodec>
#include <QTextCursor>
#include <QTextDocument>

namespace {

struct RtfState {
    bool bold = false;
    bool italic = false;
    bool underline = false;
    int fontSizeHalfPoints = 24;
    Qt::Alignment alignment = Qt::AlignLeft;
    int leftIndentTwips = 0;
    int firstIndentTwips = 0;
    int unicodeSkip = 1;
    bool skip = false;
};

const QSet<QByteArray> &skippedDestinations() {
    static const QSet<QByteArray> words = {
        "fonttbl", "colortbl", "stylesheet", "info", "pict", "object", "fldinst",
        "themedata", "colorschememapping", "latentstyles", "datastore", "xmlnstbl",
        "rsidtbl", "listtable", "listoverridetable", "header", "footer", "headerl",
        "headerr", "headerf", "footerl", "footerr", "footerf", "ftnsep", "ftnsepc",
        "ftncn", "aftnsep", "aftnsepc", "aftncn", "pnseclvl", "generator", "mmathPr",
        "filetbl", "revtbl", "pgdsctbl", "docvar", "userprops", "nonshppict", "shp",
        "shpinst", "footnote", "txe", "xe", "tc", "bkmkstart", "bkmkend"
    };
    return words;
}

class RtfToDocument {
public:
    explicit RtfToDocument(QTextDocument *doc) : m_cursor(doc) {
        m_codec = QTextCodec::codecForName("windows-1251");
    }

    void parse(const QByteArray &rtf) {
        const int size = rtf.size();
        int i = 0;
        while (i < size) {
            const char c = rtf.at(i);
            if (c == '{') {
                flush();
                m_stack.push(m_state);
                m_groupStart = true;
                ++i;
                continue;
            }
            if (c == '}') {
                flush();
                if (!m_stack.isEmpty()) {
                    m_state = m_stack.pop();
                }
                m_groupStart = false;
                ++i;
                continue;
            }
            if (c == '\\') {
                i = parseControl(rtf, i + 1);
                continue;
            }
            if (c == '\r' || c == '\n') {
                ++i;
                continue;
            }
            m_groupStart = false;
            appendByte(static_cast<unsigned char>(c));
            ++i;
        }
        flush();
        m_cursor.setBlockFormat(blockFormat());
    }

private:
    int parseControl(const QByteArray &rtf, int i) {
        const int size = rtf.size();
        if (i >= size) {
            return i;
        }
        const char c = rtf.at(i);
        if (!QChar::isLetter(static_cast<uchar>(c)) || static_cast<uchar>(c) > 0x7f) {
            const bool first = m_groupStart;
            m_groupStart = false;
            switch (c) {
            case '\'': {
                if (i + 2 < size) {
                    bool ok = false;
                    const int value = rtf.mid(i + 1, 2).toInt(&ok, 16);
                    if (ok) {
                        appendByte(static_cast<unsigned char>(value));
                    }
                }
                return i + 3;
            }
            case '{':
            case '}':
            case '\\':
                appendByte(static_cast<unsigned char>(c));
                return i + 1;
            case '~':
                appendChar(QChar(0x00a0));
                return i + 1;
            case '_':
                appendChar(QChar('-'));
                return i + 1;
            case '*':
                if (first) {
                    m_state.skip = true;
                }
                return i + 1;
            case '\r':
            case '\n':
                newParagraph();
                return i + 1;
            default:
                return i + 1;
            }
        }

        int end = i;
        while (end < size && static_cast<uchar>(rtf.at(end)) < 0x80 && QChar::isLetter(static_cast<uchar>(rtf.at(end)))) {
            ++end;
        }
        const QByteArray word = rtf.mid(i, end - i);
        bool hasParam = false;
        int param = 0;
        int paramStart = end;
        if (end < size && (rtf.at(end) == '-' || QChar::isDigit(static_cast<uchar>(rtf.at(end))))) {
            ++end;
            while (end < size && QChar::isDigit(static_cast<uchar>(rtf.at(end)))) {
                ++end;
            }
            param = rtf.mid(paramStart, end - paramStart).toInt(&hasParam);
        }
        if (end < size && rtf.at(end) == ' ') {
            ++end;
        }

        const bool first = m_groupStart;
        m_groupStart = false;
        if (first && skippedDestinations().contains(word)) {
            m_state.skip = true;
            return end;
        }
        if (word == "bin" && hasParam) {
            return end + qMax(0, param);
        }
        applyWord(word, hasParam, param);
        return end;
    }

    void applyWord(const QByteArray &word, bool hasParam, int param) {
        const bool on = !hasParam || param != 0;
        if (word == "par" || word == "sect" || word == "page" || word == "row") {
            newParagraph();
        } else if (word == "line") {
            appendChar(QChar::LineSeparator);
        } else if (word == "tab" || word == "cell") {
            appendChar(QChar('\t'));
        } else if (word == "emdash") {
            appendChar(QChar(0x2014));
        } else if (word == "endash") {
            appendChar(QChar(0x2013));
        } else if (word == "bullet") {
            appendChar(QChar(0x2022));
        } else if (word == "lquote") {
            appendChar(QChar(0x2018));
        } else if (word == "rquote") {
            appendChar(QChar(0x2019));
        } else if (word == "ldblquote") {
            appendChar(QChar(0x201c));
        } else if (word == "rdblquote") {
            appendChar(QChar(0x201d));
        } else if (word == "emspace" || word == "enspace" || word == "qmspace") {
            appendChar(QChar(' '));
        } else if (word == "u" && hasParam) {
            appendChar(QChar(static_cast<ushort>(param < 0 ? param + 65536 : param)));
            m_pendingSkip = m_state.unicodeSkip;
        } else if (word == "uc" && hasParam) {
            m_state.unicodeSkip = qMax(0, param);
        } else if (word == "ansicpg" && hasParam) {
            if (QTextCodec *codec = QTextCodec::codecForName("windows-" + QByteArray::number(param))) {
                m_codec = codec;
            }
        } else {
            flush();
            if (word == "b") {
                m_state.bold = on;
            } else if (word == "i") {
                m_state.italic = on;
            } else if (word == "ulnone") {
                m_state.underline = false;
            } else if (word == "ul" || word == "uld" || word == "uldb" || word == "ulw"
                       || word == "uldash" || word == "ulwave" || word == "ulth") {
                m_state.underline = on;
            } else if (word == "fs" && hasParam && param > 0) {
                m_state.fontSizeHalfPoints = param;
            } else if (word == "plain") {
                m_state.bold = false;
                m_state.italic = false;
                m_state.underline = false;
                m_state.fontSizeHalfPoints = 24;
            } else if (word == "pard") {
                m_state.alignment = Qt::AlignLeft;
                m_state.leftIndentTwips = 0;
                m_state.firstIndentTwips = 0;
            } else if (word == "ql") {
                m_state.alignment = Qt::AlignLeft;
            } else if (word == "qc") {
                m_state.alignment = Qt::AlignHCenter;
            } else if (word == "qr") {
                m_state.alignment = Qt::AlignRight;
            } else if (word == "qj") {
                m_state.alignment = Qt::AlignJustify;
            } else if (word == "li" && hasParam) {
                m_state.leftIndentTwips = param;
            } else if (word == "fi" && hasParam) {
                m_state.firstIndentTwips = param;
            }
        }
    }

    void appendByte(unsigned char byte) {
        if (m_state.skip) {
            return;
        }
        if (m_pendingSkip > 0) {
            --m_pendingSkip;
            return;
        }
        if (byte < 0x80 || !m_codec) {
            m_text.append(QChar(byte));
            return;
        }
        m_text.append(m_codec->toUnicode(reinterpret_cast<const char *>(&byte), 1));
    }

    void appendChar(QChar ch) {
        if (m_state.skip) {
            return;
        }
        m_pendingSkip = 0;
        m_text.append(ch);
    }

    QTextCharFormat charFormat() const {
        QTextCharFormat format;
        format.setFontFamily(QStringLiteral("Times New Roman"));
        format.setFontPointSize(m_state.fontSizeHalfPoints / 2.0);
        format.setFontWeight(m_state.bold ? QFont::Bold : QFont::Normal);
        format.setFontItalic(m_state.italic);
        format.setFontUnderline(m_state.underline);
        return format;
    }

    QTextBlockFormat blockFormat() const {
        QTextBlockFormat format;
        format.setAlignment(m_state.alignment);
        format.setLeftMargin(m_state.leftIndentTwips / 15.0);
        format.setTextIndent(m_state.firstIndentTwips / 15.0);
        format.setTopMargin(0);
        format.setBottomMargin(0);
        return format;
    }

    void flush() {
        if (m_text.isEmpty()) {
            return;
        }
        m_cursor.insertText(m_text, charFormat());
        m_text.clear();
    }

    void newParagraph() {
        if (m_state.skip) {
            return;
        }
        flush();
        m_cursor.setBlockFormat(blockFormat());
        m_cursor.setBlockCharFormat(charFormat());
        m_cursor.insertBlock(blockFormat(), charFormat());
    }

    QTextCursor m_cursor;
    QTextCodec *m_codec = nullptr;
    RtfState m_state;
    QStack<RtfState> m_stack;
    QString m_text;
    int m_pendingSkip = 0;
    bool m_groupStart = false;
};

} // namespace

QString rtfToHtml(const QByteArray &rtf) {
    if (!rtf.trimmed().startsWith("{\\rtf")) {
        return {};
    }
    QTextDocument doc;
    QFont font(QStringLiteral("Times New Roman"));
    font.setPointSizeF(12.0);
    doc.setDefaultFont(font);
    RtfToDocument converter(&doc);
    converter.parse(rtf);
    if (doc.toPlainText().trimmed().isEmpty()) {
        return {};
    }
    return doc.toHtml();
}
