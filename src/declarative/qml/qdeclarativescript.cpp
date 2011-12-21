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

#include "qdeclarativescript_p.h"

#include "parser/qdeclarativejsengine_p.h"
#include "parser/qdeclarativejsparser_p.h"
#include "parser/qdeclarativejslexer_p.h"
#include "parser/qdeclarativejsmemorypool_p.h"
#include "parser/qdeclarativejsastvisitor_p.h"
#include "parser/qdeclarativejsast_p.h"
#include <private/qdeclarativerewrite_p.h>

#include <QStack>
#include <QCoreApplication>
#include <QtDebug>

QT_BEGIN_NAMESPACE

using namespace QDeclarativeJS;
using namespace QDeclarativeScript;

// 
// Parser IR classes
//
QDeclarativeScript::Object::Object()
: type(-1), idIndex(-1), metatype(0), synthCache(0), defaultProperty(0), parserStatusCast(-1),
  componentCompileState(0), nextAliasingObject(0), nextIdObject(0)
{
}

QDeclarativeScript::Object::~Object() 
{ 
    if (synthCache) synthCache->release();
}

void Object::setBindingBit(int b)
{
    while (bindingBitmask.size() < 4 * (1 + b / 32))
        bindingBitmask.append(char(0));

    quint32 *bits = (quint32 *)bindingBitmask.data();
    bits[b / 32] |= (1 << (b % 32));
}

const QMetaObject *Object::metaObject() const
{
    if (!metadata.isEmpty() && metatype)
        return &extObject;
    else
        return metatype;
}

QDeclarativeScript::Property *Object::getDefaultProperty()
{
    if (!defaultProperty) {
        defaultProperty = pool()->New<Property>();
        defaultProperty->parent = this;
    }
    return defaultProperty;
}

void QDeclarativeScript::Object::addValueProperty(Property *p)
{
    valueProperties.append(p);
}

void QDeclarativeScript::Object::addSignalProperty(Property *p)
{
    signalProperties.append(p);
}

void QDeclarativeScript::Object::addAttachedProperty(Property *p)
{
    attachedProperties.append(p);
}

void QDeclarativeScript::Object::addGroupedProperty(Property *p)
{
    groupedProperties.append(p);
}

void QDeclarativeScript::Object::addValueTypeProperty(Property *p)
{
    valueTypeProperties.append(p);
}

void QDeclarativeScript::Object::addScriptStringProperty(Property *p)
{
    scriptStringProperties.append(p);
}

// This lookup is optimized for missing, and having to create a new property.
Property *QDeclarativeScript::Object::getProperty(const QHashedStringRef &name, bool create)
{
    if (create) {
        quint32 h = name.hash();
        if (propertiesHashField.testAndSet(h)) {
            for (Property *p = properties.first(); p; p = properties.next(p)) {
                if (p->name() == name)
                    return p;
            }
        }

        Property *property = pool()->New<Property>();
        property->parent = this;
        property->_name = name;
        property->isDefault = false;
        properties.prepend(property);
        return property;
    } else {
        for (Property *p = properties.first(); p; p = properties.next(p)) {
            if (p->name() == name)
                return p;
        }
    }

    return 0;
}

Property *QDeclarativeScript::Object::getProperty(const QStringRef &name, bool create)
{
    return getProperty(QHashedStringRef(name), create);
}

Property *QDeclarativeScript::Object::getProperty(const QString &name, bool create)
{
    for (Property *p = properties.first(); p; p = properties.next(p)) {
        if (p->name() == name)
            return p;
    }

    if (create) {
        Property *property = pool()->New<Property>();
        property->parent = this;
        property->_name = QStringRef(pool()->NewString(name));
        propertiesHashField.testAndSet(property->_name.hash());
        property->isDefault = false;
        properties.prepend(property);
        return property;
    } else {
        return 0;
    }
}

QDeclarativeScript::Object::DynamicProperty::DynamicProperty()
: isDefaultProperty(false), isReadOnly(false), type(Variant), defaultValue(0), nextProperty(0),
  resolvedCustomTypeName(0)
{
}

QDeclarativeScript::Object::DynamicSignal::DynamicSignal()
: nextSignal(0)
{
}

// Returns length in utf8 bytes
int QDeclarativeScript::Object::DynamicSignal::parameterTypesLength() const
{
    int rv = 0;
    for (int ii = 0; ii < parameterTypes.count(); ++ii)
        rv += parameterTypes.at(ii).length();
    return rv;
}

// Returns length in utf8 bytes
int QDeclarativeScript::Object::DynamicSignal::parameterNamesLength() const
{
    int rv = 0;
    for (int ii = 0; ii < parameterNames.count(); ++ii)
        rv += parameterNames.at(ii).utf8length();
    return rv;
}

QDeclarativeScript::Object::DynamicSlot::DynamicSlot()
: nextSlot(0)
{
}

int QDeclarativeScript::Object::DynamicSlot::parameterNamesLength() const
{
    int rv = 0;
    for (int ii = 0; ii < parameterNames.count(); ++ii)
        rv += parameterNames.at(ii).length();
    return rv;
}

QDeclarativeScript::Property::Property()
: parent(0), type(0), index(-1), value(0), isDefault(true), isDeferred(false), 
  isValueTypeSubProperty(false), isAlias(false), isReadOnlyDeclaration(false),
  scriptStringScope(-1), nextMainProperty(0), nextProperty(0)
{
}

QDeclarativeScript::Object *QDeclarativeScript::Property::getValue(const LocationSpan &l)
{
    if (!value) { value = pool()->New<Object>(); value->location = l; }
    return value;
}

void QDeclarativeScript::Property::addValue(Value *v)
{
    values.append(v);
}

void QDeclarativeScript::Property::addOnValue(Value *v)
{
    onValues.append(v);
}

bool QDeclarativeScript::Property::isEmpty() const
{
    return !value && values.isEmpty() && onValues.isEmpty();
}

QDeclarativeScript::Value::Value()
: type(Unknown), object(0), bindingReference(0), nextValue(0)
{
}

