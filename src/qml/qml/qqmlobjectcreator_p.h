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
#include <private/qqmltypecompiler_p.h>
#include <private/qfinitestack_p.h>
#include <private/qrecursionwatcher_p.h>
#include <private/qqmlprofiler_p.h>

QT_BEGIN_NAMESPACE

class QQmlAbstractBinding;
struct QQmlTypeCompiler;
class QQmlInstantiationInterrupt;
struct QQmlVmeProfiler;

struct QQmlObjectCreatorSharedState
{
    QQmlContextData *rootContext;
    QQmlContextData *creationContext;
    QFiniteStack<QQmlAbstractBinding*> allCreatedBindings;
    QFiniteStack<QQmlParserStatus*> allParserStatusCallbacks;
    QFiniteStack<QObject*> allCreatedObjects;
    QQmlComponentAttached *componentAttached;
    QList<QQmlEnginePrivate::FinalizeCallback> finalizeCallbacks;
    QQmlVmeProfiler profiler;
    QRecursionNode recursionNode;
};

class QQmlObjectCreator
{
    Q_DECLARE_TR_FUNCTIONS(QQmlObjectCreator)
public:
    QQmlObjectCreator(QQmlContextData *parentContext, QQmlCompiledData *compiledData, QQmlContextData *creationContext, void *activeVMEDataForRootContext = 0);
    ~QQmlObjectCreator();

    QObject *create(int subComponentIndex = -1, QObject *parent = 0, QQmlInstantiationInterrupt *interrupt = 0);
    bool populateDeferredProperties(QObject *instance);
    QQmlContextData *finalize(QQmlInstantiationInterrupt &interrupt);
    void clear();

    QQmlComponentAttached **componentAttachment() { return &sharedState->componentAttached; }

    QList<QQmlEnginePrivate::FinalizeCallback> *finalizeCallbacks() { return &sharedState->finalizeCallbacks; }

    QList<QQmlError> errors;

    QQmlContextData *parentContextData() const { return parentContext; }
    QFiniteStack<QObject*> &allCreatedObjects() const { return sharedState->allCreatedObjects; }

private:
    QQmlObjectCreator(QQmlContextData *contextData, QQmlCompiledData *compiledData, QQmlObjectCreatorSharedState *inheritedSharedState);

    void init(QQmlContextData *parentContext);

    QObject *createInstance(int index, QObject *parent = 0, bool isContextObject = false);

    bool populateInstance(int index, QObject *instance,
                          QObject *bindingTarget, QQmlPropertyData *valueTypeProperty,
                          const QBitArray &bindingsToSkip = QBitArray());

    void setupBindings(const QBitArray &bindingsToSkip);
    bool setPropertyBinding(QQmlPropertyData *property, const QV4::CompiledData::Binding *binding);
    void setPropertyValue(QQmlPropertyData *property, const QV4::CompiledData::Binding *binding);
    void setupFunctions();

    QString stringAt(int idx) const { return qmlUnit->header.stringAt(idx); }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    enum Phase {
        Startup,
        CreatingObjects,
        CreatingObjectsPhase2,
        ObjectsCreated,
        Finalizing,
        Done
    } phase;

    QQmlEngine *engine;
    QQmlCompiledData *compiledData;
    const QV4::CompiledData::QmlUnit *qmlUnit;
    QQmlContextData *parentContext;
    QQmlContextData *context;
    const QHash<int, QQmlCompiledData::TypeReference*> &resolvedTypes;
    const QVector<QQmlPropertyCache *> &propertyCaches;
    const QVector<QByteArray> &vmeMetaObjectData;
    QHash<int, int> objectIndexToId;
    QFlagPointer<QQmlObjectCreatorSharedState> sharedState;
    void *activeVMEDataForRootContext;

    QObject *_qobject;
    QObject *_scopeObject;
    QObject *_bindingTarget;

    QQmlPropertyData *_valueTypeProperty; // belongs to _qobjectForBindings's property cache
    const QV4::CompiledData::Object *_compiledObject;
    QQmlData *_ddata;
    QQmlRefPointer<QQmlPropertyCache> _propertyCache;
    QQmlVMEMetaObject *_vmeMetaObject;
    QQmlListProperty<void> _currentList;
    QV4::ExecutionContext *_qmlContext;
};

QT_END_NAMESPACE

#endif // QQMLOBJECTCREATOR_P_H
