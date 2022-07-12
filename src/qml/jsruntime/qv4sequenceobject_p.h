// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4SEQUENCEWRAPPER_P_H
#define QV4SEQUENCEWRAPPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqml.h>

#include "qv4value_p.h"
#include "qv4object_p.h"
#include "qv4context_p.h"
#include "qv4string_p.h"

#if QT_CONFIG(qml_itemmodel)
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qitemselectionmodel.h>
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Sequence;
struct Q_QML_PRIVATE_EXPORT SequencePrototype : public QV4::Object
{
    V4_PROTOTYPE(arrayPrototype)
    void init();

    static ReturnedValue method_valueOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sort(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue newSequence(QV4::ExecutionEngine *engine, QMetaType sequenceType, QObject *object, int propertyIndex, bool readOnly, bool *succeeded);
    static ReturnedValue fromVariant(QV4::ExecutionEngine *engine, const QVariant &v, bool *succeeded);
    static ReturnedValue fromData(QV4::ExecutionEngine *engine, QMetaType type, const void *data, bool *succeeded);

    static QMetaType metaTypeForSequence(const Sequence *object);
    static QVariant toVariant(const Sequence *object);
    static QVariant toVariant(const Value &array, QMetaType typeHint, bool *succeeded);
    static void *getRawContainerPtr(const Sequence *object, QMetaType typeHint);
};

namespace Heap {

struct Sequence : Object {
    void init(const QQmlType &qmlType, const void *container);
    void init(QObject *object, int propertyIndex, const QQmlType &qmlType, bool readOnly);
    void destroy();

    mutable void *container;
    const QQmlTypePrivate *typePrivate;
    QV4QPointer<QObject> object;
    int propertyIndex;
    bool isReference : 1;
    bool isReadOnly : 1;
};

}

struct Q_QML_PRIVATE_EXPORT Sequence : public QV4::Object
{
    V4_OBJECT2(Sequence, QV4::Object)
    Q_MANAGED_TYPE(V4Sequence)
    V4_PROTOTYPE(sequencePrototype)
    V4_NEEDS_DESTROY
public:
    static const QMetaType valueMetaType(const Heap::Sequence *p);
    static QV4::ReturnedValue virtualGet(
            const QV4::Managed *that, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *that, PropertyKey id, const QV4::Value &value, Value *receiver);
    static bool virtualDeleteProperty(QV4::Managed *that, PropertyKey id);
    static bool virtualIsEqualTo(Managed *that, Managed *other);
    static QV4::OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);

    qsizetype size() const;
    QVariant at(qsizetype index) const;
    void append(const QVariant &item);
    void append(qsizetype num, const QVariant &item);
    void replace(qsizetype index, const QVariant &item);
    void removeLast(qsizetype num);
    QVariant toVariant() const;

    QV4::ReturnedValue containerGetIndexed(qsizetype index, bool *hasProperty) const;
    bool containerPutIndexed(qsizetype index, const QV4::Value &value);
    bool containerDeleteIndexedProperty(qsizetype index);
    bool containerIsEqualTo(Managed *other);
    bool sort(const FunctionObject *f, const Value *, const Value *argv, int argc);
    void *getRawContainerPtr() const;
    void loadReference() const;
    void storeReference();
};

}

#define QT_DECLARE_SEQUENTIAL_CONTAINER(LOCAL, FOREIGN, VALUE) \
    struct LOCAL \
    { \
        Q_GADGET \
        QML_ANONYMOUS \
        QML_SEQUENTIAL_CONTAINER(VALUE) \
        QML_FOREIGN(FOREIGN) \
        QML_ADDED_IN_VERSION(2, 0) \
    }

// We use the original QT_COORD_TYPE name because that will match up with relevant other
// types in plugins.qmltypes (if you use either float or double, that is; otherwise you're
// on your own).
#ifdef QT_COORD_TYPE
QT_DECLARE_SEQUENTIAL_CONTAINER(QStdRealVectorForeign, std::vector<qreal>, QT_COORD_TYPE);
QT_DECLARE_SEQUENTIAL_CONTAINER(QRealListForeign, QList<qreal>, QT_COORD_TYPE);
#else
QT_DECLARE_SEQUENTIAL_CONTAINER(QRealStdVectorForeign, std::vector<qreal>, double);
QT_DECLARE_SEQUENTIAL_CONTAINER(QRealListForeign, QList<qreal>, double);
#endif

QT_DECLARE_SEQUENTIAL_CONTAINER(QDoubleStdVectorForeign, std::vector<double>, double);
QT_DECLARE_SEQUENTIAL_CONTAINER(QFloatStdVectorForeign, std::vector<float>, float);
QT_DECLARE_SEQUENTIAL_CONTAINER(QIntStdVectorForeign, std::vector<int>, int);
QT_DECLARE_SEQUENTIAL_CONTAINER(QBoolStdVectorForeign, std::vector<bool>, bool);
QT_DECLARE_SEQUENTIAL_CONTAINER(QStringStdVectorForeign, std::vector<QString>, QString);
QT_DECLARE_SEQUENTIAL_CONTAINER(QUrlStdVectorForeign, std::vector<QUrl>, QUrl);

#if QT_CONFIG(qml_itemmodel)
QT_DECLARE_SEQUENTIAL_CONTAINER(QModelIndexListForeign, QModelIndexList, QModelIndex);
QT_DECLARE_SEQUENTIAL_CONTAINER(QModelIndexStdVectorForeign, std::vector<QModelIndex>, QModelIndex);
QT_DECLARE_SEQUENTIAL_CONTAINER(QItemSelectionForeign, QItemSelection, QItemSelectionRange);
#endif

#undef QT_DECLARE_SEQUENTIAL_CONTAINER

QT_END_NAMESPACE

#endif // QV4SEQUENCEWRAPPER_P_H
