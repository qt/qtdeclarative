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

#include "qqmlscript_p.h"

#include "parser/qqmljsengine_p.h"
#include "parser/qqmljsparser_p.h"
#include "parser/qqmljslexer_p.h"
#include "parser/qqmljsmemorypool_p.h"
#include "parser/qqmljsastvisitor_p.h"
#include "parser/qqmljsast_p.h"
#include <private/qqmlrewrite_p.h>

#include <QStack>
#include <QStringList>
#include <QCoreApplication>
#include <QtDebug>

QT_BEGIN_NAMESPACE

using namespace QQmlJS;
using namespace QQmlScript;

// 
// Parser IR classes
//
QQmlScript::Object::Object()
: type(-1), typeReference(0), idIndex(-1), metatype(0), synthCache(0),
  defaultProperty(0), parserStatusCast(-1), componentCompileState(0), nextAliasingObject(0),
  nextIdObject(0)
{
}

QQmlScript::Object::~Object() 
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

QQmlScript::Property *Object::getDefaultProperty()
{
    if (!defaultProperty) {
        defaultProperty = pool()->New<Property>();
        defaultProperty->parent = this;
    }
    return defaultProperty;
}

void QQmlScript::Object::addValueProperty(Property *p)
{
    valueProperties.append(p);
}

void QQmlScript::Object::addSignalProperty(Property *p)
{
    signalProperties.append(p);
}

void QQmlScript::Object::addAttachedProperty(Property *p)
{
    attachedProperties.append(p);
}

void QQmlScript::Object::addGroupedProperty(Property *p)
{
    groupedProperties.append(p);
}

void QQmlScript::Object::addValueTypeProperty(Property *p)
{
    valueTypeProperties.append(p);
}

void QQmlScript::Object::addScriptStringProperty(Property *p)
{
    scriptStringProperties.append(p);
}

// This lookup is optimized for missing, and having to create a new property.
Property *QQmlScript::Object::getProperty(const QHashedStringRef &name, bool create)
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

Property *QQmlScript::Object::getProperty(const QStringRef &name, bool create)
{
    return getProperty(QHashedStringRef(name), create);
}

Property *QQmlScript::Object::getProperty(const QString &name, bool create)
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

int QQmlScript::Object::aggregateDynamicSignalParameterCount() const
{
    int sum = 0;
    for (DynamicSignal *s = dynamicSignals.first(); s; s = dynamicSignals.next(s))
        sum += s->parameterTypes.count() + 1; // +1 for return type
    return sum;
}

int QQmlScript::Object::aggregateDynamicSlotParameterCount() const
{
    int sum = 0;
    for (DynamicSlot *s = dynamicSlots.first(); s; s = dynamicSlots.next(s))
        sum += s->parameterNames.count() + 1; // +1 for return type
    return sum;
}

QQmlScript::Object::DynamicProperty::DynamicProperty()
: isDefaultProperty(false), isReadOnly(false), type(Variant), defaultValue(0), nextProperty(0),
  nameIndex(-1)
{
}

QQmlScript::Object::DynamicSignal::DynamicSignal()
: nextSignal(0), nameIndex(-1)
{
}

QQmlScript::Object::DynamicSlot::DynamicSlot()
: nextSlot(0), nameIndex(-1)
{
}

int QQmlScript::Object::DynamicSlot::parameterNamesLength() const
{
    int rv = 0;
    for (int ii = 0; ii < parameterNames.count(); ++ii)
        rv += parameterNames.at(ii).length();
    return rv;
}

QQmlScript::Property::Property()
: parent(0), type(0), index(-1), value(0), isDefault(true), isDeferred(false), 
  isValueTypeSubProperty(false), isAlias(false), isReadOnlyDeclaration(false),
  scriptStringScope(-1), nextMainProperty(0), nextProperty(0)
{
}

QQmlScript::Object *QQmlScript::Property::getValue(const LocationSpan &l)
{
    if (!value) { value = pool()->New<Object>(); value->location = l; }
    return value;
}

void QQmlScript::Property::addValue(Value *v)
{
    values.append(v);
}

