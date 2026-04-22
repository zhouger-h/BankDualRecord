/****************************************************************************
** Meta object code from reading C++ file 'DualRecordWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../core/DualRecordWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DualRecordWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DualRecordWidget_t {
    QByteArrayData data[28];
    char stringdata0[414];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DualRecordWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DualRecordWidget_t qt_meta_stringdata_DualRecordWidget = {
    {
QT_MOC_LITERAL(0, 0, 16), // "DualRecordWidget"
QT_MOC_LITERAL(1, 17, 13), // "recordStarted"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 6), // "taskId"
QT_MOC_LITERAL(4, 39, 8), // "filePath"
QT_MOC_LITERAL(5, 48, 13), // "recordStopped"
QT_MOC_LITERAL(6, 62, 11), // "recordError"
QT_MOC_LITERAL(7, 74, 8), // "errorMsg"
QT_MOC_LITERAL(8, 83, 12), // "uploadQueued"
QT_MOC_LITERAL(9, 96, 11), // "windowShown"
QT_MOC_LITERAL(10, 108, 12), // "windowHidden"
QT_MOC_LITERAL(11, 121, 18), // "onRecordBtnClicked"
QT_MOC_LITERAL(12, 140, 18), // "onConfigBtnClicked"
QT_MOC_LITERAL(13, 159, 16), // "onExitBtnClicked"
QT_MOC_LITERAL(14, 176, 15), // "onCameraChanged"
QT_MOC_LITERAL(15, 192, 5), // "index"
QT_MOC_LITERAL(16, 198, 14), // "updateDuration"
QT_MOC_LITERAL(17, 213, 27), // "onMediaRecorderStateChanged"
QT_MOC_LITERAL(18, 241, 21), // "QMediaRecorder::State"
QT_MOC_LITERAL(19, 263, 5), // "state"
QT_MOC_LITERAL(20, 269, 28), // "onMediaRecorderStatusChanged"
QT_MOC_LITERAL(21, 298, 22), // "QMediaRecorder::Status"
QT_MOC_LITERAL(22, 321, 6), // "status"
QT_MOC_LITERAL(23, 328, 20), // "onMediaRecorderError"
QT_MOC_LITERAL(24, 349, 21), // "QMediaRecorder::Error"
QT_MOC_LITERAL(25, 371, 5), // "error"
QT_MOC_LITERAL(26, 377, 15), // "onRecorderReady"
QT_MOC_LITERAL(27, 393, 20) // "onRecordStartTimeout"

    },
    "DualRecordWidget\0recordStarted\0\0taskId\0"
    "filePath\0recordStopped\0recordError\0"
    "errorMsg\0uploadQueued\0windowShown\0"
    "windowHidden\0onRecordBtnClicked\0"
    "onConfigBtnClicked\0onExitBtnClicked\0"
    "onCameraChanged\0index\0updateDuration\0"
    "onMediaRecorderStateChanged\0"
    "QMediaRecorder::State\0state\0"
    "onMediaRecorderStatusChanged\0"
    "QMediaRecorder::Status\0status\0"
    "onMediaRecorderError\0QMediaRecorder::Error\0"
    "error\0onRecorderReady\0onRecordStartTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DualRecordWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   94,    2, 0x06 /* Public */,
       5,    2,   99,    2, 0x06 /* Public */,
       6,    2,  104,    2, 0x06 /* Public */,
       8,    1,  109,    2, 0x06 /* Public */,
       9,    0,  112,    2, 0x06 /* Public */,
      10,    0,  113,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    0,  114,    2, 0x08 /* Private */,
      12,    0,  115,    2, 0x08 /* Private */,
      13,    0,  116,    2, 0x08 /* Private */,
      14,    1,  117,    2, 0x08 /* Private */,
      16,    0,  120,    2, 0x08 /* Private */,
      17,    1,  121,    2, 0x08 /* Private */,
      20,    1,  124,    2, 0x08 /* Private */,
      23,    1,  127,    2, 0x08 /* Private */,
      26,    0,  130,    2, 0x08 /* Private */,
      27,    0,  131,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    7,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   15,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 18,   19,
    QMetaType::Void, 0x80000000 | 21,   22,
    QMetaType::Void, 0x80000000 | 24,   25,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void DualRecordWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DualRecordWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->recordStarted((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->recordStopped((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 2: _t->recordError((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: _t->uploadQueued((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->windowShown(); break;
        case 5: _t->windowHidden(); break;
        case 6: _t->onRecordBtnClicked(); break;
        case 7: _t->onConfigBtnClicked(); break;
        case 8: _t->onExitBtnClicked(); break;
        case 9: _t->onCameraChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->updateDuration(); break;
        case 11: _t->onMediaRecorderStateChanged((*reinterpret_cast< QMediaRecorder::State(*)>(_a[1]))); break;
        case 12: _t->onMediaRecorderStatusChanged((*reinterpret_cast< QMediaRecorder::Status(*)>(_a[1]))); break;
        case 13: _t->onMediaRecorderError((*reinterpret_cast< QMediaRecorder::Error(*)>(_a[1]))); break;
        case 14: _t->onRecorderReady(); break;
        case 15: _t->onRecordStartTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMediaRecorder::State >(); break;
            }
            break;
        case 12:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMediaRecorder::Status >(); break;
            }
            break;
        case 13:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMediaRecorder::Error >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DualRecordWidget::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DualRecordWidget::recordStarted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DualRecordWidget::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DualRecordWidget::recordStopped)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DualRecordWidget::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DualRecordWidget::recordError)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (DualRecordWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DualRecordWidget::uploadQueued)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (DualRecordWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DualRecordWidget::windowShown)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (DualRecordWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DualRecordWidget::windowHidden)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DualRecordWidget::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_DualRecordWidget.data,
    qt_meta_data_DualRecordWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DualRecordWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DualRecordWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DualRecordWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int DualRecordWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void DualRecordWidget::recordStarted(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DualRecordWidget::recordStopped(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DualRecordWidget::recordError(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DualRecordWidget::uploadQueued(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void DualRecordWidget::windowShown()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void DualRecordWidget::windowHidden()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
