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

#include "qdeclarativelistmodel_p_p.h"
#include "qdeclarativelistmodelworkeragent_p.h"
#include "qdeclarativeopenmetaobject_p.h"
#include <private/qdeclarativejsast_p.h>
#include <private/qdeclarativejsengine_p.h>

#include <private/qdeclarativecustomparser_p.h>
#include <private/qdeclarativescript_p.h>
#include <private/qdeclarativeengine_p.h>
#include <qdeclarativecontext.h>
#include <qdeclarativeinfo.h>

#include <QtCore/qdebug.h>
#include <QtCore/qstack.h>
#include <QXmlStreamReader>

Q_DECLARE_METATYPE(QListModelInterface *)

QT_BEGIN_NAMESPACE

// Set to 1024 as a debugging aid - easier to distinguish uids from indices of elements/models.
enum { MIN_LISTMODEL_UID = 1024 };

QAtomicInt ListModel::uidCounter(MIN_LISTMODEL_UID);

const ListLayout::Role &ListLayout::getRoleOrCreate(const QString &key, Role::DataType type)
{
    QStringHash<Role *>::Node *node = roleHash.findNode(key);
    if (node) {
        const Role &r = *node->value;
        if (type != r.type) {
            qmlInfo(0) << "Can't assign to pre-existing role of different type " << r.name;
        }
        return r;
    }

    return createRole(key, type);
}

const ListLayout::Role &ListLayout::getRoleOrCreate(v8::Handle<v8::String> key, Role::DataType type)
{
    QHashedV8String hashedKey(key);
    QStringHash<Role *>::Node *node = roleHash.findNode(hashedKey);
    if (node) {
        const Role &r = *node->value;
        if (type != r.type) {
            qmlInfo(0) << "Can't assign to pre-existing role of different type " << r.name;
        }
        return r;
    }

    QString qkey;
    qkey.resize(key->Length());
    key->Write(reinterpret_cast<uint16_t*>(qkey.data()));

    return createRole(qkey, type);
}

const ListLayout::Role &ListLayout::createRole(const QString &key, ListLayout::Role::DataType type)
{
    const int dataSizes[] = { sizeof(QString), sizeof(double), sizeof(bool), sizeof(QDeclarativeListModel *), sizeof(ListModel *), sizeof(QDeclarativeGuard<QObject>) };
    const int dataAlignments[] = { sizeof(QString), sizeof(double), sizeof(bool), sizeof(QDeclarativeListModel *), sizeof(ListModel *), sizeof(QObject *) };

    Role *r = new Role;
    r->name = key;
    r->type = type;

    if (type == Role::List) {
        r->subLayout = new ListLayout;
    } else {
        r->subLayout = 0;
    }

    int dataSize = dataSizes[type];
    int dataAlignment = dataAlignments[type];

    int dataOffset = (currentBlockOffset + dataAlignment-1) & ~(dataAlignment-1);
    if (dataOffset + dataSize > ListElement::BLOCK_SIZE) {
        r->blockIndex = ++currentBlock;
        r->blockOffset = 0;
        currentBlockOffset = dataSize;
    } else {
        r->blockIndex = currentBlock;
        r->blockOffset = dataOffset;
        currentBlockOffset = dataOffset + dataSize;
    }

    int roleIndex = roles.count();
    r->index = roleIndex;

    roles.append(r);
    roleHash.insert(key, r);

    return *r;
}

ListLayout::ListLayout(const ListLayout *other) : currentBlock(0), currentBlockOffset(0)
{
    for (int i=0 ; i < other->roles.count() ; ++i) {
        Role *role = new Role(other->roles[i]);
        roles.append(role);
        roleHash.insert(role->name, role);
    }
    currentBlockOffset = other->currentBlockOffset;
    currentBlock = other->currentBlock;
}

ListLayout::~ListLayout()
{
    for (int i=0 ; i < roles.count() ; ++i) {
        delete roles[i];
    }
}

void ListLayout::sync(ListLayout *src, ListLayout *target)
{
    int roleOffset = target->roles.count();
    int newRoleCount = src->roles.count() - roleOffset;

    for (int i=0 ; i < newRoleCount ; ++i) {
        Role *role = new Role(src->roles[roleOffset + i]);
        target->roles.append(role);
        target->roleHash.insert(role->name, role);
    }

    target->currentBlockOffset = src->currentBlockOffset;
    target->currentBlock = src->currentBlock;
}

ListLayout::Role::Role(const Role *other)
{
    name = other->name;
    type = other->type;
    blockIndex = other->blockIndex;
    blockOffset = other->blockOffset;
    index = other->index;
    if (other->subLayout)
        subLayout = new ListLayout(other->subLayout);
    else
        subLayout = 0;
}

ListLayout::Role::~Role()
{
    delete subLayout;
}

const ListLayout::Role *ListLayout::getRoleOrCreate(const QString &key, const QVariant &data)
{
    Role::DataType type;

    switch (data.type()) {
        case QVariant::Double:      type = Role::Number;      break;
        case QVariant::Int:         type = Role::Number;      break;
        case QVariant::UserType:    type = Role::List;        break;
        case QVariant::Bool:        type = Role::Bool;        break;
        case QVariant::String:      type = Role::String;      break;
        default:                    type = Role::Invalid;     break;
    }

    if (type == Role::Invalid) {
        qmlInfo(0) << "Can't create role for unsupported data type";
        return 0;
    }

    return &getRoleOrCreate(key, type);
}

const ListLayout::Role *ListLayout::getExistingRole(const QString &key)
{
    Role *r = 0;
    QStringHash<Role *>::Node *node = roleHash.findNode(key);
    if (node)
        r = node->value;
    return r;
}

const ListLayout::Role *ListLayout::getExistingRole(v8::Handle<v8::String> key)
{
    Role *r = 0;
    QHashedV8String hashedKey(key);
    QStringHash<Role *>::Node *node = roleHash.findNode(hashedKey);
    if (node)
        r = node->value;
    return r;
}

ModelObject *ListModel::getOrCreateModelObject(QDeclarativeListModel *model, int elementIndex)
{
    ListElement *e = elements[elementIndex];
    if (e->m_objectCache == 0) {
        e->m_objectCache = new ModelObject(model, elementIndex);
    }
    return e->m_objectCache;
}