void QQmlScript::Property::addOnValue(Value *v)
{
    onValues.append(v);
}

bool QQmlScript::Property::isEmpty() const
{
    return !value && values.isEmpty() && onValues.isEmpty();
}

QQmlScript::Value::Value()
: type(Unknown), object(0), bindingReference(0), nextValue(0)
{
}

QQmlScript::Variant::Variant()
: t(Invalid)
{
}

QQmlScript::Variant::Variant(const Variant &o)
: t(o.t), d(o.d), asWritten(o.asWritten)
{
}

QQmlScript::Variant::Variant(bool v)
: t(Boolean), b(v)
{
}

QQmlScript::Variant::Variant(double v, const QStringRef &asWritten)
: t(Number), d(v), asWritten(asWritten)
{
}

QQmlScript::Variant::Variant(QQmlJS::AST::StringLiteral *v)
: t(String), l(v)
{
}

QQmlScript::Variant::Variant(const QStringRef &asWritten, QQmlJS::AST::Node *n)
: t(Script), n(n), asWritten(asWritten)
{
}

QQmlScript::Variant &QQmlScript::Variant::operator=(const Variant &o)
{
    t = o.t;
    d = o.d;
    asWritten = o.asWritten;
    return *this;
}

QQmlScript::Variant::Type QQmlScript::Variant::type() const
{
    return t;
}

bool QQmlScript::Variant::asBoolean() const
{
    return b;
}

QString QQmlScript::Variant::asString() const
{
    if (t == String) {
        return l->value.toString();
    } else {
        return asWritten.toString();
    }
}

double QQmlScript::Variant::asNumber() const
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

QString QQmlScript::Variant::asScript() const
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

QQmlJS::AST::Node *QQmlScript::Variant::asAST() const
{
    if (type() == Script)
        return n;
    else
        return 0;
}

bool QQmlScript::Variant::isStringList() const
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

QStringList QQmlScript::Variant::asStringList() const
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
namespace {

class ProcessAST: protected AST::Visitor
{
    struct State {
        State() : object(0), property(0) {}
        State(QQmlScript::Object *o) : object(o), property(0) {}
        State(QQmlScript::Object *o, Property *p) : object(o), property(p) {}

        QQmlScript::Object *object;
        Property *property;
    };

    struct StateStack : public QStack<State>
    {
        void pushObject(QQmlScript::Object *obj)
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
    ProcessAST(QQmlScript::Parser *parser);
    virtual ~ProcessAST();

    void operator()(const QString &code, AST::Node *node);

    static void extractVersion(QStringRef string, int *maj, int *min);

protected:

    QQmlScript::Object *defineObjectBinding(AST::UiQualifiedId *propertyName, bool onAssignment,
                                const QString &objectType,
                                AST::SourceLocation typeLocation,
                                LocationSpan location,
                                AST::UiObjectInitializer *initializer = 0);

