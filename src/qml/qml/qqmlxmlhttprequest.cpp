/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmlxmlhttprequest_p.h"

#include <private/qv8engine_p.h>

#include "qqmlengine.h"
#include "qqmlengine_p.h"
#include <private/qqmlrefcount_p.h>
#include "qqmlengine_p.h"
#include "qqmlexpression_p.h"
#include "qqmlglobal_p.h"
#include <private/qv4domerrors_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qv4scopedvalue_p.h>

#include <QtCore/qobject.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qjsengine.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtCore/qtextcodec.h>
#include <QtCore/qxmlstream.h>
#include <QtCore/qstack.h>
#include <QtCore/qdebug.h>

#include <private/qv4objectproto_p.h>

using namespace QV4;

#ifndef QT_NO_XMLSTREAMREADER

#define V4THROW_REFERENCE(string) \
    ctx->throwError(Value::fromObject(ctx->engine->newReferenceErrorObject(QStringLiteral(string))))

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(xhrDump, QML_XHR_DUMP);

struct QQmlXMLHttpRequestData {
    QQmlXMLHttpRequestData();
    ~QQmlXMLHttpRequestData();

    PersistentValue nodeFunction;

    PersistentValue nodePrototype;
    PersistentValue elementPrototype;
    PersistentValue attrPrototype;
    PersistentValue characterDataPrototype;
    PersistentValue textPrototype;
    PersistentValue cdataPrototype;
    PersistentValue documentPrototype;
};

static inline QQmlXMLHttpRequestData *xhrdata(QV8Engine *engine)
{
    return (QQmlXMLHttpRequestData *)engine->xmlHttpRequestData();
}

static Value constructMeObject(const Value &thisObj, QV8Engine *e)
{
    ExecutionEngine *v4 = QV8Engine::getV4(e);
    Object *meObj = v4->newObject();
    meObj->put(v4->newString(QStringLiteral("ThisObject")), thisObj);
    meObj->put(v4->newString(QStringLiteral("ActivationObject")), QmlContextWrapper::qmlScope(e, e->callingContext(), 0));
    return Value::fromObject(meObj);
}

QQmlXMLHttpRequestData::QQmlXMLHttpRequestData()
{
}

QQmlXMLHttpRequestData::~QQmlXMLHttpRequestData()
{
}

namespace {

class DocumentImpl;
class NodeImpl 
{
public:
    NodeImpl() : type(Element), document(0), parent(0) {}
    virtual ~NodeImpl() { 
        for (int ii = 0; ii < children.count(); ++ii)
            delete children.at(ii);
        for (int ii = 0; ii < attributes.count(); ++ii)
            delete attributes.at(ii);
    }

    // These numbers are copied from the Node IDL definition
    enum Type { 
        Attr = 2, 
        CDATA = 4, 
        Comment = 8, 
        Document = 9, 
        DocumentFragment = 11, 
        DocumentType = 10,
        Element = 1, 
        Entity = 6, 
        EntityReference = 5,
        Notation = 12, 
        ProcessingInstruction = 7, 
        Text = 3
    };
    Type type;

    QString namespaceUri;
    QString name;

    QString data;

    void addref();
    void release();

    DocumentImpl *document;
    NodeImpl *parent;

    QList<NodeImpl *> children;
    QList<NodeImpl *> attributes;
};

class DocumentImpl : public QQmlRefCount, public NodeImpl
{
public:
    DocumentImpl() : root(0) { type = Document; }
    virtual ~DocumentImpl() {
        if (root) delete root;
    }

    QString version;
    QString encoding;
    bool isStandalone;

    NodeImpl *root;

    void addref() { QQmlRefCount::addref(); }
    void release() { QQmlRefCount::release(); }
};

class NamedNodeMap : public Object
{
    Q_MANAGED
public:
    NamedNodeMap(ExecutionEngine *engine, NodeImpl *data, const QList<NodeImpl *> &list)
        : Object(engine)
        , d(data)
        , list(list)
    {
        vtbl = &static_vtbl;

        if (d)
            d->addref();
    }
    ~NamedNodeMap() {
        if (d)
            d->release();
    }

    // C++ API
    static Value create(QV8Engine *, NodeImpl *, const QList<NodeImpl *> &);

    // JS API
    static void destroy(Managed *that) {
        that->as<NamedNodeMap>()->~NamedNodeMap();
    }
    static Value get(Managed *m, String *name, bool *hasProperty);
    static Value getIndexed(Managed *m, uint index, bool *hasProperty);

    QList<NodeImpl *> list; // Only used in NamedNodeMap
    NodeImpl *d;
};

DEFINE_MANAGED_VTABLE(NamedNodeMap);

class NodeList : public Object
{
    Q_MANAGED
public:
    NodeList(ExecutionEngine *engine, NodeImpl *data)
        : Object(engine)
        , d(data)
    {
        vtbl = &static_vtbl;

        if (d)
            d->addref();
    }
    ~NodeList() {
        if (d)
            d->release();
    }

    // JS API
    static void destroy(Managed *that) {
        that->as<NodeList>()->~NodeList();
    }
    static Value get(Managed *m, String *name, bool *hasProperty);
    static Value getIndexed(Managed *m, uint index, bool *hasProperty);

    // C++ API
    static Value create(QV8Engine *, NodeImpl *);

    NodeImpl *d;
};

DEFINE_MANAGED_VTABLE(NodeList);

class NodePrototype : public Object
{
    Q_MANAGED
public:
    NodePrototype(ExecutionEngine *engine)
        : Object(engine)
    {
        vtbl = &static_vtbl;
        defineAccessorProperty(engine, QStringLiteral("nodeName"), method_get_nodeName, 0);
        defineAccessorProperty(engine, QStringLiteral("nodeValue"), method_get_nodeValue, 0);
        defineAccessorProperty(engine, QStringLiteral("nodeType"), method_get_nodeType, 0);

        defineAccessorProperty(engine, QStringLiteral("parentNode"), method_get_parentNode, 0);
        defineAccessorProperty(engine, QStringLiteral("childNodes"), method_get_childNodes, 0);
        defineAccessorProperty(engine, QStringLiteral("firstChild"), method_get_firstChild, 0);
        defineAccessorProperty(engine, QStringLiteral("lastChild"), method_get_lastChild, 0);
        defineAccessorProperty(engine, QStringLiteral("previousSibling"), method_get_previousSibling, 0);
        defineAccessorProperty(engine, QStringLiteral("nextSibling"), method_get_nextSibling, 0);
        defineAccessorProperty(engine, QStringLiteral("attributes"), method_get_attributes, 0);
    }

    static void initClass(ExecutionEngine *engine);

    // JS API
    static Value method_get_nodeName(SimpleCallContext *ctx);
    static Value method_get_nodeValue(SimpleCallContext *ctx);
    static Value method_get_nodeType(SimpleCallContext *ctx);

