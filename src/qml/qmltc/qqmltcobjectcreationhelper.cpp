/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qqmltcobjectcreationhelper_p.h"

#include <private/qqmlmetatype_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qobject_p.h>
#include <private/qqmltype_p_p.h>

QT_BEGIN_NAMESPACE

void qmltcCreateDynamicMetaObject(QObject *object, const QMetaObject *staticMetaObject,
                                  const QmltcTypeData &data)
{
    // TODO: when/if qmltc-compiled types would be registered via
    // qmltyperegistrar, instead of creating a dummy QQmlTypePrivate, fetch the
    // good QQmlType via QQmlMetaType::qmlType(). to do it correctly, one needs,
    // along with the meta object, module name and revision. all that should be
    // available ahead-of-time to qmltc.
    auto qmlTypePrivate = new QQmlTypePrivate(data.regType);
    // place the newly created QQmlTypePrivate into the qml meta data storage so
    // that it is properly deleted
    QQmlMetaType::registerMetaObjectForType(staticMetaObject, qmlTypePrivate);

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