    QQmlScript::Variant getVariant(AST::Statement *stmt);
    QQmlScript::Variant getVariant(AST::ExpressionNode *expr);

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
    QQmlScript::Object *currentObject() const;
    Property *currentProperty() const;

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
    QQmlScript::Parser *_parser;
    StateStack _stateStack;
    const QString *_contents;
};

ProcessAST::ProcessAST(QQmlScript::Parser *parser)
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

QQmlScript::Object *ProcessAST::currentObject() const
{
    return state().object;
}

Property *ProcessAST::currentProperty() const
{
    return state().property;
}

void ProcessAST::extractVersion(QStringRef string, int *maj, int *min)
{
    *maj = -1; *min = -1;

    if (!string.isEmpty()) {

        int dot = string.indexOf(QLatin1Char('.'));

        if (dot < 0) {
            *maj = string.toString().toInt();
            *min = 0;
        } else {
            const QString *s = string.string();
            int p = string.position();
            *maj = QStringRef(s, p, dot).toString().toInt();
            *min = QStringRef(s, p + dot + 1, string.size() - dot - 1).toString().toInt();
        }
    }
}

QString ProcessAST::asString(AST::UiQualifiedId *node) const
{
    QString s;

    for (AST::UiQualifiedId *it = node; it; it = it->next) {
        s.append(it->name);

        if (it->next)
            s.append(QLatin1Char('.'));
    }

    return s;
}

QQmlScript::Object *
ProcessAST::defineObjectBinding(AST::UiQualifiedId *propertyName,
                                bool onAssignment,
                                const QString &objectType,
                                AST::SourceLocation typeLocation,
                                LocationSpan location,
                                AST::UiObjectInitializer *initializer)
{
    int lastTypeDot = objectType.lastIndexOf(QLatin1Char('.'));

    // With no preceding qualification, first char is at (-1 + 1) == 0
    bool isType = !objectType.isEmpty() && objectType.at(lastTypeDot+1).isUpper();

    int propertyCount = 0;
    for (AST::UiQualifiedId *name = propertyName; name; name = name->next){
        ++propertyCount;
        _stateStack.pushProperty(name->name,
                                 this->location(name));
    }

    if (!onAssignment && propertyCount && currentProperty() && !currentProperty()->values.isEmpty()) {
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","Property value set multiple times"));
        error.setLine(this->location(propertyName).start.line);
        error.setColumn(this->location(propertyName).start.column);
        _parser->_errors << error;
        return 0;
    }

    if (!isType) {

        // Is the identifier qualified by a namespace?
        int namespaceLength = 0;
        if (lastTypeDot > 0) {
            const QString qualifier(objectType.left(lastTypeDot));

            for (int ii = 0; ii < _parser->_imports.count(); ++ii) {
                const QQmlScript::Import &import = _parser->_imports.at(ii);
                if (import.qualifier == qualifier) {
                    // The qualifier is a namespace - expect a type here
                    namespaceLength = qualifier.length() + 1;
                    break;
                }
            }
        }

        if (propertyCount || !currentObject() || namespaceLength) {
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Expected type name"));
            error.setLine(typeLocation.startLine);
            error.setColumn(typeLocation.startColumn + namespaceLength);
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

	QQmlScript::Object *obj = _parser->_pool.New<QQmlScript::Object>();

        obj->type = _parser->findOrCreateTypeId(objectType, obj);
        obj->typeReference = _parser->_refTypes.at(obj->type);
        obj->location = location;

        if (propertyCount) {
            Property *prop = currentProperty();
            QQmlScript::Value *v = _parser->_pool.New<QQmlScript::Value>();
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
                QQmlScript::Value *v = _parser->_pool.New<QQmlScript::Value>();
                v->object = obj;
                v->location = obj->location;
                if (state.property) {
                    state.property->addValue(v);
                } else {
                    Property *defaultProp = state.object->getDefaultProperty();
                    if (defaultProp->location.start.line == 0) {
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
    QQmlScript::Import import;

    if (!node->fileName.isNull()) {
        uri = node->fileName.toString();

        if (uri.endsWith(QLatin1String(".js"))) {
            import.type = QQmlScript::Import::Script;
        } else {
            import.type = QQmlScript::Import::File;
        }
    } else {
        import.type = QQmlScript::Import::Library;
        uri = asString(node->importUri);
    }

    AST::SourceLocation startLoc = node->importToken;
    AST::SourceLocation endLoc = node->semicolonToken;

    // Qualifier
    if (!node->importId.isNull()) {
        import.qualifier = node->importId.toString();
        if (!import.qualifier.at(0).isUpper()) {
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Invalid import qualifier ID"));
            error.setLine(node->importIdToken.startLine);
            error.setColumn(node->importIdToken.startColumn);
            _parser->_errors << error;
            return false;
        }
        if (import.qualifier == QLatin1String("Qt")) {
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Reserved name \"Qt\" cannot be used as an qualifier"));
            error.setLine(node->importIdToken.startLine);
            error.setColumn(node->importIdToken.startColumn);
            _parser->_errors << error;
            return false;
        }

        // Check for script qualifier clashes
        bool isScript = import.type == QQmlScript::Import::Script;
        for (int ii = 0; ii < _parser->_imports.count(); ++ii) {
            const QQmlScript::Import &other = _parser->_imports.at(ii);
            bool otherIsScript = other.type == QQmlScript::Import::Script;

            if ((isScript || otherIsScript) && import.qualifier == other.qualifier) {
                QQmlError error;
                error.setDescription(QCoreApplication::translate("QQmlParser","Script import qualifiers must be unique."));
                error.setLine(node->importIdToken.startLine);
                error.setColumn(node->importIdToken.startColumn);
                _parser->_errors << error;
                return false;
            }
        }

    } else if (import.type == QQmlScript::Import::Script) {
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","Script import requires a qualifier"));
        error.setLine(node->fileNameToken.startLine);
        error.setColumn(node->fileNameToken.startColumn);
        _parser->_errors << error;
        return false;
    }

    if (node->versionToken.isValid()) {
        extractVersion(textRefAt(node->versionToken), &import.majorVersion, &import.minorVersion);
    } else if (import.type == QQmlScript::Import::Library) {
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","Library import requires a version"));
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
        size_t nameLength;
        Object::DynamicProperty::Type type;
    } propTypeNameToTypes[] = {
        { "int", strlen("int"), Object::DynamicProperty::Int },
        { "bool", strlen("bool"), Object::DynamicProperty::Bool },
        { "double", strlen("double"), Object::DynamicProperty::Real },
        { "real", strlen("real"), Object::DynamicProperty::Real },
        { "string", strlen("string"), Object::DynamicProperty::String },
        { "url", strlen("url"), Object::DynamicProperty::Url },
        { "color", strlen("color"), Object::DynamicProperty::Color },
        // Internally QTime, QDate and QDateTime are all supported.
        // To be more consistent with JavaScript we expose only
        // QDateTime as it matches closely with the Date JS type.
        // We also call it "date" to match.
        // { "time", strlen("time"), Object::DynamicProperty::Time },
        // { "date", strlen("date"), Object::DynamicProperty::Date },
        { "date", strlen("date"), Object::DynamicProperty::DateTime },
        { "rect", strlen("rect"), Object::DynamicProperty::Rect },
        { "point", strlen("point"), Object::DynamicProperty::Point },
        { "size", strlen("size"), Object::DynamicProperty::Size },
        { "font", strlen("font"), Object::DynamicProperty::Font },
        { "vector2d", strlen("vector2d"), Object::DynamicProperty::Vector2D },
        { "vector3d", strlen("vector3d"), Object::DynamicProperty::Vector3D },
        { "vector4d", strlen("vector4d"), Object::DynamicProperty::Vector4D },
        { "quaternion", strlen("quaternion"), Object::DynamicProperty::Quaternion },
        { "matrix4x4", strlen("matrix4x4"), Object::DynamicProperty::Matrix4x4 },
        { "variant", strlen("variant"), Object::DynamicProperty::Variant },
        { "var", strlen("var"), Object::DynamicProperty::Var }
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
            signal->parameterTypes = _parser->_pool.NewRawList<Object::DynamicProperty::Type>(paramLength);
            signal->parameterTypeNames = _parser->_pool.NewRawList<QHashedStringRef>(paramLength);
            signal->parameterNames = _parser->_pool.NewRawList<QHashedStringRef>(paramLength);
        }

        int index = 0;
        while (p) {
            const QStringRef &memberType = p->type;

            if (memberType.isEmpty()) {
                QQmlError error;
                error.setDescription(QCoreApplication::translate("QQmlParser","Expected parameter type"));
                error.setLine(node->typeToken.startLine);
                error.setColumn(node->typeToken.startColumn);
                _parser->_errors << error;
                return false;
            }

            const TypeNameToType *type = 0;
            for(int typeIndex = 0; typeIndex < propTypeNameToTypesCount; ++typeIndex) {
                const TypeNameToType *t = propTypeNameToTypes + typeIndex;
                if (t->nameLength == size_t(memberType.length()) &&
                    QHashedString::compare(memberType.constData(), t->name, t->nameLength)) {
                    type = t;
                    break;
                }
            }

            if (!type) {
                if (memberType.at(0).isUpper()) {
                    // Must be a QML object type.
                    // Lazily determine type during compilation.
                    signal->parameterTypes[index] = Object::DynamicProperty::Custom;
                    signal->parameterTypeNames[index] = QHashedStringRef(p->type);
                } else {
                    QQmlError error;
                    QString errStr = QCoreApplication::translate("QQmlParser","Invalid signal parameter type: ");
                    errStr.append(memberType.toString());
                    error.setDescription(errStr);
                    error.setLine(node->typeToken.startLine);
                    error.setColumn(node->typeToken.startColumn);
                    _parser->_errors << error;
                    return false;
                }
            } else {
                // the parameter is a known basic type
                signal->parameterTypes[index] = type->type;
            }

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
            if (t->nameLength == size_t(memberType.length()) &&
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
                QQmlError error;
                error.setDescription(QCoreApplication::translate("QQmlParser","Invalid property type modifier"));
                error.setLine(node->typeModifierToken.startLine);
                error.setColumn(node->typeModifierToken.startColumn);
                _parser->_errors << error;
                return false;
            }
            typeFound = true;
        } else if (!node->typeModifier.isNull()) {
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Unexpected property type modifier"));
            error.setLine(node->typeModifierToken.startLine);
            error.setColumn(node->typeModifierToken.startColumn);
            _parser->_errors << error;
            return false;
        }

        if(!typeFound) {
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Expected property type"));
            error.setLine(node->typeToken.startLine);
            error.setColumn(node->typeToken.startColumn);
            _parser->_errors << error;
            return false;
        }

        Object::DynamicProperty *property = _parser->_pool.New<Object::DynamicProperty>();
        property->isDefaultProperty = node->isDefaultMember;
        property->isReadOnly = node->isReadonlyMember;
        property->type = type;
        property->nameLocation.line = node->identifierToken.startLine;
        property->nameLocation.column = node->identifierToken.startColumn;
        if (type >= Object::DynamicProperty::Custom) {
            // This forces the type to be added to the resolved types list
            _parser->findOrCreateTypeId(memberType.toString(), _stateStack.top().object);
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
            QQmlScript::Value *value = _parser->_pool.New<QQmlScript::Value>();
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

QQmlScript::Variant ProcessAST::getVariant(AST::Statement *stmt)
{
    if (stmt) {
        if (AST::ExpressionStatement *exprStmt = AST::cast<AST::ExpressionStatement *>(stmt))
            return getVariant(exprStmt->expression);

        return QQmlScript::Variant(asStringRef(stmt), stmt);
    }

    return QQmlScript::Variant();
}

QQmlScript::Variant ProcessAST::getVariant(AST::ExpressionNode *expr)
{
    if (AST::StringLiteral *lit = AST::cast<AST::StringLiteral *>(expr)) {
        return QQmlScript::Variant(lit);
    } else if (expr->kind == AST::Node::Kind_TrueLiteral) {
        return QQmlScript::Variant(true);
    } else if (expr->kind == AST::Node::Kind_FalseLiteral) {
        return QQmlScript::Variant(false);
    } else if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(expr)) {
        return QQmlScript::Variant(lit->value, asStringRef(expr));
    } else {

        if (AST::UnaryMinusExpression *unaryMinus = AST::cast<AST::UnaryMinusExpression *>(expr)) {
           if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(unaryMinus->expression)) {
               return QQmlScript::Variant(-lit->value, asStringRef(expr));
           }
        }

        return  QQmlScript::Variant(asStringRef(expr), expr);
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
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","Property value set multiple times"));
        error.setLine(this->location(propertyName).start.line);
        error.setColumn(this->location(propertyName).start.column);
        _parser->_errors << error;
        return 0;
    }

    QQmlScript::Variant primitive;

    if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement *>(node->statement)) {
        primitive = getVariant(stmt->expression);
    } else { // do binding
        primitive = QQmlScript::Variant(asStringRef(node->statement), node->statement);
    }

    prop->location.range.length = prop->location.range.offset + prop->location.range.length - node->qualifiedId->identifierToken.offset;
    prop->location.range.offset = node->qualifiedId->identifierToken.offset;
    QQmlScript::Value *v = _parser->_pool.New<QQmlScript::Value>();
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
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","Property value set multiple times"));
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
    QQmlScript::Object *obj = currentObject();

    if (AST::FunctionDeclaration *funDecl = AST::cast<AST::FunctionDeclaration *>(node->sourceElement)) {

        Object::DynamicSlot *slot = _parser->_pool.New<Object::DynamicSlot>();
        slot->location = location(funDecl->identifierToken, funDecl->lastSourceLocation());

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
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","JavaScript declaration outside Script element"));
        error.setLine(node->firstSourceLocation().startLine);
        error.setColumn(node->firstSourceLocation().startColumn);
        _parser->_errors << error;
    }
    return false;
}

} // end of anonymous namespace


QQmlScript::Parser::Parser()
: root(0), data(0)
{

}

QQmlScript::Parser::~Parser()
{
    clear();
}

namespace QQmlScript {
class ParserJsASTData
{
public:
    ParserJsASTData(const QString &filename)
    : filename(filename) {}

    QString filename;
    Engine engine;
};
}

QByteArray QQmlScript::Parser::preparseData() const
{
    return QByteArray();
}

bool QQmlScript::Parser::parse(const QString &qmlcode, const QByteArray & /* preparseData */,
                               const QUrl &url, const QString &urlString)
{
    clear();

    if (urlString.isEmpty()) {
        _scriptFile = url.toString();
    } else {
       // Q_ASSERT(urlString == url.toString());
        _scriptFile = urlString;
    }

    QString *code = _pool.NewString(qmlcode);

    data = new QQmlScript::ParserJsASTData(_scriptFile);

    Lexer lexer(&data->engine);
    lexer.setCode(*code, /*line = */ 1);

    QQmlJS::Parser parser(&data->engine);

    if (! parser.parse() || !parser.diagnosticMessages().isEmpty()) {

        // Extract errors from the parser
        foreach (const DiagnosticMessage &m, parser.diagnosticMessages()) {

            if (m.isWarning()) {
                qWarning("%s:%d : %s", qPrintable(_scriptFile), m.loc.startLine, qPrintable(m.message));
                continue;
            }

            QQmlError error;
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

QList<QQmlScript::TypeReference*> QQmlScript::Parser::referencedTypes() const
{
    return _refTypes;
}

QQmlScript::Object *QQmlScript::Parser::tree() const
{
    return root;
}

QList<QQmlScript::Import> QQmlScript::Parser::imports() const
{
    return _imports;
}

QList<QQmlError> QQmlScript::Parser::errors() const
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

static QQmlScript::LocationSpan
locationFromLexer(const QQmlJS::Lexer &lex, int startLine, int startColumn, int startOffset)
{
    QQmlScript::LocationSpan l;

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
QQmlScript::Object::ScriptBlock::Pragmas QQmlScript::Parser::extractPragmas(QString &script)
{
    QQmlScript::Object::ScriptBlock::Pragmas rv = QQmlScript::Object::ScriptBlock::None;

    const QString pragma(QLatin1String("pragma"));
    const QString library(QLatin1String("library"));

    QQmlJS::Lexer l(0);
    l.setCode(script, 0);

    int token = l.lex();

    while (true) {
        if (token != QQmlJSGrammar::T_DOT)
            return rv;

        int startOffset = l.tokenOffset();
        int startLine = l.tokenStartLine();

        token = l.lex();

        if (token != QQmlJSGrammar::T_IDENTIFIER ||
            l.tokenStartLine() != startLine ||
            script.mid(l.tokenOffset(), l.tokenLength()) != pragma)
            return rv;

        token = l.lex();

        if (token != QQmlJSGrammar::T_IDENTIFIER ||
            l.tokenStartLine() != startLine)
            return rv;

        QString pragmaValue = script.mid(l.tokenOffset(), l.tokenLength());
        int endOffset = l.tokenLength() + l.tokenOffset();

        token = l.lex();
        if (l.tokenStartLine() == startLine)
            return rv;

        if (pragmaValue == library) {
            rv |= QQmlScript::Object::ScriptBlock::Shared;
            replaceWithSpace(script, startOffset, endOffset - startOffset);
        } else {
            return rv;
        }
    }
    return rv;
}

#define CHECK_LINE if (l.tokenStartLine() != startLine) return rv;
#define CHECK_TOKEN(t) if (token != QQmlJSGrammar:: t) return rv;

static const int uriTokens[] = {
    QQmlJSGrammar::T_IDENTIFIER, 
    QQmlJSGrammar::T_PROPERTY, 
    QQmlJSGrammar::T_SIGNAL, 
    QQmlJSGrammar::T_READONLY, 
    QQmlJSGrammar::T_ON, 
    QQmlJSGrammar::T_BREAK, 
    QQmlJSGrammar::T_CASE, 
    QQmlJSGrammar::T_CATCH, 
    QQmlJSGrammar::T_CONTINUE, 
    QQmlJSGrammar::T_DEFAULT, 
    QQmlJSGrammar::T_DELETE, 
    QQmlJSGrammar::T_DO, 
    QQmlJSGrammar::T_ELSE, 
    QQmlJSGrammar::T_FALSE, 
    QQmlJSGrammar::T_FINALLY, 
    QQmlJSGrammar::T_FOR, 
    QQmlJSGrammar::T_FUNCTION, 
    QQmlJSGrammar::T_IF, 
    QQmlJSGrammar::T_IN, 
    QQmlJSGrammar::T_INSTANCEOF, 
    QQmlJSGrammar::T_NEW, 
    QQmlJSGrammar::T_NULL, 
    QQmlJSGrammar::T_RETURN, 
    QQmlJSGrammar::T_SWITCH, 
    QQmlJSGrammar::T_THIS, 
    QQmlJSGrammar::T_THROW, 
    QQmlJSGrammar::T_TRUE, 
    QQmlJSGrammar::T_TRY, 
    QQmlJSGrammar::T_TYPEOF, 
    QQmlJSGrammar::T_VAR, 
    QQmlJSGrammar::T_VOID, 
    QQmlJSGrammar::T_WHILE, 
    QQmlJSGrammar::T_CONST, 
    QQmlJSGrammar::T_DEBUGGER, 
    QQmlJSGrammar::T_RESERVED_WORD, 
    QQmlJSGrammar::T_WITH, 

    QQmlJSGrammar::EOF_SYMBOL
};
static inline bool isUriToken(int token)
{
    const int *current = uriTokens;
    while (*current != QQmlJSGrammar::EOF_SYMBOL) {
        if (*current == token)
            return true;
        ++current;
    }
    return false;
}

QQmlScript::Parser::JavaScriptMetaData QQmlScript::Parser::extractMetaData(QString &script, QQmlError *error)
{
    Q_ASSERT(error);

    JavaScriptMetaData rv;

    QQmlScript::Object::ScriptBlock::Pragmas &pragmas = rv.pragmas;

    const QString pragma(QLatin1String("pragma"));
    const QString js(QLatin1String(".js"));
    const QString library(QLatin1String("library"));

    QQmlJS::Lexer l(0);
    l.setCode(script, 0);

    int token = l.lex();

    while (true) {
        if (token != QQmlJSGrammar::T_DOT)
            return rv;

        int startOffset = l.tokenOffset();
        int startLine = l.tokenStartLine();
        int startColumn = l.tokenStartColumn();

        QQmlError importError;
        importError.setLine(startLine + 1); // 0-based, adjust to be 1-based

        token = l.lex();

        CHECK_LINE;

        if (token == QQmlJSGrammar::T_IMPORT) {

            // .import <URI> <Version> as <Identifier>
            // .import <file.js> as <Identifier>

            token = l.lex();

            CHECK_LINE;

            if (token == QQmlJSGrammar::T_STRING_LITERAL) {

                QString file = l.tokenText();

                if (!file.endsWith(js)) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Imported file must be a script"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                bool invalidImport = false;

                token = l.lex();

                if ((token != QQmlJSGrammar::T_AS) || (l.tokenStartLine() != startLine)) {
                    invalidImport = true;
                } else {
                    token = l.lex();

                    if ((token != QQmlJSGrammar::T_IDENTIFIER) || (l.tokenStartLine() != startLine))
                        invalidImport = true;
                }


                if (invalidImport) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","File import requires a qualifier"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                int endOffset = l.tokenLength() + l.tokenOffset();

                QString importId = script.mid(l.tokenOffset(), l.tokenLength());

                QQmlScript::LocationSpan location =
                    locationFromLexer(l, startLine, startColumn, startOffset);

                token = l.lex();

                if (!importId.at(0).isUpper() || (l.tokenStartLine() == startLine)) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Invalid import qualifier"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

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

                while (true) {
                    if (!isUriToken(token)) {
                        importError.setDescription(QCoreApplication::translate("QQmlParser","Invalid module URI"));
                        importError.setColumn(l.tokenStartColumn());
                        *error = importError;
                        return rv;
                    }

                    uri.append(l.tokenText());

                    token = l.lex();
                    CHECK_LINE;
                    if (token != QQmlJSGrammar::T_DOT)
                        break;

                    uri.append(QLatin1Char('.'));

                    token = l.lex();
                    CHECK_LINE;
                }

                if (token != QQmlJSGrammar::T_NUMERIC_LITERAL) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Module import requires a version"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                int vmaj, vmin;
                ProcessAST::extractVersion(QStringRef(&script, l.tokenOffset(), l.tokenLength()),
                                           &vmaj, &vmin);

                bool invalidImport = false;

                token = l.lex();

                if ((token != QQmlJSGrammar::T_AS) || (l.tokenStartLine() != startLine)) {
                    invalidImport = true;
                } else {
                    token = l.lex();

                    if ((token != QQmlJSGrammar::T_IDENTIFIER) || (l.tokenStartLine() != startLine))
                        invalidImport = true;
                }


                if (invalidImport) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Module import requires a qualifier"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                int endOffset = l.tokenLength() + l.tokenOffset();

                QString importId = script.mid(l.tokenOffset(), l.tokenLength());

                QQmlScript::LocationSpan location =
                    locationFromLexer(l, startLine, startColumn, startOffset);

                token = l.lex();

                if (!importId.at(0).isUpper() || (l.tokenStartLine() == startLine)) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Invalid import qualifier"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                replaceWithSpace(script, startOffset, endOffset - startOffset);

                Import import;
                import.type = Import::Library;
                import.uri = uri;
                import.majorVersion = vmaj;
                import.minorVersion = vmin;
                import.qualifier = importId;
                import.location = location;

                rv.imports << import;
            }

        } else if (token == QQmlJSGrammar::T_IDENTIFIER &&
                   script.mid(l.tokenOffset(), l.tokenLength()) == pragma) {

            token = l.lex();

            CHECK_TOKEN(T_IDENTIFIER);
            CHECK_LINE;

            QString pragmaValue = script.mid(l.tokenOffset(), l.tokenLength());
            int endOffset = l.tokenLength() + l.tokenOffset();

            if (pragmaValue == library) {
                pragmas |= QQmlScript::Object::ScriptBlock::Shared;
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

void QQmlScript::Parser::clear()
{
    _imports.clear();
    _refTypes.clear();
    _errors.clear();

    if (data) {
        delete data;
        data = 0;
    }

    _pool.clear();
}

int QQmlScript::Parser::findOrCreateTypeId(const QString &name, Object *object)
{
    for (int ii = 0; ii < _refTypes.size(); ++ii) {
        if (_refTypes.at(ii)->name == name)
            return ii;
    }

    TypeReference *type = _pool.New<TypeReference>();
    type->name = name;
    type->firstUse = object;
    _refTypes.append(type);
    return _refTypes.size() - 1;
}

void QQmlScript::Parser::setTree(QQmlScript::Object *tree)
{
    Q_ASSERT(! root);

    root = tree;
}

QT_END_NAMESPACE
