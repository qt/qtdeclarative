/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVELISTMODEL_P_P_H
#define QDECLARATIVELISTMODEL_P_P_H

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

#include "private/qdeclarativelistmodel_p.h"
#include "private/qdeclarativeengine_p.h"
#include "private/qdeclarativeopenmetaobject_p.h"
#include "qdeclarative.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeOpenMetaObject;
class QDeclarativeListModelWorkerAgent;
struct ModelNode;
class FlatNodeData;

class FlatListModel
{
public:
    FlatListModel(QDeclarativeListModel *base);
    ~FlatListModel();

    QVariant data(int index, int role) const;

    QList<int> roles() const;
    QString toString(int role) const;

    int count() const;
    void clear();
    void remove(int index);
    bool insert(int index, v8::Handle<v8::Value>);
    v8::Handle<v8::Value> get(int index) const;
    void set(int index, v8::Handle<v8::Value>, QList<int> *roles);
    void setProperty(int index, const QString& property, const QVariant& value, QList<int> *roles);
    void move(int from, int to, int count);

private:    
    friend class QDeclarativeListModelWorkerAgent;
    friend class QDeclarativeListModel;
    friend class QDeclarativeListModelV8Data;
    friend class FlatNodeData;

    bool addValue(v8::Handle<v8::Value> value, QHash<int, QVariant> *row, QList<int> *roles);
    void insertedNode(int index);
    void removedNode(int index);
    void moveNodes(int from, int to, int n);

    QV8Engine *engine() const;
    QV8Engine *m_engine;
    QHash<int, QString> m_roles;
    QHash<QString, int> m_strings;
    QList<QHash<int, QVariant> > m_values;
    QDeclarativeListModel *m_listModel;

    QList<FlatNodeData *> m_nodeData;
    QDeclarativeListModelWorkerAgent *m_parentAgent;
};

/*
    FlatNodeData and FlatNodeObjectData allow objects returned by get() to still
    point to the correct list index if move(), insert() or remove() are called.
*/
class QV8ListModelResource;
class FlatNodeData
{
public:
    FlatNodeData(int i)
        : index(i) {}

    ~FlatNodeData();

    void addData(QV8ListModelResource *data);
    void removeData(QV8ListModelResource *data);

    int index;

private:
    QSet<QV8ListModelResource*> objects;
};

class QV8ListModelResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(ListModelType);
public:
    QV8ListModelResource(FlatListModel *model, FlatNodeData *data, QV8Engine *engine);
    ~QV8ListModelResource();

    FlatListModel *model;
    FlatNodeData *nodeData;
};

class NestedListModel
{
public:
    NestedListModel(QDeclarativeListModel *base);
    ~NestedListModel();

    QHash<int,QVariant> data(int index, const QList<int> &roles, bool *hasNested = 0) const;
    QVariant data(int index, int role) const;

    QList<int> roles() const;
    QString toString(int role) const;

    int count() const;
    void clear();
    void remove(int index);
    bool insert(int index, v8::Handle<v8::Value>);
    v8::Handle<v8::Value> get(int index) const;
    void set(int index, v8::Handle<v8::Value>, QList<int> *roles);
    void setProperty(int index, const QString& property, const QVariant& value, QList<int> *roles);
    void move(int from, int to, int count);

    QVariant valueForNode(ModelNode *, bool *hasNested = 0) const;
    void checkRoles() const;

    ModelNode *_root;
    bool m_ownsRoot;
    QDeclarativeListModel *m_listModel;

    QV8Engine *engine() const;
private:
    friend struct ModelNode;
    mutable QStringList roleStrings;
    mutable bool _rolesOk;
};


class ModelNodeMetaObject;
class ModelObject : public QObject
{
    Q_OBJECT
public:
    ModelObject(ModelNode *node, NestedListModel *model, QV8Engine *eng);
    void setValue(const QByteArray &name, const QVariant &val);
    void setNodeUpdatesEnabled(bool enable);

    NestedListModel *m_model;
    ModelNode *m_node;

private:
    ModelNodeMetaObject *m_meta;
};

class ModelNodeMetaObject : public QDeclarativeOpenMetaObject
{
public:
    ModelNodeMetaObject(QV8Engine *eng, ModelObject *object);

    bool m_enabled;

protected:
    void propertyWritten(int index);

private:
    QV8Engine *m_engine;
    ModelObject *m_obj;
};

/*
    A ModelNode is created for each item in a NestedListModel.
*/
struct ModelNode
{
    ModelNode(NestedListModel *model);
    ~ModelNode();

    QList<QVariant> values;
    QHash<QString, ModelNode *> properties;

    void clear();

    QDeclarativeListModel *model(const NestedListModel *model);
    ModelObject *object(const NestedListModel *model);

    bool setObjectValue(v8::Handle<v8::Value> valuemap, bool writeToCache = true);
    void setListValue(v8::Handle<v8::Value> valuelist);
    bool setProperty(const QString& prop, const QVariant& val);
    void changedProperty(const QString &name) const;
    void updateListIndexes();
    static void dump(ModelNode *node, int ind);

    QDeclarativeListModel *modelCache;
    ModelObject *objectCache;
    bool isArray;

    NestedListModel *m_model;
    int listIndex;  // only used for top-level nodes within a list
};


QT_END_NAMESPACE

Q_DECLARE_METATYPE(ModelNode *)

QT_END_HEADER

#endif // QDECLARATIVELISTMODEL_P_P_H