void ListModel::sync(ListModel *src, ListModel *target, QHash<int, ListModel *> *targetModelHash)
{
    // Sanity check
    target->m_uid = src->m_uid;
    if (targetModelHash)
        targetModelHash->insert(target->m_uid, target);

    // Build hash of elements <-> uid for each of the lists
    QHash<int, ElementSync> elementHash;
    for (int i=0 ; i < target->elements.count() ; ++i) {
        ListElement *e = target->elements.at(i);
        int uid = e->getUid();
        ElementSync sync;
        sync.target = e;
        elementHash.insert(uid, sync);
    }
    for (int i=0 ; i < src->elements.count() ; ++i) {
        ListElement *e = src->elements.at(i);
        int uid = e->getUid();

        QHash<int, ElementSync>::iterator it = elementHash.find(uid);
        if (it == elementHash.end()) {
            ElementSync sync;
            sync.src = e;
            elementHash.insert(uid, sync);
        } else {
            ElementSync &sync = it.value();
            sync.src = e;
        }
    }

    // Get list of elements that are in the target but no longer in the source. These get deleted first.
    QHash<int, ElementSync>::iterator it = elementHash.begin();
    QHash<int, ElementSync>::iterator end = elementHash.end();
    while (it != end) {
        const ElementSync &s = it.value();
        if (s.src == 0) {
            s.target->destroy(target->m_layout);
            target->elements.removeOne(s.target);
            delete s.target;
        }
        ++it;
    }

    // Sync the layouts
    ListLayout::sync(src->m_layout, target->m_layout);

    // Clear the target list, and append in correct order from the source
    target->elements.clear();
    for (int i=0 ; i < src->elements.count() ; ++i) {
        ListElement *srcElement = src->elements.at(i);
        it = elementHash.find(srcElement->getUid());
        const ElementSync &s = it.value();
        ListElement *targetElement = s.target;
        if (targetElement == 0) {
            targetElement = new ListElement(srcElement->getUid());
        }
        ListElement::sync(srcElement, src->m_layout, targetElement, target->m_layout, targetModelHash);
        target->elements.append(targetElement);
    }

    target->updateCacheIndices();

    // Update values stored in target meta objects
    for (int i=0 ; i < target->elements.count() ; ++i) {
        ListElement *e = target->elements[i];
        if (e->m_objectCache)
            e->m_objectCache->updateValues();
    }
}

int ListModel::allocateUid()
{
    return uidCounter.fetchAndAddOrdered(1);
}

ListModel::ListModel(ListLayout *layout, QDeclarativeListModel *modelCache, int uid) : m_layout(layout), m_modelCache(modelCache)
{
    if (uid == -1)
        uid = allocateUid();
    m_uid = uid;
}

void ListModel::destroy()
{
    clear();
    m_uid = -1;
    m_layout = 0;
    if (m_modelCache && m_modelCache->m_primary == false)
        delete m_modelCache;
    m_modelCache = 0;
}

int ListModel::appendElement()
{
    int elementIndex = elements.count();
    newElement(elementIndex);
    return elementIndex;
}

void ListModel::insertElement(int index)
{
    newElement(index);
    updateCacheIndices();
}

void ListModel::move(int from, int to, int n)
{
    if (from > to) {
        // Only move forwards - flip if backwards moving
        int tfrom = from;
        int tto = to;
        from = tto;
        to = tto+n;
        n = tfrom-tto;
    }

    QPODVector<ListElement *, 4> store;
    for (int i=0 ; i < (to-from) ; ++i)
        store.append(elements[from+n+i]);
    for (int i=0 ; i < n ; ++i)
        store.append(elements[from+i]);
    for (int i=0 ; i < store.count() ; ++i)
        elements[from+i] = store[i];

    updateCacheIndices();
}

void ListModel::newElement(int index)
{
    ListElement *e = new ListElement;
    elements.insert(index, e);
}

void ListModel::updateCacheIndices()
{
    for (int i=0 ; i < elements.count() ; ++i) {
        ListElement *e = elements.at(i);
        if (e->m_objectCache) {
            e->m_objectCache->m_elementIndex = i;
        }
    }
}

QVariant ListModel::getProperty(int elementIndex, int roleIndex, const QDeclarativeListModel *owner, QV8Engine *eng)
{
    ListElement *e = elements[elementIndex];
    const ListLayout::Role &r = m_layout->getExistingRole(roleIndex);
    return e->getProperty(r, owner, eng);
}

ListModel *ListModel::getListProperty(int elementIndex, const ListLayout::Role &role)
{
    ListElement *e = elements[elementIndex];
    return e->getListProperty(role);
}

void ListModel::set(int elementIndex, v8::Handle<v8::Object> object, QList<int> *roles)
{
    ListElement *e = elements[elementIndex];

    v8::Local<v8::Array> propertyNames = object->GetPropertyNames();
    int propertyCount = propertyNames->Length();

    for (int i=0 ; i < propertyCount ; ++i) {
        v8::Local<v8::String> propertyName = propertyNames->Get(i)->ToString();
        v8::Local<v8::Value> propertyValue = object->Get(propertyName);

        // Check if this key exists yet
        int roleIndex = -1;

        // Add the value now
        if (propertyValue->IsString()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::String);
            v8::Handle<v8::String> jsString = propertyValue->ToString();
            QString qstr;
            qstr.resize(jsString->Length());
            jsString->Write(reinterpret_cast<uint16_t*>(qstr.data()));
            roleIndex = e->setStringProperty(r, qstr);
        } else if (propertyValue->IsNumber()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::Number);
            roleIndex = e->setDoubleProperty(r, propertyValue->NumberValue());
        } else if (propertyValue->IsArray()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::List);
            ListModel *subModel = new ListModel(r.subLayout, 0, -1);

            v8::Handle<v8::Array> subArray = v8::Handle<v8::Array>::Cast(propertyValue);
            int arrayLength = subArray->Length();
            for (int j=0 ; j < arrayLength ; ++j) {
                v8::Handle<v8::Object> subObject = subArray->Get(j)->ToObject();
                subModel->append(subObject);
            }

            roleIndex = e->setListProperty(r, subModel);
        } else if (propertyValue->IsInt32()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::Number);
            roleIndex = e->setDoubleProperty(r, (double) propertyValue->Int32Value());
        } else if (propertyValue->IsBoolean()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::Bool);
            roleIndex = e->setBoolProperty(r, propertyValue->BooleanValue());
        } else if (propertyValue->IsObject()) {
            QV8ObjectResource *r = (QV8ObjectResource *) propertyValue->ToObject()->GetExternalResource();
            if (r && r->resourceType() == QV8ObjectResource::QObjectType) {
                QObject *o = QV8QObjectWrapper::toQObject(r);
                const ListLayout::Role &role = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::QObject);
                if (role.type == ListLayout::Role::QObject)
                    e->setQObjectProperty(role, o);
            }
        } else if (propertyValue.IsEmpty() || propertyValue->IsUndefined() || propertyValue->IsNull()) {
            const ListLayout::Role *r = m_layout->getExistingRole(propertyName);
            if (r)
                e->clearProperty(*r);
        }

        if (roleIndex != -1)
            roles->append(roleIndex);
    }

    if (e->m_objectCache) {
        e->m_objectCache->updateValues(*roles);
    }
}