QDeclarativeScript::Variant::Variant()
: t(Invalid)
{
}

QDeclarativeScript::Variant::Variant(const Variant &o)
: t(o.t), d(o.d), asWritten(o.asWritten)
{
}

QDeclarativeScript::Variant::Variant(bool v)
: t(Boolean), b(v)
{
}

QDeclarativeScript::Variant::Variant(double v, const QStringRef &asWritten)
: t(Number), d(v), asWritten(asWritten)
{
}

QDeclarativeScript::Variant::Variant(QDeclarativeJS::AST::StringLiteral *v)
: t(String), l(v)
{
}

QDeclarativeScript::Variant::Variant(const QStringRef &asWritten, QDeclarativeJS::AST::Node *n)
: t(Script), n(n), asWritten(asWritten)
{
}

QDeclarativeScript::Variant &QDeclarativeScript::Variant::operator=(const Variant &o)
{
    t = o.t;
    d = o.d;
    asWritten = o.asWritten;
    return *this;
}

QDeclarativeScript::Variant::Type QDeclarativeScript::Variant::type() const
{
    return t;
}

bool QDeclarativeScript::Variant::asBoolean() const
{
    return b;
}

QString QDeclarativeScript::Variant::asString() const
{
    if (t == String) {
        return l->value.toString();
    } else {
        return asWritten.toString();
    }
}

double QDeclarativeScript::Variant::asNumber() const
{
    return d;
}

//reverse of Lexer::singleEscape()
QString escapedString(const QString &string)
{
    QString tmp = QLatin1String("\"");
    for (int i = 0; i < string.length(); ++i) {
        const QChar &c = string.at(i);
        switch(c.unicode()) {
        case 0x08:
            tmp += QLatin1String("\\b");
            break;
        case 0x09:
            tmp += QLatin1String("\\t");
            break;
        case 0x0A:
            tmp += QLatin1String("\\n");
            break;
        case 0x0B:
            tmp += QLatin1String("\\v");
            break;
        case 0x0C:
            tmp += QLatin1String("\\f");
            break;
        case 0x0D:
            tmp += QLatin1String("\\r");
            break;
        case 0x22:
            tmp += QLatin1String("\\\"");
            break;
        case 0x27:
            tmp += QLatin1String("\\\'");
            break;
        case 0x5C:
            tmp += QLatin1String("\\\\");
            break;
        default:
            tmp += c;
            break;
        }
    }
    tmp += QLatin1Char('\"');
    return tmp;
}

QString QDeclarativeScript::Variant::asScript() const
{
    switch(type()) { 
    default:
    case Invalid:
        return QString();
    case Boolean:
        return b?QLatin1String("true"):QLatin1String("false");
    case Number:
        if (asWritten.isEmpty())
            return QString::number(d);
        else 
            return asWritten.toString();
    case String:
        return escapedString(asString());
    case Script:
        if (AST::IdentifierExpression *i = AST::cast<AST::IdentifierExpression *>(n)) {
            return i->name.toString();
        } else
            return asWritten.toString();
    }
}

QDeclarativeJS::AST::Node *QDeclarativeScript::Variant::asAST() const
{
    if (type() == Script)
        return n;
    else
        return 0;
}

bool QDeclarativeScript::Variant::isStringList() const
{
    if (isString())
        return true;

    if (type() != Script || !n)
        return false;

    AST::ArrayLiteral *array = AST::cast<AST::ArrayLiteral *>(n);
    if (!array)
        return false;

    AST::ElementList *elements = array->elements;

    while (elements) {

        if (!AST::cast<AST::StringLiteral *>(elements->expression))
            return false;

        elements = elements->next;
    }

    return true;
}

QStringList QDeclarativeScript::Variant::asStringList() const
{
    QStringList rv;
    if (isString()) {
        rv << asString();
        return rv;
    }

    AST::ArrayLiteral *array = AST::cast<AST::ArrayLiteral *>(n);
    if (!array)
        return rv;

    AST::ElementList *elements = array->elements;
    while (elements) {

        AST::StringLiteral *string = AST::cast<AST::StringLiteral *>(elements->expression);
        if (!string)
            return QStringList();
        rv.append(string->value.toString());

        elements = elements->next;
    }

    return  rv;
}

//
// Actual parser classes
//
void QDeclarativeScript::Import::extractVersion(int *maj, int *min) const
{
    *maj = -1; *min = -1;

    if (!version.isEmpty()) {
        int dot = version.indexOf(QLatin1Char('.'));
        if (dot < 0) {
            *maj = version.toInt();
            *min = 0;
        } else {
            *maj = version.left(dot).toInt();
            *min = version.mid(dot+1).toInt();
        }
    }
}

namespace {

class ProcessAST: protected AST::Visitor
{
    struct State {
        State() : object(0), property(0) {}
        State(QDeclarativeScript::Object *o) : object(o), property(0) {}
        State(QDeclarativeScript::Object *o, Property *p) : object(o), property(p) {}

        QDeclarativeScript::Object *object;
        Property *property;
    };

    struct StateStack : public QStack<State>
    {
        void pushObject(QDeclarativeScript::Object *obj)
        {
            push(State(obj));
        }

        void pushProperty(const QString &name, const LocationSpan &location)
        {
            const State &state = top();
            if (state.property) {
                State s(state.property->getValue(location),
                        state.property->getValue(location)->getProperty(name));
                s.property->location = location;
                push(s);
            } else {
                State s(state.object, state.object->getProperty(name));

                s.property->location = location;
                push(s);
            }
        }

        void pushProperty(const QStringRef &name, const LocationSpan &location)
        {
            const State &state = top();
            if (state.property) {
                State s(state.property->getValue(location),
                        state.property->getValue(location)->getProperty(name));
                s.property->location = location;
                push(s);
            } else {
                State s(state.object, state.object->getProperty(name));

                s.property->location = location;
                push(s);
            }
        }
    };

public:
    ProcessAST(QDeclarativeScript::Parser *parser);
    virtual ~ProcessAST();

    void operator()(const QString &code, AST::Node *node);

protected:

