/****************************************************************************
** Meta object code from reading C++ file 'UploadManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../core/UploadManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'UploadManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_UploadWorker_t {
    QByteArrayData data[14];
    char stringdata0[133];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_UploadWorker_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_UploadWorker_t qt_meta_stringdata_UploadWorker = {
    {
QT_MOC_LITERAL(0, 0, 12), // "UploadWorker"
QT_MOC_LITERAL(1, 13, 13), // "uploadSuccess"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 9), // "localPath"
QT_MOC_LITERAL(4, 38, 10), // "remotePath"
QT_MOC_LITERAL(5, 49, 12), // "uploadFailed"
QT_MOC_LITERAL(6, 62, 8), // "errorMsg"
QT_MOC_LITERAL(7, 71, 10), // "retryCount"
QT_MOC_LITERAL(8, 82, 14), // "uploadProgress"
QT_MOC_LITERAL(9, 97, 4), // "sent"
QT_MOC_LITERAL(10, 102, 5), // "total"
QT_MOC_LITERAL(11, 108, 8), // "doUpload"
QT_MOC_LITERAL(12, 117, 10), // "UploadTask"
QT_MOC_LITERAL(13, 128, 4) // "task"

    },
    "UploadWorker\0uploadSuccess\0\0localPath\0"
    "remotePath\0uploadFailed\0errorMsg\0"
    "retryCount\0uploadProgress\0sent\0total\0"
    "doUpload\0UploadTask\0task"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_UploadWorker[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   34,    2, 0x06 /* Public */,
       5,    3,   39,    2, 0x06 /* Public */,
       8,    3,   46,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    1,   53,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Int,    3,    6,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong, QMetaType::LongLong,    3,    9,   10,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 12,   13,

       0        // eod
};

void UploadWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UploadWorker *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->uploadSuccess((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->uploadFailed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 2: _t->uploadProgress((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2])),(*reinterpret_cast< qint64(*)>(_a[3]))); break;
        case 3: _t->doUpload((*reinterpret_cast< const UploadTask(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< UploadTask >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UploadWorker::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UploadWorker::uploadSuccess)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UploadWorker::*)(const QString & , const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UploadWorker::uploadFailed)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UploadWorker::*)(const QString & , qint64 , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UploadWorker::uploadProgress)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject UploadWorker::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_UploadWorker.data,
    qt_meta_data_UploadWorker,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *UploadWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UploadWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_UploadWorker.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UploadWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void UploadWorker::uploadSuccess(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void UploadWorker::uploadFailed(const QString & _t1, const QString & _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void UploadWorker::uploadProgress(const QString & _t1, qint64 _t2, qint64 _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
struct qt_meta_stringdata_UploadManager_t {
    QByteArrayData data[19];
    char stringdata0[209];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_UploadManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_UploadManager_t qt_meta_stringdata_UploadManager = {
    {
QT_MOC_LITERAL(0, 0, 13), // "UploadManager"
QT_MOC_LITERAL(1, 14, 13), // "uploadStarted"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 9), // "localPath"
QT_MOC_LITERAL(4, 39, 13), // "uploadSuccess"
QT_MOC_LITERAL(5, 53, 12), // "uploadFailed"
QT_MOC_LITERAL(6, 66, 5), // "error"
QT_MOC_LITERAL(7, 72, 14), // "uploadProgress"
QT_MOC_LITERAL(8, 87, 4), // "sent"
QT_MOC_LITERAL(9, 92, 5), // "total"
QT_MOC_LITERAL(10, 98, 12), // "queueChanged"
QT_MOC_LITERAL(11, 111, 5), // "count"
QT_MOC_LITERAL(12, 117, 15), // "onUploadSuccess"
QT_MOC_LITERAL(13, 133, 10), // "remotePath"
QT_MOC_LITERAL(14, 144, 14), // "onUploadFailed"
QT_MOC_LITERAL(15, 159, 8), // "errorMsg"
QT_MOC_LITERAL(16, 168, 10), // "retryCount"
QT_MOC_LITERAL(17, 179, 16), // "onUploadProgress"
QT_MOC_LITERAL(18, 196, 12) // "processQueue"

    },
    "UploadManager\0uploadStarted\0\0localPath\0"
    "uploadSuccess\0uploadFailed\0error\0"
    "uploadProgress\0sent\0total\0queueChanged\0"
    "count\0onUploadSuccess\0remotePath\0"
    "onUploadFailed\0errorMsg\0retryCount\0"
    "onUploadProgress\0processQueue"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_UploadManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x06 /* Public */,
       4,    1,   62,    2, 0x06 /* Public */,
       5,    2,   65,    2, 0x06 /* Public */,
       7,    3,   70,    2, 0x06 /* Public */,
      10,    1,   77,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    2,   80,    2, 0x08 /* Private */,
      14,    3,   85,    2, 0x08 /* Private */,
      17,    3,   92,    2, 0x08 /* Private */,
      18,    0,   99,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong, QMetaType::LongLong,    3,    8,    9,
    QMetaType::Void, QMetaType::Int,   11,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,   13,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Int,    3,   15,   16,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong, QMetaType::LongLong,    3,    8,    9,
    QMetaType::Void,

       0        // eod
};

void UploadManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UploadManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->uploadStarted((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->uploadSuccess((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->uploadFailed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: _t->uploadProgress((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2])),(*reinterpret_cast< qint64(*)>(_a[3]))); break;
        case 4: _t->queueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->onUploadSuccess((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 6: _t->onUploadFailed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 7: _t->onUploadProgress((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2])),(*reinterpret_cast< qint64(*)>(_a[3]))); break;
        case 8: _t->processQueue(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UploadManager::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UploadManager::uploadStarted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UploadManager::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UploadManager::uploadSuccess)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UploadManager::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UploadManager::uploadFailed)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UploadManager::*)(const QString & , qint64 , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UploadManager::uploadProgress)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (UploadManager::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UploadManager::queueChanged)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject UploadManager::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_UploadManager.data,
    qt_meta_data_UploadManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *UploadManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UploadManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_UploadManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UploadManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void UploadManager::uploadStarted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void UploadManager::uploadSuccess(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void UploadManager::uploadFailed(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void UploadManager::uploadProgress(const QString & _t1, qint64 _t2, qint64 _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void UploadManager::queueChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