    static Value method_get_parentNode(SimpleCallContext *ctx);
    static Value method_get_childNodes(SimpleCallContext *ctx);
    static Value method_get_firstChild(SimpleCallContext *ctx);
    static Value method_get_lastChild(SimpleCallContext *ctx);
    static Value method_get_previousSibling(SimpleCallContext *ctx);
    static Value method_get_nextSibling(SimpleCallContext *ctx);
    static Value method_get_attributes(SimpleCallContext *ctx);

    //static Value ownerDocument(SimpleCallContext *ctx);
    //static Value namespaceURI(SimpleCallContext *ctx);
    //static Value prefix(SimpleCallContext *ctx);
    //static Value localName(SimpleCallContext *ctx);
    //static Value baseURI(SimpleCallContext *ctx);
    //static Value textContent(SimpleCallContext *ctx);

    static Value getProto(ExecutionEngine *v4);

};

DEFINE_MANAGED_VTABLE(NodePrototype);

class Node : public Object
{
    Q_MANAGED

    Node(ExecutionEngine *engine, NodeImpl *data)
        : Object(engine)
        , d(data)
    {
        vtbl = &static_vtbl;

        if (d)
            d->addref();
    }
    ~Node() {
        if (d)
            d->release();
    }

    // JS API
    static void destroy(Managed *that) {
        that->as<Node>()->~Node();
    }

    // C++ API
    static Value create(QV8Engine *, NodeImpl *);

    Node(const Node &o);
    bool isNull() const;

    NodeImpl *d;

private:
    Node &operator=(const Node &);
};

DEFINE_MANAGED_VTABLE(Node);

class Element : public Node
{
public:
    // C++ API
    static Value prototype(ExecutionEngine *);
};

class Attr : public Node
{
public:
    // JS API
    static Value name(SimpleCallContext *ctx);
//    static Value specified(SimpleCallContext *);
    static Value value(SimpleCallContext *ctx);
    static Value ownerElement(SimpleCallContext *ctx);
//    static Value schemaTypeInfo(SimpleCallContext *);
//    static Value isId(SimpleCallContext *c);

    // C++ API
    static Value prototype(ExecutionEngine *);
};

class CharacterData : public Node
{
public:
    // JS API
    static Value length(SimpleCallContext *ctx);

    // C++ API
    static Value prototype(ExecutionEngine *v4);
};

class Text : public CharacterData
{
public:
    // JS API
    static Value isElementContentWhitespace(SimpleCallContext *ctx);
    static Value wholeText(SimpleCallContext *ctx);

    // C++ API
    static Value prototype(ExecutionEngine *);
};

class CDATA : public Text
{
public:
    // C++ API
    static Value prototype(ExecutionEngine *v4);
};

class Document : public Node
{
public:
    // JS API
    static Value xmlVersion(SimpleCallContext *ctx);
    static Value xmlEncoding(SimpleCallContext *ctx);
    static Value xmlStandalone(SimpleCallContext *ctx);
    static Value documentElement(SimpleCallContext *ctx);

    // C++ API
    static Value prototype(ExecutionEngine *);
    static Value load(QV8Engine *engine, const QByteArray &data);
};

}

void NodeImpl::addref() 
{
    document->addref();
}

void NodeImpl::release()
{
    document->release();
}

Value NodePrototype::method_get_nodeName(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    QString name;
    switch (r->d->type) {
    case NodeImpl::Document:
        name = QStringLiteral("#document");
        break;
    case NodeImpl::CDATA:
        name = QStringLiteral("#cdata-section");
        break;
    case NodeImpl::Text:
        name = QStringLiteral("#text");
        break;
    default:
        name = r->d->name;
        break;
    }
    return Value::fromString(ctx->engine->newString(name));
}

Value NodePrototype::method_get_nodeValue(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    if (r->d->type == NodeImpl::Document ||
        r->d->type == NodeImpl::DocumentFragment ||
        r->d->type == NodeImpl::DocumentType ||
        r->d->type == NodeImpl::Element ||
        r->d->type == NodeImpl::Entity ||
        r->d->type == NodeImpl::EntityReference ||
        r->d->type == NodeImpl::Notation)
        return Value::nullValue();

    return Value::fromString(ctx->engine->newString(r->d->data));
}

Value NodePrototype::method_get_nodeType(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    return Value::fromInt32(r->d->type);
}

Value NodePrototype::method_get_parentNode(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    QV8Engine *engine = ctx->engine->v8Engine;

    if (r->d->parent) return Node::create(engine, r->d->parent);
    else return Value::nullValue();
}

Value NodePrototype::method_get_childNodes(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    QV8Engine *engine = ctx->engine->v8Engine;

    return NodeList::create(engine, r->d);
}

Value NodePrototype::method_get_firstChild(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    QV8Engine *engine = ctx->engine->v8Engine;

    if (r->d->children.isEmpty()) return Value::nullValue();
    else return Node::create(engine, r->d->children.first());
}

Value NodePrototype::method_get_lastChild(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    QV8Engine *engine = ctx->engine->v8Engine;

    if (r->d->children.isEmpty()) return Value::nullValue();
    else return Node::create(engine, r->d->children.last());
}

Value NodePrototype::method_get_previousSibling(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    QV8Engine *engine = ctx->engine->v8Engine;

    if (!r->d->parent) return Value::nullValue();

    for (int ii = 0; ii < r->d->parent->children.count(); ++ii) {
        if (r->d->parent->children.at(ii) == r->d) {
            if (ii == 0) return Value::nullValue();
            else return Node::create(engine, r->d->parent->children.at(ii - 1));
        }
    }

    return Value::nullValue();
}

Value NodePrototype::method_get_nextSibling(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    QV8Engine *engine = ctx->engine->v8Engine;

    if (!r->d->parent) return Value::nullValue();

    for (int ii = 0; ii < r->d->parent->children.count(); ++ii) {
        if (r->d->parent->children.at(ii) == r->d) {
            if ((ii + 1) == r->d->parent->children.count()) return Value::nullValue();
            else return Node::create(engine, r->d->parent->children.at(ii + 1)); 
        }
    }

    return Value::nullValue();
}

Value NodePrototype::method_get_attributes(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r)
        ctx->throwTypeError();

    QV8Engine *engine = ctx->engine->v8Engine;

    if (r->d->type != NodeImpl::Element)
        return Value::nullValue();
    else
        return NamedNodeMap::create(engine, r->d, r->d->attributes);
}

Value NodePrototype::getProto(ExecutionEngine *v4)
{
    QQmlXMLHttpRequestData *d = xhrdata(v4->v8Engine);
    if (d->nodePrototype.isEmpty()) {
        Object *p = new (v4->memoryManager) NodePrototype(v4);
        d->nodePrototype = Value::fromObject(p);
        v4->v8Engine->freezeObject(d->nodePrototype.value());
    }
    return d->nodePrototype.value();
}

