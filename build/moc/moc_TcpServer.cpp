/****************************************************************************
** Meta object code from reading C++ file 'TcpServer.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../core/TcpServer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TcpServer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ClientSession_t {
    QByteArrayData data[8];
    char stringdata0[84];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ClientSession_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ClientSession_t qt_meta_stringdata_ClientSession = {
    {
QT_MOC_LITERAL(0, 0, 13), // "ClientSession"
QT_MOC_LITERAL(1, 14, 15), // "commandReceived"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 8), // "clientId"
QT_MOC_LITERAL(4, 40, 3), // "cmd"
QT_MOC_LITERAL(5, 44, 12), // "disconnected"
QT_MOC_LITERAL(6, 57, 11), // "onReadyRead"
QT_MOC_LITERAL(7, 69, 14) // "onDisconnected"

    },
    "ClientSession\0commandReceived\0\0clientId\0"
    "cmd\0disconnected\0onReadyRead\0"
    "onDisconnected"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ClientSession[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   34,    2, 0x06 /* Public */,
       5,    1,   39,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   42,    2, 0x08 /* Private */,
       7,    0,   43,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonObject,    3,    4,
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ClientSession::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ClientSession *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->commandReceived((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QJsonObject(*)>(_a[2]))); break;
        case 1: _t->disconnected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->onReadyRead(); break;
        case 3: _t->onDisconnected(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ClientSession::*)(const QString & , const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClientSession::commandReceived)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ClientSession::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClientSession::disconnected)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ClientSession::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_ClientSession.data,
    qt_meta_data_ClientSession,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ClientSession::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ClientSession::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ClientSession.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ClientSession::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void ClientSession::commandReceived(const QString & _t1, const QJsonObject & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ClientSession::disconnected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_TcpCommandServer_t {
    QByteArrayData data[19];
    char stringdata0[238];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TcpCommandServer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TcpCommandServer_t qt_meta_stringdata_TcpCommandServer = {
    {
QT_MOC_LITERAL(0, 0, 16), // "TcpCommandServer"
QT_MOC_LITERAL(1, 17, 13), // "serverStarted"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 4), // "port"
QT_MOC_LITERAL(4, 37, 13), // "serverStopped"
QT_MOC_LITERAL(5, 51, 15), // "clientConnected"
QT_MOC_LITERAL(6, 67, 8), // "clientId"
QT_MOC_LITERAL(7, 76, 18), // "clientDisconnected"
QT_MOC_LITERAL(8, 95, 15), // "onNewConnection"
QT_MOC_LITERAL(9, 111, 15), // "onClientCommand"
QT_MOC_LITERAL(10, 127, 3), // "cmd"
QT_MOC_LITERAL(11, 131, 20), // "onClientDisconnected"
QT_MOC_LITERAL(12, 152, 15), // "onRecordStarted"
QT_MOC_LITERAL(13, 168, 6), // "taskId"
QT_MOC_LITERAL(14, 175, 8), // "filePath"
QT_MOC_LITERAL(15, 184, 15), // "onRecordStopped"
QT_MOC_LITERAL(16, 200, 13), // "onRecordError"
QT_MOC_LITERAL(17, 214, 8), // "errorMsg"
QT_MOC_LITERAL(18, 223, 14) // "onUploadQueued"

    },
    "TcpCommandServer\0serverStarted\0\0port\0"
    "serverStopped\0clientConnected\0clientId\0"
    "clientDisconnected\0onNewConnection\0"
    "onClientCommand\0cmd\0onClientDisconnected\0"
    "onRecordStarted\0taskId\0filePath\0"
    "onRecordStopped\0onRecordError\0errorMsg\0"
    "onUploadQueued"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TcpCommandServer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   69,    2, 0x06 /* Public */,
       4,    0,   72,    2, 0x06 /* Public */,
       5,    1,   73,    2, 0x06 /* Public */,
       7,    1,   76,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    0,   79,    2, 0x08 /* Private */,
       9,    2,   80,    2, 0x08 /* Private */,
      11,    1,   85,    2, 0x08 /* Private */,
      12,    2,   88,    2, 0x08 /* Private */,
      15,    2,   93,    2, 0x08 /* Private */,
      16,    2,   98,    2, 0x08 /* Private */,
      18,    1,  103,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::UShort,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonObject,    6,   10,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   13,   14,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   13,   14,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   13,   17,
    QMetaType::Void, QMetaType::QString,   14,

       0        // eod
};

void TcpCommandServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TcpCommandServer *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->serverStarted((*reinterpret_cast< quint16(*)>(_a[1]))); break;
        case 1: _t->serverStopped(); break;
        case 2: _t->clientConnected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->clientDisconnected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->onNewConnection(); break;
        case 5: _t->onClientCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QJsonObject(*)>(_a[2]))); break;
        case 6: _t->onClientDisconnected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->onRecordStarted((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 8: _t->onRecordStopped((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 9: _t->onRecordError((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 10: _t->onUploadQueued((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TcpCommandServer::*)(quint16 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TcpCommandServer::serverStarted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TcpCommandServer::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TcpCommandServer::serverStopped)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TcpCommandServer::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TcpCommandServer::clientConnected)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (TcpCommandServer::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TcpCommandServer::clientDisconnected)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject TcpCommandServer::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_TcpCommandServer.data,
    qt_meta_data_TcpCommandServer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TcpCommandServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TcpCommandServer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TcpCommandServer.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TcpCommandServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void TcpCommandServer::serverStarted(quint16 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TcpCommandServer::serverStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void TcpCommandServer::clientConnected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void TcpCommandServer::clientDisconnected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
