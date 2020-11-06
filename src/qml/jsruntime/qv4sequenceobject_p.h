/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
#include <private/qqmlmodelindexvaluetype_p.h>
#include <QtCore/qabstractitemmodel.h>
#endif

QT_REQUIRE_CONFIG(qml_sequence_object);

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Q_QML_PRIVATE_EXPORT SequencePrototype : public QV4::Object
{
    V4_PROTOTYPE(arrayPrototype)
    void init();

    static ReturnedValue method_valueOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sort(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue newSequence(QV4::ExecutionEngine *engine, int sequenceTypeId, QObject *object, int propertyIndex, bool readOnly, bool *succeeded);
    static ReturnedValue fromVariant(QV4::ExecutionEngine *engine, const QVariant& v, bool *succeeded);
    static int metaTypeForSequence(const Object *object);
    static QVariant toVariant(Object *object);
    static QVariant toVariant(const Value &array, int typeHint, bool *succeeded);
    static void* getRawContainerPtr(const Object *object, int typeHint);
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

QT_DECLARE_SEQUENTIAL_CONTAINER(QIntStdVectorForeign, std::vector<int>, int);
QT_DECLARE_SEQUENTIAL_CONTAINER(QBoolStdVectorForeign, std::vector<bool>, bool);
QT_DECLARE_SEQUENTIAL_CONTAINER(QStringStdVectorForeign, std::vector<QString>, QString);
QT_DECLARE_SEQUENTIAL_CONTAINER(QUrlStdVectorForeign, std::vector<QUrl>, QUrl);

QT_DECLARE_SEQUENTIAL_CONTAINER(QIntListForeign, QList<int>, int);
QT_DECLARE_SEQUENTIAL_CONTAINER(QBoolListForeign, QList<bool>, bool);
QT_DECLARE_SEQUENTIAL_CONTAINER(QStringListForeign, QStringList, QString);
QT_DECLARE_SEQUENTIAL_CONTAINER(QUrlListForeign, QList<QUrl>, QUrl);

#if QT_CONFIG(qml_itemmodel)
QT_DECLARE_SEQUENTIAL_CONTAINER(QModelIndexListForeign, QModelIndexList, QModelIndex);
QT_DECLARE_SEQUENTIAL_CONTAINER(QModelIndexStdVectorForeign, std::vector<QModelIndex>, QModelIndex);
QT_DECLARE_SEQUENTIAL_CONTAINER(QItemSelectionForeign, QItemSelection, QItemSelectionRange);
#endif

#undef QT_DECLARE_SEQUENTIAL_CONTAINER

QT_END_NAMESPACE

#endif // QV4SEQUENCEWRAPPER_P_H