    QDeclarativeScript::Object *defineObjectBinding(AST::UiQualifiedId *propertyName, bool onAssignment,
                                const QString &objectType,
                                AST::SourceLocation typeLocation,
                                LocationSpan location,
                                AST::UiObjectInitializer *initializer = 0);

    QDeclarativeScript::Variant getVariant(AST::Statement *stmt);
    QDeclarativeScript::Variant getVariant(AST::ExpressionNode *expr);

    LocationSpan location(AST::SourceLocation start, AST::SourceLocation end);
    LocationSpan location(AST::UiQualifiedId *);

    using AST::Visitor::visit;
    using AST::Visitor::endVisit;

    virtual bool visit(AST::UiProgram *node);
    virtual bool visit(AST::UiImport *node);
    virtual bool visit(AST::UiObjectDefinition *node);
    virtual bool visit(AST::UiPublicMember *node);
    virtual bool visit(AST::UiObjectBinding *node);

    virtual bool visit(AST::UiScriptBinding *node);
    virtual bool visit(AST::UiArrayBinding *node);
    virtual bool visit(AST::UiSourceElement *node);

    void accept(AST::Node *node);

    QString asString(AST::UiQualifiedId *node) const;

    const State state() const;
    QDeclarativeScript::Object *currentObject() const;
    Property *currentProperty() const;

    QString qualifiedNameId() const;

    QString textAt(const AST::SourceLocation &loc) const
    { return _contents->mid(loc.offset, loc.length); }

    QStringRef textRefAt(const AST::SourceLocation &loc) const
    { return QStringRef(_contents, loc.offset, loc.length); }

    QString textAt(const AST::SourceLocation &first,
                   const AST::SourceLocation &last) const
    { return _contents->mid(first.offset, last.offset + last.length - first.offset); }

    QStringRef textRefAt(const AST::SourceLocation &first,
                         const AST::SourceLocation &last) const
    { return QStringRef(_contents, first.offset, last.offset + last.length - first.offset); }

    QString asString(AST::ExpressionNode *expr)
    {
        if (! expr)
            return QString();

        return textAt(expr->firstSourceLocation(), expr->lastSourceLocation());
    }

    QStringRef asStringRef(AST::ExpressionNode *expr)
    {
        if (! expr)
            return QStringRef();

        return textRefAt(expr->firstSourceLocation(), expr->lastSourceLocation());
    }

    QString asString(AST::Statement *stmt)
    {
        if (! stmt)
            return QString();

        QString s = textAt(stmt->firstSourceLocation(), stmt->lastSourceLocation());
        s += QLatin1Char('\n');
        return s;
    }

    QStringRef asStringRef(AST::Statement *stmt)
    {
        if (! stmt)
            return QStringRef();

        return textRefAt(stmt->firstSourceLocation(), stmt->lastSourceLocation());
    }

private:
    QDeclarativeScript::Parser *_parser;
    StateStack _stateStack;
    QStringList _scope;
    const QString *_contents;
};

ProcessAST::ProcessAST(QDeclarativeScript::Parser *parser)
    : _parser(parser)
{
}

ProcessAST::~ProcessAST()
{
}

void ProcessAST::operator()(const QString &code, AST::Node *node)
{
    _contents = &code;
    accept(node);
}

void ProcessAST::accept(AST::Node *node)
{
    AST::Node::acceptChild(node, this);
}

const ProcessAST::State ProcessAST::state() const
{
    if (_stateStack.isEmpty())
        return State();

    return _stateStack.back();
}

QDeclarativeScript::Object *ProcessAST::currentObject() const
{
    return state().object;
}

Property *ProcessAST::currentProperty() const
{
    return state().property;
}

QString ProcessAST::qualifiedNameId() const
{
    return _scope.join(QLatin1String("/"));
}

QString ProcessAST::asString(AST::UiQualifiedId *node) const
{
    QString s;

    for (AST::UiQualifiedId *it = node; it; it = it->next) {
        s.append(it->name.toString());

        if (it->next)
            s.append(QLatin1Char('.'));
    }

    return s;
}

QDeclarativeScript::Object *
ProcessAST::defineObjectBinding(AST::UiQualifiedId *propertyName,
                                bool onAssignment,
                                const QString &objectType,
                                AST::SourceLocation typeLocation,
                                LocationSpan location,
                                AST::UiObjectInitializer *initializer)
{
    int lastTypeDot = objectType.lastIndexOf(QLatin1Char('.'));
    bool isType = !objectType.isEmpty() &&
                    (objectType.at(0).isUpper() ||
                        (lastTypeDot >= 0 && objectType.at(lastTypeDot+1).isUpper()));

    int propertyCount = 0;
    for (AST::UiQualifiedId *name = propertyName; name; name = name->next){
        ++propertyCount;
        _stateStack.pushProperty(name->name,
                                 this->location(name));
    }

    if (!onAssignment && propertyCount && currentProperty() && !currentProperty()->values.isEmpty()) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Property value set multiple times"));
        error.setLine(this->location(propertyName).start.line);
        error.setColumn(this->location(propertyName).start.column);
        _parser->_errors << error;
        return 0;
    }

    if (!isType) {

        if(propertyCount || !currentObject()) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Expected type name"));
            error.setLine(typeLocation.startLine);
            error.setColumn(typeLocation.startColumn);
            _parser->_errors << error;
            return 0;
        }

        LocationSpan loc = ProcessAST::location(typeLocation, typeLocation);
        if (propertyName)
            loc = ProcessAST::location(propertyName);

        _stateStack.pushProperty(objectType, loc);
       accept(initializer);
        _stateStack.pop();

