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

#include "QtQuick1/private/qdeclarativelistmodel_p_p.h"
#include "QtQuick1/private/qdeclarativelistmodelworkeragent_p.h"
#include "QtQuick1/private/qdeclarativeopenmetaobject_p.h"

#include <QtDeclarative/private/qdeclarativecustomparser_p.h>
#include <QtDeclarative/private/qdeclarativeparser_p.h>
#include <QtDeclarative/private/qdeclarativeengine_p.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeinfo.h>

#include <QtCore/qdebug.h>
#include <QtCore/qstack.h>
#include <QXmlStreamReader>
#include <QtDeclarative/qscriptvalueiterator.h>

Q_DECLARE_METATYPE(QListModelInterface *)

QT_BEGIN_NAMESPACE



template<typename T>
void qdeclarativelistmodel_move(int from, int to, int n, T *items)
{
    if (n == 1) {
        items->move(from, to);
    } else {
        T replaced;
        int i=0;
        typename T::ConstIterator it=items->begin(); it += from+n;
        for (; i<to-from; ++i,++it)
            replaced.append(*it);
        i=0;
        it=items->begin(); it += from;
        for (; i<n; ++i,++it)
            replaced.append(*it);
        typename T::ConstIterator f=replaced.begin();
        typename T::Iterator t=items->begin(); t += from;
        for (; f != replaced.end(); ++f, ++t)
            *t = *f;
    }
}

QDeclarative1ListModelParser::ListInstruction *QDeclarative1ListModelParser::ListModelData::instructions() const
{
    return (QDeclarative1ListModelParser::ListInstruction *)((char *)this + sizeof(ListModelData));
}

/*!
    \qmlclass ListModel QDeclarative1ListModel
    \inqmlmodule QtQuick 1
    \ingroup qml-working-with-data
    \since QtQuick 1.0
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

    \snippet doc/src/snippets/qtquick1/listmodel.qml 0

    \clearfloat
    Roles (properties) in each element must begin with a lower-case letter and
    should be common to all elements in a model. The ListElement documentation
    provides more guidelines for how elements should be defined.

    Since the example model contains an \c id property, it can be referenced
    by views, such as the ListView in this example:

    \snippet doc/src/snippets/qtquick1/listmodel-simple.qml 0
    \dots 8
    \snippet doc/src/snippets/qtquick1/listmodel-simple.qml 1

    It is possible for roles to contain list data.  In the following example we
    create a list of fruit attributes:

    \snippet doc/src/snippets/qtquick1/listmodel-nested.qml model

    The delegate displays all the fruit attributes:

    \div {class="float-right"}
    \inlineimage listmodel-nested.png
    \enddiv

    \snippet doc/src/snippets/qtquick1/listmodel-nested.qml delegate

    \clearfloat
    \section1 Modifying List Models

    The content of a ListModel may be created and modified using the clear(),
    append(), set(), insert() and setProperty() methods.  For example:

    \snippet doc/src/snippets/qtquick1/listmodel-modify.qml delegate

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

    \section1 Restrictions

    If a list model is to be accessed from a WorkerScript, it cannot
    contain list-type data. So, the following model cannot be used from a WorkerScript
    because of the list contained in the "attributes" property:

    \code
    ListModel {
        id: fruitModel
        ListElement {
            name: "Apple"
            cost: 2.45
            attributes: [
                ListElement { description: "Core" },
                ListElement { description: "Deciduous" }
            ]
        }
    }
    \endcode

    In addition, the WorkerScript cannot add list-type data to the model.

    \sa {qmlmodels}{Data Models}, {declarative/threading/threadedlistmodel}{Threaded ListModel example}, QtDeclarative
*/


/*
    A ListModel internally uses either a NestedListModel_1 or FlatListModel_1.

    A NestedListModel_1 can contain lists of ListElements (which
    when retrieved from get() is accessible as a list model within the list
    model) whereas a FlatListModel_1 cannot.

    ListModel uses a NestedListModel_1 to begin with, and if the model is later 
    used from a WorkerScript, it changes to use a FlatListModel_1 instead. This
    is because ModelNode (which abstracts the nested list model data) needs
    access to the declarative engine and script engine, which cannot be
    safely used from outside of the main thread.
*/

QDeclarative1ListModel::QDeclarative1ListModel(QObject *parent)
: QListModelInterface(parent), m_agent(0), m_nested(new NestedListModel_1(this)), m_flat(0)
{
}

QDeclarative1ListModel::QDeclarative1ListModel(const QDeclarative1ListModel *orig, QDeclarative1ListModelWorkerAgent *parent)
: QListModelInterface(parent), m_agent(0), m_nested(0), m_flat(0)
{
    m_flat = new FlatListModel_1(this);
    m_flat->m_parentAgent = parent;

    if (orig->m_flat) {
        m_flat->m_roles = orig->m_flat->m_roles;
        m_flat->m_strings = orig->m_flat->m_strings;
        m_flat->m_values = orig->m_flat->m_values;

        m_flat->m_nodeData.reserve(m_flat->m_values.count());
        for (int i=0; i<m_flat->m_values.count(); i++)
            m_flat->m_nodeData << 0;
    }
}

