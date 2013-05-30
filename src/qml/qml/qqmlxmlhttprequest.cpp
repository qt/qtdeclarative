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

#include <QtCore/qobject.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qjsengine.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtCore/qtextcodec.h>
#include <QtCore/qxmlstream.h>
#include <QtCore/qstack.h>
#include <QtCore/qdebug.h>

#ifndef QT_NO_XMLSTREAMREADER

#define V8THROW_REFERENCE(string) { \
    v8::ThrowException(v8::Exception::ReferenceError(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}

#define V4THROW_REFERENCE(string) \
    v8::Isolate::GetEngine()->current->throwError(QV4::Value::fromObject(v8::Isolate::GetEngine()->newReferenceErrorObject(QStringLiteral(string))))

#define D(arg) (arg)->release()
#define A(arg) (arg)->addref()

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(xhrDump, QML_XHR_DUMP);

struct QQmlXMLHttpRequestData {
    QQmlXMLHttpRequestData();
    ~QQmlXMLHttpRequestData();

    QV4::PersistentValue nodeFunction;

    QV4::PersistentValue namedNodeMapPrototype;
    QV4::PersistentValue nodeListPrototype;
    QV4::PersistentValue nodePrototype;
    QV4::PersistentValue elementPrototype;
    QV4::PersistentValue attrPrototype;
    QV4::PersistentValue characterDataPrototype;
    QV4::PersistentValue textPrototype;
    QV4::PersistentValue cdataPrototype;
    QV4::PersistentValue documentPrototype;

    v8::Handle<v8::Object> newNode();
};

static inline QQmlXMLHttpRequestData *xhrdata(QV8Engine *engine)
{
    return (QQmlXMLHttpRequestData *)engine->xmlHttpRequestData();
}

static v8::Handle<v8::Object> constructMeObject(v8::Handle<v8::Object> thisObj, QV8Engine *e)
{
    v8::Handle<v8::Object> meObj = v8::Object::New();
    meObj->Set(v8::String::New("ThisObject"), thisObj);
    meObj->Set(v8::String::New("ActivationObject"), QV4::QmlContextWrapper::qmlScope(e, e->callingContext(), 0));
    return meObj;
}

QQmlXMLHttpRequestData::QQmlXMLHttpRequestData()
{
}

QQmlXMLHttpRequestData::~QQmlXMLHttpRequestData()
{
}

v8::Handle<v8::Object> QQmlXMLHttpRequestData::newNode()
{
    if (nodeFunction.isEmpty()) {
        v8::Handle<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
        ft->InstanceTemplate()->SetHasExternalResource(true);
        nodeFunction = ft->GetFunction()->v4Value();
    }

    return nodeFunction.value().asFunctionObject()->newInstance();
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

class NamedNodeMap
{
public:
    // JS API
    static v8::Handle<v8::Value> length(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> indexed(uint32_t index, const v8::AccessorInfo& info);
    static v8::Handle<v8::Value> named(v8::Handle<v8::String> property, const v8::AccessorInfo& args);

    // C++ API
    static v8::Handle<v8::Object> prototype(QV8Engine *);
    static v8::Handle<v8::Value> create(QV8Engine *, NodeImpl *, QList<NodeImpl *> *);
};

class NodeList 
{
public:
    // JS API
    static v8::Handle<v8::Value> length(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> indexed(uint32_t index, const v8::AccessorInfo& info);

    // C++ API
    static v8::Handle<v8::Object> prototype(QV8Engine *);
    static v8::Handle<v8::Value> create(QV8Engine *, NodeImpl *);
};

class Node
{
public:
    // JS API
    static v8::Handle<v8::Value> nodeName(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> nodeValue(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> nodeType(v8::Handle<v8::String>, const v8::AccessorInfo& args);

    static v8::Handle<v8::Value> parentNode(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> childNodes(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> firstChild(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> lastChild(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> previousSibling(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> nextSibling(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> attributes(v8::Handle<v8::String>, const v8::AccessorInfo& args);

    //static v8::Handle<v8::Value> ownerDocument(v8::Local<v8::String>, const v8::AccessorInfo& args);
    //static v8::Handle<v8::Value> namespaceURI(v8::Local<v8::String>, const v8::AccessorInfo& args);
    //static v8::Handle<v8::Value> prefix(v8::Local<v8::String>, const v8::AccessorInfo& args);
    //static v8::Handle<v8::Value> localName(v8::Local<v8::String>, const v8::AccessorInfo& args);
    //static v8::Handle<v8::Value> baseURI(v8::Local<v8::String>, const v8::AccessorInfo& args);
    //static v8::Handle<v8::Value> textContent(v8::Local<v8::String>, const v8::AccessorInfo& args);

    // C++ API
    static v8::Handle<v8::Object> prototype(QV8Engine *);
    static v8::Handle<v8::Value> create(QV8Engine *, NodeImpl *);

    Node();
    Node(const Node &o);
    ~Node();
    bool isNull() const;

    NodeImpl *d;

private:
    Node &operator=(const Node &);
};

class Element : public Node
{
public:
    // C++ API
    static v8::Handle<v8::Object> prototype(QV8Engine *);
};

class Attr : public Node
{
public:
    // JS API
    static v8::Handle<v8::Value> name(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> specified(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> value(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> ownerElement(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> schemaTypeInfo(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> isId(v8::Handle<v8::String>, const v8::AccessorInfo& args);

    // C++ API
    static v8::Handle<v8::Object> prototype(QV8Engine *);
};

class CharacterData : public Node
{
public:
    // JS API
    static v8::Handle<v8::Value> length(v8::Handle<v8::String>, const v8::AccessorInfo& args);

    // C++ API
    static v8::Handle<v8::Object> prototype(QV8Engine *);
};

class Text : public CharacterData
{
public:
    // JS API
    static v8::Handle<v8::Value> isElementContentWhitespace(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> wholeText(v8::Handle<v8::String>, const v8::AccessorInfo& args);

    // C++ API
    static v8::Handle<v8::Object> prototype(QV8Engine *);
};

class CDATA : public Text
{
public:
    // C++ API
    static v8::Handle<v8::Object> prototype(QV8Engine *);
};

class Document : public Node
{
public:
    // JS API
    static v8::Handle<v8::Value> xmlVersion(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> xmlEncoding(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> xmlStandalone(v8::Handle<v8::String>, const v8::AccessorInfo& args);
    static v8::Handle<v8::Value> documentElement(v8::Handle<v8::String>, const v8::AccessorInfo& args);

    // C++ API
    static v8::Handle<v8::Object> prototype(QV8Engine *);
    static v8::Handle<v8::Value> load(QV8Engine *engine, const QByteArray &data);
};

}

class QQmlDOMNodeResource : public QV8ObjectResource, public Node
{
    V8_RESOURCE_TYPE(DOMNodeType);
public:
    QQmlDOMNodeResource(QV8Engine *e);

    QList<NodeImpl *> *list; // Only used in NamedNodeMap
};

QQmlDOMNodeResource::QQmlDOMNodeResource(QV8Engine *e)
: QV8ObjectResource(e), list(0)
{
}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(Node)
Q_DECLARE_METATYPE(NodeList)
Q_DECLARE_METATYPE(NamedNodeMap)

QT_BEGIN_NAMESPACE

void NodeImpl::addref() 
{
    A(document);
}

void NodeImpl::release()
{
    D(document);
}

v8::Handle<v8::Value> Node::nodeName(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    switch (r->d->type) {
    case NodeImpl::Document:
        return v8::String::New("#document");
    case NodeImpl::CDATA:
        return v8::String::New("#cdata-section");
    case NodeImpl::Text:
        return v8::String::New("#text");
    default:
        return engine->toString(r->d->name);
    }
}

v8::Handle<v8::Value> Node::nodeValue(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    if (r->d->type == NodeImpl::Document ||
        r->d->type == NodeImpl::DocumentFragment ||
        r->d->type == NodeImpl::DocumentType ||
        r->d->type == NodeImpl::Element ||
        r->d->type == NodeImpl::Entity ||
        r->d->type == NodeImpl::EntityReference ||
        r->d->type == NodeImpl::Notation)
        return QV4::Value::nullValue();

    return engine->toString(r->d->data);
}

v8::Handle<v8::Value> Node::nodeType(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    return QV4::Value::fromInt32(r->d->type);
}

v8::Handle<v8::Value> Node::parentNode(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    if (r->d->parent) return Node::create(engine, r->d->parent);
    else return QV4::Value::nullValue();
}

v8::Handle<v8::Value> Node::childNodes(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    return NodeList::create(engine, r->d);
}

v8::Handle<v8::Value> Node::firstChild(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    if (r->d->children.isEmpty()) return QV4::Value::nullValue();
    else return Node::create(engine, r->d->children.first());
}

v8::Handle<v8::Value> Node::lastChild(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    if (r->d->children.isEmpty()) return QV4::Value::nullValue();
    else return Node::create(engine, r->d->children.last());
}

v8::Handle<v8::Value> Node::previousSibling(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    if (!r->d->parent) return QV4::Value::nullValue();

    for (int ii = 0; ii < r->d->parent->children.count(); ++ii) {
        if (r->d->parent->children.at(ii) == r->d) {
            if (ii == 0) return QV4::Value::nullValue();
            else return Node::create(engine, r->d->parent->children.at(ii - 1));
        }
    }

    return QV4::Value::nullValue();
}

v8::Handle<v8::Value> Node::nextSibling(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    if (!r->d->parent) return QV4::Value::nullValue();

    for (int ii = 0; ii < r->d->parent->children.count(); ++ii) {
        if (r->d->parent->children.at(ii) == r->d) {
            if ((ii + 1) == r->d->parent->children.count()) return QV4::Value::nullValue();
            else return Node::create(engine, r->d->parent->children.at(ii + 1)); 
        }
    }

    return QV4::Value::nullValue();
}

v8::Handle<v8::Value> Node::attributes(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    if (r->d->type != NodeImpl::Element)
        return QV4::Value::nullValue();
    else
        return NamedNodeMap::create(engine, r->d, &r->d->attributes);
}

v8::Handle<v8::Object> Node::prototype(QV8Engine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    if (d->nodePrototype.isEmpty()) {
        v8::Handle<v8::Object> p = v8::Object::New();
        p->SetAccessor(v8::String::New("nodeName"), nodeName,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("nodeValue"), nodeValue,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("nodeType"), nodeType,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("parentNode"), parentNode,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("childNodes"), childNodes,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("firstChild"), firstChild,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("lastChild"), lastChild,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("previousSibling"), previousSibling,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("nextSibling"), nextSibling,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("attributes"), attributes,
                                      0, v8::External::New(engine));
        d->nodeListPrototype = p->v4Value();
        engine->freezeObject(d->nodePrototype);
    }
    return d->nodePrototype.value();
}

v8::Handle<v8::Value> Node::create(QV8Engine *engine, NodeImpl *data)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    v8::Handle<v8::Object> instance = d->newNode();

    switch (data->type) {
    case NodeImpl::Attr:
        instance->SetPrototype(Attr::prototype(engine));
        break;
    case NodeImpl::Comment:
    case NodeImpl::Document:
    case NodeImpl::DocumentFragment:
    case NodeImpl::DocumentType:
    case NodeImpl::Entity:
    case NodeImpl::EntityReference:
    case NodeImpl::Notation:
    case NodeImpl::ProcessingInstruction:
        return QV4::Value::undefinedValue();
    case NodeImpl::CDATA:
        instance->SetPrototype(CDATA::prototype(engine));
        break;
    case NodeImpl::Text:
        instance->SetPrototype(Text::prototype(engine));
        break;
    case NodeImpl::Element:
        instance->SetPrototype(Element::prototype(engine));
        break;
    }

    QQmlDOMNodeResource *r = new QQmlDOMNodeResource(engine);
    r->d = data;
    if (data) A(data);
    instance->SetExternalResource(r);

    return instance;
}

v8::Handle<v8::Object> Element::prototype(QV8Engine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    if (d->elementPrototype.isEmpty()) {
        v8::Handle<v8::Object> p = v8::Object::New();
        p->SetPrototype(Node::prototype(engine));
        p->SetAccessor(v8::String::New("tagName"), nodeName,
                                         0, v8::External::New(engine));
        d->elementPrototype = p->v4Value();
        engine->freezeObject(d->elementPrototype);
    }
    return d->elementPrototype.value();
}

v8::Handle<v8::Object> Attr::prototype(QV8Engine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    if (d->attrPrototype.isEmpty()) {
        v8::Handle<v8::Object> p = v8::Object::New();
        p->SetPrototype(Node::prototype(engine));
        p->SetAccessor(v8::String::New("name"), name,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("value"), value,
                                      0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("ownerElement"), ownerElement,
                                      0, v8::External::New(engine));
        d->attrPrototype = p->v4Value();
        engine->freezeObject(d->attrPrototype);
    }
    return d->attrPrototype.value();
}

v8::Handle<v8::Value> Attr::name(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    return engine->toString(r->d->name);
}

v8::Handle<v8::Value> Attr::value(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    return engine->toString(r->d->data);
}

v8::Handle<v8::Value> Attr::ownerElement(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    return Node::create(engine, r->d->parent);
}

v8::Handle<v8::Value> CharacterData::length(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();
    Q_UNUSED(engine)
    return QV4::Value::fromInt32(r->d->data.length());
}

v8::Handle<v8::Object> CharacterData::prototype(QV8Engine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    if (d->characterDataPrototype.isEmpty()) {
        v8::Handle<v8::Object> p = v8::Object::New();
        p->SetPrototype(Node::prototype(engine));
        p->SetAccessor(v8::String::New("data"), nodeValue,
                                               0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("length"), length,
                                               0, v8::External::New(engine));
        d->characterDataPrototype = p->v4Value();
        engine->freezeObject(d->characterDataPrototype);
    }
    return d->characterDataPrototype.value();
}

v8::Handle<v8::Value> Text::isElementContentWhitespace(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();
    Q_UNUSED(engine)
    return QV4::Value::fromBoolean(r->d->data.trimmed().isEmpty());
}

v8::Handle<v8::Value> Text::wholeText(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    return engine->toString(r->d->data);
}

v8::Handle<v8::Object> Text::prototype(QV8Engine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    if (d->textPrototype.isEmpty()) {
        v8::Handle<v8::Object> p = v8::Object::New();
        p->SetPrototype(CharacterData::prototype(engine));
        p->SetAccessor(v8::String::New("isElementContentWhitespace"), isElementContentWhitespace,
                                               0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("wholeText"), wholeText,
                                               0, v8::External::New(engine));
        d->textPrototype = p->v4Value();
        engine->freezeObject(d->textPrototype);
    }
    return d->textPrototype.value();
}

v8::Handle<v8::Object> CDATA::prototype(QV8Engine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    if (d->cdataPrototype.isEmpty()) {
        QV4::Object *p = QV8Engine::getV4(engine)->newObject();
        p->prototype = Text::prototype(engine)->v4Value().asObject();
        d->cdataPrototype = QV4::Value::fromObject(p);
        engine->freezeObject(d->cdataPrototype);
    }
    return d->cdataPrototype.value();
}

v8::Handle<v8::Object> Document::prototype(QV8Engine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    if (d->documentPrototype.isEmpty()) {
        v8::Handle<v8::Object> p = v8::Object::New();
        p->SetPrototype(Node::prototype(engine));
        p->SetAccessor(v8::String::New("xmlVersion"), xmlVersion,
                                          0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("xmlEncoding"), xmlEncoding,
                                          0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("xmlStandalone"), xmlStandalone,
                                          0, v8::External::New(engine));
        p->SetAccessor(v8::String::New("documentElement"), documentElement,
                                          0, v8::External::New(engine));
        d->documentPrototype = p->v4Value();
        engine->freezeObject(d->documentPrototype);
    }
    return d->documentPrototype.value();
}

v8::Handle<v8::Value> Document::load(QV8Engine *engine, const QByteArray &data)
{
    Q_ASSERT(engine);

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
        if (document) D(document);
        return QV4::Value::nullValue();
    }

    v8::Handle<v8::Object> instance = xhrdata(engine)->newNode();
    QQmlDOMNodeResource *r = new QQmlDOMNodeResource(engine);
    r->d = document;
    instance->SetExternalResource(r);
    instance->SetPrototype(Document::prototype(engine));
    return instance;
}

Node::Node()
: d(0)
{
}

Node::Node(const Node &o)
: d(o.d)
{
    if (d) A(d);
}

Node::~Node()
{
    if (d) D(d);
}

bool Node::isNull() const
{
    return d == 0;
}

v8::Handle<v8::Value> NamedNodeMap::length(v8::Handle<v8::String>, const v8::AccessorInfo &args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();
    Q_UNUSED(engine)
    return QV4::Value::fromInt32(r->list->count());
}

v8::Handle<v8::Value> NamedNodeMap::indexed(uint32_t index, const v8::AccessorInfo& args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r || !r->list) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    if ((int)index < r->list->count()) {
        return Node::create(engine, r->list->at(index));
    } else {
        return QV4::Value::undefinedValue();
    }
}

v8::Handle<v8::Value> NamedNodeMap::named(v8::Handle<v8::String> property, const v8::AccessorInfo& args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r || !r->list) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    QString str = property->v4Value().toQString();
    for (int ii = 0; ii < r->list->count(); ++ii) {
        if (r->list->at(ii)->name == str) {
            return Node::create(engine, r->list->at(ii));
        }
    }

    return QV4::Value::undefinedValue();
}

v8::Handle<v8::Object> NamedNodeMap::prototype(QV8Engine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    if (d->namedNodeMapPrototype.isEmpty()) {
        v8::Handle<v8::ObjectTemplate> ot = v8::ObjectTemplate::New();
        ot->SetAccessor(v8::String::New("length"), length, 0, v8::External::New(engine));
        ot->SetIndexedPropertyHandler(indexed, 0, 0, 0, 0, v8::External::New(engine));
        ot->SetFallbackPropertyHandler(named, 0, 0, 0, 0, v8::External::New(engine));
        d->namedNodeMapPrototype = ot->NewInstance()->v4Value();
        engine->freezeObject(d->namedNodeMapPrototype);
    }
    return d->namedNodeMapPrototype.value();
}

v8::Handle<v8::Value> NamedNodeMap::create(QV8Engine *engine, NodeImpl *data, QList<NodeImpl *> *list)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    v8::Handle<v8::Object> instance = d->newNode();
    instance->SetPrototype(NamedNodeMap::prototype(engine));
    QQmlDOMNodeResource *r = new QQmlDOMNodeResource(engine);
    r->d = data;
    r->list = list;
    if (data) A(data);
    instance->SetExternalResource(r);
    return instance;
}

v8::Handle<v8::Value> NodeList::indexed(uint32_t index, const v8::AccessorInfo& args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    if ((int)index < r->d->children.count()) {
        return Node::create(engine, r->d->children.at(index));
    } else {
        return QV4::Value::undefinedValue();
    }
}

v8::Handle<v8::Value> NodeList::length(v8::Handle<v8::String>, const v8::AccessorInfo& args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();
    Q_UNUSED(engine)
    return QV4::Value::fromInt32(r->d->children.count());
}

v8::Handle<v8::Object> NodeList::prototype(QV8Engine *engine)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    if (d->nodeListPrototype.isEmpty()) {
        v8::Handle<v8::ObjectTemplate> ot = v8::ObjectTemplate::New();
        ot->SetAccessor(v8::String::New("length"), length, 0, v8::External::New(engine));
        ot->SetIndexedPropertyHandler(indexed, 0, 0, 0, 0, v8::External::New(engine));
        d->nodeListPrototype = ot->NewInstance()->v4Value();
        engine->freezeObject(d->nodeListPrototype);
    }
    return d->nodeListPrototype.value();
}

v8::Handle<v8::Value> NodeList::create(QV8Engine *engine, NodeImpl *data)
{
    QQmlXMLHttpRequestData *d = xhrdata(engine);
    v8::Handle<v8::Object> instance = d->newNode();
    instance->SetPrototype(NodeList::prototype(engine));
    QQmlDOMNodeResource *r = new QQmlDOMNodeResource(engine);
    r->d = data;
    if (data) A(data);
    instance->SetExternalResource(r);
    return instance;
}

v8::Handle<v8::Value> Document::documentElement(v8::Handle<v8::String>, const v8::AccessorInfo& args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r || r->d->type != NodeImpl::Document) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    return Node::create(engine, static_cast<DocumentImpl *>(r->d)->root);
}

v8::Handle<v8::Value> Document::xmlStandalone(v8::Handle<v8::String>, const v8::AccessorInfo& args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r || r->d->type != NodeImpl::Document) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();
    Q_UNUSED(engine)
    return QV4::Value::fromBoolean(static_cast<DocumentImpl *>(r->d)->isStandalone);
}

v8::Handle<v8::Value> Document::xmlVersion(v8::Handle<v8::String>, const v8::AccessorInfo& args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r || r->d->type != NodeImpl::Document) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    return engine->toString(static_cast<DocumentImpl *>(r->d)->version);
}

v8::Handle<v8::Value> Document::xmlEncoding(v8::Handle<v8::String>, const v8::AccessorInfo& args)
{
    QQmlDOMNodeResource *r = v8_resource_cast<QQmlDOMNodeResource>(args.This());
    if (!r || r->d->type != NodeImpl::Document) return QV4::Value::undefinedValue();
    QV8Engine *engine = V8ENGINE();

    return engine->toString(static_cast<DocumentImpl *>(r->d)->encoding);
}

class QQmlXMLHttpRequest : public QObject, public QV8ObjectResource
{
Q_OBJECT
V8_RESOURCE_TYPE(XMLHttpRequestType)
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

    QV4::Value open(v8::Handle<v8::Object> me, const QString &, const QUrl &);
    QV4::Value send(v8::Handle<v8::Object> me, const QByteArray &);
    QV4::Value abort(v8::Handle<v8::Object> me);

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

    QV4::ExecutionEngine *v4;
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

    v8::Handle<v8::Object> getMe() const;
    void setMe(v8::Handle<v8::Object> me);
    QV4::PersistentValue m_me;

    void dispatchCallback(const QV4::Value &me);
    void printError(const QV4::Exception &e);

    int m_status;
    QString m_statusText;
    QNetworkRequest m_request;
    QQmlGuard<QNetworkReply> m_network;
    void destroyNetwork();

    QNetworkAccessManager *m_nam;
    QNetworkAccessManager *networkAccessManager() { return m_nam; }
};

QQmlXMLHttpRequest::QQmlXMLHttpRequest(QV8Engine *engine, QNetworkAccessManager *manager)
    : QV8ObjectResource(engine), v4(QV8Engine::getV4(engine))
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

QV4::Value QQmlXMLHttpRequest::open(v8::Handle<v8::Object> me, const QString &method,
                                                       const QUrl &url)
{
    destroyNetwork();
    m_sendFlag = false;
    m_errorFlag = false;
    m_responseEntityBody = QByteArray();
    m_method = method;
    m_url = url;
    m_state = Opened;
    dispatchCallback(me->v4Value());
    return QV4::Value::undefinedValue();
}

void QQmlXMLHttpRequest::addHeader(const QString &name, const QString &value)
{
    QByteArray utfname = name.toUtf8();

    if (m_request.hasRawHeader(utfname)) {
        m_request.setRawHeader(utfname, m_request.rawHeader(utfname) + ',' + value.toUtf8());
    } else {
        m_request.setRawHeader(utfname, value.toUtf8());
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

QV4::Value QQmlXMLHttpRequest::send(v8::Handle<v8::Object> me, const QByteArray &data)
{
    m_errorFlag = false;
    m_sendFlag = true;
    m_redirectCount = 0;
    m_data = data;

    setMe(me);

    requestFromUrl(m_url);

    return QV4::Value::undefinedValue();
}

QV4::Value QQmlXMLHttpRequest::abort(v8::Handle<v8::Object> me)
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
        dispatchCallback(me->v4Value());
    }

    m_state = Unsent;

    return QV4::Value::undefinedValue();
}

v8::Handle<v8::Object> QQmlXMLHttpRequest::getMe() const
{
    return m_me.value();
}

void QQmlXMLHttpRequest::setMe(v8::Handle<v8::Object> me)
{
    m_me = me->v4Value();
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

    setMe(v8::Handle<v8::Object>());
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

void QQmlXMLHttpRequest::dispatchCallback(const QV4::Value &me)
{
    QV4::ExecutionContext *ctx = v4->current;
    try {
        QV4::Object *o = me.asObject();
        if (!o)
            __qmljs_throw(ctx, QV4::Value::fromObject(
                          v4->newErrorObject(QV4::Value::fromString(ctx, QStringLiteral("QQmlXMLHttpRequest: internal error: empty ThisObject")))));

        QV4::Object *thisObj = o->get(v4->newString(QStringLiteral("ThisObject"))).asObject();
        if (!thisObj)
            __qmljs_throw(ctx, QV4::Value::fromObject(
                          v4->newErrorObject(QV4::Value::fromString(ctx, QStringLiteral("QQmlXMLHttpRequest: internal error: empty ThisObject")))));

        QV4::FunctionObject *callback = thisObj->get(v4->newString(QStringLiteral("onreadystatechange"))).asFunctionObject();
        if (!callback) {
            // not an error, but no onreadystatechange function to call.
            return;
        }

        QV4::Value activationObject = o->get(v4->newString(QStringLiteral("ActivationObject")));
        if (!activationObject.asObject()) {
            v8::ThrowException(v8::Exception::Error(v8::String::New("QQmlXMLHttpRequest: internal error: empty ActivationObject")));
            return;
        }

        QQmlContextData *callingContext = QV4::QmlContextWrapper::getContext(activationObject);
        if (callingContext)
            callback->call(v4->current, activationObject, 0, 0);

        // if the callingContext object is no longer valid, then it has been
        // deleted explicitly (e.g., by a Loader deleting the itemContext when
        // the source is changed).  We do nothing in this case, as the evaluation
        // cannot succeed.
    } catch(QV4::Exception &e) {
        e.accept(ctx);
        printError(e);
    }
}

// Must have a handle scope
void QQmlXMLHttpRequest::printError(const QV4::Exception &e)
{
    QQmlError error;
    QQmlExpressionPrivate::exceptionToError(e, error);
    QQmlEnginePrivate::warning(QQmlEnginePrivate::get(engine->engine()), error);
}

void QQmlXMLHttpRequest::destroyNetwork()
{
    if (m_network) {
        m_network->disconnect();
        m_network->deleteLater();
        m_network = 0;
    }
}

// XMLHttpRequest methods
static QV4::Value qmlxmlhttprequest_open(const v8::Arguments &args)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(args.This());
    if (!r)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");

    if (args.Length() < 2 || args.Length() > 5)
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Incorrect argument count");

    QV8Engine *engine = r->engine;

    // Argument 0 - Method
    QString method = args[0]->v4Value().toQString().toUpper();
    if (method != QLatin1String("GET") && 
        method != QLatin1String("PUT") &&
        method != QLatin1String("HEAD") &&
        method != QLatin1String("POST") &&
        method != QLatin1String("DELETE"))
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Unsupported HTTP method type");

    // Argument 1 - URL
    QUrl url = QUrl(args[1]->v4Value().toQString());

    if (url.isRelative()) 
        url = engine->callingContext()->resolvedUrl(url);

    // Argument 2 - async (optional)
    if (args.Length() > 2 && !args[2]->BooleanValue())
        V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "Synchronous XMLHttpRequest calls are not supported");

    // Argument 3/4 - user/pass (optional)
    QString username, password;
    if (args.Length() > 3)
        username = args[3]->v4Value().toQString();
    if (args.Length() > 4)
        password = args[4]->v4Value().toQString();

    // Clear the fragment (if any)
    url.setFragment(QString());

    // Set username/password
    if (!username.isNull()) url.setUserName(username);
    if (!password.isNull()) url.setPassword(password);

    return r->open(constructMeObject(args.This(), engine), method, url);
}

static QV4::Value qmlxmlhttprequest_setRequestHeader(const v8::Arguments &args)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(args.This());
    if (!r)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");

    if (args.Length() != 2)
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Incorrect argument count");

    if (r->readyState() != QQmlXMLHttpRequest::Opened || r->sendFlag())
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    QV8Engine *engine = r->engine;

    QString name = args[0]->v4Value().toQString();
    QString value = args[1]->v4Value().toQString();

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
        return QV4::Value::undefinedValue();

    r->addHeader(name, value);

    return QV4::Value::undefinedValue();
}

static QV4::Value qmlxmlhttprequest_send(const v8::Arguments &args)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(args.This());
    if (!r)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");

    QV8Engine *engine = r->engine;

    if (r->readyState() != QQmlXMLHttpRequest::Opened ||
        r->sendFlag())
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    QByteArray data;
    if (args.Length() > 0)
        data = args[0]->v4Value().toQString().toUtf8();

    return r->send(constructMeObject(args.This(), engine), data);
}

static QV4::Value qmlxmlhttprequest_abort(const v8::Arguments &args)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(args.This());
    if (!r)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");

    return r->abort(constructMeObject(args.This(), r->engine));
}

static QV4::Value qmlxmlhttprequest_getResponseHeader(const v8::Arguments &args)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(args.This());
    if (!r)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");

    QV8Engine *engine = r->engine;

    if (args.Length() != 1)
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Incorrect argument count");

    if (r->readyState() != QQmlXMLHttpRequest::Loading &&
        r->readyState() != QQmlXMLHttpRequest::Done &&
        r->readyState() != QQmlXMLHttpRequest::HeadersReceived)
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    return engine->toString(r->header(args[0]->v4Value().toQString()));
}

static QV4::Value qmlxmlhttprequest_getAllResponseHeaders(const v8::Arguments &args)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(args.This());
    if (!r)
        V4THROW_REFERENCE("Not an XMLHttpRequest object");

    QV8Engine *engine = r->engine;

    if (args.Length() != 0) 
        V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "Incorrect argument count");

    if (r->readyState() != QQmlXMLHttpRequest::Loading &&
        r->readyState() != QQmlXMLHttpRequest::Done &&
        r->readyState() != QQmlXMLHttpRequest::HeadersReceived)
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    return engine->toString(r->headers());
}

// XMLHttpRequest properties
static v8::Handle<v8::Value> qmlxmlhttprequest_readyState(v8::Handle<v8::String> /* property */,
                                                          const v8::AccessorInfo& info)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(info.This());
    if (!r)
        V8THROW_REFERENCE("Not an XMLHttpRequest object");

    return QV4::Value::fromUInt32(r->readyState());
}

static v8::Handle<v8::Value> qmlxmlhttprequest_status(v8::Handle<v8::String> /* property */,
                                                      const v8::AccessorInfo& info)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(info.This());
    if (!r)
        V8THROW_REFERENCE("Not an XMLHttpRequest object");

    if (r->readyState() == QQmlXMLHttpRequest::Unsent ||
        r->readyState() == QQmlXMLHttpRequest::Opened)
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    if (r->errorFlag())
        return QV4::Value::fromInt32(0);
    else
        return QV4::Value::fromInt32(r->replyStatus());
}

static v8::Handle<v8::Value> qmlxmlhttprequest_statusText(v8::Handle<v8::String> /* property */,
                                                          const v8::AccessorInfo& info)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(info.This());
    if (!r)
        V8THROW_REFERENCE("Not an XMLHttpRequest object");

    QV8Engine *engine = r->engine;

    if (r->readyState() == QQmlXMLHttpRequest::Unsent ||
        r->readyState() == QQmlXMLHttpRequest::Opened)
        V4THROW_DOM(DOMEXCEPTION_INVALID_STATE_ERR, "Invalid state");

    if (r->errorFlag())
        return engine->toString(QString());
    else
        return engine->toString(r->replyStatusText());
}