Value Node::create(QV8Engine *engine, NodeImpl *data)
{
    ExecutionEngine *v4 = QV8Engine::getV4(engine);

    QQmlXMLHttpRequestData *d = xhrdata(engine);
    Node *instance = new (v4->memoryManager) Node(v4, data);

    switch (data->type) {
    case NodeImpl::Attr:
        instance->setPrototype(Attr::prototype(v4).asObject());
        break;
    case NodeImpl::Comment:
    case NodeImpl::Document:
    case NodeImpl::DocumentFragment:
    case NodeImpl::DocumentType:
    case NodeImpl::Entity:
    case NodeImpl::EntityReference:
    case NodeImpl::Notation:
    case NodeImpl::ProcessingInstruction:
        return Value::undefinedValue();
    case NodeImpl::CDATA:
        instance->setPrototype(CDATA::prototype(v4).asObject());
        break;
    case NodeImpl::Text:
        instance->setPrototype(Text::prototype(v4).asObject());
        break;
    case NodeImpl::Element:
        instance->setPrototype(Element::prototype(v4).asObject());
        break;
    }

    return Value::fromObject(instance);
}

Value Element::prototype(ExecutionEngine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine->v8Engine);
    if (d->elementPrototype.isEmpty()) {
        Object *p = engine->newObject();
        p->setPrototype(NodePrototype::getProto(engine).asObject());
        p->defineAccessorProperty(engine, QStringLiteral("tagName"), NodePrototype::method_get_nodeName, 0);
        d->elementPrototype = Value::fromObject(p);
        engine->v8Engine->freezeObject(d->elementPrototype.value());
    }
    return d->elementPrototype.value();
}

Value Attr::prototype(ExecutionEngine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine->v8Engine);
    if (d->attrPrototype.isEmpty()) {
        Object *p = engine->newObject();
        p->setPrototype(NodePrototype::getProto(engine).asObject());
        p->defineAccessorProperty(engine, QStringLiteral("name"), name, 0);
        p->defineAccessorProperty(engine, QStringLiteral("value"), value, 0);
        p->defineAccessorProperty(engine, QStringLiteral("ownerElement"), ownerElement, 0);
        d->attrPrototype = Value::fromObject(p);
        engine->v8Engine->freezeObject(d->attrPrototype.value());
    }
    return d->attrPrototype.value();
}

Value Attr::name(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r) return Value::undefinedValue();
    QV8Engine *engine = ctx->engine->v8Engine;

    return engine->toString(r->d->name);
}

Value Attr::value(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r) return Value::undefinedValue();
    QV8Engine *engine = ctx->engine->v8Engine;

    return engine->toString(r->d->data);
}

Value Attr::ownerElement(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r) return Value::undefinedValue();
    QV8Engine *engine = ctx->engine->v8Engine;

    return Node::create(engine, r->d->parent);
}

Value CharacterData::length(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r) return Value::undefinedValue();
    QV8Engine *engine = ctx->engine->v8Engine;
    Q_UNUSED(engine)
    return Value::fromInt32(r->d->data.length());
}

Value CharacterData::prototype(ExecutionEngine *v4)
{
    QQmlXMLHttpRequestData *d = xhrdata(v4->v8Engine);
    if (d->characterDataPrototype.isEmpty()) {
        Object *p = v4->newObject();
        p->setPrototype(NodePrototype::getProto(v4).asObject());
        p->defineAccessorProperty(v4, QStringLiteral("data"), NodePrototype::method_get_nodeValue, 0);
        p->defineAccessorProperty(v4, QStringLiteral("length"), length, 0);
        d->characterDataPrototype = Value::fromObject(p);
        v4->v8Engine->freezeObject(d->characterDataPrototype);
    }
    return d->characterDataPrototype.value();
}

Value Text::isElementContentWhitespace(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r) return Value::undefinedValue();

    return Value::fromBoolean(r->d->data.trimmed().isEmpty());
}

Value Text::wholeText(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r) return Value::undefinedValue();
    QV8Engine *engine = ctx->engine->v8Engine;

    return engine->toString(r->d->data);
}

Value Text::prototype(ExecutionEngine *v4)
{
    QQmlXMLHttpRequestData *d = xhrdata(v4->v8Engine);
    if (d->textPrototype.isEmpty()) {
        Object *p = v4->newObject();
        p->setPrototype(CharacterData::prototype(v4).asObject());
        p->defineAccessorProperty(v4, QStringLiteral("isElementContentWhitespace"), isElementContentWhitespace, 0);
        p->defineAccessorProperty(v4, QStringLiteral("wholeText"), wholeText, 0);
        d->textPrototype = Value::fromObject(p);
        v4->v8Engine->freezeObject(d->textPrototype);
    }
    return d->textPrototype.value();
}

Value CDATA::prototype(ExecutionEngine *v4)
{
    // ### why not just use TextProto???
    QQmlXMLHttpRequestData *d = xhrdata(v4->v8Engine);
    if (d->cdataPrototype.isEmpty()) {
        Object *p = v4->newObject();
        p->setPrototype(Text::prototype(v4).asObject());
        d->cdataPrototype = Value::fromObject(p);
        v4->v8Engine->freezeObject(d->cdataPrototype);
    }
    return d->cdataPrototype.value();
}

Value Document::prototype(ExecutionEngine *v4)
{
    QQmlXMLHttpRequestData *d = xhrdata(v4->v8Engine);
    if (d->documentPrototype.isEmpty()) {
        Object *p = v4->newObject();
        p->setPrototype(NodePrototype::getProto(v4).asObject());
        p->defineAccessorProperty(v4, QStringLiteral("xmlVersion"), xmlVersion, 0);
        p->defineAccessorProperty(v4, QStringLiteral("xmlEncoding"), xmlEncoding, 0);
        p->defineAccessorProperty(v4, QStringLiteral("xmlStandalone"), xmlStandalone, 0);
        p->defineAccessorProperty(v4, QStringLiteral("documentElement"), documentElement, 0);
        d->documentPrototype = Value::fromObject(p);
        v4->v8Engine->freezeObject(d->documentPrototype);
    }
    return d->documentPrototype.value();
}

Value Document::load(QV8Engine *engine, const QByteArray &data)
{
    Q_ASSERT(engine);
    ExecutionEngine *v4 = QV8Engine::getV4(engine);

    DocumentImpl *document = 0;
    QStack<NodeImpl *> nodeStack;

    QXmlStreamReader reader(data);

    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::NoToken:
            break;
        case QXmlStreamReader::Invalid:
            break;
        case QXmlStreamReader::StartDocument:
            Q_ASSERT(!document);
            document = new DocumentImpl;
            document->document = document;
            document->version = reader.documentVersion().toString();
            document->encoding = reader.documentEncoding().toString();
            document->isStandalone = reader.isStandaloneDocument();
            break;
        case QXmlStreamReader::EndDocument:
            break;
        case QXmlStreamReader::StartElement: 
        {
            Q_ASSERT(document);
            NodeImpl *node = new NodeImpl;
            node->document = document;
            node->namespaceUri = reader.namespaceUri().toString();
            node->name = reader.name().toString();
            if (nodeStack.isEmpty()) {
                document->root = node;
            } else {
                node->parent = nodeStack.top();
                node->parent->children.append(node);
            }
            nodeStack.append(node);

            foreach (const QXmlStreamAttribute &a, reader.attributes()) {
                NodeImpl *attr = new NodeImpl;
                attr->document = document;
                attr->type = NodeImpl::Attr;
                attr->namespaceUri = a.namespaceUri().toString();
                attr->name = a.name().toString();
                attr->data = a.value().toString();
                attr->parent = node;
                node->attributes.append(attr);
            }
        } 
            break;
        case QXmlStreamReader::EndElement:
            nodeStack.pop();
            break;
        case QXmlStreamReader::Characters:
        {
            NodeImpl *node = new NodeImpl;
            node->document = document;
            node->type = reader.isCDATA()?NodeImpl::CDATA:NodeImpl::Text;
            node->parent = nodeStack.top();
            node->parent->children.append(node);
            node->data = reader.text().toString();
        }
            break;
        case QXmlStreamReader::Comment:
            break;
        case QXmlStreamReader::DTD:
            break;
        case QXmlStreamReader::EntityReference:
            break;
        case QXmlStreamReader::ProcessingInstruction:
            break;
        }
    }

    if (!document || reader.hasError()) {
        if (document)
            document->release();
        return Value::nullValue();
    }

    Object *instance = new (v4->memoryManager) Node(v4, document);
    instance->setPrototype(Document::prototype(v4).asObject());
    return Value::fromObject(instance);
}

