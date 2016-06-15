/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
#ifndef QQMLPROPERTYCACHECREATOR_P_H
#define QQMLPROPERTYCACHECREATOR_P_H

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

#include "qqmltypecompiler_p.h"

QT_BEGIN_NAMESPACE

class QQmlPropertyCacheCreator
{
    Q_DECLARE_TR_FUNCTIONS(QQmlPropertyCacheCreator)
public:
    QQmlPropertyCacheCreator(QQmlPropertyCacheVector *propertyCaches, QQmlEnginePrivate *enginePrivate, const QQmlTypeCompiler *compiler, const QQmlImports *imports);
    ~QQmlPropertyCacheCreator();

    QQmlCompileError buildMetaObjects();

protected:
    struct InstantiationContext {
        InstantiationContext();
        InstantiationContext(int referencingObjectIndex, const QV4::CompiledData::Binding *instantiatingBinding, const QString &instantiatingPropertyName, const QQmlPropertyCache *referencingObjectPropertyCache);
        int referencingObjectIndex;
        const QV4::CompiledData::Binding *instantiatingBinding;
        QQmlPropertyData *instantiatingProperty;
    };

    QQmlCompileError buildMetaObjectRecursively(int objectIndex, const InstantiationContext &context);
    QQmlPropertyCache *propertyCacheForObject(const QmlIR::Object *obj, const InstantiationContext &context, QQmlCompileError *error) const;
    QQmlCompileError createMetaObject(int objectIndex, const QmlIR::Object *obj, QQmlPropertyCache *baseTypeCache);

    QString stringAt(int index) const { return compiler->stringAt(index); }

    QQmlEnginePrivate * const enginePrivate;
    const QQmlTypeCompiler *compiler;
    const QVector<QmlIR::Object*> &qmlObjects;
    const QQmlImports * const imports;
    QQmlPropertyCacheVector *propertyCaches;
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYCACHECREATOR_P_H
