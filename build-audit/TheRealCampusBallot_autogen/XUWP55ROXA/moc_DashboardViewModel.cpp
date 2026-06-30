/****************************************************************************
** Meta object code from reading C++ file 'DashboardViewModel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/viewmodels/DashboardViewModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DashboardViewModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN6Ballot10ViewModels18DashboardViewModelE_t {};
} // unnamed namespace

template <> constexpr inline auto Ballot::ViewModels::DashboardViewModel::qt_create_metaobjectdata<qt_meta_tag_ZN6Ballot10ViewModels18DashboardViewModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Ballot::ViewModels::DashboardViewModel",
        "totalStudentsChanged",
        "",
        "votesCastChanged",
        "turnoutChanged",
        "roleChanged",
        "votingStatusChanged",
        "currentElectionTitleChanged",
        "dbStatusChanged",
        "storageTypeChanged",
        "serverStatusChanged",
        "auditStatusChanged",
        "backupStatusChanged",
        "errorOccurred",
        "error",
        "totalStudents",
        "votesCast",
        "turnout",
        "isMaster",
        "votingStatus",
        "currentElectionTitle",
        "dbStatus",
        "storageType",
        "serverStatus",
        "auditStatus",
        "backupStatus"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'totalStudentsChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'votesCastChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'turnoutChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'roleChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'votingStatusChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentElectionTitleChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'dbStatusChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'storageTypeChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'serverStatusChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'auditStatusChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'backupStatusChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'totalStudents'
        QtMocHelpers::PropertyData<int>(15, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'votesCast'
        QtMocHelpers::PropertyData<int>(16, QMetaType::Int, QMC::DefaultPropertyFlags, 1),
        // property 'turnout'
        QtMocHelpers::PropertyData<double>(17, QMetaType::Double, QMC::DefaultPropertyFlags, 2),
        // property 'isMaster'
        QtMocHelpers::PropertyData<bool>(18, QMetaType::Bool, QMC::DefaultPropertyFlags, 3),
        // property 'votingStatus'
        QtMocHelpers::PropertyData<QString>(19, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
        // property 'currentElectionTitle'
        QtMocHelpers::PropertyData<QString>(20, QMetaType::QString, QMC::DefaultPropertyFlags, 5),
        // property 'dbStatus'
        QtMocHelpers::PropertyData<QString>(21, QMetaType::QString, QMC::DefaultPropertyFlags, 6),
        // property 'storageType'
        QtMocHelpers::PropertyData<QString>(22, QMetaType::QString, QMC::DefaultPropertyFlags, 7),
        // property 'serverStatus'
        QtMocHelpers::PropertyData<QString>(23, QMetaType::QString, QMC::DefaultPropertyFlags, 8),
        // property 'auditStatus'
        QtMocHelpers::PropertyData<QString>(24, QMetaType::QString, QMC::DefaultPropertyFlags, 9),
        // property 'backupStatus'
        QtMocHelpers::PropertyData<QString>(25, QMetaType::QString, QMC::DefaultPropertyFlags, 10),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DashboardViewModel, qt_meta_tag_ZN6Ballot10ViewModels18DashboardViewModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Ballot::ViewModels::DashboardViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Ballot10ViewModels18DashboardViewModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Ballot10ViewModels18DashboardViewModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6Ballot10ViewModels18DashboardViewModelE_t>.metaTypes,
    nullptr
} };

void Ballot::ViewModels::DashboardViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DashboardViewModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->totalStudentsChanged(); break;
        case 1: _t->votesCastChanged(); break;
        case 2: _t->turnoutChanged(); break;
        case 3: _t->roleChanged(); break;
        case 4: _t->votingStatusChanged(); break;
        case 5: _t->currentElectionTitleChanged(); break;
        case 6: _t->dbStatusChanged(); break;
        case 7: _t->storageTypeChanged(); break;
        case 8: _t->serverStatusChanged(); break;
        case 9: _t->auditStatusChanged(); break;
        case 10: _t->backupStatusChanged(); break;
        case 11: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::totalStudentsChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::votesCastChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::turnoutChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::roleChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::votingStatusChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::currentElectionTitleChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::dbStatusChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::storageTypeChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::serverStatusChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::auditStatusChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)()>(_a, &DashboardViewModel::backupStatusChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (DashboardViewModel::*)(const QString & )>(_a, &DashboardViewModel::errorOccurred, 11))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<int*>(_v) = _t->totalStudents(); break;
        case 1: *reinterpret_cast<int*>(_v) = _t->votesCast(); break;
        case 2: *reinterpret_cast<double*>(_v) = _t->turnout(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->isMaster(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->votingStatus(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->currentElectionTitle(); break;
        case 6: *reinterpret_cast<QString*>(_v) = _t->dbStatus(); break;
        case 7: *reinterpret_cast<QString*>(_v) = _t->storageType(); break;
        case 8: *reinterpret_cast<QString*>(_v) = _t->serverStatus(); break;
        case 9: *reinterpret_cast<QString*>(_v) = _t->auditStatus(); break;
        case 10: *reinterpret_cast<QString*>(_v) = _t->backupStatus(); break;
        default: break;
        }
    }
}

const QMetaObject *Ballot::ViewModels::DashboardViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Ballot::ViewModels::DashboardViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6Ballot10ViewModels18DashboardViewModelE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Ballot::ViewModels::DashboardViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void Ballot::ViewModels::DashboardViewModel::totalStudentsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Ballot::ViewModels::DashboardViewModel::votesCastChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Ballot::ViewModels::DashboardViewModel::turnoutChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void Ballot::ViewModels::DashboardViewModel::roleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void Ballot::ViewModels::DashboardViewModel::votingStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void Ballot::ViewModels::DashboardViewModel::currentElectionTitleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void Ballot::ViewModels::DashboardViewModel::dbStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void Ballot::ViewModels::DashboardViewModel::storageTypeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void Ballot::ViewModels::DashboardViewModel::serverStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void Ballot::ViewModels::DashboardViewModel::auditStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void Ballot::ViewModels::DashboardViewModel::backupStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void Ballot::ViewModels::DashboardViewModel::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}
QT_WARNING_POP