Node::Node(const Node &o)
    : Object(o.engine()), d(o.d)
{
    if (d)
        d->addref();
}

bool Node::isNull() const
{
    return d == 0;
}

Value NamedNodeMap::getIndexed(Managed *m, uint index, bool *hasProperty)
{
    QV4::ExecutionEngine *v4 = m->engine();
    NamedNodeMap *r = m->as<NamedNodeMap>();
    if (!r)
        v4->current->throwTypeError();

    QV8Engine *engine = v4->v8Engine;

    if ((int)index < r->list.count()) {
        if (hasProperty)
            *hasProperty = true;
        return Node::create(engine, r->list.at(index));
    }
    if (hasProperty)
        *hasProperty = false;
    return Value::undefinedValue();
}

Value NamedNodeMap::get(Managed *m, String *name, bool *hasProperty)
{
    NamedNodeMap *r = m->as<NamedNodeMap>();
    QV4::ExecutionEngine *v4 = m->engine();
    if (!r)
        v4->current->throwTypeError();

    name->makeIdentifier();
    if (name->isEqualTo(v4->id_length))
        return Value::fromInt32(r->list.count());

    QV8Engine *engine = v4->v8Engine;

    QString str = name->toQString();
    for (int ii = 0; ii < r->list.count(); ++ii) {
        if (r->list.at(ii)->name == str) {
            if (hasProperty)
                *hasProperty = true;
            return Node::create(engine, r->list.at(ii));
        }
    }

    if (hasProperty)
        *hasProperty = false;
    return Value::undefinedValue();
}

Value NamedNodeMap::create(QV8Engine *engine, NodeImpl *data, const QList<NodeImpl *> &list)
{
    ExecutionEngine *v4 = QV8Engine::getV4(engine);

    NamedNodeMap *instance = new (v4->memoryManager) NamedNodeMap(v4, data, list);
    return Value::fromObject(instance);
}

Value NodeList::getIndexed(Managed *m, uint index, bool *hasProperty)
{
    QV4::ExecutionEngine *v4 = m->engine();
    NodeList *r = m->as<NodeList>();
    if (!r)
        v4->current->throwTypeError();

    QV8Engine *engine = v4->v8Engine;

    if ((int)index < r->d->children.count()) {
        if (hasProperty)
            *hasProperty = true;
        return Node::create(engine, r->d->children.at(index));
    }
    if (hasProperty)
        *hasProperty = false;
    return Value::undefinedValue();
}

Value NodeList::get(Managed *m, String *name, bool *hasProperty)
{
    QV4::ExecutionEngine *v4 = m->engine();
    NodeList *r = m->as<NodeList>();
    if (!r)
        v4->current->throwTypeError();

    name->makeIdentifier();

    if (name->isEqualTo(v4->id_length))
        return Value::fromInt32(r->d->children.count());
    return Object::get(m, name, hasProperty);
}

Value NodeList::create(QV8Engine *engine, NodeImpl *data)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    ExecutionEngine *v4 = QV8Engine::getV4(engine);
    NodeList *instance = new (v4->memoryManager) NodeList(v4, data);
    return Value::fromObject(instance);
}

Value Document::documentElement(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r || r->d->type != NodeImpl::Document) return Value::undefinedValue();
    QV8Engine *engine = ctx->engine->v8Engine;

    return Node::create(engine, static_cast<DocumentImpl *>(r->d)->root);
}

Value Document::xmlStandalone(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r || r->d->type != NodeImpl::Document) return Value::undefinedValue();
    QV8Engine *engine = ctx->engine->v8Engine;
    Q_UNUSED(engine)
    return Value::fromBoolean(static_cast<DocumentImpl *>(r->d)->isStandalone);
}

Value Document::xmlVersion(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r || r->d->type != NodeImpl::Document) return Value::undefinedValue();
    QV8Engine *engine = ctx->engine->v8Engine;

    return engine->toString(static_cast<DocumentImpl *>(r->d)->version);
}

Value Document::xmlEncoding(SimpleCallContext *ctx)
{
    Node *r = ctx->thisObject.as<Node>();
    if (!r || r->d->type != NodeImpl::Document) return Value::undefinedValue();
    QV8Engine *engine = ctx->engine->v8Engine;

    return engine->toString(static_cast<DocumentImpl *>(r->d)->encoding);
}

class QQmlXMLHttpRequest : public QObject
{
    Q_OBJECT
public:
    enum State { Unsent = 0, 
                 Opened = 1, HeadersReceived = 2,
                 Loading = 3, Done = 4 };

    QQmlXMLHttpRequest(QV8Engine *engine, QNetworkAccessManager *manager);
    virtual ~QQmlXMLHttpRequest();

    bool sendFlag() const;
    bool errorFlag() const;
    quint32 readyState() const;
    int replyStatus() const;
    QString replyStatusText() const;

    Value open(const Value &me, const QString &, const QUrl &);
    Value send(const Value &me, const QByteArray &);
    Value abort(const Value &me);

    void addHeader(const QString &, const QString &);
    QString header(const QString &name);
    QString headers();


    QString responseBody();
    const QByteArray & rawResponseBody() const;
    bool receivedXml() const;
private slots:
    void readyRead();
    void error(QNetworkReply::NetworkError);
    void finished();

private:
    void requestFromUrl(const QUrl &url);

    ExecutionEngine *v4;
    State m_state;
    bool m_errorFlag;
    bool m_sendFlag;
    QString m_method;
    QUrl m_url;
    QByteArray m_responseEntityBody;
    QByteArray m_data;
    int m_redirectCount;

    typedef QPair<QByteArray, QByteArray> HeaderPair;
    typedef QList<HeaderPair> HeadersList;
    HeadersList m_headersList;
    void fillHeadersList();

    bool m_gotXml;
    QByteArray m_mime;
    QByteArray m_charset;
    QTextCodec *m_textCodec;
#ifndef QT_NO_TEXTCODEC
    QTextCodec* findTextCodec() const;
#endif
    void readEncoding();