        return 0;

    } else {
        // Class

        QString resolvableObjectType = objectType;
        if (lastTypeDot >= 0)
            resolvableObjectType.replace(QLatin1Char('.'),QLatin1Char('/'));

	QDeclarativeScript::Object *obj = _parser->_pool.New<QDeclarativeScript::Object>();

        QDeclarativeScript::TypeReference *typeRef = _parser->findOrCreateType(resolvableObjectType);
        obj->type = typeRef->id;

        typeRef->refObjects.append(obj);

        // XXX this doesn't do anything (_scope never builds up)
        _scope.append(resolvableObjectType);
        obj->typeName = qualifiedNameId();
        _scope.removeLast();

        obj->location = location;

        if (propertyCount) {
            Property *prop = currentProperty();
            QDeclarativeScript::Value *v = _parser->_pool.New<QDeclarativeScript::Value>();
            v->object = obj;
            v->location = obj->location;
            if (onAssignment)
                prop->addOnValue(v);
            else
                prop->addValue(v);

            while (propertyCount--)
                _stateStack.pop();

        } else {

            if (! _parser->tree()) {
                _parser->setTree(obj);
            } else {
                const State state = _stateStack.top();
                QDeclarativeScript::Value *v = _parser->_pool.New<QDeclarativeScript::Value>();
                v->object = obj;
                v->location = obj->location;
                if (state.property) {
                    state.property->addValue(v);
                } else {
                    Property *defaultProp = state.object->getDefaultProperty();
                    if (defaultProp->location.start.line == -1) {
                        defaultProp->location = v->location;
                        defaultProp->location.end = defaultProp->location.start;
                        defaultProp->location.range.length = 0;
                    }
                    defaultProp->addValue(v);
                }
            }
        }

        _stateStack.pushObject(obj);
        accept(initializer);
        _stateStack.pop();

        return obj;
    }
}

LocationSpan ProcessAST::location(AST::UiQualifiedId *id)
{
    return location(id->identifierToken, id->identifierToken);
}

LocationSpan ProcessAST::location(AST::SourceLocation start, AST::SourceLocation end)
{
    LocationSpan rv;
    rv.start.line = start.startLine;
    rv.start.column = start.startColumn;
    rv.end.line = end.startLine;
    rv.end.column = end.startColumn + end.length - 1;
    rv.range.offset = start.offset;
    rv.range.length = end.offset + end.length - start.offset;
    return rv;
}

// UiProgram: UiImportListOpt UiObjectMemberList ;
bool ProcessAST::visit(AST::UiProgram *node)
{
    accept(node->imports);
    accept(node->members->member);
    return false;
}

// UiImport: T_IMPORT T_STRING_LITERAL ;
bool ProcessAST::visit(AST::UiImport *node)
{
    QString uri;
    QDeclarativeScript::Import import;

    if (!node->fileName.isNull()) {
        uri = node->fileName.toString();

        if (uri.endsWith(QLatin1String(".js"))) {
            import.type = QDeclarativeScript::Import::Script;
        } else {
            import.type = QDeclarativeScript::Import::File;
        }
    } else {
        import.type = QDeclarativeScript::Import::Library;
        uri = asString(node->importUri);
    }

    AST::SourceLocation startLoc = node->importToken;
    AST::SourceLocation endLoc = node->semicolonToken;

    // Qualifier
    if (!node->importId.isNull()) {
        import.qualifier = node->importId.toString();
        if (!import.qualifier.at(0).isUpper()) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Invalid import qualifier ID"));
            error.setLine(node->importIdToken.startLine);
            error.setColumn(node->importIdToken.startColumn);
            _parser->_errors << error;
            return false;
        }
        if (import.qualifier == QLatin1String("Qt")) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Reserved name \"Qt\" cannot be used as an qualifier"));
            error.setLine(node->importIdToken.startLine);
            error.setColumn(node->importIdToken.startColumn);
            _parser->_errors << error;
            return false;
        }

        // Check for script qualifier clashes
        bool isScript = import.type == QDeclarativeScript::Import::Script;
        for (int ii = 0; ii < _parser->_imports.count(); ++ii) {
            const QDeclarativeScript::Import &other = _parser->_imports.at(ii);
            bool otherIsScript = other.type == QDeclarativeScript::Import::Script;

            if ((isScript || otherIsScript) && import.qualifier == other.qualifier) {
                QDeclarativeError error;
                error.setDescription(QCoreApplication::translate("QDeclarativeParser","Script import qualifiers must be unique."));
                error.setLine(node->importIdToken.startLine);
                error.setColumn(node->importIdToken.startColumn);
                _parser->_errors << error;
                return false;
            }
        }

    } else if (import.type == QDeclarativeScript::Import::Script) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Script import requires a qualifier"));
        error.setLine(node->fileNameToken.startLine);
        error.setColumn(node->fileNameToken.startColumn);
        _parser->_errors << error;
        return false;
    }

    if (node->versionToken.isValid()) {
        import.version = textAt(node->versionToken);
    } else if (import.type == QDeclarativeScript::Import::Library) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Library import requires a version"));
        error.setLine(node->importIdToken.startLine);
        error.setColumn(node->importIdToken.startColumn);
        _parser->_errors << error;
        return false;
    }


    import.location = location(startLoc, endLoc);
    import.uri = uri;

    _parser->_imports << import;

    return false;
}

