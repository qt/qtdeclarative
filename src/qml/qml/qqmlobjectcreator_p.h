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
#include <private/qqmlcompiler_p.h>
#include <QLinkedList>

QT_BEGIN_NAMESPACE

class QQmlAbstractBinding;

class QQmlPropertyCacheCreator
{
    Q_DECLARE_TR_FUNCTIONS(QQmlPropertyCacheCreator)
public:
    QQmlPropertyCacheCreator(QQmlEnginePrivate *enginePrivate, const QV4::CompiledData::QmlUnit *unit,
                             const QUrl &url, const QQmlImports *imports,
                             QHash<int, QQmlCompiledData::TypeReference> *resolvedTypes);

    QList<QQmlError> errors;

    bool create(const QV4::CompiledData::Object *obj, QQmlPropertyCache **cache, QByteArray *vmeMetaObjectData);

protected:
    QString stringAt(int idx) const { return unit->header.stringAt(idx); }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    QQmlEnginePrivate *enginePrivate;
    const QV4::CompiledData::QmlUnit *unit;
    QUrl url;
    const QQmlImports *imports;
    QHash<int, QQmlCompiledData::TypeReference> *resolvedTypes;
};

class QmlObjectCreator
{
    Q_DECLARE_TR_FUNCTIONS(QmlObjectCreator)
public:
    QmlObjectCreator(QQmlContextData *contextData, const QV4::CompiledData::QmlUnit *qmlUnit, const QV4::CompiledData::CompilationUnit *jsUnit,
                     const QHash<int, QQmlCompiledData::TypeReference> &resolvedTypes, const QList<QQmlPropertyCache *> &propertyCaches, const QList<QByteArray> &vmeMetaObjectData,
                     const QHash<int, int> &objectIndexToId);

    QObject *create(QObject *parent = 0)
    { return create(unit->indexOfRootObject, parent); }
    QObject *create(int index, QObject *parent = 0);

    void finalize();

    QList<QQmlError> errors;

    QQmlComponentAttached *componentAttached;
    QList<QQmlEnginePrivate::FinalizeCallback> finalizeCallbacks;

private:
    bool populateInstance(int index, QObject *instance, QQmlRefPointer<QQmlPropertyCache> cache);

    void setupBindings();
    bool setPropertyValue(QQmlPropertyData *property, int index, const QV4::CompiledData::Binding *binding);
    void setPropertyValue(QQmlPropertyData *property, const QV4::CompiledData::Binding *binding);
    void setupFunctions();

    QString stringAt(int idx) const { return unit->header.stringAt(idx); }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    QQmlEngine *engine;
    QUrl url;
    const QV4::CompiledData::QmlUnit *unit;
    const QV4::CompiledData::CompilationUnit *jsUnit;
    QQmlContextData *context;
    const QHash<int, QQmlCompiledData::TypeReference> resolvedTypes;
    const QList<QQmlPropertyCache *> propertyCaches;
    const QList<QByteArray> vmeMetaObjectData;
    const QHash<int, int> &objectIndexToId;
    QLinkedList<QVector<QQmlAbstractBinding*> > allCreatedBindings;

    QObject *_qobject;
    const QV4::CompiledData::Object *_compiledObject;
    QQmlData *_ddata;
    QQmlRefPointer<QQmlPropertyCache> _propertyCache;
    QQmlVMEMetaObject *_vmeMetaObject;
    QVector<QQmlAbstractBinding*> _createdBindings;
    QQmlListProperty<void> _currentList;
    QV4::ExecutionContext *_qmlContext;
};

QT_END_NAMESPACE

#endif // QQMLOBJECTCREATOR_P_H