    Value getMe() const;
    void setMe(const Value &me);
    PersistentValue m_me;

    void dispatchCallback(const Value &me);
    void printError(const Exception &e);

    int m_status;
    QString m_statusText;
    QNetworkRequest m_request;
    QStringList m_addedHeaders;
    QPointer<QNetworkReply> m_network;
    void destroyNetwork();

    QNetworkAccessManager *m_nam;
    QNetworkAccessManager *networkAccessManager() { return m_nam; }
};

QQmlXMLHttpRequest::QQmlXMLHttpRequest(QV8Engine *engine, QNetworkAccessManager *manager)
    : v4(QV8Engine::getV4(engine))
    , m_state(Unsent), m_errorFlag(false), m_sendFlag(false)
    , m_redirectCount(0), m_gotXml(false), m_textCodec(0), m_network(0), m_nam(manager)
{
}

QQmlXMLHttpRequest::~QQmlXMLHttpRequest()
{
    destroyNetwork();
}

bool QQmlXMLHttpRequest::sendFlag() const
{
    return m_sendFlag;
}

bool QQmlXMLHttpRequest::errorFlag() const
{
    return m_errorFlag;
}

quint32 QQmlXMLHttpRequest::readyState() const
{
    return m_state;
}

int QQmlXMLHttpRequest::replyStatus() const
{
    return m_status;
}

QString QQmlXMLHttpRequest::replyStatusText() const
{
    return m_statusText;
}

Value QQmlXMLHttpRequest::open(const Value &me, const QString &method,
                                                       const QUrl &url)
{
    destroyNetwork();
    m_sendFlag = false;
    m_errorFlag = false;
    m_responseEntityBody = QByteArray();
    m_method = method;
    m_url = url;
    m_state = Opened;
    m_addedHeaders.clear();
    dispatchCallback(me);
    return Value::undefinedValue();
}

void QQmlXMLHttpRequest::addHeader(const QString &name, const QString &value)
{
    QByteArray utfname = name.toUtf8();

    if (m_addedHeaders.contains(name, Qt::CaseInsensitive)) {
        m_request.setRawHeader(utfname, m_request.rawHeader(utfname) + ',' + value.toUtf8());
    } else {
        m_request.setRawHeader(utfname, value.toUtf8());
        m_addedHeaders.append(name);
    }
}

QString QQmlXMLHttpRequest::header(const QString &name)
{
    QByteArray utfname = name.toLower().toUtf8();

    foreach (const HeaderPair &header, m_headersList) {
        if (header.first == utfname)
            return QString::fromUtf8(header.second);
    }
    return QString();
}

QString QQmlXMLHttpRequest::headers()
{
    QString ret;

    foreach (const HeaderPair &header, m_headersList) {
        if (ret.length())
            ret.append(QLatin1String("\r\n"));
        ret = ret % QString::fromUtf8(header.first) % QLatin1String(": ")
                % QString::fromUtf8(header.second);
    }
    return ret;
}

void QQmlXMLHttpRequest::fillHeadersList()
{
    QList<QByteArray> headerList = m_network->rawHeaderList();

    m_headersList.clear();
    foreach (const QByteArray &header, headerList) {
        HeaderPair pair (header.toLower(), m_network->rawHeader(header));
        if (pair.first == "set-cookie" ||
            pair.first == "set-cookie2")
            continue;

        m_headersList << pair;
    }
}

void QQmlXMLHttpRequest::requestFromUrl(const QUrl &url)
{
    QNetworkRequest request = m_request;
    request.setUrl(url);
    if(m_method == QLatin1String("POST") ||
       m_method == QLatin1String("PUT")) {
        QVariant var = request.header(QNetworkRequest::ContentTypeHeader);
        if (var.isValid()) {
            QString str = var.toString();
            int charsetIdx = str.indexOf(QLatin1String("charset="));
            if (charsetIdx == -1) {
                // No charset - append
                if (!str.isEmpty()) str.append(QLatin1Char(';'));
                str.append(QLatin1String("charset=UTF-8"));
            } else {
                charsetIdx += 8;
                int n = 0;
                int semiColon = str.indexOf(QLatin1Char(';'), charsetIdx);
                if (semiColon == -1) {
                    n = str.length() - charsetIdx;
                } else {
                    n = semiColon - charsetIdx;
                }

                str.replace(charsetIdx, n, QLatin1String("UTF-8"));
            }
            request.setHeader(QNetworkRequest::ContentTypeHeader, str);
        } else {
            request.setHeader(QNetworkRequest::ContentTypeHeader, 
                              QLatin1String("text/plain;charset=UTF-8"));
        }
    }

    if (xhrDump()) {
        qWarning().nospace() << "XMLHttpRequest: " << qPrintable(m_method) << ' ' << qPrintable(url.toString());
        if (!m_data.isEmpty()) {
            qWarning().nospace() << "                " 
                                 << qPrintable(QString::fromUtf8(m_data));
        }
    }

    if (m_method == QLatin1String("GET"))
        m_network = networkAccessManager()->get(request);
    else if (m_method == QLatin1String("HEAD"))
        m_network = networkAccessManager()->head(request);
    else if (m_method == QLatin1String("POST"))
        m_network = networkAccessManager()->post(request, m_data);
    else if (m_method == QLatin1String("PUT"))
        m_network = networkAccessManager()->put(request, m_data);
    else if (m_method == QLatin1String("DELETE"))
        m_network = networkAccessManager()->deleteResource(request);

    QObject::connect(m_network, SIGNAL(readyRead()),
                     this, SLOT(readyRead()));
    QObject::connect(m_network, SIGNAL(error(QNetworkReply::NetworkError)),
                     this, SLOT(error(QNetworkReply::NetworkError)));
    QObject::connect(m_network, SIGNAL(finished()),
                     this, SLOT(finished()));
}

Value QQmlXMLHttpRequest::send(const Value &me, const QByteArray &data)
{
    m_errorFlag = false;
    m_sendFlag = true;
    m_redirectCount = 0;
    m_data = data;

    setMe(me);

    requestFromUrl(m_url);

    return Value::undefinedValue();
}

Value QQmlXMLHttpRequest::abort(const Value &me)
{
    destroyNetwork();
    m_responseEntityBody = QByteArray();
    m_errorFlag = true;
    m_request = QNetworkRequest();

    if (!(m_state == Unsent || 
          (m_state == Opened && !m_sendFlag) ||
          m_state == Done)) {

        m_state = Done;
        m_sendFlag = false;
        dispatchCallback(me);
    }

    m_state = Unsent;

    return Value::undefinedValue();
}

Value QQmlXMLHttpRequest::getMe() const
{
    return m_me.value();
}

void QQmlXMLHttpRequest::setMe(const Value &me)
{
    m_me = me;
}