bool ProcessAST::visit(AST::UiPublicMember *node)
{
    static const struct TypeNameToType {
        const char *name;
        int nameLength;
        Object::DynamicProperty::Type type;
        const char *qtName;
        int qtNameLength;
    } propTypeNameToTypes[] = {
        { "int", strlen("int"), Object::DynamicProperty::Int, "int", strlen("int") },
        { "bool", strlen("bool"), Object::DynamicProperty::Bool, "bool", strlen("bool") },
        { "double", strlen("double"), Object::DynamicProperty::Real, "double", strlen("double") },
        { "real", strlen("real"), Object::DynamicProperty::Real, "qreal", strlen("qreal") },
        { "string", strlen("string"), Object::DynamicProperty::String, "QString", strlen("QString") },
        { "url", strlen("url"), Object::DynamicProperty::Url, "QUrl", strlen("QUrl") },
        { "color", strlen("color"), Object::DynamicProperty::Color, "QColor", strlen("QColor") },
        // Internally QTime, QDate and QDateTime are all supported.
        // To be more consistent with JavaScript we expose only
        // QDateTime as it matches closely with the Date JS type.
        // We also call it "date" to match.
        // { "time", strlen("time"), Object::DynamicProperty::Time, "QTime", strlen("QTime") },
        // { "date", strlen("date"), Object::DynamicProperty::Date, "QDate", strlen("QDate") },
        { "date", strlen("date"), Object::DynamicProperty::DateTime, "QDateTime", strlen("QDateTime") },
        { "variant", strlen("variant"), Object::DynamicProperty::Variant, "QVariant", strlen("QVariant") },
        { "var", strlen("var"), Object::DynamicProperty::Var, "QVariant", strlen("QVariant") }
    };
    static const int propTypeNameToTypesCount = sizeof(propTypeNameToTypes) /
                                                sizeof(propTypeNameToTypes[0]);

    if(node->type == AST::UiPublicMember::Signal) {
        Object::DynamicSignal *signal = _parser->_pool.New<Object::DynamicSignal>();
        signal->name = node->name;

        AST::UiParameterList *p = node->parameters;
        int paramLength = 0;
        while (p) { paramLength++; p = p->next; }
        p = node->parameters;

        if (paramLength) {
            signal->parameterTypes = _parser->_pool.NewRawList<QHashedCStringRef>(paramLength);
            signal->parameterNames = _parser->_pool.NewRawList<QHashedStringRef>(paramLength);
        }

        int index = 0;
        while (p) {
            const QStringRef &memberType = p->type;

            const TypeNameToType *type = 0;
            for(int typeIndex = 0; typeIndex < propTypeNameToTypesCount; ++typeIndex) {
                const TypeNameToType *t = propTypeNameToTypes + typeIndex;
                if (t->nameLength == memberType.length() && 
                    QHashedString::compare(memberType.constData(), t->name, t->nameLength)) {
                    type = t;
                    break;
                }
            }

            if (!type) {
                QDeclarativeError error;
                error.setDescription(QCoreApplication::translate("QDeclarativeParser","Expected parameter type"));
                error.setLine(node->typeToken.startLine);
                error.setColumn(node->typeToken.startColumn);
                _parser->_errors << error;
                return false;
            }
            
            signal->parameterTypes[index] = QHashedCStringRef(type->qtName, type->qtNameLength);
            signal->parameterNames[index] = QHashedStringRef(p->name);
            p = p->next;
            index++;
        }

        signal->location = location(node->typeToken, node->semicolonToken);
        _stateStack.top().object->dynamicSignals.append(signal);
    } else {
        const QStringRef &memberType = node->memberType;
        const QStringRef &name = node->name;

        bool typeFound = false;
        Object::DynamicProperty::Type type;

        if ((unsigned)memberType.length() == strlen("alias") && 
            QHashedString::compare(memberType.constData(), "alias", strlen("alias"))) {
            type = Object::DynamicProperty::Alias;
            typeFound = true;
        } 

        for(int ii = 0; !typeFound && ii < propTypeNameToTypesCount; ++ii) {
            const TypeNameToType *t = propTypeNameToTypes + ii;
            if (t->nameLength == memberType.length() && 
                QHashedString::compare(memberType.constData(), t->name, t->nameLength)) {
                type = t->type;
                typeFound = true;
            }
        }

        if (!typeFound && memberType.at(0).isUpper()) {
            const QStringRef &typeModifier = node->typeModifier;

            if (typeModifier.isEmpty()) {
                type = Object::DynamicProperty::Custom;
            } else if((unsigned)typeModifier.length() == strlen("list") && 
                      QHashedString::compare(typeModifier.constData(), "list", strlen("list"))) {
                type = Object::DynamicProperty::CustomList;
            } else {
                QDeclarativeError error;
                error.setDescription(QCoreApplication::translate("QDeclarativeParser","Invalid property type modifier"));
                error.setLine(node->typeModifierToken.startLine);
                error.setColumn(node->typeModifierToken.startColumn);
                _parser->_errors << error;
                return false;
            }
            typeFound = true;
        } else if (!node->typeModifier.isNull()) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Unexpected property type modifier"));
            error.setLine(node->typeModifierToken.startLine);
            error.setColumn(node->typeModifierToken.startColumn);
            _parser->_errors << error;
            return false;
        }

        if(!typeFound) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Expected property type"));
            error.setLine(node->typeToken.startLine);
            error.setColumn(node->typeToken.startColumn);
            _parser->_errors << error;
            return false;
        }

        Object::DynamicProperty *property = _parser->_pool.New<Object::DynamicProperty>();
        property->isDefaultProperty = node->isDefaultMember;
        property->isReadOnly = node->isReadonlyMember;
        property->type = type;
        if (type >= Object::DynamicProperty::Custom) {
            QDeclarativeScript::TypeReference *typeRef =
                _parser->findOrCreateType(memberType.toString());
            typeRef->refObjects.append(_stateStack.top().object);
            property->customType = memberType;
        }

        property->name = QHashedStringRef(name);
        property->location = location(node->firstSourceLocation(),
                                      node->lastSourceLocation());

        if (node->statement) { // default value
            property->defaultValue = _parser->_pool.New<Property>();
            property->defaultValue->parent = _stateStack.top().object;
            property->defaultValue->location =
                    location(node->statement->firstSourceLocation(),
                             node->statement->lastSourceLocation());
            QDeclarativeScript::Value *value = _parser->_pool.New<QDeclarativeScript::Value>();
            value->location = location(node->statement->firstSourceLocation(),
                                       node->statement->lastSourceLocation());
            value->value = getVariant(node->statement);
            property->defaultValue->values.append(value);
        }

        _stateStack.top().object->dynamicProperties.append(property);

        // process QML-like initializers (e.g. property Object o: Object {})
        accept(node->binding);
    }

    return false;
}


// UiObjectMember: UiQualifiedId UiObjectInitializer ;
bool ProcessAST::visit(AST::UiObjectDefinition *node)
{
    LocationSpan l = location(node->firstSourceLocation(),
                              node->lastSourceLocation());

    const QString objectType = asString(node->qualifiedTypeNameId);
    const AST::SourceLocation typeLocation = node->qualifiedTypeNameId->identifierToken;

    defineObjectBinding(/*propertyName = */ 0, false, objectType,
                        typeLocation, l, node->initializer);

    return false;
}


