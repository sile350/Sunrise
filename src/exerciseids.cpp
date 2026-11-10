#include "exerciseids.h"

namespace {

struct Row {
    const char *menuId;
    const char *runtimeId;   // имя движка после ремапов exbegin
    const char *assetFolder; // папка данных/картинок
    const char *htmlFolder;  // папка or.html/template.html
    ExCoverKind cover;
    ExEngineKind engine;
    int dataNumber;
    const char *title;
};

// Итог каскада pagemanager.cs + ремапов exbegin.initEx для разделов 1..3.
const Row kRows[] = {
    // --- 1. Речь: восстановление ---
    {"1.1.1", "1.1.1", "1.1.1", "1.1.1", ExCoverKind::C1, ExEngineKind::None, 1, "Выполнение речевых инструкций"},
    {"1.1.2", "1.1.2", "1.1.2", "1.1.2", ExCoverKind::C112, ExEngineKind::Mpictures, 1, "Покажи / подпиши сюжетные картинки"},
    {"1.1.3", "1.1.2", "1.1.3", "1.1.3", ExCoverKind::C112, ExEngineKind::Mpictures, 1, "Покажи / подпиши предметные картинки"},
    {"1.1.4", "1.1.4", "1.1.4", "1.1.4", ExCoverKind::C114, ExEngineKind::Mpictures, 1, "Проведение беседы (ситуативные вопросы)"},
    {"1.2.1", "1.2.1", "1.2.1", "1.2.1", ExCoverKind::C121, ExEngineKind::Mpictures, 1, "Автоматизированные ряды"},
    {"1.2.2", "1.2.2", "1.2.2", "1.2.2", ExCoverKind::C122, ExEngineKind::Movewords, 1, "Стойкие речевые ряды"},
    {"1.2.3", "1.2.3", "1.2.3", "1.2.3", ExCoverKind::C123, ExEngineKind::Mpictures, 1, "ФИО, адрес, телефон (свои и вымышленных героев)"},
    // --- 2. ---
    {"2.1.1", "2.1.1", "2.1.1", "2.1.1", ExCoverKind::C211, ExEngineKind::Movewords, 1, "Составление фраз из отдельных слов, согласованных между собой"},
    {"2.1.2", "2.1.2", "2.1.2", "2.1.2", ExCoverKind::C212, ExEngineKind::Movewords, 1, "Составь предложение из двух несогласованных между собой слов"},
    {"2.1.3", "2.1.3", "2.1.3", "2.1.3", ExCoverKind::C213, ExEngineKind::Movewords, 1, "Составление фраз из отдельных слов, несогласованных между собой"},
    {"2.1.4", "2.1.4", "2.1.4", "2.1.4", ExCoverKind::C214, ExEngineKind::Movewords, 1, "Установление причинно-следственной связи"},
    {"2.2.1", "2.2.1", "2.2.1", "2.2.1", ExCoverKind::C221, ExEngineKind::Mpictures, 1, "Составь рассказ по серии сюжетных картинок"},
    {"2.2.2", "2.2.2", "2.2.2", "2.2.2", ExCoverKind::C222, ExEngineKind::Stories, 1, "Пересказ рассказа и ответы на вопросы"},
    // 2.2.3 -> 1.1.4
    {"2.2.3", "1.1.4", "1.1.4", "1.1.4", ExCoverKind::C114, ExEngineKind::Mpictures, 1, "Проведение беседы (ситуативные вопросы)"},
    {"2.2.4", "2.2.3", "2.2.3", "2.2.3", ExCoverKind::C223, ExEngineKind::Movewords, 1, "Понимание (дифференцирование) логико-грамматических оборотов"},
    {"2.2.5", "2.2.4", "2.2.4", "2.2.4", ExCoverKind::C224, ExEngineKind::Movewords, 1, "Объяснение значения пословиц или устойчивых речевых выражений"},
    {"2.2.6", "2.2.5", "2.2.5", "2.2.5", ExCoverKind::C225, ExEngineKind::Movewords, 1, "Решение логических и математических задач"},
    {"2.2.7", "2.2.6", "2.2.6", "2.2.6", ExCoverKind::C226, ExEngineKind::Movewords, 1, "Составление рассказа"},
    // --- 3. ---
    {"3.1.1", "3.1.1", "3.1.1", "3.1.1", ExCoverKind::C311, ExEngineKind::Movewords, 1, "Вставить в слова пропущенные слоги (но, ло, би)"},
    {"3.1.2", "3.1.2", "3.1.2", "3.1.2", ExCoverKind::C312, ExEngineKind::Movewords, 1, "Составление слов из отдельных букв с возможностью опоры на первый слог"},
    // 3.2.1 -> 1.2.2
    {"3.2.1", "1.2.2", "1.2.2", "1.2.2", ExCoverKind::C122, ExEngineKind::Movewords, 1, "Стойкие речевые ряды"},
    // 3.2.2: обложка _312 с dataNumber=2, html из 3.2.2, данные из 3.1.2\data2.txt
    {"3.2.2", "3.1.2", "3.1.2", "3.2.2", ExCoverKind::C312, ExEngineKind::Movewords, 2, "Составление слов из слогов"},
    // 3.2.4 -> 3.1.2
    {"3.2.4", "3.1.2", "3.1.2", "3.1.2", ExCoverKind::C312, ExEngineKind::Movewords, 1, "Составление слов из отдельных букв с возможностью опоры на первый слог"},
    {"3.2.5", "3.2.3", "3.2.3", "3.2.3", ExCoverKind::C323, ExEngineKind::Movewords, 1, "Синонимы/антонимы"},
    {"3.2.6", "3.2.4", "3.2.4", "3.2.4", ExCoverKind::C324, ExEngineKind::Movewords, 1, "Вставить пропущенные буквы"},
    // 3.2.7 -> 2.2.7 -> 2.2.6
    {"3.2.7", "2.2.6", "2.2.6", "2.2.6", ExCoverKind::C226, ExEngineKind::Movewords, 1, "Составление рассказа"},
    // --- 4. Афферентно-моторная афазия (алиасы + уникальные) ---
    {"4.1.1", "1.2.2", "1.2.2", "1.2.2", ExCoverKind::C122, ExEngineKind::Movewords, 1, "Стойкие речевые ряды"},
    {"4.1.2", "1.2.1", "1.2.1", "1.2.1", ExCoverKind::C121, ExEngineKind::Mpictures, 1, "Автоматизированные ряды"},
    {"4.1.3", "1.1.4", "1.1.4", "1.1.4", ExCoverKind::C114, ExEngineKind::Mpictures, 1, "Проведение беседы (ситуативные вопросы)"},
    {"4.2.1", "1.2.2", "1.2.2", "1.2.2", ExCoverKind::C122, ExEngineKind::Movewords, 1, "Стойкие речевые ряды"},
    {"4.3.1", "1.2.3", "1.2.3", "1.2.3", ExCoverKind::C123, ExEngineKind::Mpictures, 1, "ФИО, адрес, телефон (свои и вымышленных героев)"},
    {"4.3.2", "1.1.4", "1.1.4", "1.1.4", ExCoverKind::C114, ExEngineKind::Mpictures, 1, "Проведение беседы (ситуативные вопросы)"},
    // 4.3.3 → 3.2.6 (каскад L705)
    {"4.3.3", "3.2.4", "3.2.4", "3.2.4", ExCoverKind::C324, ExEngineKind::Movewords, 1, "Вставить пропущенные буквы"},
    // 4.3.4 → 3.2.2 (каскад L750) — слоги dataNumber=2
    {"4.3.4", "3.1.2", "3.1.2", "3.2.2", ExCoverKind::C312, ExEngineKind::Movewords, 2, "Составление слов из слогов"},
    // 4.3.5 → 3.1.2 (каскад L748)
    {"4.3.5", "3.1.2", "3.1.2", "3.1.2", ExCoverKind::C312, ExEngineKind::Movewords, 1, "Составление слов из отдельных букв с возможностью опоры на первый слог"},
    {"4.3.6", "4.3.6", "4.3.6", "4.3.6", ExCoverKind::C436, ExEngineKind::Cross, 1, "Кроссворды"},
    // 4.3.7 → 2.2.2 (каскад L758)
    {"4.3.7", "2.2.2", "2.2.2", "2.2.2", ExCoverKind::C222, ExEngineKind::Stories, 1, "Пересказ рассказа и ответы на вопросы"},
    // 4.3.8 → 2.2.7 (каскад L692)
    {"4.3.8", "2.2.6", "2.2.6", "2.2.6", ExCoverKind::C226, ExEngineKind::Movewords, 1, "Составление рассказа"},
    // --- 5. Акустико-гностическая афазия ---
    {"5.1.1", "1.1.1", "1.1.1", "1.1.1", ExCoverKind::C1, ExEngineKind::None, 1, "Выполнение речевых инструкций"},
    {"5.1.2", "1.1.2", "1.1.2", "1.1.2", ExCoverKind::C112, ExEngineKind::Mpictures, 1, "Покажи / подпиши сюжетные картинки"},
    {"5.1.3", "1.1.2", "1.1.3", "1.1.3", ExCoverKind::C112, ExEngineKind::Mpictures, 1, "Покажи / подпиши предметные картинки"},
    {"5.1.4", "5.1.4", "5.1.4", "5.1.4", ExCoverKind::C514, ExEngineKind::ManBody, 1, "Разбор схемы тела на картинке с переносом на себя"},
    {"5.1.5", "5.1.5", "5.1.5", "5.1.5", ExCoverKind::C515, ExEngineKind::Movewords, 1, "Определение предметов по их функциям"},
    {"5.1.6", "1.1.4", "1.1.4", "1.1.4", ExCoverKind::C114, ExEngineKind::Mpictures, 1, "Проведение беседы (ситуативные вопросы)"},
    {"5.2.1", "5.2.1", "5.2.1", "5.2.1", ExCoverKind::C521, ExEngineKind::Movewords, 1, "Словосочетания с опорой на картинку"},
    {"5.2.2", "5.2.2", "5.2.2", "5.2.2", ExCoverKind::C522, ExEngineKind::Movewords, 1, "Выбирать слова с оппозиционными фонемами"},
    {"5.2.3", "5.2.3", "5.2.3", "5.2.3", ExCoverKind::C523, ExEngineKind::None, 1, "Подбирать определения к словам с целью расширения семантики слова"},
    // 5.2.4 → 3.2.5 (каскад L764)
    {"5.2.4", "3.2.3", "3.2.3", "3.2.3", ExCoverKind::C323, ExEngineKind::Movewords, 1, "Синонимы/антонимы"},
    // 5.2.5 → 4.3.6 (каскад L720)
    {"5.2.5", "4.3.6", "4.3.6", "4.3.6", ExCoverKind::C436, ExEngineKind::Cross, 1, "Кроссворды"},
};

// pagemanager.cs: ename, если он отличается от id пункта меню (срабатывает последнее совпадение).
QString pagemanagerEname(const QString &menuId) {
    if (menuId == QLatin1String("2.2.3")) {
        return QStringLiteral("1.1.4");
    }
    if (menuId == QLatin1String("3.2.1")) {
        return QStringLiteral("1.2.2");
    }
    if (menuId == QLatin1String("3.2.4")) {
        return QStringLiteral("3.1.2");
    }
    if (menuId == QLatin1String("3.2.7")) {
        return QStringLiteral("2.2.7");
    }
    if (menuId == QLatin1String("4.1.1") || menuId == QLatin1String("4.2.1")) {
        return QStringLiteral("1.2.2");
    }
    if (menuId == QLatin1String("4.1.2")) {
        return QStringLiteral("1.2.1");
    }
    if (menuId == QLatin1String("4.1.3") || menuId == QLatin1String("4.3.2") || menuId == QLatin1String("5.1.6")) {
        return QStringLiteral("1.1.4");
    }
    if (menuId == QLatin1String("4.3.1")) {
        return QStringLiteral("1.2.3");
    }
    if (menuId == QLatin1String("4.3.3")) {
        return QStringLiteral("3.2.6");
    }
    if (menuId == QLatin1String("4.3.4")) {
        return QStringLiteral("3.2.2");
    }
    if (menuId == QLatin1String("4.3.5")) {
        return QStringLiteral("3.1.2");
    }
    if (menuId == QLatin1String("5.2.5")) {
        return QStringLiteral("4.3.6");
    }
    if (menuId == QLatin1String("5.2.4")) {
        return QStringLiteral("3.2.5");
    }
    if (menuId == QLatin1String("5.1.1")) {
        return QStringLiteral("1.1.1");
    }
    if (menuId == QLatin1String("5.1.2")) {
        return QStringLiteral("1.1.2");
    }
    if (menuId == QLatin1String("5.1.3")) {
        return QStringLiteral("1.1.3");
    }
    if (menuId == QLatin1String("4.3.7")) {
        return QStringLiteral("2.2.2");
    }
    if (menuId == QLatin1String("4.3.8")) {
        return QStringLiteral("2.2.7");
    }
    return menuId;
}

} // namespace

ExerciseRuntime resolveMenuIdToRuntime(const QString &menuId) {
    ExerciseRuntime r;
    const QString id = menuId.trimmed();
    for (const Row &row : kRows) {
        if (id == QLatin1String(row.menuId)) {
            r.valid = true;
            r.menuId = id;
            r.runtimeId = QString::fromLatin1(row.runtimeId);
            r.assetFolder = QString::fromLatin1(row.assetFolder);
            r.htmlFolder = QString::fromLatin1(row.htmlFolder);
            r.dataFolder = r.assetFolder;
            r.coverKind = row.cover;
            r.engineKind = row.engine;
            r.dataNumber = row.dataNumber;
            r.title = QString::fromUtf8(row.title);
            r.helpFile = pagemanagerEname(id) + QStringLiteral(".htm");
            return r;
        }
    }
    r.menuId = id;
    return r;
}