void QQmlXMLHttpRequest::readyRead()
{
    m_status = 
        m_network->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    m_statusText =
        QString::fromUtf8(m_network->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray());

    // ### We assume if this is called the headers are now available
    if (m_state < HeadersReceived) {
        m_state = HeadersReceived;
        fillHeadersList ();
        dispatchCallback(m_me.value());
    }

    bool wasEmpty = m_responseEntityBody.isEmpty();
    m_responseEntityBody.append(m_network->readAll());
    if (wasEmpty && !m_responseEntityBody.isEmpty())
        m_state = Loading;
    dispatchCallback(m_me.value());
}

static const char *errorToString(QNetworkReply::NetworkError error)
{
    int idx = QNetworkReply::staticMetaObject.indexOfEnumerator("NetworkError");
    if (idx == -1) return "EnumLookupFailed";

    QMetaEnum e = QNetworkReply::staticMetaObject.enumerator(idx);

    const char *name = e.valueToKey(error);
    if (!name) return "EnumLookupFailed";
    else return name;
}

void QQmlXMLHttpRequest::error(QNetworkReply::NetworkError error)
{
    m_status =
        m_network->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    m_statusText =
        QString::fromUtf8(m_network->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray());

    m_request = QNetworkRequest();
    m_data.clear();
    destroyNetwork();

    if (xhrDump()) {
        qWarning().nospace() << "XMLHttpRequest: ERROR " << qPrintable(m_url.toString());
        qWarning().nospace() << "    " << error << ' ' << errorToString(error) << ' ' << m_statusText;
    }

    if (error == QNetworkReply::ContentAccessDenied ||
        error == QNetworkReply::ContentOperationNotPermittedError ||
        error == QNetworkReply::ContentNotFoundError ||
        error == QNetworkReply::AuthenticationRequiredError ||
        error == QNetworkReply::ContentReSendError ||
        error == QNetworkReply::UnknownContentError) {
        m_state = Loading;
        dispatchCallback(m_me.value());
    } else {
        m_errorFlag = true;
        m_responseEntityBody = QByteArray();
    } 

    m_state = Done;

    dispatchCallback(m_me.value());
}

#define XMLHTTPREQUEST_MAXIMUM_REDIRECT_RECURSION 15
void QQmlXMLHttpRequest::finished()
{
    m_redirectCount++;
    if (m_redirectCount < XMLHTTPREQUEST_MAXIMUM_REDIRECT_RECURSION) {
        QVariant redirect = m_network->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = m_network->url().resolved(redirect.toUrl());
            if (url.scheme() != QLatin1String("file")) {
                // See http://www.ietf.org/rfc/rfc2616.txt, section 10.3.4 "303 See Other":
                // Result of 303 redirection should be a new "GET" request.
                const QVariant code = m_network->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                if (code.isValid() && code.toInt() == 303 && m_method != QLatin1String("GET"))
                    m_method = QStringLiteral("GET");
                destroyNetwork();
                requestFromUrl(url);
                return;
            }
        }
    }

    m_status =
        m_network->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    m_statusText =
        QString::fromUtf8(m_network->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray());

    if (m_state < HeadersReceived) {
        m_state = HeadersReceived;
        fillHeadersList ();
        dispatchCallback(m_me);
    }
    m_responseEntityBody.append(m_network->readAll());
    readEncoding();

    if (xhrDump()) {
        qWarning().nospace() << "XMLHttpRequest: RESPONSE " << qPrintable(m_url.toString());
        if (!m_responseEntityBody.isEmpty()) {
            qWarning().nospace() << "                " 
                                 << qPrintable(QString::fromUtf8(m_responseEntityBody));
        }
    }

    m_data.clear();
    destroyNetwork();
    if (m_state < Loading) {
        m_state = Loading;
        dispatchCallback(m_me);
    }
    m_state = Done;

    dispatchCallback(m_me);

    setMe(Value::emptyValue());
}


void QQmlXMLHttpRequest::readEncoding()
{
    foreach (const HeaderPair &header, m_headersList) {
        if (header.first == "content-type") {
            int separatorIdx = header.second.indexOf(';');
            if (separatorIdx == -1) {
                m_mime = header.second;
            } else {
                m_mime = header.second.mid(0, separatorIdx);
                int charsetIdx = header.second.indexOf("charset=");
                if (charsetIdx != -1) {
                    charsetIdx += 8;
                    separatorIdx = header.second.indexOf(';', charsetIdx);
                    m_charset = header.second.mid(charsetIdx, separatorIdx >= 0 ? separatorIdx : header.second.length());
                }
            }
            break;
        }
    }

    if (m_mime.isEmpty() || m_mime == "text/xml" || m_mime == "application/xml" || m_mime.endsWith("+xml")) 
        m_gotXml = true;
}

bool QQmlXMLHttpRequest::receivedXml() const
{
    return m_gotXml;
}


#ifndef QT_NO_TEXTCODEC
QTextCodec* QQmlXMLHttpRequest::findTextCodec() const
{
    QTextCodec *codec = 0;

    if (!m_charset.isEmpty()) 
        codec = QTextCodec::codecForName(m_charset);

    if (!codec && m_gotXml) {
        QXmlStreamReader reader(m_responseEntityBody);
        reader.readNext();
        codec = QTextCodec::codecForName(reader.documentEncoding().toString().toUtf8());
    }

    if (!codec && m_mime == "text/html") 
        codec = QTextCodec::codecForHtml(m_responseEntityBody, 0);

    if (!codec)
        codec = QTextCodec::codecForUtfText(m_responseEntityBody, 0);

    if (!codec)
        codec = QTextCodec::codecForName("UTF-8");
    return codec;
}
#endif


QString QQmlXMLHttpRequest::responseBody()
{
#ifndef QT_NO_TEXTCODEC
    if (!m_textCodec)
        m_textCodec = findTextCodec();
    if (m_textCodec)
        return m_textCodec->toUnicode(m_responseEntityBody);
#endif

    return QString::fromUtf8(m_responseEntityBody);
}

const QByteArray &QQmlXMLHttpRequest::rawResponseBody() const
{
    return m_responseEntityBody;
}

void QQmlXMLHttpRequest::dispatchCallback(const Value &me)
{
    ExecutionContext *ctx = v4->current;
    QV4::Scope scope(v4);
    try {
        Object *o = me.asObject();
        if (!o)
            ctx->throwError(QStringLiteral("QQmlXMLHttpRequest: internal error: empty ThisObject"));

        Object *thisObj = o->get(v4->newString(QStringLiteral("ThisObject"))).asObject();
        if (!thisObj)
            ctx->throwError(QStringLiteral("QQmlXMLHttpRequest: internal error: empty ThisObject"));

        FunctionObject *callback = thisObj->get(v4->newString(QStringLiteral("onreadystatechange"))).asFunctionObject();
        if (!callback) {
            // not an error, but no onreadystatechange function to call.
            return;
        }

        Value activationObject = o->get(v4->newString(QStringLiteral("ActivationObject")));
        if (!activationObject.asObject())
            v4->current->throwError(QStringLiteral("QQmlXMLHttpRequest: internal error: empty ActivationObject"));

        QQmlContextData *callingContext = QmlContextWrapper::getContext(activationObject);
        if (callingContext) {
            QV4::ScopedCallData callData(scope, 0);
            callData->thisObject = activationObject;
            callback->call(callData);
        }

        // if the callingContext object is no longer valid, then it has been
        // deleted explicitly (e.g., by a Loader deleting the itemContext when
        // the source is changed).  We do nothing in this case, as the evaluation
        // cannot succeed.
    } catch(Exception &e) {
        e.accept(ctx);
        printError(e);
    }
}