void ListModel::set(int elementIndex, v8::Handle<v8::Object> object)
{
    ListElement *e = elements[elementIndex];

    v8::Local<v8::Array> propertyNames = object->GetPropertyNames();
    int propertyCount = propertyNames->Length();

    for (int i=0 ; i < propertyCount ; ++i) {
        v8::Local<v8::String> propertyName = propertyNames->Get(i)->ToString();
        v8::Local<v8::Value> propertyValue = object->Get(propertyName);

        // Add the value now
        if (propertyValue->IsString()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::String);
            if (r.type == ListLayout::Role::String) {
                v8::Handle<v8::String> jsString = propertyValue->ToString();
                QString qstr;
                qstr.resize(jsString->Length());
                jsString->Write(reinterpret_cast<uint16_t*>(qstr.data()));
                e->setStringPropertyFast(r, qstr);
            }
        } else if (propertyValue->IsNumber()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::Number);
            if (r.type == ListLayout::Role::Number) {
                e->setDoublePropertyFast(r, propertyValue->NumberValue());
            }
        } else if (propertyValue->IsArray()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::List);
            if (r.type == ListLayout::Role::List) {
                ListModel *subModel = new ListModel(r.subLayout, 0, -1);

                v8::Handle<v8::Array> subArray = v8::Handle<v8::Array>::Cast(propertyValue);
                int arrayLength = subArray->Length();
                for (int j=0 ; j < arrayLength ; ++j) {
                    v8::Handle<v8::Object> subObject = subArray->Get(j)->ToObject();
                    subModel->append(subObject);
                }

                e->setListPropertyFast(r, subModel);
            }
        } else if (propertyValue->IsInt32()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::Number);
            if (r.type == ListLayout::Role::Number) {
                e->setDoublePropertyFast(r, (double) propertyValue->Int32Value());
            }
        } else if (propertyValue->IsBoolean()) {
            const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::Bool);
            if (r.type == ListLayout::Role::Bool) {
                e->setBoolPropertyFast(r, propertyValue->BooleanValue());
            }
        } else if (propertyValue->IsObject()) {
            QV8ObjectResource *r = (QV8ObjectResource *) propertyValue->ToObject()->GetExternalResource();
            if (r && r->resourceType() == QV8ObjectResource::QObjectType) {
                QObject *o = QV8QObjectWrapper::toQObject(r);
                const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::QObject);
                if (r.type == ListLayout::Role::QObject)
                    e->setQObjectPropertyFast(r, o);
            }
        } else if (propertyValue.IsEmpty() || propertyValue->IsUndefined() || propertyValue->IsNull()) {
            const ListLayout::Role *r = m_layout->getExistingRole(propertyName);
            if (r)
                e->clearProperty(*r);
        }
    }
}

void ListModel::clear()
{
    int elementCount = elements.count();
    for (int i=0 ; i < elementCount ; ++i) {
        elements[i]->destroy(m_layout);
        delete elements[i];
    }
    elements.clear();
}

void ListModel::remove(int index)
{
    elements[index]->destroy(m_layout);
    delete elements[index];
    elements.remove(index);
    updateCacheIndices();
}

void ListModel::insert(int elementIndex, v8::Handle<v8::Object> object)
{
    insertElement(elementIndex);
    set(elementIndex, object);
}

int ListModel::append(v8::Handle<v8::Object> object)
{
    int elementIndex = appendElement();
    set(elementIndex, object);
    return elementIndex;
}

int ListModel::setOrCreateProperty(int elementIndex, const QString &key, const QVariant &data)
{
    int roleIndex = -1;

    if (elementIndex >= 0 && elementIndex < elements.count()) {
        ListElement *e = elements[elementIndex];
        const ListLayout::Role *r = m_layout->getRoleOrCreate(key, data);
        if (r) {
            roleIndex = e->setVariantProperty(*r, data);

            if (roleIndex != -1 && e->m_objectCache) {
                QList<int> roles;
                roles << roleIndex;
                e->m_objectCache->updateValues(roles);
            }
        }
    }

    return roleIndex;
}

int ListModel::setExistingProperty(int elementIndex, const QString &key, v8::Handle<v8::Value> data)
{
    int roleIndex = -1;

    if (elementIndex >= 0 && elementIndex < elements.count()) {
        ListElement *e = elements[elementIndex];
        const ListLayout::Role *r = m_layout->getExistingRole(key);
        if (r)
            roleIndex = e->setJsProperty(*r, data);
    }

    return roleIndex;
}

inline char *ListElement::getPropertyMemory(const ListLayout::Role &role)
{
    ListElement *e = this;
    int blockIndex = 0;
    while (blockIndex < role.blockIndex) {
        if (e->next == 0) {
            e->next = new ListElement;
            e->next->uid = uid;
        }
        e = e->next;
        ++blockIndex;
    }

    char *mem = &e->data[role.blockOffset];
    return mem;
}

QString *ListElement::getStringProperty(const ListLayout::Role &role)
{
    char *mem = getPropertyMemory(role);
    QString *s = reinterpret_cast<QString *>(mem);
    return s->data_ptr() ? s : 0;
}

QObject *ListElement::getQObjectProperty(const ListLayout::Role &role)
{
    char *mem = getPropertyMemory(role);
    QDeclarativeGuard<QObject> *o = reinterpret_cast<QDeclarativeGuard<QObject> *>(mem);
    return o->data();
}

QDeclarativeGuard<QObject> *ListElement::getGuardProperty(const ListLayout::Role &role)
{
    char *mem = getPropertyMemory(role);
    QDeclarativeGuard<QObject> *o = reinterpret_cast<QDeclarativeGuard<QObject> *>(mem);
    return o;
}

ListModel *ListElement::getListProperty(const ListLayout::Role &role)
{
    char *mem = getPropertyMemory(role);
    ListModel **value = reinterpret_cast<ListModel **>(mem);
    return *value;
}

QVariant ListElement::getProperty(const ListLayout::Role &role, const QDeclarativeListModel *owner, QV8Engine *eng)
{
    char *mem = getPropertyMemory(role);

    QVariant data;

    switch (role.type) {
        case ListLayout::Role::Number:
            {
                double *value = reinterpret_cast<double *>(mem);
                data = *value;
            }
            break;
        case ListLayout::Role::String:
            {
                QString *value = reinterpret_cast<QString *>(mem);
                if (value->data_ptr() != 0)
                    data = *value;
            }
            break;
        case ListLayout::Role::Bool:
            {
                bool *value = reinterpret_cast<bool *>(mem);
                data = *value;
            }
            break;
        case ListLayout::Role::List:
            {
                ListModel **value = reinterpret_cast<ListModel **>(mem);
                ListModel *model = *value;

                if (model) {
                    if (model->m_modelCache == 0) {
                        model->m_modelCache = new QDeclarativeListModel(owner, model, eng);
                        QDeclarativeEngine::setContextForObject(model->m_modelCache, QDeclarativeEngine::contextForObject(owner));
                    }

                    QObject *object = model->m_modelCache;
                    data = QVariant::fromValue(object);
                }
            }
            break;
        case ListLayout::Role::QObject:
            {
                QDeclarativeGuard<QObject> *guard = reinterpret_cast<QDeclarativeGuard<QObject> *>(mem);
                QObject *object = guard->data();
                if (object)
                    data = QVariant::fromValue(object);
            }
            break;
        default:
            break;
    }

    return data;
}

int ListElement::setStringProperty(const ListLayout::Role &role, const QString &s)
{
    int roleIndex = -1;

    if (role.type == ListLayout::Role::String) {
        char *mem = getPropertyMemory(role);
        QString *c = reinterpret_cast<QString *>(mem);
        bool changed;
        if (c->data_ptr() == 0) {
            new (mem) QString(s);
            changed = true;
        } else {
            changed = c->compare(s) != 0;
            *c = s;
        }
        if (changed)
            roleIndex = role.index;
    } else {
        qmlInfo(0) << "Unable to assign string to role '" << role.name << "' of different type.";
    }

    return roleIndex;
}

int ListElement::setDoubleProperty(const ListLayout::Role &role, double d)
{
    int roleIndex = -1;

    if (role.type == ListLayout::Role::Number) {
        char *mem = getPropertyMemory(role);
        double *value = new (mem) double;
        bool changed = *value != d;
        *value = d;
        if (changed)
            roleIndex = role.index;
    } else {
        qmlInfo(0) << "Unable to assign number to role '" << role.name << "' of different type.";
    }

    return roleIndex;
}

