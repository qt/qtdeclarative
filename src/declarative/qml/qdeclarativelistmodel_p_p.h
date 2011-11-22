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

#include "qdeclarativelistmodel_p.h"
#include <private/qdeclarativeengine_p.h>
#include "qdeclarativeopenmetaobject_p.h"
#include <qdeclarative.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class ModelObject;

class ModelNodeMetaObject : public QDeclarativeOpenMetaObject
{
public:
    ModelNodeMetaObject(ModelObject *object);
    ~ModelNodeMetaObject();

    bool m_enabled;

protected:
    void propertyWritten(int index);

private:

    ModelObject *m_obj;
};

class ModelObject : public QObject
{
    Q_OBJECT
public:
    ModelObject(QDeclarativeListModel *model, int elementIndex);

    void setValue(const QByteArray &name, const QVariant &val, bool force)
    {
        if (force) {
            QVariant existingValue = m_meta->value(name);
            if (existingValue.isValid()) {
                (*m_meta)[name] = QVariant();
            }
        }
        m_meta->setValue(name, val);
    }

    void setNodeUpdatesEnabled(bool enable)
    {
        m_meta->m_enabled = enable;
    }

    void updateValues();
    void updateValues(const QList<int> &roles);

    QDeclarativeListModel *m_model;
    int m_elementIndex;

private:
    ModelNodeMetaObject *m_meta;
};

class ListLayout
{
public:
    ListLayout() : currentBlock(0), currentBlockOffset(0) {}
    ListLayout(const ListLayout *other);
    ~ListLayout();

    class Role
    {
    public:

        Role() : type(Invalid), blockIndex(-1), blockOffset(-1), index(-1), subLayout(0) {}
        explicit Role(const Role *other);
        ~Role();

        // This enum must be kept in sync with the roleTypeNames variable in qdeclarativelistmodel.cpp
        enum DataType
        {
            Invalid = -1,

            String,
            Number,
            Bool,
            List,
            QObject,
            VariantMap,

            MaxDataType
        };

        QString name;
        DataType type;
        int blockIndex;
        int blockOffset;
        int index;
        ListLayout *subLayout;
    };

    const Role *getRoleOrCreate(const QString &key, const QVariant &data);
    const Role &getRoleOrCreate(v8::Handle<v8::String> key, Role::DataType type);
    const Role &getRoleOrCreate(const QString &key, Role::DataType type);

    const Role &getExistingRole(int index) { return *roles.at(index); }
    const Role *getExistingRole(const QString &key);
    const Role *getExistingRole(v8::Handle<v8::String> key);

    int roleCount() const { return roles.count(); }

    static void sync(ListLayout *src, ListLayout *target);

private:
    const Role &createRole(const QString &key, Role::DataType type);

    int currentBlock;
    int currentBlockOffset;
    QVector<Role *> roles;
    QStringHash<Role *> roleHash;
};

class ListElement
{
public:

    ListElement();
    ListElement(int existingUid);
    ~ListElement();

    static void sync(ListElement *src, ListLayout *srcLayout, ListElement *target, ListLayout *targetLayout, QHash<int, ListModel *> *targetModelHash);

    enum
    {
        BLOCK_SIZE = 64 - sizeof(int) - sizeof(ListElement *) - sizeof(ModelObject *)
    };

private:

    void destroy(ListLayout *layout);

    int setVariantProperty(const ListLayout::Role &role, const QVariant &d);

    int setJsProperty(const ListLayout::Role &role, v8::Handle<v8::Value> d, QV8Engine *eng);

    int setStringProperty(const ListLayout::Role &role, const QString &s);
    int setDoubleProperty(const ListLayout::Role &role, double n);
    int setBoolProperty(const ListLayout::Role &role, bool b);
    int setListProperty(const ListLayout::Role &role, ListModel *m);
    int setQObjectProperty(const ListLayout::Role &role, QObject *o);
    int setVariantMapProperty(const ListLayout::Role &role, v8::Handle<v8::Object> o, QV8Engine *eng);
    int setVariantMapProperty(const ListLayout::Role &role, QVariantMap *m);

    void setStringPropertyFast(const ListLayout::Role &role, const QString &s);
    void setDoublePropertyFast(const ListLayout::Role &role, double n);
    void setBoolPropertyFast(const ListLayout::Role &role, bool b);
    void setQObjectPropertyFast(const ListLayout::Role &role, QObject *o);
    void setListPropertyFast(const ListLayout::Role &role, ListModel *m);
    void setVariantMapFast(const ListLayout::Role &role, v8::Handle<v8::Object> o, QV8Engine *eng);

    void clearProperty(const ListLayout::Role &role);

    QVariant getProperty(const ListLayout::Role &role, const QDeclarativeListModel *owner, QV8Engine *eng);
    ListModel *getListProperty(const ListLayout::Role &role);
    QString *getStringProperty(const ListLayout::Role &role);
    QObject *getQObjectProperty(const ListLayout::Role &role);
    QDeclarativeGuard<QObject> *getGuardProperty(const ListLayout::Role &role);
    QVariantMap *getVariantMapProperty(const ListLayout::Role &role);

    inline char *getPropertyMemory(const ListLayout::Role &role);

    int getUid() const { return uid; }

    char data[BLOCK_SIZE];
    ListElement *next;

    int uid;
    ModelObject *m_objectCache;

    friend class ListModel;
};

class ListModel
{
public:

    ListModel(ListLayout *layout, QDeclarativeListModel *modelCache, int uid);
    ~ListModel() {}

    void destroy();

    int setOrCreateProperty(int elementIndex, const QString &key, const QVariant &data);
    int setExistingProperty(int uid, const QString &key, v8::Handle<v8::Value> data, QV8Engine *eng);

    QVariant getProperty(int elementIndex, int roleIndex, const QDeclarativeListModel *owner, QV8Engine *eng);
    ListModel *getListProperty(int elementIndex, const ListLayout::Role &role);

    int roleCount() const
    {
        return m_layout->roleCount();
    }

    const ListLayout::Role &getExistingRole(int index)
    {
        return m_layout->getExistingRole(index);
    }

    const ListLayout::Role &getOrCreateListRole(const QString &name)
    {
        return m_layout->getRoleOrCreate(name, ListLayout::Role::List);
    }

    int elementCount() const
    {
        return elements.count();
    }

    void set(int elementIndex, v8::Handle<v8::Object> object, QList<int> *roles, QV8Engine *eng);
    void set(int elementIndex, v8::Handle<v8::Object> object, QV8Engine *eng);

    int append(v8::Handle<v8::Object> object, QV8Engine *eng);
    void insert(int elementIndex, v8::Handle<v8::Object> object, QV8Engine *eng);

    void clear();
    void remove(int index, int count);

    int appendElement();
    void insertElement(int index);

    void move(int from, int to, int n);

    int getUid() const { return m_uid; }

    static int allocateUid();

    static void sync(ListModel *src, ListModel *target, QHash<int, ListModel *> *srcModelHash);

    ModelObject *getOrCreateModelObject(QDeclarativeListModel *model, int elementIndex);

private:
    QPODVector<ListElement *, 4> elements;
    ListLayout *m_layout;
    int m_uid;

    QDeclarativeListModel *m_modelCache;

    struct ElementSync
    {
        ElementSync() : src(0), target(0) {}

        ListElement *src;
        ListElement *target;
    };

    void newElement(int index);

    void updateCacheIndices();

    friend class ListElement;
    friend class QDeclarativeListModelWorkerAgent;

    static QAtomicInt uidCounter;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(ListModel *);

QT_END_HEADER

#endif // QDECLARATIVELISTMODEL_P_P_H