// Must have a handle scope
void QQmlXMLHttpRequest::printError(const Exception &e)
{
    QQmlError error;
    QQmlExpressionPrivate::exceptionToError(e, error);
    QQmlEnginePrivate::warning(QQmlEnginePrivate::get(v4->v8Engine->engine()), error);
}

void QQmlXMLHttpRequest::destroyNetwork()
{
    if (m_network) {
        m_network->disconnect();
        m_network->deleteLater();
        m_network = 0;
    }
}


struct QQmlXMLHttpRequestWrapper : public Object
{
    Q_MANAGED
    QQmlXMLHttpRequestWrapper(ExecutionEngine *engine, QQmlXMLHttpRequest *request)
        : Object(engine)
        , request(request)
    {
        vtbl = &static_vtbl;
    }
    ~QQmlXMLHttpRequestWrapper() {
        delete request;
    }

    static void destroy(Managed *that) {
        that->as<QQmlXMLHttpRequestWrapper>()->~QQmlXMLHttpRequestWrapper();
    }

    QQmlXMLHttpRequest *request;
};

DEFINE_MANAGED_VTABLE(QQmlXMLHttpRequestWrapper);

struct QQmlXMLHttpRequestCtor : public FunctionObject
{
    Q_MANAGED
    QQmlXMLHttpRequestCtor(ExecutionEngine *engine)
        : FunctionObject(engine->rootContext, engine->newString(QStringLiteral("XMLHttpRequest")))
    {
        vtbl = &static_vtbl;
        defineReadonlyProperty(engine, QStringLiteral("UNSENT"), Value::fromInt32(0));
        defineReadonlyProperty(engine, QStringLiteral("OPENED"), Value::fromInt32(1));
        defineReadonlyProperty(engine, QStringLiteral("HEADERS_RECEIVED"), Value::fromInt32(2));
        defineReadonlyProperty(engine, QStringLiteral("LOADING"), Value::fromInt32(3));
        defineReadonlyProperty(engine, QStringLiteral("DONE"), Value::fromInt32(4));
        if (!proto)
            setupProto();
        defineDefaultProperty(engine->id_prototype, Value::fromObject(proto));
    }
    ~QQmlXMLHttpRequestCtor()
    {}

    static void destroy(Managed *that) {
        that->as<QQmlXMLHttpRequestCtor>()->~QQmlXMLHttpRequestCtor();
    }
    static void markObjects(Managed *that) {
        QQmlXMLHttpRequestCtor *c = that->as<QQmlXMLHttpRequestCtor>();
        if (c->proto)
            c->proto->mark();
    }
    static Value construct(Managed *that, QV4::CallData *)
    {
        QQmlXMLHttpRequestCtor *ctor = that->as<QQmlXMLHttpRequestCtor>();
        if (!ctor)
            that->engine()->current->throwTypeError();

        QV8Engine *engine = that->engine()->v8Engine;
        QQmlXMLHttpRequest *r = new QQmlXMLHttpRequest(engine, engine->networkAccessManager());
        QQmlXMLHttpRequestWrapper *w = new (that->engine()->memoryManager) QQmlXMLHttpRequestWrapper(that->engine(), r);
        w->setPrototype(ctor->proto);
        return Value::fromObject(w);
    }

    static ReturnedValue call(Managed *, QV4::CallData *) {
        return Value::undefinedValue().asReturnedValue();
    }

    void setupProto();

    static Value method_open(SimpleCallContext *ctx);
    static Value method_setRequestHeader(SimpleCallContext *ctx);
    static Value method_send(SimpleCallContext *ctx);
    static Value method_abort(SimpleCallContext *ctx);
    static Value method_getResponseHeader(SimpleCallContext *ctx);
    static Value method_getAllResponseHeaders(SimpleCallContext *ctx);

    static Value method_get_readyState(SimpleCallContext *ctx);
    static Value method_get_status(SimpleCallContext *ctx);
    static Value method_get_statusText(SimpleCallContext *ctx);
    static Value method_get_responseText(SimpleCallContext *ctx);
    static Value method_get_responseXML(SimpleCallContext *ctx);


    Object *proto;
};

DEFINE_MANAGED_VTABLE(QQmlXMLHttpRequestCtor);

void QQmlXMLHttpRequestCtor::setupProto()
{
    ExecutionEngine *v4 = engine();
    proto = v4->newObject();

    // Methods
    proto->defineDefaultProperty(v4, QStringLiteral("open"), method_open);
    proto->defineDefaultProperty(v4, QStringLiteral("setRequestHeader"), method_setRequestHeader);
    proto->defineDefaultProperty(v4, QStringLiteral("send"), method_send);
    proto->defineDefaultProperty(v4, QStringLiteral("abort"), method_abort);
    proto->defineDefaultProperty(v4, QStringLiteral("getResponseHeader"), method_getResponseHeader);
    proto->defineDefaultProperty(v4, QStringLiteral("getAllResponseHeaders"), method_getAllResponseHeaders);

    // Read-only properties
    proto->defineAccessorProperty(v4, QStringLiteral("readyState"), method_get_readyState, 0);
    proto->defineAccessorProperty(v4, QStringLiteral("status"),method_get_status, 0);
    proto->defineAccessorProperty(v4, QStringLiteral("statusText"),method_get_statusText, 0);
    proto->defineAccessorProperty(v4, QStringLiteral("responseText"),method_get_responseText, 0);
    proto->defineAccessorProperty(v4, QStringLiteral("responseXML"),method_get_responseXML, 0);

    // State values
    proto->defineReadonlyProperty(v4, QStringLiteral("UNSENT"), Value::fromInt32(0));
    proto->defineReadonlyProperty(v4, QStringLiteral("OPENED"), Value::fromInt32(1));
    proto->defineReadonlyProperty(v4, QStringLiteral("HEADERS_RECEIVED"), Value::fromInt32(2));
    proto->defineReadonlyProperty(v4, QStringLiteral("LOADING"), Value::fromInt32(3));
    proto->defineReadonlyProperty(v4, QStringLiteral("DONE"), Value::fromInt32(4));
}