int ListElement::setBoolProperty(const ListLayout::Role &role, bool b)
{
    int roleIndex = -1;

    if (role.type == ListLayout::Role::Bool) {
        char *mem = getPropertyMemory(role);
        bool *value = new (mem) bool;
        bool changed = *value != b;
        *value = b;
        if (changed)
            roleIndex = role.index;
    } else {
        qmlInfo(0) << "Unable to assign bool to role '" << role.name << "' of different type.";
    }

    return roleIndex;
}

int ListElement::setListProperty(const ListLayout::Role &role, ListModel *m)
{
    int roleIndex = -1;

    if (role.type == ListLayout::Role::List) {
        char *mem = getPropertyMemory(role);
        ListModel **value = new (mem) ListModel *;
        if (*value) {
            (*value)->destroy();
            delete *value;
        }
        *value = m;
        roleIndex = role.index;
    } else {
        qmlInfo(0) << "Unable to assign list to role '" << role.name << "' of different type.";
    }

    return roleIndex;
}

int ListElement::setQObjectProperty(const ListLayout::Role &role, QObject *o)
{
    int roleIndex = -1;

    if (role.type == ListLayout::Role::QObject) {
        char *mem = getPropertyMemory(role);
        QDeclarativeGuard<QObject> *g = reinterpret_cast<QDeclarativeGuard<QObject> *>(mem);
        bool changed = g->data() != o;
        g->~QDeclarativeGuard();
        new (mem) QDeclarativeGuard<QObject>(o);
        if (changed)
            roleIndex = role.index;
    } else {
        qmlInfo(0) << "Unable to assign string to role '" << role.name << "' of different type.";
    }

    return roleIndex;
}

void ListElement::setStringPropertyFast(const ListLayout::Role &role, const QString &s)
{
    char *mem = getPropertyMemory(role);
    new (mem) QString(s);
}

void ListElement::setDoublePropertyFast(const ListLayout::Role &role, double d)
{
    char *mem = getPropertyMemory(role);
    double *value = new (mem) double;
    *value = d;
}

void ListElement::setBoolPropertyFast(const ListLayout::Role &role, bool b)
{
    char *mem = getPropertyMemory(role);
    bool *value = new (mem) bool;
    *value = b;
}

void ListElement::setQObjectPropertyFast(const ListLayout::Role &role, QObject *o)
{
    char *mem = getPropertyMemory(role);
    new (mem) QDeclarativeGuard<QObject>(o);
}

void ListElement::setListPropertyFast(const ListLayout::Role &role, ListModel *m)
{
    char *mem = getPropertyMemory(role);
    ListModel **value = new (mem) ListModel *;
    *value = m;
}

void ListElement::clearProperty(const ListLayout::Role &role)
{
    switch (role.type) {
    case ListLayout::Role::String:
        setStringProperty(role, QString());
        break;
    case ListLayout::Role::Number:
        setDoubleProperty(role, 0.0);
        break;
    case ListLayout::Role::Bool:
        setBoolProperty(role, false);
        break;
    case ListLayout::Role::List:
        setListProperty(role, 0);
        break;
    case ListLayout::Role::QObject:
        setQObjectProperty(role, 0);
        break;
    default:
        break;
    }
}

ListElement::ListElement()
{
    m_objectCache = 0;
    uid = ListModel::allocateUid();
    next = 0;
    qMemSet(data, 0, sizeof(data));
}

ListElement::ListElement(int existingUid)
{
    m_objectCache = 0;
    uid = existingUid;
    next = 0;
    qMemSet(data, 0, sizeof(data));
}

ListElement::~ListElement()
{
    delete next;
}

void ListElement::sync(ListElement *src, ListLayout *srcLayout, ListElement *target, ListLayout *targetLayout, QHash<int, ListModel *> *targetModelHash)
{
    for (int i=0 ; i < srcLayout->roleCount() ; ++i) {
        const ListLayout::Role &srcRole = srcLayout->getExistingRole(i);
        const ListLayout::Role &targetRole = targetLayout->getExistingRole(i);

        switch (srcRole.type) {
            case ListLayout::Role::List:
                {
                    ListModel *srcSubModel = src->getListProperty(srcRole);
                    ListModel *targetSubModel = target->getListProperty(targetRole);

                    if (srcSubModel) {
                        if (targetSubModel == 0) {
                            targetSubModel = new ListModel(targetRole.subLayout, 0, srcSubModel->getUid());
                            target->setListPropertyFast(targetRole, targetSubModel);
                        }
                        ListModel::sync(srcSubModel, targetSubModel, targetModelHash);
                    }
                }
                break;
            case ListLayout::Role::QObject:
                {
                    QObject *object = src->getQObjectProperty(srcRole);
                    target->setQObjectProperty(targetRole, object);
                }
                break;
            case ListLayout::Role::String:
            case ListLayout::Role::Number:
            case ListLayout::Role::Bool:
                {
                    QVariant v = src->getProperty(srcRole, 0, 0);
                    target->setVariantProperty(targetRole, v);
                }
            default:
                break;
        }
    }

}

void ListElement::destroy(ListLayout *layout)
{
    if (layout) {
        for (int i=0 ; i < layout->roleCount() ; ++i) {
            const ListLayout::Role &r = layout->getExistingRole(i);

            switch (r.type) {
                case ListLayout::Role::String:
                    {
                        QString *string = getStringProperty(r);
                        if (string)
                            string->~QString();
                    }
                    break;
                case ListLayout::Role::List:
                    {
                        ListModel *model = getListProperty(r);
                        if (model) {
                            model->destroy();
                            delete model;
                        }
                    }
                    break;
                case ListLayout::Role::QObject:
                    {
                        QDeclarativeGuard<QObject> *guard = getGuardProperty(r);
                        guard->~QDeclarativeGuard();
                    }
                    break;
                default:
                    // other types don't need explicit cleanup.
                    break;
            }
        }

        delete m_objectCache;
    }

    if (next)
        next->destroy(0);
    uid = -1;
}

int ListElement::setVariantProperty(const ListLayout::Role &role, const QVariant &d)
{
    int roleIndex = -1;

    switch (role.type) {
        case ListLayout::Role::Number:
            roleIndex = setDoubleProperty(role, d.toDouble());
            break;
        case ListLayout::Role::String:
            roleIndex = setStringProperty(role, d.toString());
            break;
        case ListLayout::Role::Bool:
            roleIndex = setBoolProperty(role, d.toBool());
            break;
        case ListLayout::Role::List:
            roleIndex = setListProperty(role, d.value<ListModel *>());
            break;
        default:
            break;
    }

    return roleIndex;
}

