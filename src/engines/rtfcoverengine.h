#ifndef RTFCOVERENGINE_H
#define RTFCOVERENGINE_H

#include <QString>

class QTextEdit;

// Упражнение 1.1.1 (_1): только RTF-текст для врача, без экрана пациента.
// Также используется для ans1.rtf в 1.1.4 (письменный режим).
namespace RtfCover {
bool loadInto(QTextEdit *edit, const QString &rtfFile);
void toggleBold(QTextEdit *edit);
void toggleUnderline(QTextEdit *edit);
}

#endif
