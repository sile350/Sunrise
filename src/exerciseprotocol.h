#ifndef EXERCISEPROTOCOL_H
#define EXERCISEPROTOCOL_H

#include <QFont>
#include <QList>
#include <QString>

class QTextDocument;

// Протокол фиксации выполнения упражнения: protocols.createP / getProtocolById / getAll / getSall.
// В БД (protocols.pr) хранится тело в разметке createP; на экран и в печать оно всегда
// пересобирается из модели, чтобы все таблицы имели одну ширину и фиксированные колонки.
namespace ExProtocol {

constexpr int kTableWidth = 671;

// Шрифт протокола; им же выводятся описание и форма оценки (в оригинале — шрифт IE по умолчанию).
QFont textFont();

struct Row {
    QString first;   // 4 колонки: Уровень / Выполнение / Сюжет / Рассказ / Упражнение
    QString orHtml;  // Характер деятельности
    QString hlpHtml; // Виды помощи
    QString score;   // Бал.
};

struct Record {
    QString dateLine;
    int maxScore = 3;
    bool resultIsCopy = false; // <div id='idcopy'>: дублирует «Максимальный балл» после «Подвести итог»
    QString result;
    QString note;
    QString firstColumn; // пусто — таблица процесса из трёх колонок
    bool hasMaxRow = false;
    QString maxValue;
    QList<Row> rows;
};

struct HeaderRow {
    QString left;
    QString right;
    QString background;
};

struct Entry {
    QString id;
    QString uprid;
    Record record;
    QList<HeaderRow> header; // пусто — продолжение группы того же упражнения
    bool withProcess = true;
};

struct EditableCell {
    int table = 0;
    int row = 0;
    int column = 0;
};

QString storedBody(const Record &record);
Record parseStoredBody(const QString &body);

// header.html: строки шапки без висящего <tr> и без вложенных <p>.
QList<HeaderRow> loadHeader(const QString &headerPath);
// Папка/файл шапки для записи без hdr (как getProtocolById для uprid).
QString defaultHeaderPath(const QString &uprid);

// Пустой протокол из template.html (показывается до первого «Сформировать протокол»).
Entry templateEntry(const QString &templatePath);

QString calculateBls(const QString &bands, int correct, int counter);
QString moscowNow();
QString escape(const QString &text);

QString entriesHtml(const QList<Entry> &entries);
QString documentHtml(const QString &preambleHtml, const QList<Entry> &entries);
// После setHtml: таблица процесса вплотную к своей сводной (как в IE), между протоколами — отступ.
void tightenTables(QTextDocument *document);

QList<EditableCell> editableCells(const QList<Entry> &entries);
// Перенести правки из документа в модели; вернуть индексы записей, чьё тело изменилось.
QList<int> applyDocumentEdits(QTextDocument *document, QList<Entry> &entries);
// «Подвести итог»: максимум по баллам строк -> «Максимальный балл» и «Результат».
bool applyItog(Record &record);

} // namespace ExProtocol

#endif
