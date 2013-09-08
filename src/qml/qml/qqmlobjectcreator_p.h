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

class QQmlPropertyCacheCreator
{
    Q_DECLARE_TR_FUNCTIONS(QQmlPropertyCacheCreator)
public:
    QQmlPropertyCacheCreator(QQmlEnginePrivate *enginePrivate, const QV4::CompiledData::QmlUnit *unit,
                             const QUrl &url, QQmlTypeNameCache *typeNameCache, const QQmlImports *imports);

    QList<QQmlError> errors;

    bool create(const QV4::CompiledData::Object *obj, QQmlPropertyCache **cache, QByteArray *vmeMetaObjectData);

protected:
    QString stringAt(int idx) const { return unit->header.stringAt(idx); }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    QQmlEnginePrivate *enginePrivate;
    const QV4::CompiledData::QmlUnit *unit;
    QUrl url;
    QQmlTypeNameCache *typeNameCache;
    const QQmlImports *imports;
};

class QmlObjectCreator
{
    Q_DECLARE_TR_FUNCTIONS(QQmlCompiler)
public:
    QmlObjectCreator(QQmlContextData *contextData, const QV4::CompiledData::QmlUnit *qmlUnit, const QV4::CompiledData::CompilationUnit *jsUnit,
                     QQmlTypeNameCache *typeNameCache, const QList<QQmlPropertyCache *> &propertyCaches, const QList<QByteArray> &vmeMetaObjectData);

    QObject *create(QObject *parent = 0)
    { return create(unit->indexOfRootObject, parent); }
    QObject *create(int index, QObject *parent = 0);

    QList<QQmlError> errors;

private:
    QVector<QQmlAbstractBinding *> setupBindings(QV4::Object *qmlGlobal);
    void setupFunctions(QV4::Object *qmlGlobal);

    QVariant variantForBinding(int expectedMetaType, const QV4::CompiledData::Binding *binding) const;

    QString valueAsString(const QV4::CompiledData::Value *value) const;
    static double valueAsNumber(const QV4::CompiledData::Value *value);
    static bool valueAsBoolean(const QV4::CompiledData::Value *value);

    QString stringAt(int idx) const { return unit->header.stringAt(idx); }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    QQmlEngine *engine;
    QUrl url;
    const QV4::CompiledData::QmlUnit *unit;
    const QV4::CompiledData::CompilationUnit *jsUnit;
    QQmlContextData *context;
    QQmlTypeNameCache *typeNameCache;
    const QList<QQmlPropertyCache *> propertyCaches;
    const QList<QByteArray> vmeMetaObjectData;

    QObject *_qobject;
    const QV4::CompiledData::Object *_compiledObject;
    QQmlData *_ddata;
    QQmlRefPointer<QQmlPropertyCache> _propertyCache;
};

QT_END_NAMESPACE

#endif // QQMLOBJECTCREATOR_P_H