// UiObjectMember: UiQualifiedId T_COLON UiQualifiedId UiObjectInitializer ;
bool ProcessAST::visit(AST::UiObjectBinding *node)
{
    LocationSpan l = location(node->qualifiedTypeNameId->identifierToken,
                              node->initializer->rbraceToken);

    const QString objectType = asString(node->qualifiedTypeNameId);
    const AST::SourceLocation typeLocation = node->qualifiedTypeNameId->identifierToken;

    defineObjectBinding(node->qualifiedId, node->hasOnToken, objectType, 
                        typeLocation, l, node->initializer);

    return false;
}

QDeclarativeScript::Variant ProcessAST::getVariant(AST::Statement *stmt)
{
    if (stmt) {
        if (AST::ExpressionStatement *exprStmt = AST::cast<AST::ExpressionStatement *>(stmt))
            return getVariant(exprStmt->expression);

        return QDeclarativeScript::Variant(asStringRef(stmt), stmt);
    }

    return QDeclarativeScript::Variant();
}

QDeclarativeScript::Variant ProcessAST::getVariant(AST::ExpressionNode *expr)
{
    if (AST::StringLiteral *lit = AST::cast<AST::StringLiteral *>(expr)) {
        return QDeclarativeScript::Variant(lit);
    } else if (expr->kind == AST::Node::Kind_TrueLiteral) {
        return QDeclarativeScript::Variant(true);
    } else if (expr->kind == AST::Node::Kind_FalseLiteral) {
        return QDeclarativeScript::Variant(false);
    } else if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(expr)) {
        return QDeclarativeScript::Variant(lit->value, asStringRef(expr));
    } else {

        if (AST::UnaryMinusExpression *unaryMinus = AST::cast<AST::UnaryMinusExpression *>(expr)) {
           if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(unaryMinus->expression)) {
               return QDeclarativeScript::Variant(-lit->value, asStringRef(expr));
           }
        }

        return  QDeclarativeScript::Variant(asStringRef(expr), expr);
    }
}


// UiObjectMember: UiQualifiedId T_COLON Statement ;
bool ProcessAST::visit(AST::UiScriptBinding *node)
{
    int propertyCount = 0;
    AST::UiQualifiedId *propertyName = node->qualifiedId;
    for (AST::UiQualifiedId *name = propertyName; name; name = name->next){
        ++propertyCount;
        _stateStack.pushProperty(name->name,
                                 location(name));
    }

    Property *prop = currentProperty();

    if (!prop->values.isEmpty()) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Property value set multiple times"));
        error.setLine(this->location(propertyName).start.line);
        error.setColumn(this->location(propertyName).start.column);
        _parser->_errors << error;
        return 0;
    }

    QDeclarativeScript::Variant primitive;

    if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement *>(node->statement)) {
        primitive = getVariant(stmt->expression);
    } else { // do binding
        primitive = QDeclarativeScript::Variant(asStringRef(node->statement), node->statement);
    }

    prop->location.range.length = prop->location.range.offset + prop->location.range.length - node->qualifiedId->identifierToken.offset;
    prop->location.range.offset = node->qualifiedId->identifierToken.offset;
    QDeclarativeScript::Value *v = _parser->_pool.New<QDeclarativeScript::Value>();
    v->value = primitive;
    v->location = location(node->statement->firstSourceLocation(),
                           node->statement->lastSourceLocation());

    prop->addValue(v);

    while (propertyCount--)
        _stateStack.pop();

    return false;
}

// UiObjectMember: UiQualifiedId T_COLON T_LBRACKET UiArrayMemberList T_RBRACKET ;
bool ProcessAST::visit(AST::UiArrayBinding *node)
{
    int propertyCount = 0;
    AST::UiQualifiedId *propertyName = node->qualifiedId;
    for (AST::UiQualifiedId *name = propertyName; name; name = name->next){
        ++propertyCount;
        _stateStack.pushProperty(name->name,
                                 location(name));
    }

    Property* prop = currentProperty();

    if (!prop->values.isEmpty()) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Property value set multiple times"));
        error.setLine(this->location(propertyName).start.line);
        error.setColumn(this->location(propertyName).start.column);
        _parser->_errors << error;
        return false;
    }

    accept(node->members);

    // For the DOM, store the position of the T_LBRACKET upto the T_RBRACKET as the range:
    prop->listValueRange.offset = node->lbracketToken.offset;
    prop->listValueRange.length = node->rbracketToken.offset + node->rbracketToken.length - node->lbracketToken.offset;

    while (propertyCount--)
        _stateStack.pop();

    return false;
}

bool ProcessAST::visit(AST::UiSourceElement *node)
{
    QDeclarativeScript::Object *obj = currentObject();

    if (AST::FunctionDeclaration *funDecl = AST::cast<AST::FunctionDeclaration *>(node->sourceElement)) {

        Object::DynamicSlot *slot = _parser->_pool.New<Object::DynamicSlot>();
        slot->location = location(funDecl->firstSourceLocation(), funDecl->lastSourceLocation());

        AST::FormalParameterList *f = funDecl->formals;
        while (f) {
            slot->parameterNames << f->name.toUtf8();
            f = f->next;
        }

        AST::SourceLocation loc = funDecl->rparenToken;
        loc.offset = loc.end();
        loc.startColumn += 1;
        QString body = textAt(loc, funDecl->rbraceToken);
        slot->name = funDecl->name;
        slot->body = body;
        obj->dynamicSlots.append(slot);

    } else {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","JavaScript declaration outside Script element"));
        error.setLine(node->firstSourceLocation().startLine);
        error.setColumn(node->firstSourceLocation().startColumn);
        _parser->_errors << error;
    }
    return false;
}

} // end of anonymous namespace


QDeclarativeScript::Parser::Parser()
: root(0), data(0)
{

}

QDeclarativeScript::Parser::~Parser()
{
    clear();
}