QDeclarative1ListModel::~QDeclarative1ListModel()
{
    if (m_agent)
        m_agent->release();

    delete m_nested;
    delete m_flat;
}

bool QDeclarative1ListModel::flatten()
{
    if (m_flat)
        return true;

    QList<int> roles = m_nested->roles();

    QList<QHash<int, QVariant> > values;
    bool hasNested = false;
    for (int i=0; i<m_nested->count(); i++) {
        values.append(m_nested->data(i, roles, &hasNested));
        if (hasNested)
            return false;
    }

    FlatListModel_1 *flat = new FlatListModel_1(this);
    flat->m_values = values;

    for (int i=0; i<roles.count(); i++) {
        QString s = m_nested->toString(roles[i]);
        flat->m_roles.insert(roles[i], s);
        flat->m_strings.insert(s, roles[i]);
    }

    flat->m_nodeData.reserve(flat->m_values.count());
    for (int i=0; i<flat->m_values.count(); i++)
        flat->m_nodeData << 0;

    m_flat = flat;
    delete m_nested;
    m_nested = 0;
    return true;
}

bool QDeclarative1ListModel::inWorkerThread() const
{
    return m_flat && m_flat->m_parentAgent;
}

QDeclarative1ListModelWorkerAgent *QDeclarative1ListModel::agent()
{
    if (m_agent)
        return m_agent;

    if (!flatten()) {
        qmlInfo(this) << "List contains list-type data and cannot be used from a worker script";
        return 0;
    }

    m_agent = new QDeclarative1ListModelWorkerAgent(this);
    return m_agent;
}

QList<int> QDeclarative1ListModel::roles() const
{
    return m_flat ? m_flat->roles() : m_nested->roles();
}

QString QDeclarative1ListModel::toString(int role) const
{
    return m_flat ? m_flat->toString(role) : m_nested->toString(role);
}

QVariant QDeclarative1ListModel::data(int index, int role) const
{
    if (index >= count() || index < 0)
        return QVariant();

    return m_flat ? m_flat->data(index, role) : m_nested->data(index, role);
}

/*!
    \qmlproperty int QtQuick1::ListModel::count
    The number of data entries in the model.
*/
int QDeclarative1ListModel::count() const
{
    return m_flat ? m_flat->count() : m_nested->count();
}

/*!
    \qmlmethod QtQuick1::ListModel::clear()

    Deletes all content from the model.

    \sa append() remove()
*/
void QDeclarative1ListModel::clear()
{
    int cleared = count();
    if (m_flat)
        m_flat->clear();
    else
        m_nested->clear();

    if (!inWorkerThread()) {
        emit itemsRemoved(0, cleared);
        emit countChanged();
    }
}

QDeclarative1ListModel *ModelNode::model(const NestedListModel_1 *model)
{
    if (!modelCache) { 
        modelCache = new QDeclarative1ListModel;
        QDeclarativeEngine::setContextForObject(modelCache,QDeclarativeEngine::contextForObject(model->m_listModel));
        modelCache->m_nested->_root = this;  // ListModel defaults to nestable model

        for (int i=0; i<values.count(); ++i) {
            ModelNode *subNode = qvariant_cast<ModelNode *>(values.at(i));
            if (subNode)
                subNode->m_model = modelCache->m_nested;
        }
    }
    return modelCache;
}

ModelObject_1 *ModelNode::object(const NestedListModel_1 *model)
{
    if (!objectCache) {
        objectCache = new ModelObject_1(this, 
                const_cast<NestedListModel_1*>(model), 
                QDeclarativeEnginePrivate::getScriptEngine(qmlEngine(model->m_listModel)));
        QHash<QString, ModelNode *>::iterator it;
        for (it = properties.begin(); it != properties.end(); ++it) {
            objectCache->setValue(it.key().toUtf8(), model->valueForNode(*it));
        }
        objectCache->setNodeUpdatesEnabled(true);
    }
    return objectCache;
}

/*!
    \qmlmethod QtQuick1::ListModel::remove(int index)

    Deletes the content at \a index from the model.

    \sa clear()
*/
void QDeclarative1ListModel::remove(int index)
{
    if (index < 0 || index >= count()) {
        qmlInfo(this) << tr("remove: index %1 out of range").arg(index);
        return;
    }

    if (m_flat)
        m_flat->remove(index);
    else
        m_nested->remove(index);

    if (!inWorkerThread()) {
        emit itemsRemoved(index, 1);
        emit countChanged();
    }
}

/*!
    \qmlmethod QtQuick1::ListModel::insert(int index, jsobject dict)

    Adds a new item to the list model at position \a index, with the
    values in \a dict.

    \code
        fruitModel.insert(2, {"cost": 5.95, "name":"Pizza"})
    \endcode

    The \a index must be to an existing item in the list, or one past
    the end of the list (equivalent to append).

    \sa set() append()
*/
void QDeclarative1ListModel::insert(int index, const QScriptValue& valuemap)
{
    if (!valuemap.isObject() || valuemap.isArray()) {
        qmlInfo(this) << tr("insert: value is not an object");
        return;
    }

    if (index < 0 || index > count()) {
        qmlInfo(this) << tr("insert: index %1 out of range").arg(index);
        return;
    }

    bool ok = m_flat ?  m_flat->insert(index, valuemap) : m_nested->insert(index, valuemap);
    if (ok && !inWorkerThread()) {
        emit itemsInserted(index, 1);
        emit countChanged();
    }
}

