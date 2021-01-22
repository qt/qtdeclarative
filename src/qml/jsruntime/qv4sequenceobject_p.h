/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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

#include "qv4value_p.h"
#include "qv4object_p.h"
#include "qv4context_p.h"
#include "qv4string_p.h"

QT_REQUIRE_CONFIG(qml_sequence_object);

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Q_QML_PRIVATE_EXPORT SequencePrototype : public QV4::Object
{
    V4_PROTOTYPE(arrayPrototype)
    void init();

    static ReturnedValue method_valueOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sort(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static bool isSequenceType(int sequenceTypeId);
    static ReturnedValue newSequence(QV4::ExecutionEngine *engine, int sequenceTypeId, QObject *object, int propertyIndex, bool readOnly, bool *succeeded);
    static ReturnedValue fromVariant(QV4::ExecutionEngine *engine, const QVariant& v, bool *succeeded);
    static int metaTypeForSequence(const Object *object);
    static QVariant toVariant(Object *object);
    static QVariant toVariant(const Value &array, int typeHint, bool *succeeded);
    static void* getRawContainerPtr(const Object *object, int typeHint);
};

}

QT_END_NAMESPACE

#endif // QV4SEQUENCEWRAPPER_P_H
