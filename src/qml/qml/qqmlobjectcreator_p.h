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

struct QQmlCompilePass
{
    QQmlCompilePass(const QUrl &url, const QV4::CompiledData::QmlUnit *unit);
    QList<QQmlError> errors;

protected:
    QString stringAt(int idx) const { return qmlUnit->header.stringAt(idx); }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    const QUrl url;
    const QV4::CompiledData::QmlUnit *qmlUnit;
};

class QQmlPropertyCacheCreator : public QQmlCompilePass
{
    Q_DECLARE_TR_FUNCTIONS(QQmlPropertyCacheCreator)
public:
    QQmlPropertyCacheCreator(QQmlEnginePrivate *enginePrivate, const QV4::CompiledData::QmlUnit *qmlUnit,
                             const QUrl &url, const QQmlImports *imports,
                             QHash<int, QQmlCompiledData::TypeReference> *resolvedTypes);

    bool create(const QV4::CompiledData::Object *obj, QQmlPropertyCache **cache, QByteArray *vmeMetaObjectData);

protected:
    QQmlEnginePrivate *enginePrivate;
    const QQmlImports *imports;
    QHash<int, QQmlCompiledData::TypeReference> *resolvedTypes;
};

class QQmlAnonymousComponentResolver : public QQmlCompilePass
{
    Q_DECLARE_TR_FUNCTIONS(QQmlAnonymousComponentResolver)
public:
    QQmlAnonymousComponentResolver(const QUrl &url, const QV4::CompiledData::QmlUnit *qmlUnit,
                                   const QHash<int, QQmlCompiledData::TypeReference> &resolvedTypes,
                                   const QList<QQmlPropertyCache *> &propertyCaches);

    bool resolve();

    QVector<int> componentRoots;
    QHash<int, int> objectIndexToComponentIndex;

protected:
    bool recordComponentSubTree(int objectIndex);

    int _componentIndex;
    QSet<int> _ids;

    const QHash<int, QQmlCompiledData::TypeReference> resolvedTypes;
    const QList<QQmlPropertyCache *> propertyCaches;
};

class QmlObjectCreator : public QQmlCompilePass
{
    Q_DECLARE_TR_FUNCTIONS(QmlObjectCreator)
public:
    QmlObjectCreator(QQmlContextData *contextData, QQmlCompiledData *compiledData);

    QObject *create(int subComponentIndex = -1, QObject *parent = 0);
    void finalize();

    QQmlComponentAttached *componentAttached;
    QList<QQmlEnginePrivate::FinalizeCallback> finalizeCallbacks;

private:
    QObject *createInstance(int index, QObject *parent = 0);

    bool populateInstance(int index, QObject *instance, QQmlRefPointer<QQmlPropertyCache> cache, QObject *scopeObjectForJavaScript);

    void setupBindings();
    bool setPropertyValue(QQmlPropertyData *property, int index, const QV4::CompiledData::Binding *binding);
    void setPropertyValue(QQmlPropertyData *property, const QV4::CompiledData::Binding *binding);
    void setupFunctions();

    QQmlEngine *engine;
    const QV4::CompiledData::CompilationUnit *jsUnit;
    QQmlContextData *parentContext;
    QQmlContextData *context;
    const QHash<int, QQmlCompiledData::TypeReference> resolvedTypes;
    const QList<QQmlPropertyCache *> propertyCaches;
    const QList<QByteArray> vmeMetaObjectData;
    QHash<int, int> objectIndexToId;
    QLinkedList<QVector<QQmlAbstractBinding*> > allCreatedBindings;
    QQmlCompiledData *compiledData;

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
