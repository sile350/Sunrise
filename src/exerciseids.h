#ifndef EXERCISEIDS_H
#define EXERCISEIDS_H

#include <QString>

// Соответствие id пункта меню (upr.html) -> рантайм-идентификатор упражнения.
// Каскад pagemanager.cs (ename), затем ремапы exbegin.initEx.
enum class ExCoverKind {
    None,
    C1,    // 1.1.1 RTF-текст
    C112,  // 1.1.2 / 1.1.3 сюжетные / предметные картинки
    C114,  // 1.1.4 диалог по картинкам
    C121,  // 1.2.1 ряды
    C122,  // 1.2.2 пословицы
    C123,  // 1.2.3 карточки
    C211,
    C212,
    C213,
    C214,
    C221,
    C222,
    C223,
    C224,
    C225,
    C226,
    C311,
    C312,
    C323,
    C324,
    C436, // 4.3.6 / 5.2.5 кроссворды
    C514, // 5.1.4 схема тела
    C515, // 5.1.5 определение предметов по функциям
    C521, // 5.2.1 словосочетания с опорой на картинку
    C522, // 5.2.2 оппозиционные фонемы
    C523  // 5.2.3 подбирать определения
};

enum class ExEngineKind { None, Movewords, Mpictures, Stories, Wolf, Cross, ManBody };

struct ExerciseRuntime {
    bool valid = false;
    QString menuId;      // исходный id пункта меню
    QString runtimeId;   // имя для движка (exbegin: ename после ремапов)
    QString assetFolder; // папка exn/{folder}: данные и картинки
    QString htmlFolder;  // папка exn/{folder}: or.html / template.html
    QString dataFolder;  // папка с txt-данными (3.2.2 -> 3.1.2)
    ExCoverKind coverKind = ExCoverKind::None;
    ExEngineKind engineKind = ExEngineKind::None;
    int dataNumber = 1;  // 3.1.2: data1.txt / data2.txt (3.2.2 -> 2)
    QString title;
    QString helpFile;    // htmls/spravka: Form1.hindex = ename + ".htm" (ename после каскада pagemanager)
};

ExerciseRuntime resolveMenuIdToRuntime(const QString &menuId);

#endif