// XMLHttpRequest methods
Value QQmlXMLHttpRequestCtor::method_open(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    if (ctx->argumentCount < 2 || ctx->argumentCount > 5)
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Incorrect argument count");

    QV8Engine *engine = ctx->engine->v8Engine;

    // Argument 0 - Method
    QString method = ctx->arguments[0].toQStringNoThrow().toUpper();
    if (method != QLatin1String("GET") && 
        method != QLatin1String("PUT") &&
        method != QLatin1String("HEAD") &&
        method != QLatin1String("POST") &&
        method != QLatin1String("DELETE"))
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Unsupported HTTP method type");

    // Argument 1 - URL
    QUrl url = QUrl(ctx->arguments[1].toQStringNoThrow());

    if (url.isRelative()) 
        url = engine->callingContext()->resolvedUrl(url);

    // Argument 2 - async (optional)
    if (ctx->argumentCount > 2 && !ctx->arguments[2].booleanValue())
        V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "Synchronous XMLHttpRequest calls are not supported");

    // Argument 3/4 - user/pass (optional)
    QString username, password;
    if (ctx->argumentCount > 3)
        username = ctx->arguments[3].toQStringNoThrow();
    if (ctx->argumentCount > 4)
        password = ctx->arguments[4].toQStringNoThrow();

    // Clear the fragment (if any)
    url.setFragment(QString());

    // Set username/password
    if (!username.isNull()) url.setUserName(username);
    if (!password.isNull()) url.setPassword(password);

    return r->open(constructMeObject(ctx->thisObject, engine), method, url);
}

Value QQmlXMLHttpRequestCtor::method_setRequestHeader(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    if (ctx->argumentCount != 2)
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Incorrect argument count");

    if (r->readyState() != QQmlXMLHttpRequest::Opened || r->sendFlag())
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    QString name = ctx->arguments[0].toQStringNoThrow();
    QString value = ctx->arguments[1].toQStringNoThrow();

    // ### Check that name and value are well formed

    QString nameUpper = name.toUpper();
    if (nameUpper == QLatin1String("ACCEPT-CHARSET") ||
        nameUpper == QLatin1String("ACCEPT-ENCODING") ||
        nameUpper == QLatin1String("CONNECTION") ||
        nameUpper == QLatin1String("CONTENT-LENGTH") ||
        nameUpper == QLatin1String("COOKIE") ||
        nameUpper == QLatin1String("COOKIE2") ||
        nameUpper == QLatin1String("CONTENT-TRANSFER-ENCODING") ||
        nameUpper == QLatin1String("DATE") ||
        nameUpper == QLatin1String("EXPECT") ||
        nameUpper == QLatin1String("HOST") ||
        nameUpper == QLatin1String("KEEP-ALIVE") ||
        nameUpper == QLatin1String("REFERER") ||
        nameUpper == QLatin1String("TE") ||
        nameUpper == QLatin1String("TRAILER") ||
        nameUpper == QLatin1String("TRANSFER-ENCODING") ||
        nameUpper == QLatin1String("UPGRADE") ||
        nameUpper == QLatin1String("USER-AGENT") ||
        nameUpper == QLatin1String("VIA") ||
        nameUpper.startsWith(QLatin1String("PROXY-")) ||
        nameUpper.startsWith(QLatin1String("SEC-"))) 
        return Value::undefinedValue();

    r->addHeader(name, value);

    return Value::undefinedValue();
}

Value QQmlXMLHttpRequestCtor::method_send(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    QV8Engine *engine = ctx->engine->v8Engine;

    if (r->readyState() != QQmlXMLHttpRequest::Opened ||
        r->sendFlag())
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    QByteArray data;
    if (ctx->argumentCount > 0)
        data = ctx->arguments[0].toQStringNoThrow().toUtf8();

    return r->send(constructMeObject(ctx->thisObject, engine), data);
}

Value QQmlXMLHttpRequestCtor::method_abort(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    return r->abort(constructMeObject(ctx->thisObject, ctx->engine->v8Engine));
}

Value QQmlXMLHttpRequestCtor::method_getResponseHeader(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    QV8Engine *engine = ctx->engine->v8Engine;

    if (ctx->argumentCount != 1)
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Incorrect argument count");

    if (r->readyState() != QQmlXMLHttpRequest::Loading &&
        r->readyState() != QQmlXMLHttpRequest::Done &&
        r->readyState() != QQmlXMLHttpRequest::HeadersReceived)
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    return engine->toString(r->header(ctx->arguments[0].toQStringNoThrow()));
}

Value QQmlXMLHttpRequestCtor::method_getAllResponseHeaders(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    QV8Engine *engine = ctx->engine->v8Engine;

    if (ctx->argumentCount != 0)
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Incorrect argument count");

    if (r->readyState() != QQmlXMLHttpRequest::Loading &&
        r->readyState() != QQmlXMLHttpRequest::Done &&
        r->readyState() != QQmlXMLHttpRequest::HeadersReceived)
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    return engine->toString(r->headers());
}

// XMLHttpRequest properties
Value QQmlXMLHttpRequestCtor::method_get_readyState(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    return Value::fromUInt32(r->readyState());
}

Value QQmlXMLHttpRequestCtor::method_get_status(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    if (r->readyState() == QQmlXMLHttpRequest::Unsent ||
        r->readyState() == QQmlXMLHttpRequest::Opened)
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    if (r->errorFlag())
        return Value::fromInt32(0);
    else
        return Value::fromInt32(r->replyStatus());
}

Value QQmlXMLHttpRequestCtor::method_get_statusText(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    QV8Engine *engine = ctx->engine->v8Engine;

    if (r->readyState() == QQmlXMLHttpRequest::Unsent ||
        r->readyState() == QQmlXMLHttpRequest::Opened)
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    if (r->errorFlag())
        return engine->toString(QString());
    else
        return engine->toString(r->replyStatusText());
}

Value QQmlXMLHttpRequestCtor::method_get_responseText(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    QV8Engine *engine = ctx->engine->v8Engine;

    if (r->readyState() != QQmlXMLHttpRequest::Loading &&
        r->readyState() != QQmlXMLHttpRequest::Done)
        return engine->toString(QString());
    else 
        return engine->toString(r->responseBody());
}

Value QQmlXMLHttpRequestCtor::method_get_responseXML(SimpleCallContext *ctx)
{
    QQmlXMLHttpRequestWrapper *w = ctx->thisObject.as<QQmlXMLHttpRequestWrapper>();
    if (!w)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");
    QQmlXMLHttpRequest *r = w->request;

    if (!r->receivedXml() ||
        (r->readyState() != QQmlXMLHttpRequest::Loading &&
         r->readyState() != QQmlXMLHttpRequest::Done)) {
        return Value::nullValue();
    } else {
        return Document::load(ctx->engine->v8Engine, r->rawResponseBody());
    }
}

void qt_rem_qmlxmlhttprequest(QV8Engine * /* engine */, void *d)
{
    QQmlXMLHttpRequestData *data = (QQmlXMLHttpRequestData *)d;
    delete data;
}

void *qt_add_qmlxmlhttprequest(QV8Engine *engine)
{
    ExecutionEngine *v4 = QV8Engine::getV4(engine);

    QQmlXMLHttpRequestCtor *ctor = new (v4->memoryManager) QQmlXMLHttpRequestCtor(v4);
    v4->globalObject->defineReadonlyProperty(v4->newString(QStringLiteral("XMLHttpRequest")), Value::fromObject(ctor));

    QQmlXMLHttpRequestData *data = new QQmlXMLHttpRequestData;
    return data;
}

QT_END_NAMESPACE

#endif // QT_NO_XMLSTREAMREADER

#include <qqmlxmlhttprequest.moc>
