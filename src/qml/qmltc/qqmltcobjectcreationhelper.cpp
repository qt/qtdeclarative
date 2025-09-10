// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltcobjectcreationhelper_p.h"

#include <private/qqmlmetatype_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qobject_p.h>
#include <private/qqmltype_p_p.h>

QT_BEGIN_NAMESPACE

void qmltcCreateDynamicMetaObject(QObject *object, const QmltcTypeData &data)
{
    // TODO: when/if qmltc-compiled types would be registered via
    // qmltyperegistrar, instead of creating a dummy QQmlTypePrivate, fetch the
    // good QQmlType via QQmlMetaType::qmlType(). to do it correctly, one needs,
    // along with the meta object, module name and revision. all that should be
    // available ahead-of-time to qmltc.
    auto qmlTypePrivate = new QQmlTypePrivate(data.regType);

    // tie qmlTypePrivate destruction to objects's destruction. the type's
    // content is not needed once the associated object is deleted
    QObject::connect(object, &QObject::destroyed, object,
                     [qmlTypePrivate](QObject *) { qmlTypePrivate->release(); },
                     Qt::DirectConnection);

    // initialize QQmlType::QQmlCppTypeData
    Q_ASSERT(data.regType == QQmlType::CppType);
    qmlTypePrivate->extraData.cppTypeData->allocationSize = data.allocationSize;
    qmlTypePrivate->extraData.cppTypeData->newFunc = nullptr;
    qmlTypePrivate->extraData.cppTypeData->userdata = nullptr;
    qmlTypePrivate->extraData.cppTypeData->noCreationReason =
            QStringLiteral("Qmltc-compiled type is not creatable via QQmlType");
    qmlTypePrivate->extraData.cppTypeData->createValueTypeFunc = nullptr;
    qmlTypePrivate->extraData.cppTypeData->parserStatusCast = -1;
    qmlTypePrivate->extraData.cppTypeData->extFunc = nullptr;
    qmlTypePrivate->extraData.cppTypeData->extMetaObject = nullptr;
    qmlTypePrivate->extraData.cppTypeData->customParser = nullptr;
    qmlTypePrivate->extraData.cppTypeData->attachedPropertiesFunc = nullptr;
    qmlTypePrivate->extraData.cppTypeData->attachedPropertiesType = nullptr;
    qmlTypePrivate->extraData.cppTypeData->propertyValueSourceCast = -1;
    qmlTypePrivate->extraData.cppTypeData->propertyValueInterceptorCast = -1;
    qmlTypePrivate->extraData.cppTypeData->finalizerCast = -1;
    qmlTypePrivate->extraData.cppTypeData->registerEnumClassesUnscoped = false;
    qmlTypePrivate->extraData.cppTypeData->registerEnumsFromRelatedTypes = false;

    qmlTypePrivate->baseMetaObject = data.metaObject;

    QQmlType qmlType(qmlTypePrivate);
    Q_ASSERT(qmlType.isValid());

    QObjectPrivate *op = QObjectPrivate::get(object);
    // ### inefficient - rather, call this function only once for the leaf type
    if (op->metaObject) {
        delete op->metaObject;
        op->metaObject = nullptr;
    }

    qmlType.createProxy(object);
}

QT_END_NAMESPACE