static v8::Handle<v8::Value> qmlxmlhttprequest_responseText(v8::Handle<v8::String> /* property */,
                                                            const v8::AccessorInfo& info)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(info.This());
    if (!r)
        V8THROW_REFERENCE("Not an XMLHttpRequest object");

    QV8Engine *engine = r->engine;

    if (r->readyState() != QQmlXMLHttpRequest::Loading &&
        r->readyState() != QQmlXMLHttpRequest::Done)
        return engine->toString(QString());
    else 
        return engine->toString(r->responseBody());
}

static v8::Handle<v8::Value> qmlxmlhttprequest_responseXML(v8::Handle<v8::String> /* property */,
                                                           const v8::AccessorInfo& info)
{
    QQmlXMLHttpRequest *r = v8_resource_cast<QQmlXMLHttpRequest>(info.This());
    if (!r)
        V8THROW_REFERENCE("Not an XMLHttpRequest object");

    if (!r->receivedXml() ||
        (r->readyState() != QQmlXMLHttpRequest::Loading &&
         r->readyState() != QQmlXMLHttpRequest::Done)) {
        return QV4::Value::nullValue();
    } else {
        return Document::load(r->engine, r->rawResponseBody());
    }
}

static QV4::Value qmlxmlhttprequest_new(const v8::Arguments &args)
{
    if (args.IsConstructCall()) {
        QV8Engine *engine = V8ENGINE();
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine->engine());
        Q_UNUSED(ep)
        QQmlXMLHttpRequest *r = new QQmlXMLHttpRequest(engine, engine->networkAccessManager());
        args.This()->SetExternalResource(r);

        return args.This()->v4Value();
    } else {
        return QV4::Value::undefinedValue();
    }
}