/*!
    \qmlmethod QtQuick1::ListModel::move(int from, int to, int n)

    Moves \a n items \a from one position \a to another.

    The from and to ranges must exist; for example, to move the first 3 items
    to the end of the list:

    \code
        fruitModel.move(0, fruitModel.count - 3, 3)
    \endcode

    \sa append()
*/
void QDeclarative1ListModel::move(int from, int to, int n)
{
    if (n==0 || from==to)
        return;
    if (!canMove(from, to, n)) {
        qmlInfo(this) << tr("move: out of range");
        return;
    }

    int origfrom = from;
    int origto = to;
    int orign = n;
    if (from > to) {
        // Only move forwards - flip if backwards moving
        int tfrom = from;
        int tto = to;
        from = tto;
        to = tto+n;
        n = tfrom-tto;
    }

    if (m_flat)
        m_flat->move(from, to, n);
    else
        m_nested->move(from, to, n);

    if (!inWorkerThread())
        emit itemsMoved(origfrom, origto, orign);
}

/*!
    \qmlmethod QtQuick1::ListModel::append(jsobject dict)

    Adds a new item to the end of the list model, with the
    values in \a dict.

    \code
        fruitModel.append({"cost": 5.95, "name":"Pizza"})
    \endcode

    \sa set() remove()
*/
void QDeclarative1ListModel::append(const QScriptValue& valuemap)
{
    if (!valuemap.isObject() || valuemap.isArray()) {
        qmlInfo(this) << tr("append: value is not an object");
        return;
    }

    insert(count(), valuemap);
}

/*!
    \qmlmethod object QtQuick1::ListModel::get(int index)

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
QScriptValue QDeclarative1ListModel::get(int index) const
{
    // the internal flat/nested class checks for bad index
    return m_flat ? m_flat->get(index) : m_nested->get(index);
}

/*!
    \qmlmethod QtQuick1::ListModel::set(int index, jsobject dict)

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
void QDeclarative1ListModel::set(int index, const QScriptValue& valuemap)
{
    QList<int> roles;
    set(index, valuemap, &roles);
    if (!roles.isEmpty() && !inWorkerThread())
        emit itemsChanged(index, 1, roles);
}

void QDeclarative1ListModel::set(int index, const QScriptValue& valuemap, QList<int> *roles)
{
    if (!valuemap.isObject() || valuemap.isArray()) {
        qmlInfo(this) << tr("set: value is not an object");
        return;
    }
    if (index > count() || index < 0) {
        qmlInfo(this) << tr("set: index %1 out of range").arg(index);
        return;
    }

    if (index == count()) {
        append(valuemap);
    } else {
        if (m_flat)
            m_flat->set(index, valuemap, roles);
        else
            m_nested->set(index, valuemap, roles);
    }
}

/*!
    \qmlmethod QtQuick1::ListModel::setProperty(int index, string property, variant value)

    Changes the \a property of the item at \a index in the list model to \a value.

    \code
        fruitModel.setProperty(3, "cost", 5.95)
    \endcode

    The \a index must be an element in the list.

    \sa append()
*/
void QDeclarative1ListModel::setProperty(int index, const QString& property, const QVariant& value)
{
    QList<int> roles;
    setProperty(index, property, value, &roles);
    if (!roles.isEmpty() && !inWorkerThread())
        emit itemsChanged(index, 1, roles);
}

void QDeclarative1ListModel::setProperty(int index, const QString& property, const QVariant& value, QList<int> *roles)
{
    if (count() == 0 || index >= count() || index < 0) {
        qmlInfo(this) << tr("set: index %1 out of range").arg(index);
        return;
    }

    if (m_flat)
        m_flat->setProperty(index, property, value, roles);
    else
        m_nested->setProperty(index, property, value, roles);
}

/*!
    \qmlmethod QtQuick1::ListModel::sync()

    Writes any unsaved changes to the list model after it has been modified
    from a worker script.
*/
void QDeclarative1ListModel::sync()
{
    // This is just a dummy method to make it look like sync() exists in
    // ListModel (and not just QDeclarative1ListModelWorkerAgent) and to let
    // us document sync().
    qmlInfo(this) << "List sync() can only be called from a WorkerScript";
}

