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
    QObject::connect(object, &QObject::destroyed,
                     [qmlTypePrivate](QObject *) { qmlTypePrivate->release(); });

    // initialize QQmlType::QQmlCppTypeData
    Q_ASSERT(data.regType == QQmlType::CppType);
    qmlTypePrivate->extraData.cd->allocationSize = data.allocationSize;
    qmlTypePrivate->extraData.cd->newFunc = nullptr;
    qmlTypePrivate->extraData.cd->userdata = nullptr;
    qmlTypePrivate->extraData.cd->noCreationReason =
            QStringLiteral("Qmltc-compiled type is not creatable via QQmlType");
    qmlTypePrivate->extraData.cd->createValueTypeFunc = nullptr;
    qmlTypePrivate->extraData.cd->parserStatusCast = -1;
    qmlTypePrivate->extraData.cd->extFunc = nullptr;
    qmlTypePrivate->extraData.cd->extMetaObject = nullptr;
    qmlTypePrivate->extraData.cd->customParser = nullptr;
    qmlTypePrivate->extraData.cd->attachedPropertiesFunc = nullptr;
    qmlTypePrivate->extraData.cd->attachedPropertiesType = nullptr;
    qmlTypePrivate->extraData.cd->propertyValueSourceCast = -1;
    qmlTypePrivate->extraData.cd->propertyValueInterceptorCast = -1;
    qmlTypePrivate->extraData.cd->finalizerCast = -1;
    qmlTypePrivate->extraData.cd->registerEnumClassesUnscoped = false;
    qmlTypePrivate->extraData.cd->registerEnumsFromRelatedTypes = false;

    qmlTypePrivate->baseMetaObject = data.metaObject;
    qmlTypePrivate->init();

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
