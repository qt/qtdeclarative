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
#ifndef QQMLTYPECOMPILER_P_H
#define QQMLTYPECOMPILER_P_H

#include <qglobal.h>
#include <qqmlerror.h>
#include <qhash.h>
#include <private/qqmlcompiler_p.h>

QT_BEGIN_NAMESPACE

class QQmlEnginePrivate;
class QQmlCompiledData;
class QQmlError;
class QQmlTypeData;
class QQmlImports;

namespace QtQml {
struct ParsedQML;
}

namespace QV4 {
namespace CompiledData {
struct QmlUnit;
struct Location;
}
}

struct QQmlTypeCompiler
{
    QQmlTypeCompiler(QQmlEnginePrivate *engine, QQmlCompiledData *compiledData, QQmlTypeData *typeData, QtQml::ParsedQML *parsedQML);

    bool compile();

    QList<QQmlError> compilationErrors() const { return errors; }
    void recordError(const QQmlError &error);

    QString stringAt(int idx) const;
    int registerString(const QString &str);

    const QV4::CompiledData::QmlUnit *qmlUnit() const;

    QQmlEnginePrivate *enginePrivate() const { return engine; }
    const QQmlImports *imports() const;
    QHash<int, QQmlCompiledData::TypeReference *> *resolvedTypes();
    QList<QtQml::QmlObject*> *qmlObjects();
    int rootObjectIndex() const;
    void setPropertyCaches(const QVector<QQmlPropertyCache *> &caches);
    const QVector<QQmlPropertyCache *> &propertyCaches() const;
    void setVMEMetaObjects(const QVector<QByteArray> &metaObjects);
    QVector<QByteArray> *vmeMetaObjects() const;
    QHash<int, int> *objectIndexToIdForRoot();
    QHash<int, QHash<int, int> > *objectIndexToIdPerComponent();
    QHash<int, QByteArray> *customParserData();
    QQmlJS::MemoryPool *memoryPool();

private:
    QList<QQmlError> errors;
    QQmlEnginePrivate *engine;
    QQmlCompiledData *compiledData;
    QQmlTypeData *typeData;
    QtQml::ParsedQML *parsedQML;
};

struct QQmlCompilePass
{
    virtual ~QQmlCompilePass() {}

    QQmlCompilePass(QQmlTypeCompiler *typeCompiler);

    QString stringAt(int idx) const { return compiler->stringAt(idx); }
protected:
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    QQmlTypeCompiler *compiler;
};

class QQmlPropertyCacheCreator : public QQmlCompilePass
{
    Q_DECLARE_TR_FUNCTIONS(QQmlPropertyCacheCreator)
public:
    QQmlPropertyCacheCreator(QQmlTypeCompiler *typeCompiler);
    ~QQmlPropertyCacheCreator();

    bool buildMetaObjects();
protected:
    bool buildMetaObjectRecursively(int objectIndex, int referencingObjectIndex, const QV4::CompiledData::Binding *instantiatingBinding);
    bool ensureMetaObject(int objectIndex);
    bool createMetaObject(int objectIndex, const QtQml::QmlObject *obj, QQmlPropertyCache *baseTypeCache);

    QQmlEnginePrivate *enginePrivate;
    const QList<QtQml::QmlObject*> &qmlObjects;
    const QQmlImports *imports;
    QHash<int, QQmlCompiledData::TypeReference*> *resolvedTypes;
    QVector<QByteArray> vmeMetaObjects;
    QVector<QQmlPropertyCache*> propertyCaches;
};

class QQmlComponentAndAliasResolver : public QQmlCompilePass
{
    Q_DECLARE_TR_FUNCTIONS(QQmlAnonymousComponentResolver)
public:
    QQmlComponentAndAliasResolver(QQmlTypeCompiler *typeCompiler);

    bool resolve();

protected:
    void findAndRegisterImplicitComponents(const QtQml::QmlObject *obj, int objectIndex);
    bool collectIdsAndAliases(int objectIndex);
    bool resolveAliases();

    QQmlEnginePrivate *enginePrivate;
    QQmlJS::MemoryPool *pool;

    QList<QtQml::QmlObject*> *qmlObjects;
    const int indexOfRootObject;

    // indices of the objects that are actually Component {}
    QVector<int> componentRoots;
    // indices of objects that are the beginning of a new component
    // scope. This is sorted and used for binary search.
    QVector<int> componentBoundaries;

    int _componentIndex;
    QHash<int, int> _idToObjectIndex;
    QHash<int, int> *_objectIndexToIdInScope;
    QList<int> _objectsWithAliases;

    QHash<int, QQmlCompiledData::TypeReference*> *resolvedTypes;
    const QVector<QQmlPropertyCache *> propertyCaches;
    QVector<QByteArray> *vmeMetaObjectData;
    QHash<int, int> *objectIndexToIdForRoot;
    QHash<int, QHash<int, int> > *objectIndexToIdPerComponent;
};

class QQmlPropertyValidator : public QQmlCompilePass, public QQmlCustomParserCompilerBackend
{
    Q_DECLARE_TR_FUNCTIONS(QQmlPropertyValidator)
public:
    QQmlPropertyValidator(QQmlTypeCompiler *typeCompiler);

    bool validate();

    // Re-implemented for QQmlCustomParser
    virtual const QQmlImports &imports() const;


private:
    bool validateObject(int objectIndex);

    bool isComponent(int objectIndex) const { return objectIndexToIdPerComponent.contains(objectIndex); }

    const QV4::CompiledData::QmlUnit *qmlUnit;
    const QHash<int, QQmlCompiledData::TypeReference*> &resolvedTypes;
    const QVector<QQmlPropertyCache *> &propertyCaches;
    const QHash<int, QHash<int, int> > objectIndexToIdPerComponent;
    QHash<int, QByteArray> *customParserData;
};

QT_END_NAMESPACE

#endif // QQMLTYPECOMPILER_P_H