namespace QDeclarativeScript {
class ParserJsASTData
{
public:
    ParserJsASTData(const QString &filename)
    : filename(filename) {}

    QString filename;
    Engine engine;
};
}

bool QDeclarativeScript::Parser::parse(const QByteArray &qmldata, const QUrl &url)
{
    clear();

    const QString fileName = url.toString();
    _scriptFile = fileName;

    QTextStream stream(qmldata, QIODevice::ReadOnly);
#ifndef QT_NO_TEXTCODEC
    stream.setCodec("UTF-8");
#endif
    QString *code = _pool.NewString(stream.readAll());

    data = new QDeclarativeScript::ParserJsASTData(fileName);

    Lexer lexer(&data->engine);
    lexer.setCode(*code, /*line = */ 1);

    QDeclarativeJS::Parser parser(&data->engine);

    if (! parser.parse() || !_errors.isEmpty()) {

        // Extract errors from the parser
        foreach (const DiagnosticMessage &m, parser.diagnosticMessages()) {

            if (m.isWarning())
                continue;

            QDeclarativeError error;
            error.setUrl(url);
            error.setDescription(m.message);
            error.setLine(m.loc.startLine);
            error.setColumn(m.loc.startColumn);
            _errors << error;

        }
    }

    if (_errors.isEmpty()) {
        ProcessAST process(this);
        process(*code, parser.ast());

        // Set the url for process errors
        for(int ii = 0; ii < _errors.count(); ++ii)
            _errors[ii].setUrl(url);
    }

    return _errors.isEmpty();
}

QList<QDeclarativeScript::TypeReference*> QDeclarativeScript::Parser::referencedTypes() const
{
    return _refTypes;
}

QDeclarativeScript::Object *QDeclarativeScript::Parser::tree() const
{
    return root;
}

QList<QDeclarativeScript::Import> QDeclarativeScript::Parser::imports() const
{
    return _imports;
}

QList<QDeclarativeError> QDeclarativeScript::Parser::errors() const
{
    return _errors;
}

static void replaceWithSpace(QString &str, int idx, int n) 
{
    QChar *data = str.data() + idx;
    const QChar space(QLatin1Char(' '));
    for (int ii = 0; ii < n; ++ii)
        *data++ = space;
}

static QDeclarativeScript::LocationSpan
locationFromLexer(const QDeclarativeJS::Lexer &lex, int startLine, int startColumn, int startOffset)
{
    QDeclarativeScript::LocationSpan l;

    l.start.line = startLine; l.start.column = startColumn;
    l.end.line = lex.tokenEndLine(); l.end.column = lex.tokenEndColumn();
    l.range.offset = startOffset;
    l.range.length = lex.tokenOffset() + lex.tokenLength() - startOffset;

    return l;
}

/*
Searches for ".pragma <value>" declarations within \a script.  Currently supported pragmas
are:
    library
*/
QDeclarativeScript::Object::ScriptBlock::Pragmas QDeclarativeScript::Parser::extractPragmas(QString &script)
{
    QDeclarativeScript::Object::ScriptBlock::Pragmas rv = QDeclarativeScript::Object::ScriptBlock::None;

    const QString pragma(QLatin1String("pragma"));
    const QString library(QLatin1String("library"));

    QDeclarativeJS::Lexer l(0);
    l.setCode(script, 0);

    int token = l.lex();

    while (true) {
        if (token != QDeclarativeJSGrammar::T_DOT)
            return rv;

        int startOffset = l.tokenOffset();
        int startLine = l.tokenStartLine();

        token = l.lex();

        if (token != QDeclarativeJSGrammar::T_IDENTIFIER ||
            l.tokenStartLine() != startLine ||
            script.mid(l.tokenOffset(), l.tokenLength()) != pragma)
            return rv;

        token = l.lex();

        if (token != QDeclarativeJSGrammar::T_IDENTIFIER ||
            l.tokenStartLine() != startLine)
            return rv;

        QString pragmaValue = script.mid(l.tokenOffset(), l.tokenLength());
        int endOffset = l.tokenLength() + l.tokenOffset();

        token = l.lex();
        if (l.tokenStartLine() == startLine)
            return rv;

        if (pragmaValue == library) {
            rv |= QDeclarativeScript::Object::ScriptBlock::Shared;
            replaceWithSpace(script, startOffset, endOffset - startOffset);
        } else {
            return rv;
        }
    }
    return rv;
}

#define CHECK_LINE if (l.tokenStartLine() != startLine) return rv;
#define CHECK_TOKEN(t) if (token != QDeclarativeJSGrammar:: t) return rv;

static const int uriTokens[] = {
    QDeclarativeJSGrammar::T_IDENTIFIER, 
    QDeclarativeJSGrammar::T_PROPERTY, 
    QDeclarativeJSGrammar::T_SIGNAL, 
    QDeclarativeJSGrammar::T_READONLY, 
    QDeclarativeJSGrammar::T_ON, 
    QDeclarativeJSGrammar::T_BREAK, 
    QDeclarativeJSGrammar::T_CASE, 
    QDeclarativeJSGrammar::T_CATCH, 
    QDeclarativeJSGrammar::T_CONTINUE, 
    QDeclarativeJSGrammar::T_DEFAULT, 
    QDeclarativeJSGrammar::T_DELETE, 
    QDeclarativeJSGrammar::T_DO, 
    QDeclarativeJSGrammar::T_ELSE, 
    QDeclarativeJSGrammar::T_FALSE, 
    QDeclarativeJSGrammar::T_FINALLY, 
    QDeclarativeJSGrammar::T_FOR, 
    QDeclarativeJSGrammar::T_FUNCTION, 
    QDeclarativeJSGrammar::T_IF, 
    QDeclarativeJSGrammar::T_IN, 
    QDeclarativeJSGrammar::T_INSTANCEOF, 
    QDeclarativeJSGrammar::T_NEW, 
    QDeclarativeJSGrammar::T_NULL, 
    QDeclarativeJSGrammar::T_RETURN, 
    QDeclarativeJSGrammar::T_SWITCH, 
    QDeclarativeJSGrammar::T_THIS, 
    QDeclarativeJSGrammar::T_THROW, 
    QDeclarativeJSGrammar::T_TRUE, 
    QDeclarativeJSGrammar::T_TRY, 
    QDeclarativeJSGrammar::T_TYPEOF, 
    QDeclarativeJSGrammar::T_VAR, 
    QDeclarativeJSGrammar::T_VOID, 
    QDeclarativeJSGrammar::T_WHILE, 
    QDeclarativeJSGrammar::T_CONST, 
    QDeclarativeJSGrammar::T_DEBUGGER, 
    QDeclarativeJSGrammar::T_RESERVED_WORD, 
    QDeclarativeJSGrammar::T_WITH, 

    QDeclarativeJSGrammar::EOF_SYMBOL
};
static inline bool isUriToken(int token)
{
    const int *current = uriTokens;
    while (*current != QDeclarativeJSGrammar::EOF_SYMBOL) {
        if (*current == token)
            return true;
        ++current;
    }
    return false;
}