int ListElement::setJsProperty(const ListLayout::Role &role, v8::Handle<v8::Value> d)
{
    // Check if this key exists yet
    int roleIndex = -1;

    // Add the value now
    if (d->IsString()) {
        v8::Handle<v8::String> jsString = d->ToString();
        QString qstr;
        qstr.resize(jsString->Length());
        jsString->Write(reinterpret_cast<uint16_t*>(qstr.data()));
        roleIndex = setStringProperty(role, qstr);
    } else if (d->IsNumber()) {
        roleIndex = setDoubleProperty(role, d->NumberValue());
    } else if (d->IsArray()) {
        ListModel *subModel = new ListModel(role.subLayout, 0, -1);
        v8::Handle<v8::Array> subArray = v8::Handle<v8::Array>::Cast(d);
        int arrayLength = subArray->Length();
        for (int j=0 ; j < arrayLength ; ++j) {
            v8::Handle<v8::Object> subObject = subArray->Get(j)->ToObject();
            subModel->append(subObject);
        }
        roleIndex = setListProperty(role, subModel);
    } else if (d->IsInt32()) {
        roleIndex = setDoubleProperty(role, (double) d->Int32Value());
    } else if (d->IsBoolean()) {
        roleIndex = setBoolProperty(role, d->BooleanValue());
    } else if (d->IsObject()) {
        QV8ObjectResource *r = (QV8ObjectResource *) d->ToObject()->GetExternalResource();
        if (role.type == ListLayout::Role::QObject && r && r->resourceType() == QV8ObjectResource::QObjectType) {
            QObject *o = QV8QObjectWrapper::toQObject(r);
            roleIndex = setQObjectProperty(role, o);
        }
    } else if (d.IsEmpty() || d->IsUndefined() || d->IsNull()) {
        clearProperty(role);
    }

    return roleIndex;
}

ModelObject::ModelObject(QDeclarativeListModel *model, int elementIndex)
: m_model(model), m_elementIndex(elementIndex), m_meta(new ModelNodeMetaObject(this))
{
    updateValues();
    setNodeUpdatesEnabled(true);
}

void ModelObject::updateValues()
{
    int roleCount = m_model->m_listModel->roleCount();
    for (int i=0 ; i < roleCount ; ++i) {
        const ListLayout::Role &role = m_model->m_listModel->getExistingRole(i);
        QByteArray name = role.name.toUtf8();
        const QVariant &data = m_model->data(m_elementIndex, i);
        setValue(name, data, role.type == ListLayout::Role::List);
    }
}

void ModelObject::updateValues(const QList<int> &roles)
{
    int roleCount = roles.count();
    for (int i=0 ; i < roleCount ; ++i) {
        int roleIndex = roles.at(i);
        const ListLayout::Role &role = m_model->m_listModel->getExistingRole(roleIndex);
        QByteArray name = role.name.toUtf8();
        const QVariant &data = m_model->data(m_elementIndex, roleIndex);
        setValue(name, data, role.type == ListLayout::Role::List);
    }
}

ModelNodeMetaObject::ModelNodeMetaObject(ModelObject *object)
: QDeclarativeOpenMetaObject(object), m_enabled(false), m_obj(object)
{
}

ModelNodeMetaObject::~ModelNodeMetaObject()
{
}

void ModelNodeMetaObject::propertyWritten(int index)
{
    if (!m_enabled)
        return;

    QV8Engine *eng = m_obj->m_model->engine();

    QString propName = QString::fromUtf8(name(index));
    QVariant value = operator[](index);

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(eng->context());

    v8::Handle<v8::Value> v = eng->fromVariant(value);

    int roleIndex = m_obj->m_model->m_listModel->setExistingProperty(m_obj->m_elementIndex, propName, v);
    if (roleIndex != -1) {
        QList<int> roles;
        roles << roleIndex;
        m_obj->m_model->emitItemsChanged(m_obj->m_elementIndex, 1, roles);
    }
}

QDeclarativeListModelParser::ListInstruction *QDeclarativeListModelParser::ListModelData::instructions() const
{
    return (QDeclarativeListModelParser::ListInstruction *)((char *)this + sizeof(ListModelData));
}

/*!
    \qmlclass ListModel QDeclarativeListModel
    \inqmlmodule QtQuick 2
    \ingroup qml-working-with-data
    \brief The ListModel element defines a free-form list data source.

    The ListModel is a simple container of ListElement definitions, each containing data roles.
    The contents can be defined dynamically, or explicitly in QML.

    The number of elements in the model can be obtained from its \l count property.
    A number of familiar methods are also provided to manipulate the contents of the
    model, including append(), insert(), move(), remove() and set(). These methods
    accept dictionaries as their arguments; these are translated to ListElement objects
    by the model.

    Elements can be manipulated via the model using the setProperty() method, which
    allows the roles of the specified element to be set and changed.

    \section1 Example Usage

    The following example shows a ListModel containing three elements, with the roles
    "name" and "cost".

    \div {class="float-right"}
    \inlineimage listmodel.png
    \enddiv

    \snippet doc/src/snippets/declarative/listmodel.qml 0

    \clearfloat
    Roles (properties) in each element must begin with a lower-case letter and
    should be common to all elements in a model. The ListElement documentation
    provides more guidelines for how elements should be defined.

    Since the example model contains an \c id property, it can be referenced
    by views, such as the ListView in this example:

    \snippet doc/src/snippets/declarative/listmodel-simple.qml 0
    \dots 8
    \snippet doc/src/snippets/declarative/listmodel-simple.qml 1

    It is possible for roles to contain list data.  In the following example we
    create a list of fruit attributes:

    \snippet doc/src/snippets/declarative/listmodel-nested.qml model

    The delegate displays all the fruit attributes:

    \div {class="float-right"}
    \inlineimage listmodel-nested.png
    \enddiv

    \snippet doc/src/snippets/declarative/listmodel-nested.qml delegate

    \clearfloat
    \section1 Modifying List Models

    The content of a ListModel may be created and modified using the clear(),
    append(), set(), insert() and setProperty() methods.  For example:

    \snippet doc/src/snippets/declarative/listmodel-modify.qml delegate

    Note that when creating content dynamically the set of available properties
    cannot be changed once set. Whatever properties are first added to the model
    are the only permitted properties in the model.

    \section1 Using Threaded List Models with WorkerScript

    ListModel can be used together with WorkerScript access a list model
    from multiple threads. This is useful if list modifications are
    synchronous and take some time: the list operations can be moved to a
    different thread to avoid blocking of the main GUI thread.

    Here is an example that uses WorkerScript to periodically append the
    current time to a list model:

    \snippet examples/declarative/threading/threadedlistmodel/timedisplay.qml 0

    The included file, \tt dataloader.js, looks like this:

    \snippet examples/declarative/threading/threadedlistmodel/dataloader.js 0

    The timer in the main example sends messages to the worker script by calling
    \l WorkerScript::sendMessage(). When this message is received,
    \l{WorkerScript::onMessage}{WorkerScript.onMessage()} is invoked in \c dataloader.js,
    which appends the current time to the list model.

    Note the call to sync() from the \l{WorkerScript::onMessage}{WorkerScript.onMessage()}
    handler. You must call sync() or else the changes made to the list from the external
    thread will not be reflected in the list model in the main thread.

    \sa {qmlmodels}{Data Models}, {declarative/threading/threadedlistmodel}{Threaded ListModel example}, QtDeclarative
*/

QDeclarativeListModel::QDeclarativeListModel(QObject *parent)
: QListModelInterface(parent)
{
    m_mainThread = true;
    m_primary = true;
    m_agent = 0;

    m_layout = new ListLayout;
    m_listModel = new ListModel(m_layout, this, -1);

    m_engine = 0;
}

QDeclarativeListModel::QDeclarativeListModel(const QDeclarativeListModel *owner, ListModel *data, QV8Engine *eng, QObject *parent)
: QListModelInterface(parent)
{
    m_mainThread = owner->m_mainThread;
    m_primary = false;
    m_agent = owner->m_agent;

    m_layout = 0;
    m_listModel = data;

    m_engine = eng;
}

