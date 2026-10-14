/****************************************************************************
** Meta object code from reading C++ file 'clinicalbrowsercontroller.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/clinicalbrowsercontroller.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'clinicalbrowsercontroller.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ClinicalBrowserController_t {
    QByteArrayData data[26];
    char stringdata0[301];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ClinicalBrowserController_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ClinicalBrowserController_t qt_meta_stringdata_ClinicalBrowserController = {
    {
QT_MOC_LITERAL(0, 0, 25), // "ClinicalBrowserController"
QT_MOC_LITERAL(1, 26, 16), // "helpIndexChanged"
QT_MOC_LITERAL(2, 43, 0), // ""
QT_MOC_LITERAL(3, 44, 8), // "helpFile"
QT_MOC_LITERAL(4, 53, 14), // "programChanged"
QT_MOC_LITERAL(5, 68, 17), // "exerciseRequested"
QT_MOC_LITERAL(6, 86, 6), // "menuId"
QT_MOC_LITERAL(7, 93, 20), // "protocolsModeChanged"
QT_MOC_LITERAL(8, 114, 2), // "on"
QT_MOC_LITERAL(9, 117, 16), // "onBeforeNavigate"
QT_MOC_LITERAL(10, 134, 10), // "IDispatch*"
QT_MOC_LITERAL(11, 145, 5), // "frame"
QT_MOC_LITERAL(12, 151, 9), // "QVariant&"
QT_MOC_LITERAL(13, 161, 3), // "url"
QT_MOC_LITERAL(14, 165, 5), // "flags"
QT_MOC_LITERAL(15, 171, 6), // "target"
QT_MOC_LITERAL(16, 178, 8), // "postData"
QT_MOC_LITERAL(17, 187, 7), // "headers"
QT_MOC_LITERAL(18, 195, 5), // "bool&"
QT_MOC_LITERAL(19, 201, 6), // "cancel"
QT_MOC_LITERAL(20, 208, 18), // "onNavigateComplete"
QT_MOC_LITERAL(21, 227, 18), // "onDocumentComplete"
QT_MOC_LITERAL(22, 246, 15), // "onBodyMouseDown"
QT_MOC_LITERAL(23, 262, 5), // "event"
QT_MOC_LITERAL(24, 268, 15), // "onDocumentClick"
QT_MOC_LITERAL(25, 284, 16) // "onDocumentEdited"

    },
    "ClinicalBrowserController\0helpIndexChanged\0"
    "\0helpFile\0programChanged\0exerciseRequested\0"
    "menuId\0protocolsModeChanged\0on\0"
    "onBeforeNavigate\0IDispatch*\0frame\0"
    "QVariant&\0url\0flags\0target\0postData\0"
    "headers\0bool&\0cancel\0onNavigateComplete\0"
    "onDocumentComplete\0onBodyMouseDown\0"
    "event\0onDocumentClick\0onDocumentEdited"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ClinicalBrowserController[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   64,    2, 0x06 /* Public */,
       4,    0,   67,    2, 0x06 /* Public */,
       5,    1,   68,    2, 0x06 /* Public */,
       7,    1,   71,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    7,   74,    2, 0x08 /* Private */,
      20,    2,   89,    2, 0x08 /* Private */,
      21,    2,   94,    2, 0x08 /* Private */,
      22,    1,   99,    2, 0x08 /* Private */,
      24,    1,  102,    2, 0x08 /* Private */,
      25,    1,  105,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::Bool,    8,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 10, 0x80000000 | 12, 0x80000000 | 12, 0x80000000 | 12, 0x80000000 | 12, 0x80000000 | 12, 0x80000000 | 18,   11,   13,   14,   15,   16,   17,   19,
    QMetaType::Void, 0x80000000 | 10, 0x80000000 | 12,   11,   13,
    QMetaType::Void, 0x80000000 | 10, 0x80000000 | 12,   11,   13,
    QMetaType::Void, 0x80000000 | 10,   23,
    QMetaType::Void, 0x80000000 | 10,   23,
    QMetaType::Void, 0x80000000 | 10,   23,

       0        // eod
};

void ClinicalBrowserController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ClinicalBrowserController *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->helpIndexChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->programChanged(); break;
        case 2: _t->exerciseRequested((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->protocolsModeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->onBeforeNavigate((*reinterpret_cast< IDispatch*(*)>(_a[1])),(*reinterpret_cast< QVariant(*)>(_a[2])),(*reinterpret_cast< QVariant(*)>(_a[3])),(*reinterpret_cast< QVariant(*)>(_a[4])),(*reinterpret_cast< QVariant(*)>(_a[5])),(*reinterpret_cast< QVariant(*)>(_a[6])),(*reinterpret_cast< bool(*)>(_a[7]))); break;
        case 5: _t->onNavigateComplete((*reinterpret_cast< IDispatch*(*)>(_a[1])),(*reinterpret_cast< QVariant(*)>(_a[2]))); break;
        case 6: _t->onDocumentComplete((*reinterpret_cast< IDispatch*(*)>(_a[1])),(*reinterpret_cast< QVariant(*)>(_a[2]))); break;
        case 7: _t->onBodyMouseDown((*reinterpret_cast< IDispatch*(*)>(_a[1]))); break;
        case 8: _t->onDocumentClick((*reinterpret_cast< IDispatch*(*)>(_a[1]))); break;
        case 9: _t->onDocumentEdited((*reinterpret_cast< IDispatch*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ClinicalBrowserController::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClinicalBrowserController::helpIndexChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ClinicalBrowserController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClinicalBrowserController::programChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ClinicalBrowserController::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClinicalBrowserController::exerciseRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ClinicalBrowserController::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClinicalBrowserController::protocolsModeChanged)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ClinicalBrowserController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ClinicalBrowserController.data,
    qt_meta_data_ClinicalBrowserController,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ClinicalBrowserController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ClinicalBrowserController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ClinicalBrowserController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ClinicalBrowserController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void ClinicalBrowserController::helpIndexChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ClinicalBrowserController::programChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ClinicalBrowserController::exerciseRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ClinicalBrowserController::protocolsModeChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
