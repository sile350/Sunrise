#pragma once

#include <QString>

namespace UsageJournal {

QString filePath();
void append(const QString &event);

} // namespace UsageJournal