QDeclarativeListModel::QDeclarativeListModel(QDeclarativeListModel *orig, QDeclarativeListModelWorkerAgent *agent)
: QListModelInterface(agent)
{
    m_mainThread = false;
    m_primary = true;
    m_agent = agent;

    m_layout = new ListLayout(orig->m_layout);
    m_listModel = new ListModel(m_layout, this, orig->m_listModel->getUid());
    ListModel::sync(orig->m_listModel, m_listModel, 0);

    m_engine = 0;
}

QDeclarativeListModel::~QDeclarativeListModel()
{
    if (m_primary) {
        m_listModel->destroy();
        delete m_listModel;

        if (m_mainThread && m_agent)
            m_agent->release();
    }

    m_listModel = 0;

    delete m_layout;
    m_layout = 0;
}

QV8Engine *QDeclarativeListModel::engine() const
{
    if (m_engine == 0) {
        m_engine  = QDeclarativeEnginePrivate::getV8Engine(qmlEngine(this));
    }

    return m_engine;
}

void QDeclarativeListModel::emitItemsChanged(int index, int count, const QList<int> &roles)
{
    if (m_mainThread) {
        emit itemsChanged(index, count, roles);
    } else {
        m_agent->data.changedChange(this, index, count, roles);
    }
}

void QDeclarativeListModel::emitItemsRemoved(int index, int count)
{
    if (m_mainThread) {
        emit itemsRemoved(index, count);
        emit countChanged();
    } else {
        if (index == 0 && count == this->count())
            m_agent->data.clearChange(this);
        m_agent->data.removeChange(this, index, count);
    }
}

void QDeclarativeListModel::emitItemsInserted(int index, int count)
{
    if (m_mainThread) {
        emit itemsInserted(index, count);
        emit countChanged();
    } else {
        m_agent->data.insertChange(this, index, count);
    }
}

void QDeclarativeListModel::emitItemsMoved(int from, int to, int n)
{
    if (m_mainThread) {
        emit itemsMoved(from, to, n);
    } else {
        m_agent->data.moveChange(this, from, n, to);
    }
}

QDeclarativeListModelWorkerAgent *QDeclarativeListModel::agent()
{
    if (m_agent)
        return m_agent;

    m_agent = new QDeclarativeListModelWorkerAgent(this);
    return m_agent;
}

QList<int> QDeclarativeListModel::roles() const
{
    QList<int> rolesArray;

    for (int i=0 ; i < m_listModel->roleCount() ; ++i)
        rolesArray << i;

    return rolesArray;
}

QString QDeclarativeListModel::toString(int role) const
{
    const ListLayout::Role &r = m_listModel->getExistingRole(role);
    return r.name;
}

QVariant QDeclarativeListModel::data(int index, int role) const
{
    if (index >= count() || index < 0)
        return QVariant();

    return m_listModel->getProperty(index, role, this, engine());
}

/*!
    \qmlproperty int QtQuick2::ListModel::count
    The number of data entries in the model.
*/
int QDeclarativeListModel::count() const
{
    return m_listModel->elementCount();
}

/*!
    \qmlmethod QtQuick2::ListModel::clear()

    Deletes all content from the model.

    \sa append() remove()
*/
void QDeclarativeListModel::clear()
{
    int cleared = count();

    m_listModel->clear();
    emitItemsRemoved(0, cleared);
}

/*!
    \qmlmethod QtQuick2::ListModel::remove(int index)

    Deletes the content at \a index from the model.

    \sa clear()
*/
void QDeclarativeListModel::remove(int index)
{
    if (index < 0 || index >= count()) {
        qmlInfo(this) << tr("remove: index %1 out of range").arg(index);
        return;
    }

    m_listModel->remove(index);

    emitItemsRemoved(index, 1);
}

/*!
    \qmlmethod QtQuick2::ListModel::insert(int index, jsobject dict)

    Adds a new item to the list model at position \a index, with the
    values in \a dict.

    \code
        fruitModel.insert(2, {"cost": 5.95, "name":"Pizza"})
    \endcode

    The \a index must be to an existing item in the list, or one past
    the end of the list (equivalent to append).

    \sa set() append()
*/

void QDeclarativeListModel::insert(QDeclarativeV8Function *args)
{
    if (args->Length() == 2) {

        v8::Handle<v8::Value> arg0 = (*args)[0];
        int index = arg0->Int32Value();

        if (index < 0 || index > count()) {
            qmlInfo(this) << tr("insert: index %1 out of range").arg(index);
            return;
        }

        v8::Handle<v8::Value> arg1 = (*args)[1];

        if (arg1->IsArray()) {
            v8::Handle<v8::Array> objectArray = v8::Handle<v8::Array>::Cast(arg1);
            int objectArrayLength = objectArray->Length();
            for (int i=0 ; i < objectArrayLength ; ++i) {
                v8::Handle<v8::Object> argObject = objectArray->Get(i)->ToObject();
                m_listModel->insert(index+i, argObject);
            }
            emitItemsInserted(index, objectArrayLength);
        } else if (arg1->IsObject()) {
            v8::Handle<v8::Object> argObject = arg1->ToObject();

            m_listModel->insert(index, argObject);
            emitItemsInserted(index, 1);
        } else {
            qmlInfo(this) << tr("insert: value is not an object");
        }
    } else {
        qmlInfo(this) << tr("insert: value is not an object");
    }
}

/*!
    \qmlmethod QtQuick2::ListModel::move(int from, int to, int n)

    Moves \a n items \a from one position \a to another.

    The from and to ranges must exist; for example, to move the first 3 items
    to the end of the list:

    \code
        fruitModel.move(0, fruitModel.count - 3, 3)
    \endcode

    \sa append()
*/
void QDeclarativeListModel::move(int from, int to, int n)
{
    if (n==0 || from==to)
        return;
    if (!canMove(from, to, n)) {
        qmlInfo(this) << tr("move: out of range");
        return;
    }

    m_listModel->move(from, to, n);
    emitItemsMoved(from, to, n);
}

/*!
    \qmlmethod QtQuick2::ListModel::append(jsobject dict)

    Adds a new item to the end of the list model, with the
    values in \a dict.

    \code
        fruitModel.append({"cost": 5.95, "name":"Pizza"})
    \endcode

    \sa set() remove()
*/
void QDeclarativeListModel::append(QDeclarativeV8Function *args)
{
    if (args->Length() == 1) {
        v8::Handle<v8::Value> arg = (*args)[0];

        if (arg->IsArray()) {
            v8::Handle<v8::Array> objectArray = v8::Handle<v8::Array>::Cast(arg);
            int objectArrayLength = objectArray->Length();
            int index = m_listModel->elementCount();
            for (int i=0 ; i < objectArrayLength ; ++i) {
                v8::Handle<v8::Object> argObject = objectArray->Get(i)->ToObject();
                m_listModel->append(argObject);
            }
            emitItemsInserted(index, objectArrayLength);
        } else if (arg->IsObject()) {
            v8::Handle<v8::Object> argObject = arg->ToObject();

            int index = m_listModel->append(argObject);
            emitItemsInserted(index, 1);

        } else {
            qmlInfo(this) << tr("append: value is not an object");
        }
    } else {
        qmlInfo(this) << tr("append: value is not an object");
    }
}