QDeclarativeScript::Parser::JavaScriptMetaData QDeclarativeScript::Parser::extractMetaData(QString &script)
{
    JavaScriptMetaData rv;

    QDeclarativeScript::Object::ScriptBlock::Pragmas &pragmas = rv.pragmas;

    const QString pragma(QLatin1String("pragma"));
    const QString js(QLatin1String(".js"));
    const QString library(QLatin1String("library"));

    QDeclarativeJS::Lexer l(0);
    l.setCode(script, 0);

    int token = l.lex();

    while (true) {
        if (token != QDeclarativeJSGrammar::T_DOT)
            return rv;

        int startOffset = l.tokenOffset();
        int startLine = l.tokenStartLine();
        int startColumn = l.tokenStartColumn();

        token = l.lex();

        CHECK_LINE;

        if (token == QDeclarativeJSGrammar::T_IMPORT) {

            // .import <URI> <Version> as <Identifier>
            // .import <file.js> as <Identifier>

            token = l.lex();

            CHECK_LINE;

            if (token == QDeclarativeJSGrammar::T_STRING_LITERAL) {

                QString file = l.tokenText();

                if (!file.endsWith(js))
                    return rv;

                token = l.lex();

                CHECK_TOKEN(T_AS);
                CHECK_LINE;

                token = l.lex();

                CHECK_TOKEN(T_IDENTIFIER);
                CHECK_LINE;

                int endOffset = l.tokenLength() + l.tokenOffset();

                QString importId = script.mid(l.tokenOffset(), l.tokenLength());

                if (!importId.at(0).isUpper())
                    return rv;

                QDeclarativeScript::LocationSpan location =
                    locationFromLexer(l, startLine, startColumn, startOffset);

                token = l.lex();
                if (l.tokenStartLine() == startLine)
                    return rv;

                replaceWithSpace(script, startOffset, endOffset - startOffset);

                Import import;
                import.type = Import::Script;
                import.uri = file;
                import.qualifier = importId;
                import.location = location;

                rv.imports << import;
            } else {
                // URI
                QString uri;
                QString version;

                while (true) {
                    if (!isUriToken(token))
                        return rv;

                    uri.append(l.tokenText());

                    token = l.lex();
                    CHECK_LINE;
                    if (token != QDeclarativeJSGrammar::T_DOT)
                        break;

                    uri.append(QLatin1Char('.'));

                    token = l.lex();
                    CHECK_LINE;
                }

                CHECK_TOKEN(T_NUMERIC_LITERAL);
                version = script.mid(l.tokenOffset(), l.tokenLength());

                token = l.lex();

                CHECK_TOKEN(T_AS);
                CHECK_LINE;

                token = l.lex();

                CHECK_TOKEN(T_IDENTIFIER);
                CHECK_LINE;

                int endOffset = l.tokenLength() + l.tokenOffset();

                QString importId = script.mid(l.tokenOffset(), l.tokenLength());

                if (!importId.at(0).isUpper())
                    return rv;

                QDeclarativeScript::LocationSpan location =
                    locationFromLexer(l, startLine, startColumn, startOffset);

                token = l.lex();
                if (l.tokenStartLine() == startLine)
                    return rv;

                replaceWithSpace(script, startOffset, endOffset - startOffset);

                Import import;
                import.type = Import::Library;
                import.uri = uri;
                import.version = version;
                import.qualifier = importId;
                import.location = location;

                rv.imports << import;
            }

        } else if (token == QDeclarativeJSGrammar::T_IDENTIFIER &&
                   script.mid(l.tokenOffset(), l.tokenLength()) == pragma) {

            token = l.lex();

            CHECK_TOKEN(T_IDENTIFIER);
            CHECK_LINE;

            QString pragmaValue = script.mid(l.tokenOffset(), l.tokenLength());
            int endOffset = l.tokenLength() + l.tokenOffset();

            if (pragmaValue == library) {
                pragmas |= QDeclarativeScript::Object::ScriptBlock::Shared;
                replaceWithSpace(script, startOffset, endOffset - startOffset);
            } else {
                return rv;
            }

            token = l.lex();
            if (l.tokenStartLine() == startLine)
                return rv;

        } else {
            return rv;
        }
    }
    return rv;
}

void QDeclarativeScript::Parser::clear()
{
    _imports.clear();
    qDeleteAll(_refTypes);
    _refTypes.clear();
    _errors.clear();

    if (data) {
        delete data;
        data = 0;
    }

    _pool.clear();
}

QDeclarativeScript::TypeReference *QDeclarativeScript::Parser::findOrCreateType(const QString &name)
{
    TypeReference *type = 0;
    int i = 0;
    for (; i < _refTypes.size(); ++i) {
        if (_refTypes.at(i)->name == name) {
            type = _refTypes.at(i);
            break;
        }
    }
    if (!type) {
        type = new TypeReference(i, name);
        _refTypes.append(type);
    }

    return type;
}

void QDeclarativeScript::Parser::setTree(QDeclarativeScript::Object *tree)
{
    Q_ASSERT(! root);

    root = tree;
}

QT_END_NAMESPACE
