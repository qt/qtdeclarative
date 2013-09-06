/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QQMLOBJECTCREATOR_P_H
#define QQMLOBJECTCREATOR_P_H

#include <private/qqmlimport_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qv4compileddata_p.h>

QT_BEGIN_NAMESPACE

class QQmlAbstractBinding;

namespace QtQml {

class Q_QML_EXPORT QmlObjectCreator
{
    Q_DECLARE_TR_FUNCTIONS(QQmlCompiler)
public:

    QmlObjectCreator(QQmlEngine *engine, const QUrl &url,
                     // extra data/output stored in these two
                     QQmlContextData *contextData,
                     QQmlCompiledData *runtimeData);

    QObject *create(QObject *parent = 0)
    { return create(unit->indexOfRootObject); }
    QObject *create(int index, QObject *parent = 0);

    QList<QQmlError> errors;

    static bool needsCustomMetaObject(const QV4::CompiledData::Object *obj);
    bool createVMEMetaObjectAndPropertyCache(const QV4::CompiledData::Object *obj, QQmlPropertyCache *baseTypeCache,
                                             // out parameters
                                             QQmlPropertyCache **cache, QByteArray *vmeMetaObjectData);
private:
    QString stringAt(int idx) const { return unit->header.stringAt(idx); }

    QVector<QQmlAbstractBinding *> setupBindings(const QV4::CompiledData::Object *obj, QV4::Object *qmlGlobal);
    void setupFunctions(const QV4::CompiledData::Object *obj, QV4::Object *qmlGlobal);

    QVariant variantForBinding(int expectedMetaType, const QV4::CompiledData::Binding *binding) const;

    QString valueAsString(const QV4::CompiledData::Value *value) const;
    static double valueAsNumber(const QV4::CompiledData::Value *value);
    static bool valueAsBoolean(const QV4::CompiledData::Value *value);

    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    QQmlEngine *engine;
    QUrl url;
    const QV4::CompiledData::QmlUnit *unit;
    const QV4::CompiledData::CompilationUnit *jsUnit;
    QQmlContextData *context;
    QQmlTypeNameCache *typeNameCache;
    QQmlCompiledData *runtimeData;
    QQmlImports imports;

    QObject *_object;
    QQmlData *_ddata;
    QQmlRefPointer<QQmlPropertyCache> _propertyCache;
};

} // end namespace QtQml

QT_END_NAMESPACE

#endif // QQMLOBJECTCREATOR_P_H