bool QDeclarative1ListModelParser::compileProperty(const QDeclarativeCustomParserProperty &prop, QList<ListInstruction> &instr, QByteArray &data)
{
    QList<QVariant> values = prop.assignedValues();
    for(int ii = 0; ii < values.count(); ++ii) {
        const QVariant &value = values.at(ii);

        if(value.userType() == qMetaTypeId<QDeclarativeCustomParserNode>()) {
            QDeclarativeCustomParserNode node =
                qvariant_cast<QDeclarativeCustomParserNode>(value);

            if (node.name() != listElementTypeName) {
                const QMetaObject *mo = resolveType(node.name());
                if (mo != &QDeclarative1ListElement::staticMetaObject) {
                    error(node, QDeclarative1ListModel::tr("ListElement: cannot contain nested elements"));
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
                    error(nodeProp, QDeclarative1ListModel::tr("ListElement: cannot contain nested elements"));
                    return false;
                }
                if (nodeProp.name() == "id") {
                    error(nodeProp, QDeclarative1ListModel::tr("ListElement: cannot use reserved \"id\" property"));
                    return false;
                }

                ListInstruction li;
                int ref = data.count();
                data.append(nodeProp.name());
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

            QDeclarativeParser::Variant variant =
                qvariant_cast<QDeclarativeParser::Variant>(value);

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
                    d[0] = char(QDeclarativeParser::Variant::Invalid); // marks empty list
                } else {
                    QByteArray script = variant.asScript().toUtf8();
                    int v = evaluateEnum(script);
                    if (v<0) {
                        if (script.startsWith("QT_TR_NOOP(\"") && script.endsWith("\")")) {
                            d[0] = char(QDeclarativeParser::Variant::String);
                            d += script.mid(12,script.length()-14);
                        } else {
                            error(prop, QDeclarative1ListModel::tr("ListElement: cannot use script for property value"));
                            return false;
                        }
                    } else {
                        d[0] = char(QDeclarativeParser::Variant::Number);
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

QByteArray QDeclarative1ListModelParser::compile(const QList<QDeclarativeCustomParserProperty> &customProps)
{
    QList<ListInstruction> instr;
    QByteArray data;
    listElementTypeName = QByteArray(); // unknown

    for(int ii = 0; ii < customProps.count(); ++ii) {
        const QDeclarativeCustomParserProperty &prop = customProps.at(ii);
        if(!prop.name().isEmpty()) { // isn't default property
            error(prop, QDeclarative1ListModel::tr("ListModel: undefined property '%1'").arg(QString::fromUtf8(prop.name())));
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

void QDeclarative1ListModelParser::setCustomData(QObject *obj, const QByteArray &d)
{
    QDeclarative1ListModel *rv = static_cast<QDeclarative1ListModel *>(obj);

    ModelNode *root = new ModelNode(rv->m_nested);
    rv->m_nested->m_ownsRoot = true;
    rv->m_nested->_root = root;
    QStack<ModelNode *> nodes;
    nodes << root;

    bool processingSet = false;

    const ListModelData *lmd = (const ListModelData *)d.constData();
    const char *data = ((const char *)lmd) + lmd->dataOffset;

    for (int ii = 0; ii < lmd->instrCount; ++ii) {
        const ListInstruction &instr = lmd->instructions()[ii];

        switch(instr.type) {
        case ListInstruction::Push:
            {
                ModelNode *n = nodes.top();
                ModelNode *n2 = new ModelNode(rv->m_nested);
                n->values << QVariant::fromValue(n2);
                nodes.push(n2);
                if (processingSet)
                    n->isArray = true;
            }
            break;

        case ListInstruction::Pop:
            nodes.pop();
            break;

        case ListInstruction::Value:
            {
                ModelNode *n = nodes.top();
                switch (QDeclarativeParser::Variant::Type(data[instr.dataIdx])) {
                 case QDeclarativeParser::Variant::Invalid:
                    n->isArray = true;
                    break;
                 case QDeclarativeParser::Variant::Boolean:
                    n->values.append(bool(data[1 + instr.dataIdx]));
                    break;
                 case QDeclarativeParser::Variant::Number:
                    n->values.append(QByteArray(data + 1 + instr.dataIdx).toDouble());
                    break;
                 case QDeclarativeParser::Variant::String:
                    n->values.append(QString::fromUtf8(data + 1 + instr.dataIdx));
                    break;
                 default:
                    Q_ASSERT("Format error in ListInstruction");
                }

                processingSet = false;
            }
            break;

        case ListInstruction::Set:
            {
                ModelNode *n = nodes.top();
                ModelNode *n2 = new ModelNode(rv->m_nested);
                n->properties.insert(QString::fromUtf8(data + instr.dataIdx), n2);
                nodes.push(n2);
                processingSet = true;
            }
            break;
        }
    }

    ModelNode *rootNode = rv->m_nested->_root;
    for (int i=0; i<rootNode->values.count(); ++i) {
        ModelNode *node = qvariant_cast<ModelNode *>(rootNode->values[i]);
        node->listIndex = i;
        node->updateListIndexes();
    }
}

bool QDeclarative1ListModelParser::definesEmptyList(const QString &s)
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
    \qmlclass ListElement QDeclarative1ListElement
    \inqmlmodule QtQuick 1
    \ingroup qml-working-with-data
    \since QtQuick 1.0
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

    \snippet doc/src/snippets/qtquick1/qml-data-models/listelements.qml model

    The delegate obtains the name and cost for each element by simply referring
    to \c name and \c cost:

    \snippet doc/src/snippets/qtquick1/qml-data-models/listelements.qml view

    \sa ListModel
*/

FlatListModel_1::FlatListModel_1(QDeclarative1ListModel *base)
    : m_scriptEngine(0), m_listModel(base), m_scriptClass(0), m_parentAgent(0)
{
}

FlatListModel_1::~FlatListModel_1()
{
    qDeleteAll(m_nodeData);
}

QVariant FlatListModel_1::data(int index, int role) const
{
    Q_ASSERT(index >= 0 && index < m_values.count());
    if (m_values[index].contains(role))
        return m_values[index][role];
    return QVariant();
}

QList<int> FlatListModel_1::roles() const
{
    return m_roles.keys();
}

QString FlatListModel_1::toString(int role) const
{
    if (m_roles.contains(role))
        return m_roles[role];
    return QString();
}

int FlatListModel_1::count() const
{
    return m_values.count();
}

void FlatListModel_1::clear()
{
    m_values.clear();

    qDeleteAll(m_nodeData);
    m_nodeData.clear();
}

void FlatListModel_1::remove(int index)
{
    m_values.removeAt(index);
    removedNode(index);
}

bool FlatListModel_1::insert(int index, const QScriptValue &value)
{
    Q_ASSERT(index >= 0 && index <= m_values.count());

    QHash<int, QVariant> row;
    if (!addValue(value, &row, 0))
        return false;

    m_values.insert(index, row);
    insertedNode(index);

    return true;
}

QScriptValue FlatListModel_1::get(int index) const
{
    QScriptEngine *scriptEngine = m_scriptEngine ? m_scriptEngine : QDeclarativeEnginePrivate::getScriptEngine(qmlEngine(m_listModel));

    if (!scriptEngine) 
        return 0;

    if (index < 0 || index >= m_values.count())
        return scriptEngine->undefinedValue();

    FlatListModel_1 *that = const_cast<FlatListModel_1*>(this);
    if (!m_scriptClass)
        that->m_scriptClass = new FlatListScriptClass_1(that, scriptEngine);

    FlatNodeData_1 *data = m_nodeData.value(index);
    if (!data) {
        data = new FlatNodeData_1(index);
        that->m_nodeData.replace(index, data);
    }

    return QScriptDeclarativeClass::newObject(scriptEngine, m_scriptClass, new FlatNodeObjectData(data));
}

void FlatListModel_1::set(int index, const QScriptValue &value, QList<int> *roles)
{
    Q_ASSERT(index >= 0 && index < m_values.count());

    QHash<int, QVariant> row = m_values[index];
    if (addValue(value, &row, roles))
        m_values[index] = row;
}

void FlatListModel_1::setProperty(int index, const QString& property, const QVariant& value, QList<int> *roles)
{
    Q_ASSERT(index >= 0 && index < m_values.count());

    QHash<QString, int>::Iterator iter = m_strings.find(property);
    int role;
    if (iter == m_strings.end()) {
        role = m_roles.count();
        m_roles.insert(role, property);
        m_strings.insert(property, role);
    } else {
        role = iter.value();
    }

    if (m_values[index][role] != value) {
        roles->append(role);
        m_values[index][role] = value;
    }
}

void FlatListModel_1::move(int from, int to, int n)
{
    qdeclarativelistmodel_move<QList<QHash<int, QVariant> > >(from, to, n, &m_values);
    moveNodes(from, to, n);
}

bool FlatListModel_1::addValue(const QScriptValue &value, QHash<int, QVariant> *row, QList<int> *roles)
{
    QScriptValueIterator it(value);
    while (it.hasNext()) {
        it.next();
        QScriptValue value = it.value();
        if (!value.isVariant() && !value.isRegExp() && !value.isDate() && value.isObject()) {
            qmlInfo(m_listModel) << "Cannot add list-type data when modifying or after modification from a worker script";
            return false;
        }

        QString name = it.name();
        QVariant v = it.value().toVariant();

        QHash<QString, int>::Iterator iter = m_strings.find(name);
        if (iter == m_strings.end()) {
            int role = m_roles.count();
            m_roles.insert(role, name);
            iter = m_strings.insert(name, role);
            if (roles)
                roles->append(role);
        } else {
            int role = iter.value();
            if (roles && row->contains(role) && row->value(role) != v)
                roles->append(role);
        }
        row->insert(*iter, v);
    }
    return true;
}

void FlatListModel_1::insertedNode(int index)
{
    if (index >= 0 && index <= m_values.count()) {
        m_nodeData.insert(index, 0);

        for (int i=index + 1; i<m_nodeData.count(); i++) {
            if (m_nodeData[i])
                m_nodeData[i]->index = i;
        }
    }
}

void FlatListModel_1::removedNode(int index)
{
    if (index >= 0 && index < m_nodeData.count()) {
        delete m_nodeData.takeAt(index);

        for (int i=index; i<m_nodeData.count(); i++) {
            if (m_nodeData[i])
                m_nodeData[i]->index = i;
        }
    }
}

void FlatListModel_1::moveNodes(int from, int to, int n)
{
    if (!m_listModel->canMove(from, to, n))
        return;

    qdeclarativelistmodel_move<QList<FlatNodeData_1 *> >(from, to, n, &m_nodeData);

    for (int i=from; i<from + (to-from); i++)  {
        if (m_nodeData[i]) 
            m_nodeData[i]->index = i;
    }
}



FlatNodeData_1::~FlatNodeData_1()
{
    for (QSet<FlatNodeObjectData *>::Iterator iter = objects.begin(); iter != objects.end(); ++iter) {
        FlatNodeObjectData *data = *iter;
        data->nodeData = 0;
    }
}

void FlatNodeData_1::addData(FlatNodeObjectData *data) 
{
    objects.insert(data);
}

void FlatNodeData_1::removeData(FlatNodeObjectData *data)
{
    objects.remove(data);
}


FlatListScriptClass_1::FlatListScriptClass_1(FlatListModel_1 *model, QScriptEngine *seng)
    : QScriptDeclarativeClass(seng),
      m_model(model)
{
}

QScriptDeclarativeClass::Value FlatListScriptClass_1::property(Object *obj, const Identifier &name)
{
    FlatNodeObjectData *objData = static_cast<FlatNodeObjectData*>(obj);
    if (!objData->nodeData) // item at this index has been deleted
        return QScriptDeclarativeClass::Value(engine(), engine()->undefinedValue());

    int index = objData->nodeData->index;
    QString propName = toString(name);
    int role = m_model->m_strings.value(propName, -1);

    if (role >= 0 && index >=0 ) {
        const QHash<int, QVariant> &row = m_model->m_values[index];
        QScriptValue sv = engine()->toScriptValue<QVariant>(row[role]);
        return QScriptDeclarativeClass::Value(engine(), sv);
    }

    return QScriptDeclarativeClass::Value(engine(), engine()->undefinedValue());
}

void FlatListScriptClass_1::setProperty(Object *obj, const Identifier &name, const QScriptValue &value)
{
    if (!value.isVariant() && !value.isRegExp() && !value.isDate() && value.isObject()) {
        qmlInfo(m_model->m_listModel) << "Cannot add list-type data when modifying or after modification from a worker script";
        return;
    }

    FlatNodeObjectData *objData = static_cast<FlatNodeObjectData*>(obj);
    if (!objData->nodeData) // item at this index has been deleted
        return;

    int index = objData->nodeData->index;
    QString propName = toString(name);

    int role = m_model->m_strings.value(propName, -1);
    if (role >= 0 && index >= 0) {
        QHash<int, QVariant> &row = m_model->m_values[index];
        row[role] = value.toVariant();

        QList<int> roles;
        roles << role;
        if (m_model->m_parentAgent) {
            // This is the list in the worker thread, so tell the agent to
            // emit itemsChanged() later
            m_model->m_parentAgent->changedData(index, 1, roles);
        } else {
            // This is the list in the main thread, so emit itemsChanged()
            emit m_model->m_listModel->itemsChanged(index, 1, roles);
        }
    }
}

QScriptClass::QueryFlags FlatListScriptClass_1::queryProperty(Object *, const Identifier &, QScriptClass::QueryFlags)
{
    return (QScriptClass::HandlesReadAccess | QScriptClass::HandlesWriteAccess);
}

bool FlatListScriptClass_1::compare(Object *obj1, Object *obj2)
{
    FlatNodeObjectData *data1 = static_cast<FlatNodeObjectData*>(obj1);
    FlatNodeObjectData *data2 = static_cast<FlatNodeObjectData*>(obj2);

    if (!data1->nodeData || !data2->nodeData)
        return false;

    return data1->nodeData->index == data2->nodeData->index;
}



NestedListModel_1::NestedListModel_1(QDeclarative1ListModel *base)
    : _root(0), m_ownsRoot(false), m_listModel(base), _rolesOk(false)
{
}

NestedListModel_1::~NestedListModel_1()
{
    if (m_ownsRoot)
        delete _root;
}

QVariant NestedListModel_1::valueForNode(ModelNode *node, bool *hasNested) const
{
    QObject *rv = 0;
    if (hasNested)
        *hasNested = false;

    if (node->isArray) {
        // List
        rv = node->model(this);
        if (hasNested)
            *hasNested = true;
    } else {
        if (!node->properties.isEmpty()) {
            // Object
            rv = node->object(this);
        } else if (node->values.count() == 0) {
            // Invalid
            return QVariant();
        } else if (node->values.count() == 1) {
            // Value
            QVariant &var = node->values[0];
            ModelNode *valueNode = qvariant_cast<ModelNode *>(var);
            if (valueNode) {
                if (!valueNode->properties.isEmpty())
                    rv = valueNode->object(this);
                else
                    rv = valueNode->model(this);
            } else {
                return var;
            }
        }
    }

    if (rv) {
        return QVariant::fromValue(rv);
    } else {
        return QVariant();
    }
}

QHash<int,QVariant> NestedListModel_1::data(int index, const QList<int> &roles, bool *hasNested) const
{
    Q_ASSERT(_root && index >= 0 && index < _root->values.count());
    checkRoles();
    QHash<int, QVariant> rv;

    ModelNode *node = qvariant_cast<ModelNode *>(_root->values.at(index));
    if (!node)
        return rv;

    for (int ii = 0; ii < roles.count(); ++ii) {
        const QString &roleString = roleStrings.at(roles.at(ii));

        QHash<QString, ModelNode *>::ConstIterator iter = node->properties.find(roleString);
        if (iter != node->properties.end()) {
            ModelNode *row = *iter;
            rv.insert(roles.at(ii), valueForNode(row, hasNested));
        }
    }

    return rv;
}

QVariant NestedListModel_1::data(int index, int role) const
{
    Q_ASSERT(_root && index >= 0 && index < _root->values.count());
    checkRoles();
    QVariant rv;
    if (roleStrings.count() < role)
        return rv;

    ModelNode *node = qvariant_cast<ModelNode *>(_root->values.at(index));
    if (!node)
        return rv;

    const QString &roleString = roleStrings.at(role);

    QHash<QString, ModelNode *>::ConstIterator iter = node->properties.find(roleString);
    if (iter != node->properties.end()) {
        ModelNode *row = *iter;
        rv = valueForNode(row);
    }

    return rv;
}

int NestedListModel_1::count() const
{
    if (!_root) return 0;
    return _root->values.count();
}

void NestedListModel_1::clear()
{
    if (_root)
        _root->clear();
}

void NestedListModel_1::remove(int index)
{
    if (!_root)
        return;
    ModelNode *node = qvariant_cast<ModelNode *>(_root->values.at(index));
    _root->values.removeAt(index);
    if (node)
        delete node;
}

bool NestedListModel_1::insert(int index, const QScriptValue& valuemap)
{
    if (!_root) {
        _root = new ModelNode(this);
        m_ownsRoot = true;
    }

    ModelNode *mn = new ModelNode(this);
    mn->listIndex = index;
    mn->setObjectValue(valuemap);
    _root->values.insert(index,QVariant::fromValue(mn));
    return true;
}

void NestedListModel_1::move(int from, int to, int n)
{
    if (!_root)
        return;
    qdeclarativelistmodel_move<QVariantList>(from, to, n, &_root->values);
}

QScriptValue NestedListModel_1::get(int index) const
{   
    QDeclarativeEngine *eng = qmlEngine(m_listModel);
    if (!eng) 
        return 0;

    if (index < 0 || index >= count()) {
        QScriptEngine *seng = QDeclarativeEnginePrivate::getScriptEngine(eng);
        if (seng)
            return seng->undefinedValue();
        return 0;
    }

    ModelNode *node = qvariant_cast<ModelNode *>(_root->values.at(index));
    if (!node)
        return 0;
    
    return QDeclarativeEnginePrivate::qmlScriptObject(node->object(this), eng);
}

void NestedListModel_1::set(int index, const QScriptValue& valuemap, QList<int> *roles)
{
    Q_ASSERT(index >=0 && index < count());

    ModelNode *node = qvariant_cast<ModelNode *>(_root->values.at(index));
    bool emitItemsChanged = node->setObjectValue(valuemap);
    if (!emitItemsChanged)
        return;

    QScriptValueIterator it(valuemap);
    while (it.hasNext()) {
        it.next();
        int r = roleStrings.indexOf(it.name());
        if (r < 0) {
            r = roleStrings.count();
            roleStrings << it.name();
        }
        roles->append(r);
    }
}

void NestedListModel_1::setProperty(int index, const QString& property, const QVariant& value, QList<int> *roles)
{
    Q_ASSERT(index >=0 && index < count());

    ModelNode *node = qvariant_cast<ModelNode *>(_root->values.at(index));
    bool emitItemsChanged = node->setProperty(property, value);
    if (!emitItemsChanged)
        return;

    int r = roleStrings.indexOf(property);
    if (r < 0) {
        r = roleStrings.count();
        roleStrings << property;
    }
    roles->append(r);
}

void NestedListModel_1::checkRoles() const
{
    if (_rolesOk || !_root)
        return;

    for (int i = 0; i<_root->values.count(); ++i) {
        ModelNode *node = qvariant_cast<ModelNode *>(_root->values.at(i));
        if (node) {
            foreach (const QString &role, node->properties.keys()) {
                if (!roleStrings.contains(role))
                    roleStrings.append(role);
            }
        }
    }

    _rolesOk = true;
}

QList<int> NestedListModel_1::roles() const
{
    checkRoles();
    QList<int> rv;
    for (int ii = 0; ii < roleStrings.count(); ++ii)
        rv << ii;
    return rv;
}

QString NestedListModel_1::toString(int role) const
{
    checkRoles();
    if (role < roleStrings.count())
        return roleStrings.at(role);
    else
        return QString();
}


ModelNode::ModelNode(NestedListModel_1 *model)
: modelCache(0), objectCache(0), isArray(false), m_model(model), listIndex(-1)
{
}

ModelNode::~ModelNode()
{
    clear();
    if (modelCache) { modelCache->m_nested->_root = 0/* ==this */; delete modelCache; modelCache = 0; }
    if (objectCache) { delete objectCache; objectCache = 0; }
}

void ModelNode::clear()
{
    ModelNode *node;
    for (int ii = 0; ii < values.count(); ++ii) {
        node = qvariant_cast<ModelNode *>(values.at(ii));
        if (node) { delete node; node = 0; }
    }
    values.clear();

    qDeleteAll(properties.values());
    properties.clear();
}

bool ModelNode::setObjectValue(const QScriptValue& valuemap, bool writeToCache)
{
    bool emitItemsChanged = false;

    QScriptValueIterator it(valuemap);
    while (it.hasNext()) {
        it.next();
        ModelNode *prev = properties.value(it.name());
        ModelNode *value = new ModelNode(m_model);
        QScriptValue v = it.value();

        if (v.isArray()) {
            value->isArray = true;
            value->setListValue(v);
            if (writeToCache && objectCache)
                objectCache->setValue(it.name().toUtf8(), QVariant::fromValue(value->model(m_model)));
            emitItemsChanged = true;    // for now, too inefficient to check whether list and sublists have changed
        } else {
            value->values << v.toVariant();
            if (writeToCache && objectCache)
                objectCache->setValue(it.name().toUtf8(), value->values.last());
            if (!emitItemsChanged && prev && prev->values.count() == 1
                    && prev->values[0] != value->values.last()) {
                emitItemsChanged = true;
            }
        }
        if (properties.contains(it.name()))
            delete properties[it.name()];
        properties.insert(it.name(), value);
    }
    return emitItemsChanged;
}

void ModelNode::setListValue(const QScriptValue& valuelist) {
    values.clear();
    int size = valuelist.property(QLatin1String("length")).toInt32();
    for (int i=0; i<size; i++) {
        ModelNode *value = new ModelNode(m_model);
        QScriptValue v = valuelist.property(i);
        if (v.isArray()) {
            value->isArray = true;
            value->setListValue(v);
        } else if (v.isObject()) {
            value->listIndex = i;
            value->setObjectValue(v);
        } else {
            value->listIndex = i;
            value->values << v.toVariant();
        }
        values.append(QVariant::fromValue(value));
    }
}

bool ModelNode::setProperty(const QString& prop, const QVariant& val) {
    QHash<QString, ModelNode *>::const_iterator it = properties.find(prop);
    bool emitItemsChanged = false;
    if (it != properties.end()) {
        if (val != (*it)->values[0])
            emitItemsChanged = true;
        (*it)->values[0] = val;
    } else {
        ModelNode *n = new ModelNode(m_model);
        n->values << val;
        properties.insert(prop,n);
    }
    if (objectCache)
        objectCache->setValue(prop.toUtf8(), val);
    return emitItemsChanged;
}

void ModelNode::updateListIndexes()
{
    for (QHash<QString, ModelNode *>::ConstIterator iter = properties.begin(); iter != properties.end(); ++iter) {
        ModelNode *node = iter.value();
        if (node->isArray) {
            for (int i=0; i<node->values.count(); ++i) {
                ModelNode *subNode = qvariant_cast<ModelNode *>(node->values.at(i));
                if (subNode)
                    subNode->listIndex = i;
            }
        }
        node->updateListIndexes();
    }
}

/*
    Need to call this to emit itemsChanged() for modifications outside of set()
    and setProperty(), i.e. if an item returned from get() is modified
*/
void ModelNode::changedProperty(const QString &name) const
{
    if (listIndex < 0)
        return;

    m_model->checkRoles();
    QList<int> roles;
    int role = m_model->roleStrings.indexOf(name);
    if (role < 0)
        roles = m_model->roles();
    else
        roles << role;
    emit m_model->m_listModel->itemsChanged(listIndex, 1, roles);
}

void ModelNode::dump(ModelNode *node, int ind)
{
    QByteArray indentBa(ind * 4, ' ');
    const char *indent = indentBa.constData();

    for (int ii = 0; ii < node->values.count(); ++ii) {
        ModelNode *subNode = qvariant_cast<ModelNode *>(node->values.at(ii));
        if (subNode) {
            qWarning().nospace() << indent << "Sub-node " << ii;
            dump(subNode, ind + 1);
        } else {
            qWarning().nospace() << indent << "Sub-node " << ii << ": " << node->values.at(ii).toString();
        }
    }

    for (QHash<QString, ModelNode *>::ConstIterator iter = node->properties.begin(); iter != node->properties.end(); ++iter) {
        qWarning().nospace() << indent << "Property " << iter.key() << ':';
        dump(iter.value(), ind + 1);
    }
}

ModelObject_1::ModelObject_1(ModelNode *node, NestedListModel_1 *model, QScriptEngine *seng)
    : m_model(model),
      m_node(node),
      m_meta(new ModelNodeMetaObject_1(seng, this))
{
}

void ModelObject_1::setValue(const QByteArray &name, const QVariant &val)
{
    m_meta->setValue(name, val);
    //setProperty(name.constData(), val);
}

void ModelObject_1::setNodeUpdatesEnabled(bool enable)
{
    m_meta->m_enabled = enable;
}


ModelNodeMetaObject_1::ModelNodeMetaObject_1(QScriptEngine *seng, ModelObject_1 *object)
    : QDeclarative1OpenMetaObject(object),
      m_enabled(false),
      m_seng(seng),
      m_obj(object)
{
}

void ModelNodeMetaObject_1::propertyWritten(int index)
{
    if (!m_enabled)
        return;

    QString propName = QString::fromUtf8(name(index));
    QVariant value = operator[](index);

    QScriptValue sv = m_seng->newObject();
    sv.setProperty(propName, m_seng->newVariant(value));
    bool changed = m_obj->m_node->setObjectValue(sv, false);
    if (changed)
        m_obj->m_node->changedProperty(propName);
}




QT_END_NAMESPACE