#define NEWFUNCTION(function) v8::FunctionTemplate::New(function)->GetFunction()

void qt_rem_qmlxmlhttprequest(QV8Engine * /* engine */, void *d)
{
    QQmlXMLHttpRequestData *data = (QQmlXMLHttpRequestData *)d;
    delete data;
}

void *qt_add_qmlxmlhttprequest(QV8Engine *engine)
{
    v8::PropertyAttribute attributes = (v8::PropertyAttribute)(v8::ReadOnly | v8::DontEnum | v8::DontDelete);

    // XMLHttpRequest
    v8::Handle<v8::FunctionTemplate> xmlhttprequest = v8::FunctionTemplate::New(qmlxmlhttprequest_new,
                                                                               v8::External::New(engine));
    xmlhttprequest->InstanceTemplate()->SetHasExternalResource(true);

    // Methods
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("open"), NEWFUNCTION(qmlxmlhttprequest_open), attributes);
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("setRequestHeader"), NEWFUNCTION(qmlxmlhttprequest_setRequestHeader), attributes);
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("send"), NEWFUNCTION(qmlxmlhttprequest_send), attributes);
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("abort"), NEWFUNCTION(qmlxmlhttprequest_abort), attributes);
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("getResponseHeader"), NEWFUNCTION(qmlxmlhttprequest_getResponseHeader), attributes);
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("getAllResponseHeaders"), NEWFUNCTION(qmlxmlhttprequest_getAllResponseHeaders), attributes);

    // Read-only properties
    xmlhttprequest->PrototypeTemplate()->SetAccessor(v8::String::New("readyState"), qmlxmlhttprequest_readyState, 0, v8::Handle<v8::Value>(), v8::DEFAULT, attributes);
    xmlhttprequest->PrototypeTemplate()->SetAccessor(v8::String::New("status"),qmlxmlhttprequest_status, 0, v8::Handle<v8::Value>(), v8::DEFAULT, attributes);
    xmlhttprequest->PrototypeTemplate()->SetAccessor(v8::String::New("statusText"),qmlxmlhttprequest_statusText, 0, v8::Handle<v8::Value>(), v8::DEFAULT, attributes);
    xmlhttprequest->PrototypeTemplate()->SetAccessor(v8::String::New("responseText"),qmlxmlhttprequest_responseText, 0, v8::Handle<v8::Value>(), v8::DEFAULT, attributes);
    xmlhttprequest->PrototypeTemplate()->SetAccessor(v8::String::New("responseXML"),qmlxmlhttprequest_responseXML, 0, v8::Handle<v8::Value>(), v8::DEFAULT, attributes);

    // State values
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("UNSENT"), QV4::Value::fromInt32(0), attributes);
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("OPENED"), QV4::Value::fromInt32(1), attributes);
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("HEADERS_RECEIVED"), QV4::Value::fromInt32(2), attributes);
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("LOADING"), QV4::Value::fromInt32(3), attributes);
    xmlhttprequest->PrototypeTemplate()->Set(v8::String::New("DONE"), QV4::Value::fromInt32(4), attributes);

    // Constructor
    xmlhttprequest->Set(v8::String::New("UNSENT"), QV4::Value::fromInt32(0), attributes);
    xmlhttprequest->Set(v8::String::New("OPENED"), QV4::Value::fromInt32(1), attributes);
    xmlhttprequest->Set(v8::String::New("HEADERS_RECEIVED"), QV4::Value::fromInt32(2), attributes);
    xmlhttprequest->Set(v8::String::New("LOADING"), QV4::Value::fromInt32(3), attributes);
    xmlhttprequest->Set(v8::String::New("DONE"), QV4::Value::fromInt32(4), attributes);
    v8::Handle<v8::Object>(engine->global())->Set(v8::String::New("XMLHttpRequest"), xmlhttprequest->GetFunction());

    QQmlXMLHttpRequestData *data = new QQmlXMLHttpRequestData;
    return data;
}

QT_END_NAMESPACE

#endif // QT_NO_XMLSTREAMREADER

#include <qqmlxmlhttprequest.moc>