/*!
    \qmlmethod object QtQuick2::ListModel::get(int index)

    Returns the item at \a index in the list model. This allows the item
    data to be accessed or modified from JavaScript:

    \code
    Component.onCompleted: {
        fruitModel.append({"cost": 5.95, "name":"Jackfruit"});
        console.log(fruitModel.get(0).cost);
        fruitModel.get(0).cost = 10.95;
    }
    \endcode

    The \a index must be an element in the list.

    Note that properties of the returned object that are themselves objects
    will also be models, and this get() method is used to access elements:

    \code
        fruitModel.append(..., "attributes":
            [{"name":"spikes","value":"7mm"},
             {"name":"color","value":"green"}]);
        fruitModel.get(0).attributes.get(1).value; // == "green"
    \endcode

    \warning The returned object is not guaranteed to remain valid. It
    should not be used in \l{Property Binding}{property bindings}.

    \sa append()
*/
QDeclarativeV8Handle QDeclarativeListModel::get(int index) const
{
    v8::Handle<v8::Value> result = v8::Undefined();

    if (index >= 0 && index < m_listModel->elementCount()) {
        QV8Engine *v8engine = engine();

        ModelObject *object = m_listModel->getOrCreateModelObject(const_cast<QDeclarativeListModel *>(this), index);
        result = v8engine->newQObject(object);
    }

    return QDeclarativeV8Handle::fromHandle(result);
}

/*!
    \qmlmethod QtQuick2::ListModel::set(int index, jsobject dict)

    Changes the item at \a index in the list model with the
    values in \a dict. Properties not appearing in \a dict
    are left unchanged.

    \code
        fruitModel.set(3, {"cost": 5.95, "name":"Pizza"})
    \endcode

    If \a index is equal to count() then a new item is appended to the
    list. Otherwise, \a index must be an element in the list.

    \sa append()
*/
void QDeclarativeListModel::set(int index, const QDeclarativeV8Handle &handle)
{
    v8::Handle<v8::Value> valuemap = handle.toHandle();

    if (!valuemap->IsObject() || valuemap->IsArray()) {
        qmlInfo(this) << tr("set: value is not an object");
        return;
    }
    if (index > count() || index < 0) {
        qmlInfo(this) << tr("set: index %1 out of range").arg(index);
        return;
    }

    v8::Handle<v8::Object> object = valuemap->ToObject();

    if (index == count()) {
        m_listModel->insert(index, object);
        emitItemsInserted(index, 1);
    } else {

        QList<int> roles;
        m_listModel->set(index, object, &roles);

        if (roles.count())
            emitItemsChanged(index, 1, roles);
    }
}

/*!
    \qmlmethod QtQuick2::ListModel::setProperty(int index, string property, variant value)

    Changes the \a property of the item at \a index in the list model to \a value.

    \code
        fruitModel.setProperty(3, "cost", 5.95)
    \endcode

    The \a index must be an element in the list.

    \sa append()
*/
void QDeclarativeListModel::setProperty(int index, const QString& property, const QVariant& value)
{
    if (count() == 0 || index >= count() || index < 0) {
        qmlInfo(this) << tr("set: index %1 out of range").arg(index);
        return;
    }

    int roleIndex = m_listModel->setOrCreateProperty(index, property, value);
    if (roleIndex != -1) {

        QList<int> roles;
        roles << roleIndex;

        emitItemsChanged(index, 1, roles);
    }
}

/*!
    \qmlmethod QtQuick2::ListModel::sync()

    Writes any unsaved changes to the list model after it has been modified
    from a worker script.
*/
void QDeclarativeListModel::sync()
{
    // This is just a dummy method to make it look like sync() exists in
    // ListModel (and not just QDeclarativeListModelWorkerAgent) and to let
    // us document sync().
    qmlInfo(this) << "List sync() can only be called from a WorkerScript";
}

bool QDeclarativeListModelParser::compileProperty(const QDeclarativeCustomParserProperty &prop, QList<ListInstruction> &instr, QByteArray &data)
{
    QList<QVariant> values = prop.assignedValues();
    for(int ii = 0; ii < values.count(); ++ii) {
        const QVariant &value = values.at(ii);

        if(value.userType() == qMetaTypeId<QDeclarativeCustomParserNode>()) {
            QDeclarativeCustomParserNode node =
                qvariant_cast<QDeclarativeCustomParserNode>(value);

            if (node.name() != listElementTypeName) {
                const QMetaObject *mo = resolveType(node.name());
                if (mo != &QDeclarativeListElement::staticMetaObject) {
                    error(node, QDeclarativeListModel::tr("ListElement: cannot contain nested elements"));
                    return false;
                }
                listElementTypeName = node.name(); // cache right name for next time
            }

            {
            ListInstruction li;
            li.type = ListInstruction::Push;
            li.dataIdx = -1;
            instr << li;
            }

            QList<QDeclarativeCustomParserProperty> props = node.properties();
            for(int jj = 0; jj < props.count(); ++jj) {
                const QDeclarativeCustomParserProperty &nodeProp = props.at(jj);
                if (nodeProp.name().isEmpty()) {
                    error(nodeProp, QDeclarativeListModel::tr("ListElement: cannot contain nested elements"));
                    return false;
                }
                if (nodeProp.name() == QStringLiteral("id")) {
                    error(nodeProp, QDeclarativeListModel::tr("ListElement: cannot use reserved \"id\" property"));
                    return false;
                }

                ListInstruction li;
                int ref = data.count();
                data.append(nodeProp.name().toUtf8());
                data.append('\0');
                li.type = ListInstruction::Set;
                li.dataIdx = ref;
                instr << li;

                if(!compileProperty(nodeProp, instr, data))
                    return false;

                li.type = ListInstruction::Pop;
                li.dataIdx = -1;
                instr << li;
            }

            {
            ListInstruction li;
            li.type = ListInstruction::Pop;
            li.dataIdx = -1;
            instr << li;
            }

        } else {

            QDeclarativeScript::Variant variant =
                qvariant_cast<QDeclarativeScript::Variant>(value);

            int ref = data.count();

            QByteArray d;
            d += char(variant.type()); // type tag
            if (variant.isString()) {
                d += variant.asString().toUtf8();
            } else if (variant.isNumber()) {
                d += QByteArray::number(variant.asNumber(),'g',20);
            } else if (variant.isBoolean()) {
                d += char(variant.asBoolean());
            } else if (variant.isScript()) {
                if (definesEmptyList(variant.asScript())) {
                    d[0] = char(QDeclarativeScript::Variant::Invalid); // marks empty list
                } else {
                    QByteArray script = variant.asScript().toUtf8();
                    int v = evaluateEnum(script);
                    if (v<0) {
                        using namespace QDeclarativeJS;
                        AST::Node *node = variant.asAST();
                        AST::StringLiteral *literal = 0;
                        if (AST::CallExpression *callExpr = AST::cast<AST::CallExpression *>(node)) {
                            if (AST::IdentifierExpression *idExpr = AST::cast<AST::IdentifierExpression *>(callExpr->base)) {
                                if (idExpr->name == QLatin1String("QT_TR_NOOP") || idExpr->name == QLatin1String("QT_TRID_NOOP")) {
                                    if (callExpr->arguments && !callExpr->arguments->next)
                                        literal = AST::cast<AST::StringLiteral *>(callExpr->arguments->expression);
                                    if (!literal) {
                                        error(prop, QDeclarativeListModel::tr("ListElement: improperly specified %1").arg(idExpr->name.toString()));
                                        return false;
                                    }
                                } else if (idExpr->name == QLatin1String("QT_TRANSLATE_NOOP")) {
                                    if (callExpr->arguments && callExpr->arguments->next && !callExpr->arguments->next->next)
                                        literal = AST::cast<AST::StringLiteral *>(callExpr->arguments->next->expression);
                                    if (!literal) {
                                        error(prop, QDeclarativeListModel::tr("ListElement: improperly specified QT_TRANSLATE_NOOP"));
                                        return false;
                                    }
                                }
                            }
                        }

                        if (literal) {
                            d[0] = char(QDeclarativeScript::Variant::String);
                            d += literal->value.toUtf8();
                        } else {
                            error(prop, QDeclarativeListModel::tr("ListElement: cannot use script for property value"));
                            return false;
                        }
                    } else {
                        d[0] = char(QDeclarativeScript::Variant::Number);
                        d += QByteArray::number(v);
                    }
                }
            }
            d.append('\0');
            data.append(d);

            ListInstruction li;
            li.type = ListInstruction::Value;
            li.dataIdx = ref;
            instr << li;
        }
    }

    return true;
}

