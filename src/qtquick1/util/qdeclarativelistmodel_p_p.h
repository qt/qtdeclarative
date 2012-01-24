/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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

#include <QtDeclarative/private/qdeclarativeengine_p.h>
#include "QtQuick1/private/qdeclarativelistmodel_p.h"
#include "QtQuick1/private/qdeclarativeopenmetaobject_p.h"
#include <QtDeclarative/qdeclarative.h>

#include <QtDeclarative/private/qscriptdeclarativeclass_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QScriptEngine;


class QDeclarative1OpenMetaObject;
class QDeclarative1ListModelWorkerAgent;
struct ModelNode;
class FlatListScriptClass_1;
class FlatNodeData_1;

class FlatListModel_1
{
public:
    FlatListModel_1(QDeclarative1ListModel *base);
    ~FlatListModel_1();

    QVariant data(int index, int role) const;

    QList<int> roles() const;
    QString toString(int role) const;

    int count() const;
    void clear();
    void remove(int index);
    bool insert(int index, const QScriptValue&);
    QScriptValue get(int index) const;
    void set(int index, const QScriptValue&, QList<int> *roles);
    void setProperty(int index, const QString& property, const QVariant& value, QList<int> *roles);
    void move(int from, int to, int count);

private:    
    friend class QDeclarative1ListModelWorkerAgent;
    friend class QDeclarative1ListModel;
    friend class FlatListScriptClass_1;
    friend class FlatNodeData_1;

    bool addValue(const QScriptValue &value, QHash<int, QVariant> *row, QList<int> *roles);
    void insertedNode(int index);
    void removedNode(int index);
    void moveNodes(int from, int to, int n);

    QScriptEngine *m_scriptEngine;
    QHash<int, QString> m_roles;
    QHash<QString, int> m_strings;
    QList<QHash<int, QVariant> > m_values;
    QDeclarative1ListModel *m_listModel;

    FlatListScriptClass_1 *m_scriptClass;
    QList<FlatNodeData_1 *> m_nodeData;
    QDeclarative1ListModelWorkerAgent *m_parentAgent;
};


/*
    Created when get() is called on a FlatListModel_1. This allows changes to the
    object returned by get() to be tracked, and passed onto the model.
*/
class FlatListScriptClass_1 : public QScriptDeclarativeClass
{
public:
    FlatListScriptClass_1(FlatListModel_1 *model, QScriptEngine *seng);

    Value property(Object *, const Identifier &);
    void setProperty(Object *, const Identifier &name, const QScriptValue &);
    QScriptClass::QueryFlags queryProperty(Object *, const Identifier &, QScriptClass::QueryFlags flags);
    bool compare(Object *, Object *);

private:
    FlatListModel_1 *m_model;
};

/*
    FlatNodeData_1 and FlatNodeObjectData allow objects returned by get() to still
    point to the correct list index if move(), insert() or remove() are called.
*/
struct FlatNodeObjectData;
class FlatNodeData_1
{
public:
    FlatNodeData_1(int i)
        : index(i) {}

    ~FlatNodeData_1();

    void addData(FlatNodeObjectData *data);
    void removeData(FlatNodeObjectData *data);

    int index;

private:
    QSet<FlatNodeObjectData*> objects;
};

struct FlatNodeObjectData : public QScriptDeclarativeClass::Object
{
    FlatNodeObjectData(FlatNodeData_1 *data) : nodeData(data) {
        nodeData->addData(this);
    }

    ~FlatNodeObjectData() {
        if (nodeData)
            nodeData->removeData(this);
    }

    FlatNodeData_1 *nodeData;
};



class NestedListModel_1
{
public:
    NestedListModel_1(QDeclarative1ListModel *base);
    ~NestedListModel_1();

    QHash<int,QVariant> data(int index, const QList<int> &roles, bool *hasNested = 0) const;
    QVariant data(int index, int role) const;

    QList<int> roles() const;
    QString toString(int role) const;

    int count() const;
    void clear();
    void remove(int index);
    bool insert(int index, const QScriptValue&);
    QScriptValue get(int index) const;
    void set(int index, const QScriptValue&, QList<int> *roles);
    void setProperty(int index, const QString& property, const QVariant& value, QList<int> *roles);
    void move(int from, int to, int count);

    QVariant valueForNode(ModelNode *, bool *hasNested = 0) const;
    void checkRoles() const;

    ModelNode *_root;
    bool m_ownsRoot;
    QDeclarative1ListModel *m_listModel;

private:
    friend struct ModelNode;
    mutable QStringList roleStrings;
    mutable bool _rolesOk;
};


class ModelNodeMetaObject_1;
class ModelObject_1 : public QObject
{
    Q_OBJECT
public:
    ModelObject_1(ModelNode *node, NestedListModel_1 *model, QScriptEngine *seng);
    void setValue(const QByteArray &name, const QVariant &val);
    void setNodeUpdatesEnabled(bool enable);

    NestedListModel_1 *m_model;
    ModelNode *m_node;

private:
    ModelNodeMetaObject_1 *m_meta;
};

class ModelNodeMetaObject_1 : public QDeclarative1OpenMetaObject
{
public:
    ModelNodeMetaObject_1(QScriptEngine *seng, ModelObject_1 *object);

    bool m_enabled;

protected:
    void propertyWritten(int index);

private:
    QScriptEngine *m_seng;
    ModelObject_1 *m_obj;
};


/*
    A ModelNode is created for each item in a NestedListModel_1.
*/
struct ModelNode
{
    ModelNode(NestedListModel_1 *model);
    ~ModelNode();

    QList<QVariant> values;
    QHash<QString, ModelNode *> properties;

    void clear();

    QDeclarative1ListModel *model(const NestedListModel_1 *model);
    ModelObject_1 *object(const NestedListModel_1 *model);

    bool setObjectValue(const QScriptValue& valuemap, bool writeToCache = true);
    void setListValue(const QScriptValue& valuelist);
    bool setProperty(const QString& prop, const QVariant& val);
    void changedProperty(const QString &name) const;
    void updateListIndexes();
    static void dump(ModelNode *node, int ind);

    QDeclarative1ListModel *modelCache;
    ModelObject_1 *objectCache;
    bool isArray;

    NestedListModel_1 *m_model;
    int listIndex;  // only used for top-level nodes within a list
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(ModelNode *)

QT_END_HEADER

#endif // QDECLARATIVELISTMODEL_P_P_H