QByteArray QDeclarativeListModelParser::compile(const QList<QDeclarativeCustomParserProperty> &customProps)
{
    QList<ListInstruction> instr;
    QByteArray data;
    listElementTypeName = QString(); // unknown

    for(int ii = 0; ii < customProps.count(); ++ii) {
        const QDeclarativeCustomParserProperty &prop = customProps.at(ii);
        if(!prop.name().isEmpty()) { // isn't default property
            error(prop, QDeclarativeListModel::tr("ListModel: undefined property '%1'").arg(prop.name()));
            return QByteArray();
        }

        if(!compileProperty(prop, instr, data)) {
            return QByteArray();
        }
    }

    int size = sizeof(ListModelData) +
               instr.count() * sizeof(ListInstruction) +
               data.count();

    QByteArray rv;
    rv.resize(size);

    ListModelData *lmd = (ListModelData *)rv.data();
    lmd->dataOffset = sizeof(ListModelData) +
                     instr.count() * sizeof(ListInstruction);
    lmd->instrCount = instr.count();
    for (int ii = 0; ii < instr.count(); ++ii)
        lmd->instructions()[ii] = instr.at(ii);
    ::memcpy(rv.data() + lmd->dataOffset, data.constData(), data.count());

    return rv;
}

void QDeclarativeListModelParser::setCustomData(QObject *obj, const QByteArray &d)
{
    QDeclarativeListModel *rv = static_cast<QDeclarativeListModel *>(obj);

    QV8Engine *engine = QDeclarativeEnginePrivate::getV8Engine(qmlEngine(rv));
    rv->m_engine = engine;

    const ListModelData *lmd = (const ListModelData *)d.constData();
    const char *data = ((const char *)lmd) + lmd->dataOffset;

    QStack<DataStackElement> stack;

    for (int ii = 0; ii < lmd->instrCount; ++ii) {
        const ListInstruction &instr = lmd->instructions()[ii];

        switch(instr.type) {
        case ListInstruction::Push:
            {
                ListModel *subModel = 0;

                if (stack.count() == 0) {
                    subModel = rv->m_listModel;
                } else {
                    const DataStackElement &e0 = stack.at(stack.size() - 1);
                    DataStackElement &e1 = stack[stack.size() - 2];

                    const ListLayout::Role &role = e1.model->getOrCreateListRole(e0.name);
                    if (role.type == ListLayout::Role::List) {
                        subModel = e1.model->getListProperty(e1.elementIndex, role);

                        if (subModel == 0) {
                            subModel = new ListModel(role.subLayout, 0, -1);
                            QVariant vModel = QVariant::fromValue(subModel);
                            e1.model->setOrCreateProperty(e1.elementIndex, e0.name, vModel);
                        }
                    }
                }

                DataStackElement e;
                e.model = subModel;
                e.elementIndex = subModel ? subModel->appendElement() : -1;
                stack.push(e);
            }
            break;

        case ListInstruction::Pop:
            stack.pop();
            break;

        case ListInstruction::Value:
            {
                const DataStackElement &e0 = stack.at(stack.size() - 1);
                DataStackElement &e1 = stack[stack.size() - 2];

                QString name = e0.name;
                QVariant value;

                switch (QDeclarativeScript::Variant::Type(data[instr.dataIdx])) {
                    case QDeclarativeScript::Variant::Invalid:
                        {
                            const ListLayout::Role &role = e1.model->getOrCreateListRole(e0.name);
                            ListModel *emptyModel = new ListModel(role.subLayout, 0, -1);
                            value = QVariant::fromValue(emptyModel);
                        }
                        break;
                    case QDeclarativeScript::Variant::Boolean:
                        value = bool(data[1 + instr.dataIdx]);
                        break;
                    case QDeclarativeScript::Variant::Number:
                        value = QByteArray(data + 1 + instr.dataIdx).toDouble();
                        break;
                    case QDeclarativeScript::Variant::String:
                        value = QString::fromUtf8(data + 1 + instr.dataIdx);
                        break;
                    default:
                        Q_ASSERT("Format error in ListInstruction");
                }

                e1.model->setOrCreateProperty(e1.elementIndex, name, value);
            }
            break;

        case ListInstruction::Set:
            {
                DataStackElement e;
                e.name = QString::fromUtf8(data + instr.dataIdx);
                stack.push(e);
            }
            break;
        }
    }
}

bool QDeclarativeListModelParser::definesEmptyList(const QString &s)
{
    if (s.startsWith(QLatin1Char('[')) && s.endsWith(QLatin1Char(']'))) {
        for (int i=1; i<s.length()-1; i++) {
            if (!s[i].isSpace())
                return false;
        }
        return true;
    }
    return false;
}


/*!
    \qmlclass ListElement QDeclarativeListElement
    \inqmlmodule QtQuick 2
    \ingroup qml-working-with-data
    \brief The ListElement element defines a data item in a ListModel.

    List elements are defined inside ListModel definitions, and represent items in a
    list that will be displayed using ListView or \l Repeater items.

    List elements are defined like other QML elements except that they contain
    a collection of \e role definitions instead of properties. Using the same
    syntax as property definitions, roles both define how the data is accessed
    and include the data itself.

    The names used for roles must begin with a lower-case letter and should be
    common to all elements in a given model. Values must be simple constants; either
    strings (quoted and optionally within a call to QT_TR_NOOP), boolean values
    (true, false), numbers, or enumeration values (such as AlignText.AlignHCenter).

    \section1 Referencing Roles

    The role names are used by delegates to obtain data from list elements.
    Each role name is accessible in the delegate's scope, and refers to the
    corresponding role in the current element. Where a role name would be
    ambiguous to use, it can be accessed via the \l{ListView::}{model}
    property (e.g., \c{model.cost} instead of \c{cost}).

    \section1 Example Usage

    The following model defines a series of list elements, each of which
    contain "name" and "cost" roles and their associated values.

    \snippet doc/src/snippets/declarative/qml-data-models/listelements.qml model

    The delegate obtains the name and cost for each element by simply referring
    to \c name and \c cost:

    \snippet doc/src/snippets/declarative/qml-data-models/listelements.qml view

    \sa ListModel
*/

QT_END_NAMESPACE
