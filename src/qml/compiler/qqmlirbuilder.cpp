/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlirbuilder_p.h"

#include <private/qv4staticvalue_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qv4compilerscanfunctions_p.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <cmath>

#ifdef CONST
#undef CONST
#endif

QT_USE_NAMESPACE

static const quint32 emptyStringIndex = 0;
using namespace QmlIR;
using namespace QQmlJS;

#define COMPILE_EXCEPTION(location, desc) \
    { \
        recordError(location, desc); \
        return false; \
    }

void Object::simplifyRequiredProperties() {
    // if a property of the current object was marked as required
    // do not store that information in the ExtraData
    // but rather mark the property as required
    QSet<int> required;
    for (auto it = this->requiredPropertyExtraDataBegin(); it != this->requiredPropertyExtraDataEnd(); ++it)
        required.insert(it->nameIndex);
    if (required.isEmpty())
        return;
    for (auto it = this->propertiesBegin(); it != this->propertiesEnd(); ++it) {
        auto requiredIt = required.find(it->nameIndex);
        if (requiredIt != required.end()) {
            it->isRequired = true;
            required.erase(requiredIt);
        }
    }
    QmlIR::RequiredPropertyExtraData *prev = nullptr;
    auto current = this->requiredPropertyExtraDatas->first;
    while (current) {
        if (required.contains(current->nameIndex))
            prev = current;
        else
            requiredPropertyExtraDatas->unlink(prev, current);
        current = current->next;
    }
}

bool Parameter::init(QV4::Compiler::JSUnitGenerator *stringGenerator, const QString &parameterName,
                     const QString &typeName)
{
    return init(this, stringGenerator, stringGenerator->registerString(parameterName), stringGenerator->registerString(typeName));
}

bool Parameter::init(QV4::CompiledData::Parameter *param, const QV4::Compiler::JSUnitGenerator *stringGenerator,
                     int parameterNameIndex, int typeNameIndex)
{
    param->nameIndex = parameterNameIndex;
    return initType(&param->type, stringGenerator, typeNameIndex);
}

bool Parameter::initType(QV4::CompiledData::ParameterType *paramType, const QV4::Compiler::JSUnitGenerator *stringGenerator, int typeNameIndex)
{
    paramType->indexIsBuiltinType = false;
    paramType->typeNameIndexOrBuiltinType = 0;
    const QString typeName = stringGenerator->stringForIndex(typeNameIndex);
    auto builtinType = stringToBuiltinType(typeName);
    if (builtinType == QV4::CompiledData::BuiltinType::InvalidBuiltin) {
        if (typeName.isEmpty() || !typeName.at(0).isUpper())
            return false;
        paramType->indexIsBuiltinType = false;
        paramType->typeNameIndexOrBuiltinType = typeNameIndex;
        Q_ASSERT(quint32(typeNameIndex) < (1u << 31));
    } else {
        paramType->indexIsBuiltinType = true;
        paramType->typeNameIndexOrBuiltinType = static_cast<quint32>(builtinType);
        Q_ASSERT(quint32(builtinType) < (1u << 31));
    }
    return true;
}

QV4::CompiledData::BuiltinType Parameter::stringToBuiltinType(const QString &typeName)
{
    static const struct TypeNameToType {
        const char *name;
        size_t nameLength;
        QV4::CompiledData::BuiltinType type;
    } propTypeNameToTypes[] = {
        { "int", strlen("int"), QV4::CompiledData::BuiltinType::Int },
        { "bool", strlen("bool"), QV4::CompiledData::BuiltinType::Bool },
        { "double", strlen("double"), QV4::CompiledData::BuiltinType::Real },
        { "real", strlen("real"), QV4::CompiledData::BuiltinType::Real },
        { "string", strlen("string"), QV4::CompiledData::BuiltinType::String },
        { "url", strlen("url"), QV4::CompiledData::BuiltinType::Url },
        { "color", strlen("color"), QV4::CompiledData::BuiltinType::Color },
        // Internally QTime, QDate and QDateTime are all supported.
        // To be more consistent with JavaScript we expose only
        // QDateTime as it matches closely with the Date JS type.
        // We also call it "date" to match.
        // { "time", strlen("time"), Property::Time },
        // { "date", strlen("date"), Property::Date },
        { "date", strlen("date"), QV4::CompiledData::BuiltinType::DateTime },
        { "rect", strlen("rect"), QV4::CompiledData::BuiltinType::Rect },
        { "point", strlen("point"), QV4::CompiledData::BuiltinType::Point },
        { "size", strlen("size"), QV4::CompiledData::BuiltinType::Size },
        { "font", strlen("font"), QV4::CompiledData::BuiltinType::Font },
        { "vector2d", strlen("vector2d"), QV4::CompiledData::BuiltinType::Vector2D },
        { "vector3d", strlen("vector3d"), QV4::CompiledData::BuiltinType::Vector3D },
        { "vector4d", strlen("vector4d"), QV4::CompiledData::BuiltinType::Vector4D },
        { "quaternion", strlen("quaternion"), QV4::CompiledData::BuiltinType::Quaternion },
        { "matrix4x4", strlen("matrix4x4"), QV4::CompiledData::BuiltinType::Matrix4x4 },
        { "variant", strlen("variant"), QV4::CompiledData::BuiltinType::Variant },
        { "var", strlen("var"), QV4::CompiledData::BuiltinType::Var }
    };
    static const int propTypeNameToTypesCount = sizeof(propTypeNameToTypes) /
                                                sizeof(propTypeNameToTypes[0]);

    for (int typeIndex = 0; typeIndex < propTypeNameToTypesCount; ++typeIndex) {
        const TypeNameToType *t = propTypeNameToTypes + typeIndex;
        if (typeName == QLatin1String(t->name, static_cast<int>(t->nameLength))) {
            return t->type;
        }
    }
    return QV4::CompiledData::BuiltinType::InvalidBuiltin;
}

void Object::init(QQmlJS::MemoryPool *pool, int typeNameIndex, int idIndex, const QQmlJS::SourceLocation &loc)
{
    inheritedTypeNameIndex = typeNameIndex;

    location.line = loc.startLine;
    location.column = loc.startColumn;

    idNameIndex = idIndex;
    id = -1;
    indexOfDefaultPropertyOrAlias = -1;
    defaultPropertyIsAlias = false;
    flags = QV4::CompiledData::Object::NoFlag;
    properties = pool->New<PoolList<Property> >();
    aliases = pool->New<PoolList<Alias> >();
    qmlEnums = pool->New<PoolList<Enum>>();
    qmlSignals = pool->New<PoolList<Signal> >();
    bindings = pool->New<PoolList<Binding> >();
    functions = pool->New<PoolList<Function> >();
    functionsAndExpressions = pool->New<PoolList<CompiledFunctionOrExpression> >();
    inlineComponents = pool->New<PoolList<InlineComponent>>();
    requiredPropertyExtraDatas = pool->New<PoolList<RequiredPropertyExtraData>>();
    declarationsOverride = nullptr;
}

QString IRBuilder::sanityCheckFunctionNames(Object *obj, const QSet<QString> &illegalNames, QQmlJS::SourceLocation *errorLocation)
{
    QSet<int> functionNames;
    for (auto functionit = obj->functionsBegin(); functionit != obj->functionsEnd(); ++functionit) {
        Function *f = functionit.ptr;
        errorLocation->startLine = f->location.line;
        errorLocation->startColumn = f->location.column;
        if (functionNames.contains(f->nameIndex))
            return tr("Duplicate method name");
        functionNames.insert(f->nameIndex);

        for (auto signalit = obj->signalsBegin(); signalit != obj->signalsEnd(); ++signalit) {
            QmlIR::Signal *s = signalit.ptr;
            if (s->nameIndex == f->nameIndex)
                return tr("Duplicate method name");
        }

        const QString name = stringAt(f->nameIndex);
        if (name.at(0).isUpper())
            return tr("Method names cannot begin with an upper case letter");
        if (illegalNames.contains(name))
            return tr("Illegal method name");
    }
    return QString(); // no error
}

QString Object::appendEnum(Enum *enumeration)
{
    Object *target = declarationsOverride;
    if (!target)
        target = this;

    for (Enum *e = qmlEnums->first; e; e = e->next) {
        if (e->nameIndex == enumeration->nameIndex)
            return tr("Duplicate scoped enum name");
    }

    target->qmlEnums->append(enumeration);
    return QString(); // no error
}

QString Object::appendSignal(Signal *signal)
{
    Object *target = declarationsOverride;
    if (!target)
        target = this;

    for (Signal *s = qmlSignals->first; s; s = s->next) {
        if (s->nameIndex == signal->nameIndex)
            return tr("Duplicate signal name");
    }

    target->qmlSignals->append(signal);
    return QString(); // no error
}

QString Object::appendProperty(Property *prop, const QString &propertyName, bool isDefaultProperty, const QQmlJS::SourceLocation &defaultToken, QQmlJS::SourceLocation *errorLocation)
{
    Object *target = declarationsOverride;
    if (!target)
        target = this;

    for (Property *p = target->properties->first; p; p = p->next)
        if (p->nameIndex == prop->nameIndex)
            return tr("Duplicate property name");

    if (propertyName.constData()->isUpper())
        return tr("Property names cannot begin with an upper case letter");

    const int index = target->properties->append(prop);
    if (isDefaultProperty) {
        if (target->indexOfDefaultPropertyOrAlias != -1) {
            *errorLocation = defaultToken;
            return tr("Duplicate default property");
        }
        target->indexOfDefaultPropertyOrAlias = index;
    }
    return QString(); // no error
}

QString Object::appendAlias(Alias *alias, const QString &aliasName, bool isDefaultProperty, const QQmlJS::SourceLocation &defaultToken, QQmlJS::SourceLocation *errorLocation)
{
    Object *target = declarationsOverride;
    if (!target)
        target = this;

    auto aliasWithSameName = std::find_if(target->aliases->begin(), target->aliases->end(), [&alias](const Alias &targetAlias){
        return targetAlias.nameIndex == alias->nameIndex;
    });
    if (aliasWithSameName != target->aliases->end())
        return tr("Duplicate alias name");

    if (aliasName.constData()->isUpper())
        return tr("Alias names cannot begin with an upper case letter");

    const int index = target->aliases->append(alias);

    if (isDefaultProperty) {
        if (target->indexOfDefaultPropertyOrAlias != -1) {
            *errorLocation = defaultToken;
            return tr("Duplicate default property");
        }
        target->indexOfDefaultPropertyOrAlias = index;
        target->defaultPropertyIsAlias = true;
    }

    return QString(); // no error
}

void Object::appendFunction(QmlIR::Function *f)
{
    Object *target = declarationsOverride;
    if (!target)
        target = this;
    target->functions->append(f);
}

void Object::appendInlineComponent(InlineComponent *ic)
{
    inlineComponents->append(ic);
}

void Object::appendRequiredPropertyExtraData(RequiredPropertyExtraData *extraData)
{
    requiredPropertyExtraDatas->append(extraData);
}

QString Object::appendBinding(Binding *b, bool isListBinding)
{
    const bool bindingToDefaultProperty = (b->propertyNameIndex == quint32(0));
    if (!isListBinding && !bindingToDefaultProperty
        && b->type != QV4::CompiledData::Binding::Type_GroupProperty
        && b->type != QV4::CompiledData::Binding::Type_AttachedProperty
        && !(b->flags & QV4::CompiledData::Binding::IsOnAssignment)) {
        Binding *existing = findBinding(b->propertyNameIndex);
        if (existing && existing->isValueBinding() == b->isValueBinding() && !(existing->flags & QV4::CompiledData::Binding::IsOnAssignment))
            return tr("Property value set multiple times");
    }
    if (bindingToDefaultProperty)
        insertSorted(b);
    else
        bindings->prepend(b);
    return QString(); // no error
}

Binding *Object::findBinding(quint32 nameIndex) const
{
    for (Binding *b = bindings->first; b; b = b->next)
        if (b->propertyNameIndex == nameIndex)
            return b;
    return nullptr;
}

void Object::insertSorted(Binding *b)
{
    Binding *insertionPoint = bindings->findSortedInsertionPoint<quint32, Binding, &Binding::offset>(b);
    bindings->insertAfter(insertionPoint, b);
}

QString Object::bindingAsString(Document *doc, int scriptIndex) const
{
    CompiledFunctionOrExpression *foe = functionsAndExpressions->slowAt(scriptIndex);
    QQmlJS::AST::Node *node = foe->node;
    if (QQmlJS::AST::ExpressionStatement *exprStmt = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement *>(node))
        node = exprStmt->expression;
    QQmlJS::SourceLocation start = node->firstSourceLocation();
    QQmlJS::SourceLocation end = node->lastSourceLocation();
    return doc->code.mid(start.offset, end.offset + end.length - start.offset);
}

QStringList Signal::parameterStringList(const QV4::Compiler::StringTableGenerator *stringPool) const
{
    QStringList result;
    result.reserve(parameters->count);
    for (Parameter *param = parameters->first; param; param = param->next)
        result << stringPool->stringForIndex(param->nameIndex);
    return result;
}

Document::Document(bool debugMode)
    : jsModule(debugMode)
    , program(nullptr)
    , jsGenerator(&jsModule)
{
}

ScriptDirectivesCollector::ScriptDirectivesCollector(Document *doc)
    : document(doc)
    , engine(&doc->jsParserEngine)
    , jsGenerator(&doc->jsGenerator)
{
}

void ScriptDirectivesCollector::pragmaLibrary()
{
    document->jsModule.unitFlags |= QV4::CompiledData::Unit::IsSharedLibrary;
}

void ScriptDirectivesCollector::importFile(const QString &jsfile, const QString &module, int lineNumber, int column)
{
    QV4::CompiledData::Import *import = engine->pool()->New<QV4::CompiledData::Import>();
    import->type = QV4::CompiledData::Import::ImportScript;
    import->uriIndex = jsGenerator->registerString(jsfile);
    import->qualifierIndex = jsGenerator->registerString(module);
    import->location.line = lineNumber;
    import->location.column = column;
    document->imports << import;
}

void ScriptDirectivesCollector::importModule(const QString &uri, const QString &version, const QString &module, int lineNumber, int column)
{
    QV4::CompiledData::Import *import = engine->pool()->New<QV4::CompiledData::Import>();
    import->type = QV4::CompiledData::Import::ImportLibrary;
    import->uriIndex = jsGenerator->registerString(uri);
    int vmaj;
    int vmin;
    IRBuilder::extractVersion(QStringRef(&version), &vmaj, &vmin);
    import->majorVersion = vmaj;
    import->minorVersion = vmin;
    import->qualifierIndex = jsGenerator->registerString(module);
    import->location.line = lineNumber;
    import->location.column = column;
    document->imports << import;
}

IRBuilder::IRBuilder(const QSet<QString> &illegalNames)
    : illegalNames(illegalNames)
    , _object(nullptr)
    , _propertyDeclaration(nullptr)
    , pool(nullptr)
    , jsGenerator(nullptr)
{
}

bool IRBuilder::generateFromQml(const QString &code, const QString &url, Document *output)
{
    QQmlJS::AST::UiProgram *program = nullptr;
    {
        QQmlJS::Lexer lexer(&output->jsParserEngine);
        lexer.setCode(code, /*line = */ 1);

        QQmlJS::Parser parser(&output->jsParserEngine);

        const bool parseResult = parser.parse();
        const auto diagnosticMessages = parser.diagnosticMessages();
        if (!parseResult || !diagnosticMessages.isEmpty()) {
            // Extract errors from the parser
            for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
                if (m.isWarning()) {
                    qWarning("%s:%d : %s", qPrintable(url), m.loc.startLine, qPrintable(m.message));
                    continue;
                }

                errors << m;
            }
            return false;
        }
        program = parser.ast();
        Q_ASSERT(program);
    }

    output->code = code;
    output->program = program;

    qSwap(_imports, output->imports);
    qSwap(_pragmas, output->pragmas);
    qSwap(_objects, output->objects);
    this->pool = output->jsParserEngine.pool();
    this->jsGenerator = &output->jsGenerator;

    Q_ASSERT(registerString(QString()) == emptyStringIndex);

    sourceCode = code;

    accept(program->headers);

    if (program->members->next) {
        QQmlJS::SourceLocation loc = program->members->next->firstSourceLocation();
        recordError(loc, QCoreApplication::translate("QQmlParser", "Unexpected object definition"));
        return false;
    }

    QQmlJS::AST::UiObjectDefinition *rootObject = QQmlJS::AST::cast<QQmlJS::AST::UiObjectDefinition*>(program->members->member);
    Q_ASSERT(rootObject);
    int rootObjectIndex = -1;
    if (defineQMLObject(&rootObjectIndex, rootObject)) {
        Q_ASSERT(rootObjectIndex == 0);
    }

    qSwap(_imports, output->imports);
    qSwap(_pragmas, output->pragmas);
    qSwap(_objects, output->objects);

    for (auto object: output->objects)
        object->simplifyRequiredProperties();

    return errors.isEmpty();
}

bool IRBuilder::isSignalPropertyName(const QString &name)
{
    if (name.length() < 3) return false;
    if (!name.startsWith(QLatin1String("on"))) return false;
    int ns = name.length();
    for (int i = 2; i < ns; ++i) {
        const QChar curr = name.at(i);
        if (curr.unicode() == '_') continue;
        if (curr.isUpper()) return true;
        return false;
    }
    return false; // consists solely of underscores - invalid.
}

bool IRBuilder::visit(QQmlJS::AST::UiArrayMemberList *ast)
{
    return QQmlJS::AST::Visitor::visit(ast);
}

bool IRBuilder::visit(QQmlJS::AST::UiProgram *)
{
    Q_ASSERT(!"should not happen");
    return false;
}

bool IRBuilder::visit(QQmlJS::AST::UiObjectDefinition *node)
{
    // The grammar can't distinguish between two different definitions here:
    //     Item { ... }
    // versus
    //     font { ... }
    // The former is a new binding with no property name and "Item" as type name,
    // and the latter is a binding to the font property with no type name but
    // only initializer.

    QQmlJS::AST::UiQualifiedId *lastId = node->qualifiedTypeNameId;
    while (lastId->next)
        lastId = lastId->next;
    bool isType = lastId->name.unicode()->isUpper();
    if (isType) {
        int idx = 0;
        if (!defineQMLObject(&idx, node))
            return false;
        const QQmlJS::SourceLocation nameLocation = node->qualifiedTypeNameId->identifierToken;
        appendBinding(nameLocation, nameLocation, emptyStringIndex, idx);
    } else {
        int idx = 0;
        if (!defineQMLObject(&idx, /*qualfied type name id*/nullptr, node->qualifiedTypeNameId->firstSourceLocation(), node->initializer, /*declarations should go here*/_object))
            return false;
        appendBinding(node->qualifiedTypeNameId, idx);
    }
    return false;
}

bool IRBuilder::visit(QQmlJS::AST::UiInlineComponent *ast)
{
    int idx = -1;
    if (insideInlineComponent) {
        recordError(ast->firstSourceLocation(), QLatin1String("Nested inline components are not supported"));
        return false;
    }
    if (inlineComponentsNames.contains(ast->name.toString())) {
        recordError(ast->firstSourceLocation(), QLatin1String("Inline component names must be unique per file"));
        return false;
    } else {
        inlineComponentsNames.insert(ast->name.toString());
    }
    {
        QScopedValueRollback<bool> rollBack {insideInlineComponent, true};
        if (!defineQMLObject(&idx, ast->component))
            return false;
    }
    Q_ASSERT(idx > 0);
    Object* definedObject = _objects.at(idx);
    definedObject->flags |= QV4::CompiledData::Object::IsInlineComponentRoot;
    definedObject->flags |= QV4::CompiledData::Object::InPartOfInlineComponent;
    definedObject->isInlineComponent = true;
    auto inlineComponent = New<InlineComponent>();
    inlineComponent->nameIndex = registerString(ast->name.toString());
    inlineComponent->objectIndex = idx;
    auto location = ast->firstSourceLocation();
    inlineComponent->location.line = location.startLine;
    inlineComponent->location.column = location.startColumn;
    _object->appendInlineComponent(inlineComponent);
    return false;
}

bool IRBuilder::visit(QQmlJS::AST::UiObjectBinding *node)
{
    int idx = 0;
    if (!defineQMLObject(&idx, node->qualifiedTypeNameId, node->qualifiedTypeNameId->firstSourceLocation(), node->initializer))
        return false;
    appendBinding(node->qualifiedId, idx, node->hasOnToken);
    return false;
}

bool IRBuilder::visit(QQmlJS::AST::UiScriptBinding *node)
{
    appendBinding(node->qualifiedId, node->statement, node);
    return false;
}

bool IRBuilder::visit(QQmlJS::AST::UiArrayBinding *node)
{
    const QQmlJS::SourceLocation qualifiedNameLocation = node->qualifiedId->identifierToken;
    Object *object = nullptr;
    QQmlJS::AST::UiQualifiedId *name = node->qualifiedId;
    if (!resolveQualifiedId(&name, &object))
        return false;

    qSwap(_object, object);

    const int propertyNameIndex = registerString(name->name.toString());

    if (bindingsTarget()->findBinding(propertyNameIndex) != nullptr) {
        recordError(name->identifierToken, tr("Property value set multiple times"));
        return false;
    }

    QVarLengthArray<QQmlJS::AST::UiArrayMemberList *, 16> memberList;
    QQmlJS::AST::UiArrayMemberList *member = node->members;
    while (member) {
        memberList.append(member);
        member = member->next;
    }
    for (int i = memberList.count() - 1; i >= 0; --i) {
        member = memberList.at(i);
        QQmlJS::AST::UiObjectDefinition *def = QQmlJS::AST::cast<QQmlJS::AST::UiObjectDefinition*>(member->member);

        int idx = 0;
        if (!defineQMLObject(&idx, def))
            return false;
        appendBinding(qualifiedNameLocation, name->identifierToken, propertyNameIndex, idx, /*isListItem*/ true);
    }

    qSwap(_object, object);
    return false;
}

bool IRBuilder::visit(QQmlJS::AST::UiHeaderItemList *list)
{
    return QQmlJS::AST::Visitor::visit(list);
}

bool IRBuilder::visit(QQmlJS::AST::UiObjectInitializer *ast)
{
    return QQmlJS::AST::Visitor::visit(ast);
}

bool IRBuilder::visit(QQmlJS::AST::UiObjectMemberList *ast)
{
    return QQmlJS::AST::Visitor::visit(ast);
}

bool IRBuilder::visit(QQmlJS::AST::UiParameterList *ast)
{
    return QQmlJS::AST::Visitor::visit(ast);
}

bool IRBuilder::visit(QQmlJS::AST::UiQualifiedId *id)
{
    return QQmlJS::AST::Visitor::visit(id);
}

void IRBuilder::accept(QQmlJS::AST::Node *node)
{
    QQmlJS::AST::Node::accept(node, this);
}

bool IRBuilder::defineQMLObject(int *objectIndex, QQmlJS::AST::UiQualifiedId *qualifiedTypeNameId, const QQmlJS::SourceLocation &location, QQmlJS::AST::UiObjectInitializer *initializer, Object *declarationsOverride)
{
    if (QQmlJS::AST::UiQualifiedId *lastName = qualifiedTypeNameId) {
        while (lastName->next)
            lastName = lastName->next;
        if (!lastName->name.constData()->isUpper()) {
            recordError(lastName->identifierToken, tr("Expected type name"));
            return false;
        }
    }

    Object *obj = New<Object>();

    _objects.append(obj);
    *objectIndex = _objects.size() - 1;
    qSwap(_object, obj);

    _object->init(pool, registerString(asString(qualifiedTypeNameId)), emptyStringIndex, location);
    _object->declarationsOverride = declarationsOverride;
    if (insideInlineComponent) {
        _object->flags |= QV4::CompiledData::Object::InPartOfInlineComponent;
    }

    // A new object is also a boundary for property declarations.
    Property *declaration = nullptr;
    qSwap(_propertyDeclaration, declaration);

    accept(initializer);

    qSwap(_propertyDeclaration, declaration);

    qSwap(_object, obj);

    if (!errors.isEmpty())
        return false;

    QQmlJS::SourceLocation loc;
    QString error = sanityCheckFunctionNames(obj, illegalNames, &loc);
    if (!error.isEmpty()) {
        recordError(loc, error);
        return false;
    }

    return true;
}

bool IRBuilder::visit(QQmlJS::AST::UiImport *node)
{
    QString uri;
    QV4::CompiledData::Import *import = New<QV4::CompiledData::Import>();

    if (!node->fileName.isNull()) {
        uri = node->fileName.toString();

        if (uri.endsWith(QLatin1String(".js")) || uri.endsWith(QLatin1String(".mjs"))) {
            import->type = QV4::CompiledData::Import::ImportScript;
        } else {
            import->type = QV4::CompiledData::Import::ImportFile;
        }
    } else {
        import->type = QV4::CompiledData::Import::ImportLibrary;
        uri = asString(node->importUri);
    }

    import->qualifierIndex = emptyStringIndex;

    // Qualifier
    if (!node->importId.isNull()) {
        QString qualifier = node->importId.toString();
        if (!qualifier.at(0).isUpper()) {
            recordError(node->importIdToken, QCoreApplication::translate("QQmlParser","Invalid import qualifier ID"));
            return false;
        }
        if (qualifier == QLatin1String("Qt")) {
            recordError(node->importIdToken, QCoreApplication::translate("QQmlParser","Reserved name \"Qt\" cannot be used as an qualifier"));
            return false;
        }
        import->qualifierIndex = registerString(qualifier);

        // Check for script qualifier clashes
        bool isScript = import->type == QV4::CompiledData::Import::ImportScript;
        for (int ii = 0; ii < _imports.count(); ++ii) {
            const QV4::CompiledData::Import *other = _imports.at(ii);
            bool otherIsScript = other->type == QV4::CompiledData::Import::ImportScript;

            if ((isScript || otherIsScript) && qualifier == jsGenerator->stringForIndex(other->qualifierIndex)) {
                recordError(node->importIdToken, QCoreApplication::translate("QQmlParser","Script import qualifiers must be unique."));
                return false;
            }
        }

    } else if (import->type == QV4::CompiledData::Import::ImportScript) {
        recordError(node->fileNameToken, QCoreApplication::translate("QQmlParser","Script import requires a qualifier"));
        return false;
    }

    if (node->version) {
        import->majorVersion = node->version->majorVersion;
        import->minorVersion = node->version->minorVersion;
    } else if (import->type == QV4::CompiledData::Import::ImportLibrary) {
        recordError(node->importIdToken, QCoreApplication::translate("QQmlParser","Library import requires a version"));
        return false;
    } else {
        // For backward compatibility in how the imports are loaded we
        // must otherwise initialize the major and minor version to -1.
        import->majorVersion = -1;
        import->minorVersion = -1;
    }

    import->location.line = node->importToken.startLine;
    import->location.column = node->importToken.startColumn;

    import->uriIndex = registerString(uri);

    _imports.append(import);

    return false;
}

bool IRBuilder::visit(QQmlJS::AST::UiPragma *node)
{
    Pragma *pragma = New<Pragma>();

    // For now the only valid pragma is Singleton, so lets validate the input
    if (!node->name.isNull())
    {
        if (QLatin1String("Singleton") == node->name)
        {
            pragma->type = Pragma::PragmaSingleton;
        } else {
            recordError(node->pragmaToken, QCoreApplication::translate("QQmlParser","Pragma requires a valid qualifier"));
            return false;
        }
    } else {
        recordError(node->pragmaToken, QCoreApplication::translate("QQmlParser","Pragma requires a valid qualifier"));
        return false;
    }

    pragma->location.line = node->pragmaToken.startLine;
    pragma->location.column = node->pragmaToken.startColumn;
    _pragmas.append(pragma);

    return false;
}

static QStringList astNodeToStringList(QQmlJS::AST::Node *node)
{
    if (node->kind == QQmlJS::AST::Node::Kind_IdentifierExpression) {
        QString name =
            static_cast<QQmlJS::AST::IdentifierExpression *>(node)->name.toString();
        return QStringList() << name;
    } else if (node->kind == QQmlJS::AST::Node::Kind_FieldMemberExpression) {
        QQmlJS::AST::FieldMemberExpression *expr = static_cast<QQmlJS::AST::FieldMemberExpression *>(node);

        QStringList rv = astNodeToStringList(expr->base);
        if (rv.isEmpty())
            return rv;
        rv.append(expr->name.toString());
        return rv;
    }
    return QStringList();
}

bool IRBuilder::visit(QQmlJS::AST::UiEnumDeclaration *node)
{
    Enum *enumeration = New<Enum>();
    QString enumName = node->name.toString();
    enumeration->nameIndex = registerString(enumName);

    if (enumName.at(0).isLower())
        COMPILE_EXCEPTION(node->enumToken, tr("Scoped enum names must begin with an upper case letter"));

    enumeration->location.line = node->enumToken.startLine;
    enumeration->location.column = node->enumToken.startColumn;

    enumeration->enumValues = New<PoolList<EnumValue>>();

    QQmlJS::AST::UiEnumMemberList *e = node->members;
    while (e) {
        EnumValue *enumValue = New<EnumValue>();
        QString member = e->member.toString();
        enumValue->nameIndex = registerString(member);
        if (member.at(0).isLower())
            COMPILE_EXCEPTION(e->memberToken, tr("Enum names must begin with an upper case letter"));

        double part;
        if (std::modf(e->value, &part) != 0.0)
            COMPILE_EXCEPTION(e->valueToken, tr("Enum value must be an integer"));
        if (e->value > std::numeric_limits<qint32>::max() || e->value < std::numeric_limits<qint32>::min())
            COMPILE_EXCEPTION(e->valueToken, tr("Enum value out of range"));
        enumValue->value = e->value;

        enumValue->location.line = e->memberToken.startLine;
        enumValue->location.column = e->memberToken.startColumn;
        enumeration->enumValues->append(enumValue);

        e = e->next;
    }

    QString error = _object->appendEnum(enumeration);
    if (!error.isEmpty()) {
        recordError(node->enumToken, error);
        return false;
    }

    return false;
}


bool IRBuilder::visit(QQmlJS::AST::UiPublicMember *node)
{
    if (node->type == QQmlJS::AST::UiPublicMember::Signal) {
        Signal *signal = New<Signal>();
        const QString signalName = node->name.toString();
        signal->nameIndex = registerString(signalName);

        QQmlJS::SourceLocation loc = node->typeToken;
        signal->location.line = loc.startLine;
        signal->location.column = loc.startColumn;

        signal->parameters = New<PoolList<Parameter> >();

        QQmlJS::AST::UiParameterList *p = node->parameters;
        while (p) {
            const QString memberType = asString(p->type);

            if (memberType.isEmpty()) {
                recordError(node->typeToken, QCoreApplication::translate("QQmlParser","Expected parameter type"));
                return false;
            }

            Parameter *param = New<Parameter>();
            if (!param->init(jsGenerator, p->name.toString(), memberType)) {
                QString errStr = QCoreApplication::translate("QQmlParser","Invalid signal parameter type: ");
                errStr.append(memberType);
                recordError(node->typeToken, errStr);
                return false;
            }
            signal->parameters->append(param);
            p = p->next;
        }

        for (const QChar &ch : signalName) {
            if (ch.isLower())
                break;
            if (ch.isUpper()) {
                COMPILE_EXCEPTION(node->identifierToken,
                                  tr("Signal names cannot begin with an upper case letter"));
            }
        }

        if (illegalNames.contains(signalName))
            COMPILE_EXCEPTION(node->identifierToken, tr("Illegal signal name"));

        QString error = _object->appendSignal(signal);
        if (!error.isEmpty()) {
            recordError(node->identifierToken, error);
            return false;
        }
    } else {
        QString memberType = asString(node->memberType);
        if (memberType == QLatin1String("alias")) {
            return appendAlias(node);
        } else {
            const QStringRef &name = node->name;

            Property *property = New<Property>();
            property->isReadOnly = node->isReadonlyMember;
            property->isRequired = node->isRequired;

            QV4::CompiledData::BuiltinType builtinPropertyType = Parameter::stringToBuiltinType(memberType);
            bool typeFound = builtinPropertyType != QV4::CompiledData::BuiltinType::InvalidBuiltin;
            if (typeFound)
                property->setBuiltinType(builtinPropertyType);

            if (!typeFound && memberType.at(0).isUpper()) {
                const QStringRef &typeModifier = node->typeModifier;

                property->setCustomType(registerString(memberType));
                if (typeModifier == QLatin1String("list")) {
                    property->isList = true;
                } else if (!typeModifier.isEmpty()) {
                    recordError(node->typeModifierToken, QCoreApplication::translate("QQmlParser","Invalid property type modifier"));
                    return false;
                }
                typeFound = true;
            } else if (!node->typeModifier.isNull()) {
                recordError(node->typeModifierToken, QCoreApplication::translate("QQmlParser","Unexpected property type modifier"));
                return false;
            }

            if (!typeFound) {
                recordError(node->typeToken, QCoreApplication::translate("QQmlParser","Expected property type"));
                return false;
            }

            const QString propName = name.toString();
            property->nameIndex = registerString(propName);

            QQmlJS::SourceLocation loc = node->firstSourceLocation();
            property->location.line = loc.startLine;
            property->location.column = loc.startColumn;

            QQmlJS::SourceLocation errorLocation;
            QString error;

            if (illegalNames.contains(propName))
                error = tr("Illegal property name");
            else
                error = _object->appendProperty(property, propName, node->isDefaultMember, node->defaultToken, &errorLocation);

            if (!error.isEmpty()) {
                if (errorLocation.startLine == 0)
                    errorLocation = node->identifierToken;

                recordError(errorLocation, error);
                return false;
            }

            qSwap(_propertyDeclaration, property);
            if (node->binding) {
                // process QML-like initializers (e.g. property Object o: Object {})
                QQmlJS::AST::Node::accept(node->binding, this);
            } else if (node->statement) {
                if (!isRedundantNullInitializerForPropertyDeclaration(_propertyDeclaration, node->statement))
                    appendBinding(node->identifierToken, node->identifierToken, _propertyDeclaration->nameIndex, node->statement, node);
            }
            qSwap(_propertyDeclaration, property);
        }
    }

    return false;
}

bool IRBuilder::visit(QQmlJS::AST::UiSourceElement *node)
{
    if (QQmlJS::AST::FunctionExpression *funDecl = node->sourceElement->asFunctionDefinition()) {
        CompiledFunctionOrExpression *foe = New<CompiledFunctionOrExpression>();
        foe->node = funDecl;
        foe->parentNode = funDecl;
        foe->nameIndex = registerString(funDecl->name.toString());
        const int index = _object->functionsAndExpressions->append(foe);

        Function *f = New<Function>();
        QQmlJS::SourceLocation loc = funDecl->identifierToken;
        f->location.line = loc.startLine;
        f->location.column = loc.startColumn;
        f->index = index;
        f->nameIndex = registerString(funDecl->name.toString());

        QString returnTypeName = funDecl->typeAnnotation ? funDecl->typeAnnotation->type->toString() : QString();
        Parameter::initType(&f->returnType, jsGenerator, registerString(returnTypeName));

        const QQmlJS::AST::BoundNames formals = funDecl->formals ? funDecl->formals->formals() : QQmlJS::AST::BoundNames();
        int formalsCount = formals.size();
        f->formals.allocate(pool, formalsCount);

        int i = 0;
        for (const auto &arg : formals) {
            f->formals[i].init(jsGenerator, arg.id, arg.typeName());
            ++i;
        }

        _object->appendFunction(f);
    } else {
        recordError(node->firstSourceLocation(), QCoreApplication::translate("QQmlParser","JavaScript declaration outside Script element"));
    }
    return false;
}

bool IRBuilder::visit(AST::UiRequired *ast)
{
    auto extraData = New<RequiredPropertyExtraData>();
    extraData->nameIndex = registerString(ast->name.toString());
    _object->appendRequiredPropertyExtraData(extraData);
    return false;
}

QString IRBuilder::asString(QQmlJS::AST::UiQualifiedId *node)
{
    QString s;

    for (QQmlJS::AST::UiQualifiedId *it = node; it; it = it->next) {
        s.append(it->name);

        if (it->next)
            s.append(QLatin1Char('.'));
    }

    return s;
}

QStringRef IRBuilder::asStringRef(QQmlJS::AST::Node *node)
{
    if (!node)
        return QStringRef();

    return textRefAt(node->firstSourceLocation(), node->lastSourceLocation());
}

void IRBuilder::extractVersion(const QStringRef &string, int *maj, int *min)
{
    *maj = -1; *min = -1;

    if (!string.isEmpty()) {

        int dot = string.indexOf(QLatin1Char('.'));

        if (dot < 0) {
            *maj = string.toInt();
            *min = 0;
        } else {
            *maj = string.left(dot).toInt();
            *min = string.mid(dot + 1).toInt();
        }
    }
}

QStringRef IRBuilder::textRefAt(const QQmlJS::SourceLocation &first, const QQmlJS::SourceLocation &last) const
{
    return QStringRef(&sourceCode, first.offset, last.offset + last.length - first.offset);
}

void IRBuilder::setBindingValue(QV4::CompiledData::Binding *binding, QQmlJS::AST::Statement *statement, QQmlJS::AST::Node *parentNode)
{
    QQmlJS::SourceLocation loc = statement->firstSourceLocation();
    binding->valueLocation.line = loc.startLine;
    binding->valueLocation.column = loc.startColumn;
    binding->type = QV4::CompiledData::Binding::Type_Invalid;
    if (_propertyDeclaration && _propertyDeclaration->isReadOnly)
        binding->flags |= QV4::CompiledData::Binding::InitializerForReadOnlyDeclaration;

    QQmlJS::AST::ExpressionStatement *exprStmt = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement *>(statement);
    if (exprStmt) {
        QQmlJS::AST::ExpressionNode * const expr = exprStmt->expression;
        if (QQmlJS::AST::StringLiteral *lit = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(expr)) {
            binding->type = QV4::CompiledData::Binding::Type_String;
            binding->stringIndex = registerString(lit->value.toString());
        } else if (expr->kind == QQmlJS::AST::Node::Kind_TrueLiteral) {
            binding->type = QV4::CompiledData::Binding::Type_Boolean;
            binding->value.b = true;
        } else if (expr->kind == QQmlJS::AST::Node::Kind_FalseLiteral) {
            binding->type = QV4::CompiledData::Binding::Type_Boolean;
            binding->value.b = false;
        } else if (QQmlJS::AST::NumericLiteral *lit = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(expr)) {
            binding->type = QV4::CompiledData::Binding::Type_Number;
            binding->value.constantValueIndex = jsGenerator->registerConstant(QV4::Encode(lit->value));
        } else if (QQmlJS::AST::CallExpression *call = QQmlJS::AST::cast<QQmlJS::AST::CallExpression *>(expr)) {
            if (QQmlJS::AST::IdentifierExpression *base = QQmlJS::AST::cast<QQmlJS::AST::IdentifierExpression *>(call->base)) {
                tryGeneratingTranslationBinding(base->name, call->arguments, binding);
                // If it wasn't a translation binding, a normal script binding will be generated
                // below.
            }
        } else if (QQmlJS::AST::cast<QQmlJS::AST::FunctionExpression *>(expr)) {
            binding->flags |= QV4::CompiledData::Binding::IsFunctionExpression;
        } else if (QQmlJS::AST::UnaryMinusExpression *unaryMinus = QQmlJS::AST::cast<QQmlJS::AST::UnaryMinusExpression *>(expr)) {
            if (QQmlJS::AST::NumericLiteral *lit = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(unaryMinus->expression)) {
                binding->type = QV4::CompiledData::Binding::Type_Number;
                binding->value.constantValueIndex = jsGenerator->registerConstant(QV4::Encode(-lit->value));
            }
        } else if (QQmlJS::AST::cast<QQmlJS::AST::NullExpression *>(expr)) {
            binding->type = QV4::CompiledData::Binding::Type_Null;
            binding->value.nullMarker = 0;
        }
    }

    // Do binding instead
    if (binding->type == QV4::CompiledData::Binding::Type_Invalid) {
        binding->type = QV4::CompiledData::Binding::Type_Script;

        CompiledFunctionOrExpression *expr = New<CompiledFunctionOrExpression>();
        expr->node = statement;
        expr->parentNode = parentNode;
        expr->nameIndex = registerString(QLatin1String("expression for ")
                                         + stringAt(binding->propertyNameIndex));
        const int index = bindingsTarget()->functionsAndExpressions->append(expr);
        binding->value.compiledScriptIndex = index;
        // We don't need to store the binding script as string, except for script strings
        // and types with custom parsers. Those will be added later in the compilation phase.
        binding->stringIndex = emptyStringIndex;
    }
}

void IRBuilder::tryGeneratingTranslationBinding(const QStringRef &base, AST::ArgumentList *args, QV4::CompiledData::Binding *binding)
{
    if (base == QLatin1String("qsTr")) {
        QV4::CompiledData::TranslationData translationData;
        translationData.number = -1;
        translationData.commentIndex = 0; // empty string
        translationData.padding = 0;

        if (!args || !args->expression)
            return; // no arguments, stop

        QStringRef translation;
        if (QQmlJS::AST::StringLiteral *arg1 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression)) {
            translation = arg1->value;
        } else {
            return; // first argument is not a string, stop
        }
        translationData.stringIndex = jsGenerator->registerString(translation.toString());

        args = args->next;

        if (args) {
            QQmlJS::AST::StringLiteral *arg2 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression);
            if (!arg2)
                return; // second argument is not a string, stop
            translationData.commentIndex = jsGenerator->registerString(arg2->value.toString());

            args = args->next;
            if (args) {
                if (QQmlJS::AST::NumericLiteral *arg3 = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(args->expression)) {
                    translationData.number = int(arg3->value);
                    args = args->next;
                } else {
                    return; // third argument is not a translation number, stop
                }
            }
        }

        if (args)
            return; // too many arguments, stop

        binding->type = QV4::CompiledData::Binding::Type_Translation;
        binding->value.translationDataIndex = jsGenerator->registerTranslation(translationData);
    } else if (base == QLatin1String("qsTrId")) {
        QV4::CompiledData::TranslationData translationData;
        translationData.number = -1;
        translationData.commentIndex = 0; // empty string, but unused
        translationData.padding = 0;

        if (!args || !args->expression)
            return; // no arguments, stop

        QStringRef id;
        if (QQmlJS::AST::StringLiteral *arg1 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression)) {
            id = arg1->value;
        } else {
            return; // first argument is not a string, stop
        }
        translationData.stringIndex = jsGenerator->registerString(id.toString());

        args = args->next;

        if (args) {
            if (QQmlJS::AST::NumericLiteral *arg3 = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(args->expression)) {
                translationData.number = int(arg3->value);
                args = args->next;
            } else {
                return; // third argument is not a translation number, stop
            }
        }

        if (args)
            return; // too many arguments, stop

        binding->type = QV4::CompiledData::Binding::Type_TranslationById;
        binding->value.translationDataIndex = jsGenerator->registerTranslation(translationData);
    } else if (base == QLatin1String("QT_TR_NOOP") || base == QLatin1String("QT_TRID_NOOP")) {
        if (!args || !args->expression)
            return; // no arguments, stop

        QStringRef str;
        if (QQmlJS::AST::StringLiteral *arg1 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression)) {
            str = arg1->value;
        } else {
            return; // first argument is not a string, stop
        }

        args = args->next;
        if (args)
            return; // too many arguments, stop

        binding->type = QV4::CompiledData::Binding::Type_String;
        binding->stringIndex = jsGenerator->registerString(str.toString());
    } else if (base == QLatin1String("QT_TRANSLATE_NOOP")) {
        if (!args || !args->expression)
            return; // no arguments, stop

        args = args->next;
        if (!args || !args->expression)
            return; // no second arguments, stop

        QStringRef str;
        if (QQmlJS::AST::StringLiteral *arg2 = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(args->expression)) {
            str = arg2->value;
        } else {
            return; // first argument is not a string, stop
        }

        args = args->next;
        if (args)
            return; // too many arguments, stop

        binding->type = QV4::CompiledData::Binding::Type_String;
        binding->stringIndex = jsGenerator->registerString(str.toString());
    }
}

void IRBuilder::appendBinding(QQmlJS::AST::UiQualifiedId *name, QQmlJS::AST::Statement *value, QQmlJS::AST::Node *parentNode)
{
    const QQmlJS::SourceLocation qualifiedNameLocation = name->identifierToken;
    Object *object = nullptr;
    if (!resolveQualifiedId(&name, &object))
        return;
    if (_object == object && name->name == QLatin1String("id")) {
        setId(name->identifierToken, value);
        return;
    }
    qSwap(_object, object);
    appendBinding(qualifiedNameLocation, name->identifierToken, registerString(name->name.toString()), value, parentNode);
    qSwap(_object, object);
}

void IRBuilder::appendBinding(QQmlJS::AST::UiQualifiedId *name, int objectIndex, bool isOnAssignment)
{
    const QQmlJS::SourceLocation qualifiedNameLocation = name->identifierToken;
    Object *object = nullptr;
    if (!resolveQualifiedId(&name, &object, isOnAssignment))
        return;
    qSwap(_object, object);
    appendBinding(qualifiedNameLocation, name->identifierToken, registerString(name->name.toString()), objectIndex, /*isListItem*/false, isOnAssignment);
    qSwap(_object, object);
}

void IRBuilder::appendBinding(const QQmlJS::SourceLocation &qualifiedNameLocation, const QQmlJS::SourceLocation &nameLocation, quint32 propertyNameIndex,
                              QQmlJS::AST::Statement *value, QQmlJS::AST::Node *parentNode)
{
    Binding *binding = New<Binding>();
    binding->propertyNameIndex = propertyNameIndex;
    binding->offset = nameLocation.offset;
    binding->location.line = nameLocation.startLine;
    binding->location.column = nameLocation.startColumn;
    binding->flags = 0;
    setBindingValue(binding, value, parentNode);
    QString error = bindingsTarget()->appendBinding(binding, /*isListBinding*/false);
    if (!error.isEmpty()) {
        recordError(qualifiedNameLocation, error);
    }
}

void IRBuilder::appendBinding(const QQmlJS::SourceLocation &qualifiedNameLocation, const QQmlJS::SourceLocation &nameLocation, quint32 propertyNameIndex, int objectIndex, bool isListItem, bool isOnAssignment)
{
    if (stringAt(propertyNameIndex) == QLatin1String("id")) {
        recordError(nameLocation, tr("Invalid component id specification"));
        return;
    }

    Binding *binding = New<Binding>();
    binding->propertyNameIndex = propertyNameIndex;
    binding->offset = nameLocation.offset;
    binding->location.line = nameLocation.startLine;
    binding->location.column = nameLocation.startColumn;

    const Object *obj = _objects.at(objectIndex);
    binding->valueLocation = obj->location;

    binding->flags = 0;

    if (_propertyDeclaration && _propertyDeclaration->isReadOnly)
        binding->flags |= QV4::CompiledData::Binding::InitializerForReadOnlyDeclaration;

    // No type name on the initializer means it must be a group property
    if (_objects.at(objectIndex)->inheritedTypeNameIndex == emptyStringIndex)
        binding->type = QV4::CompiledData::Binding::Type_GroupProperty;
    else
        binding->type = QV4::CompiledData::Binding::Type_Object;

    if (isOnAssignment)
        binding->flags |= QV4::CompiledData::Binding::IsOnAssignment;
    if (isListItem)
        binding->flags |= QV4::CompiledData::Binding::IsListItem;

    binding->value.objectIndex = objectIndex;
    QString error = bindingsTarget()->appendBinding(binding, isListItem);
    if (!error.isEmpty()) {
        recordError(qualifiedNameLocation, error);
    }
}

bool IRBuilder::appendAlias(QQmlJS::AST::UiPublicMember *node)
{
    Alias *alias = New<Alias>();
    alias->flags = 0;
    if (node->isReadonlyMember)
        alias->flags |= QV4::CompiledData::Alias::IsReadOnly;

    const QString propName = node->name.toString();
    alias->nameIndex = registerString(propName);

    QQmlJS::SourceLocation loc = node->firstSourceLocation();
    alias->location.line = loc.startLine;
    alias->location.column = loc.startColumn;

    alias->propertyNameIndex = emptyStringIndex;

    if (!node->statement && !node->binding)
        COMPILE_EXCEPTION(loc, tr("No property alias location"));

    QQmlJS::SourceLocation rhsLoc;
    if (node->binding)
        rhsLoc = node->binding->firstSourceLocation();
    else if (node->statement)
        rhsLoc = node->statement->firstSourceLocation();
    else
        rhsLoc = node->semicolonToken;
    alias->referenceLocation.line = rhsLoc.startLine;
    alias->referenceLocation.column = rhsLoc.startColumn;

    QStringList aliasReference;

    if (QQmlJS::AST::ExpressionStatement *stmt = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement*>(node->statement)) {
        aliasReference = astNodeToStringList(stmt->expression);
        if (aliasReference.isEmpty()) {
            if (isStatementNodeScript(node->statement)) {
                COMPILE_EXCEPTION(rhsLoc, tr("Invalid alias reference. An alias reference must be specified as <id>, <id>.<property> or <id>.<value property>.<property>"));
            } else {
                COMPILE_EXCEPTION(rhsLoc, tr("Invalid alias location"));
            }
        }
    } else {
        COMPILE_EXCEPTION(rhsLoc, tr("Invalid alias reference. An alias reference must be specified as <id>, <id>.<property> or <id>.<value property>.<property>"));
    }

    if (aliasReference.count() < 1 || aliasReference.count() > 3)
        COMPILE_EXCEPTION(rhsLoc, tr("Invalid alias reference. An alias reference must be specified as <id>, <id>.<property> or <id>.<value property>.<property>"));

     alias->idIndex = registerString(aliasReference.first());

     QString propertyValue = aliasReference.value(1);
     if (aliasReference.count() == 3)
         propertyValue += QLatin1Char('.') + aliasReference.at(2);
     alias->propertyNameIndex = registerString(propertyValue);

     QQmlJS::SourceLocation errorLocation;
     QString error;

     if (illegalNames.contains(propName))
         error = tr("Illegal property name");
     else
         error = _object->appendAlias(alias, propName, node->isDefaultMember, node->defaultToken, &errorLocation);

     if (!error.isEmpty()) {
         if (errorLocation.startLine == 0)
             errorLocation = node->identifierToken;

         recordError(errorLocation, error);
         return false;
     }

     return false;
}

Object *IRBuilder::bindingsTarget() const
{
    if (_propertyDeclaration && _object->declarationsOverride)
        return _object->declarationsOverride;
    return _object;
}

bool IRBuilder::setId(const QQmlJS::SourceLocation &idLocation, QQmlJS::AST::Statement *value)
{
    QQmlJS::SourceLocation loc = value->firstSourceLocation();
    QStringRef str;

    QQmlJS::AST::Node *node = value;
    if (QQmlJS::AST::ExpressionStatement *stmt = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement *>(node)) {
        if (QQmlJS::AST::StringLiteral *lit = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(stmt->expression)) {
            str = lit->value;
            node = nullptr;
        } else
            node = stmt->expression;
    }

    if (node && str.isEmpty())
        str = asStringRef(node);

    if (str.isEmpty())
        COMPILE_EXCEPTION(loc, tr( "Invalid empty ID"));

    QChar ch = str.at(0);
    if (ch.isLetter() && !ch.isLower())
        COMPILE_EXCEPTION(loc, tr( "IDs cannot start with an uppercase letter"));

    QChar u(QLatin1Char('_'));
    if (!ch.isLetter() && ch != u)
        COMPILE_EXCEPTION(loc, tr( "IDs must start with a letter or underscore"));

    for (int ii = 1; ii < str.count(); ++ii) {
        ch = str.at(ii);
        if (!ch.isLetterOrNumber() && ch != u)
            COMPILE_EXCEPTION(loc, tr( "IDs must contain only letters, numbers, and underscores"));
    }

    QString idQString(str.toString());
    if (illegalNames.contains(idQString))
        COMPILE_EXCEPTION(loc, tr( "ID illegally masks global JavaScript property"));

    if (_object->idNameIndex != emptyStringIndex)
        COMPILE_EXCEPTION(idLocation, tr("Property value set multiple times"));

    _object->idNameIndex = registerString(idQString);
    _object->locationOfIdProperty.line = idLocation.startLine;
    _object->locationOfIdProperty.column = idLocation.startColumn;

    return true;
}

bool IRBuilder::resolveQualifiedId(QQmlJS::AST::UiQualifiedId **nameToResolve, Object **object, bool onAssignment)
{
    QQmlJS::AST::UiQualifiedId *qualifiedIdElement = *nameToResolve;

    if (qualifiedIdElement->name == QLatin1String("id") && qualifiedIdElement->next)
        COMPILE_EXCEPTION(qualifiedIdElement->identifierToken, tr( "Invalid use of id property"));

    // If it's a namespace, prepend the qualifier and we'll resolve it later to the correct type.
    QString currentName = qualifiedIdElement->name.toString();
    if (qualifiedIdElement->next) {
        for (const QV4::CompiledData::Import* import : qAsConst(_imports))
            if (import->qualifierIndex != emptyStringIndex
                && stringAt(import->qualifierIndex) == currentName) {
                qualifiedIdElement = qualifiedIdElement->next;
                currentName += QLatin1Char('.') + qualifiedIdElement->name;

                if (!qualifiedIdElement->name.unicode()->isUpper())
                    COMPILE_EXCEPTION(qualifiedIdElement->firstSourceLocation(), tr("Expected type name"));

                break;
            }
    }

    *object = _object;
    while (qualifiedIdElement->next) {
        const quint32 propertyNameIndex = registerString(currentName);
        const bool isAttachedProperty = qualifiedIdElement->name.unicode()->isUpper();

        Binding *binding = (*object)->findBinding(propertyNameIndex);
        if (binding) {
            if (isAttachedProperty) {
                if (!binding->isAttachedProperty())
                    binding = nullptr;
            } else if (!binding->isGroupProperty()) {
                binding = nullptr;
            }
        }
        if (!binding) {
            binding = New<Binding>();
            binding->propertyNameIndex = propertyNameIndex;
            binding->offset = qualifiedIdElement->identifierToken.offset;
            binding->location.line = qualifiedIdElement->identifierToken.startLine;
            binding->location.column = qualifiedIdElement->identifierToken.startColumn;
            binding->valueLocation.line = qualifiedIdElement->next->identifierToken.startLine;
            binding->valueLocation.column = qualifiedIdElement->next->identifierToken.startColumn;
            binding->flags = 0;

            if (onAssignment)
                binding->flags |= QV4::CompiledData::Binding::IsOnAssignment;

            if (isAttachedProperty)
                binding->type = QV4::CompiledData::Binding::Type_AttachedProperty;
            else
                binding->type = QV4::CompiledData::Binding::Type_GroupProperty;

            int objIndex = 0;
            if (!defineQMLObject(&objIndex, nullptr, QQmlJS::SourceLocation(), nullptr, nullptr))
                return false;
            binding->value.objectIndex = objIndex;

            QString error = (*object)->appendBinding(binding, /*isListBinding*/false);
            if (!error.isEmpty()) {
                recordError(qualifiedIdElement->identifierToken, error);
                return false;
            }
            *object = _objects.at(objIndex);
        } else {
            Q_ASSERT(binding->isAttachedProperty() || binding->isGroupProperty());
            *object = _objects.at(binding->value.objectIndex);
        }

        qualifiedIdElement = qualifiedIdElement->next;
        if (qualifiedIdElement)
            currentName = qualifiedIdElement->name.toString();
    }
    *nameToResolve = qualifiedIdElement;
    return true;
}

void IRBuilder::recordError(const QQmlJS::SourceLocation &location, const QString &description)
{
    QQmlJS::DiagnosticMessage error;
    error.loc = location;
    error.message = description;
    errors << error;
}

bool IRBuilder::isStatementNodeScript(QQmlJS::AST::Statement *statement)
{
    if (QQmlJS::AST::ExpressionStatement *stmt = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement *>(statement)) {
        QQmlJS::AST::ExpressionNode *expr = stmt->expression;
        if (QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(expr))
            return false;
        else if (expr->kind == QQmlJS::AST::Node::Kind_TrueLiteral)
            return false;
        else if (expr->kind == QQmlJS::AST::Node::Kind_FalseLiteral)
            return false;
        else if (QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(expr))
            return false;
        else {

            if (QQmlJS::AST::UnaryMinusExpression *unaryMinus = QQmlJS::AST::cast<QQmlJS::AST::UnaryMinusExpression *>(expr)) {
               if (QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(unaryMinus->expression)) {
                   return false;
               }
            }
        }
    }

    return true;
}

bool IRBuilder::isRedundantNullInitializerForPropertyDeclaration(Property *property, QQmlJS::AST::Statement *statement)
{
    if (property->isBuiltinType || property->isList)
        return false;
    QQmlJS::AST::ExpressionStatement *exprStmt = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement *>(statement);
    if (!exprStmt)
        return false;
    QQmlJS::AST::ExpressionNode * const expr = exprStmt->expression;
    return QQmlJS::AST::cast<QQmlJS::AST::NullExpression *>(expr);
}

void QmlUnitGenerator::generate(Document &output, const QV4::CompiledData::DependentTypesHasher &dependencyHasher)
{
    output.jsGenerator.stringTable.registerString(output.jsModule.fileName);
    output.jsGenerator.stringTable.registerString(output.jsModule.finalUrl);

    QV4::CompiledData::Unit *jsUnit = nullptr;

    // We may already have unit data if we're loading an ahead-of-time generated cache file.
    if (output.javaScriptCompilationUnit.data) {
        jsUnit = const_cast<QV4::CompiledData::Unit *>(output.javaScriptCompilationUnit.data);
        output.javaScriptCompilationUnit.dynamicStrings = output.jsGenerator.stringTable.allStrings();
    } else {
        QV4::CompiledData::Unit *createdUnit;
        jsUnit = createdUnit = output.jsGenerator.generateUnit();

        // enable flag if we encountered pragma Singleton
        for (Pragma *p : qAsConst(output.pragmas)) {
            if (p->type == Pragma::PragmaSingleton) {
                createdUnit->flags |= QV4::CompiledData::Unit::IsSingleton;
                break;
            }
        }

        if (dependencyHasher) {
            const QByteArray checksum = dependencyHasher();
            if (checksum.size() == sizeof(createdUnit->dependencyMD5Checksum)) {
                memcpy(createdUnit->dependencyMD5Checksum, checksum.constData(),
                       sizeof(createdUnit->dependencyMD5Checksum));
            }
        }

        createdUnit->sourceFileIndex = output.jsGenerator.stringTable.getStringId(output.jsModule.fileName);
        createdUnit->finalUrlIndex = output.jsGenerator.stringTable.getStringId(output.jsModule.finalUrl);
    }

    // No more new strings after this point, we're calculating offsets.
    output.jsGenerator.stringTable.freeze();

    const uint importSize = sizeof(QV4::CompiledData::Import) * output.imports.count();
    const uint objectOffsetTableSize = output.objects.count() * sizeof(quint32);

    QHash<const Object*, quint32> objectOffsets;

    const unsigned int objectOffset = sizeof(QV4::CompiledData::QmlUnit) + importSize;
    uint nextOffset = objectOffset + objectOffsetTableSize;
    for (Object *o : qAsConst(output.objects)) {
        objectOffsets.insert(o, nextOffset);
        nextOffset += QV4::CompiledData::Object::calculateSizeExcludingSignalsAndEnums(o->functionCount(), o->propertyCount(), o->aliasCount(), o->enumCount(), o->signalCount(), o->bindingCount(), o->namedObjectsInComponent.size(), o->inlineComponentCount(), o->requiredPropertyExtraDataCount());

        int signalTableSize = 0;
        for (const Signal *s = o->firstSignal(); s; s = s->next)
            signalTableSize += QV4::CompiledData::Signal::calculateSize(s->parameters->count);

        nextOffset += signalTableSize;

        int enumTableSize = 0;
        for (const Enum *e = o->firstEnum(); e; e = e->next)
            enumTableSize += QV4::CompiledData::Enum::calculateSize(e->enumValues->count);

        nextOffset += enumTableSize;
    }

    const uint totalSize = nextOffset;
    char *data = (char*)malloc(totalSize);
    memset(data, 0, totalSize);
    QV4::CompiledData::QmlUnit *qmlUnit = reinterpret_cast<QV4::CompiledData::QmlUnit *>(data);
    qmlUnit->offsetToImports = sizeof(*qmlUnit);
    qmlUnit->nImports = output.imports.count();
    qmlUnit->offsetToObjects = objectOffset;
    qmlUnit->nObjects = output.objects.count();

    // write imports
    char *importPtr = data + qmlUnit->offsetToImports;
    for (const QV4::CompiledData::Import *imp : qAsConst(output.imports)) {
        QV4::CompiledData::Import *importToWrite = reinterpret_cast<QV4::CompiledData::Import*>(importPtr);
        *importToWrite = *imp;
        importPtr += sizeof(QV4::CompiledData::Import);
    }

    // write objects
    quint32_le *objectTable = reinterpret_cast<quint32_le*>(data + qmlUnit->offsetToObjects);
    for (int i = 0; i < output.objects.count(); ++i) {
        const Object *o = output.objects.at(i);
        char * const objectPtr = data + objectOffsets.value(o);
        *objectTable++ = objectOffsets.value(o);

        QV4::CompiledData::Object *objectToWrite = reinterpret_cast<QV4::CompiledData::Object*>(objectPtr);
        objectToWrite->inheritedTypeNameIndex = o->inheritedTypeNameIndex;
        objectToWrite->indexOfDefaultPropertyOrAlias = o->indexOfDefaultPropertyOrAlias;
        objectToWrite->defaultPropertyIsAlias = o->defaultPropertyIsAlias;
        objectToWrite->flags = o->flags;
        objectToWrite->idNameIndex = o->idNameIndex;
        objectToWrite->id = o->id;
        objectToWrite->location = o->location;
        objectToWrite->locationOfIdProperty = o->locationOfIdProperty;

        quint32 nextOffset = sizeof(QV4::CompiledData::Object);

        objectToWrite->nFunctions = o->functionCount();
        objectToWrite->offsetToFunctions = nextOffset;
        nextOffset += objectToWrite->nFunctions * sizeof(quint32);

        objectToWrite->nProperties = o->propertyCount();
        objectToWrite->offsetToProperties = nextOffset;
        nextOffset += objectToWrite->nProperties * sizeof(QV4::CompiledData::Property);

        objectToWrite->nAliases = o->aliasCount();
        objectToWrite->offsetToAliases = nextOffset;
        nextOffset += objectToWrite->nAliases * sizeof(QV4::CompiledData::Alias);

        objectToWrite->nEnums = o->enumCount();
        objectToWrite->offsetToEnums = nextOffset;
        nextOffset += objectToWrite->nEnums * sizeof(quint32);

        objectToWrite->nSignals = o->signalCount();
        objectToWrite->offsetToSignals = nextOffset;
        nextOffset += objectToWrite->nSignals * sizeof(quint32);

        objectToWrite->nBindings = o->bindingCount();
        objectToWrite->offsetToBindings = nextOffset;
        nextOffset += objectToWrite->nBindings * sizeof(QV4::CompiledData::Binding);

        objectToWrite->nNamedObjectsInComponent = o->namedObjectsInComponent.size();
        objectToWrite->offsetToNamedObjectsInComponent = nextOffset;
        nextOffset += objectToWrite->nNamedObjectsInComponent * sizeof(quint32);

        objectToWrite->nInlineComponents = o->inlineComponentCount();
        objectToWrite->offsetToInlineComponents = nextOffset;
        nextOffset += objectToWrite->nInlineComponents * sizeof (QV4::CompiledData::InlineComponent);

        objectToWrite->nRequiredPropertyExtraData = o->requiredPropertyExtraDataCount();
        objectToWrite->offsetToRequiredPropertyExtraData = nextOffset;
        nextOffset += objectToWrite->nRequiredPropertyExtraData * sizeof(QV4::CompiledData::RequiredPropertyExtraData);

        quint32_le *functionsTable = reinterpret_cast<quint32_le *>(objectPtr + objectToWrite->offsetToFunctions);
        for (const Function *f = o->firstFunction(); f; f = f->next)
            *functionsTable++ = o->runtimeFunctionIndices.at(f->index);

        char *propertiesPtr = objectPtr + objectToWrite->offsetToProperties;
        for (const Property *p = o->firstProperty(); p; p = p->next) {
            QV4::CompiledData::Property *propertyToWrite = reinterpret_cast<QV4::CompiledData::Property*>(propertiesPtr);
            *propertyToWrite = *p;
            propertiesPtr += sizeof(QV4::CompiledData::Property);
        }

        char *aliasesPtr = objectPtr + objectToWrite->offsetToAliases;
        for (const Alias *a = o->firstAlias(); a; a = a->next) {
            QV4::CompiledData::Alias *aliasToWrite = reinterpret_cast<QV4::CompiledData::Alias*>(aliasesPtr);
            *aliasToWrite = *a;
            aliasesPtr += sizeof(QV4::CompiledData::Alias);
        }

        char *bindingPtr = objectPtr + objectToWrite->offsetToBindings;
        bindingPtr = writeBindings(bindingPtr, o, &QV4::CompiledData::Binding::isValueBindingNoAlias);
        bindingPtr = writeBindings(bindingPtr, o, &QV4::CompiledData::Binding::isSignalHandler);
        bindingPtr = writeBindings(bindingPtr, o, &QV4::CompiledData::Binding::isAttachedProperty);
        bindingPtr = writeBindings(bindingPtr, o, &QV4::CompiledData::Binding::isGroupProperty);
        bindingPtr = writeBindings(bindingPtr, o, &QV4::CompiledData::Binding::isValueBindingToAlias);
        Q_ASSERT((bindingPtr - objectToWrite->offsetToBindings - objectPtr) / sizeof(QV4::CompiledData::Binding) == unsigned(o->bindingCount()));

        quint32_le *signalOffsetTable = reinterpret_cast<quint32_le *>(objectPtr + objectToWrite->offsetToSignals);
        quint32 signalTableSize = 0;
        char *signalPtr = objectPtr + nextOffset;
        for (const Signal *s = o->firstSignal(); s; s = s->next) {
            *signalOffsetTable++ = signalPtr - objectPtr;
            QV4::CompiledData::Signal *signalToWrite = reinterpret_cast<QV4::CompiledData::Signal*>(signalPtr);

            signalToWrite->nameIndex = s->nameIndex;
            signalToWrite->location = s->location;
            signalToWrite->nParameters = s->parameters->count;

            QV4::CompiledData::Parameter *parameterToWrite = reinterpret_cast<QV4::CompiledData::Parameter*>(signalPtr + sizeof(*signalToWrite));
            for (Parameter *param = s->parameters->first; param; param = param->next, ++parameterToWrite)
                *parameterToWrite = *param;

            int size = QV4::CompiledData::Signal::calculateSize(s->parameters->count);
            signalTableSize += size;
            signalPtr += size;
        }
        nextOffset += signalTableSize;

        quint32_le *enumOffsetTable = reinterpret_cast<quint32_le*>(objectPtr + objectToWrite->offsetToEnums);
        quint32 enumTableSize = 0;
        char *enumPtr = objectPtr + nextOffset;
        for (const Enum *e = o->firstEnum(); e; e = e->next) {
            *enumOffsetTable++ = enumPtr - objectPtr;
            QV4::CompiledData::Enum *enumToWrite = reinterpret_cast<QV4::CompiledData::Enum*>(enumPtr);

            enumToWrite->nameIndex = e->nameIndex;
            enumToWrite->location = e->location;
            enumToWrite->nEnumValues = e->enumValues->count;

            QV4::CompiledData::EnumValue *enumValueToWrite = reinterpret_cast<QV4::CompiledData::EnumValue*>(enumPtr + sizeof(*enumToWrite));
            for (EnumValue *enumValue = e->enumValues->first; enumValue; enumValue = enumValue->next, ++enumValueToWrite)
                *enumValueToWrite = *enumValue;

            int size = QV4::CompiledData::Enum::calculateSize(e->enumValues->count);
            enumTableSize += size;
            enumPtr += size;
        }

        quint32_le *namedObjectInComponentPtr = reinterpret_cast<quint32_le *>(objectPtr + objectToWrite->offsetToNamedObjectsInComponent);
        for (int i = 0; i < o->namedObjectsInComponent.size(); ++i) {
            *namedObjectInComponentPtr++ = o->namedObjectsInComponent.at(i);
        }

        char *inlineComponentPtr = objectPtr + objectToWrite->offsetToInlineComponents;
        for (auto it = o->inlineComponentsBegin(); it != o->inlineComponentsEnd(); ++it) {
            const InlineComponent *ic = it.ptr;
            QV4::CompiledData::InlineComponent *icToWrite = reinterpret_cast<QV4::CompiledData::InlineComponent*>(inlineComponentPtr);
            *icToWrite = *ic;
            inlineComponentPtr += sizeof(QV4::CompiledData::InlineComponent);
        }

        char *requiredPropertyExtraDataPtr = objectPtr + objectToWrite->offsetToRequiredPropertyExtraData;
        for (auto it = o->requiredPropertyExtraDataBegin(); it != o->requiredPropertyExtraDataEnd(); ++it) {
            const RequiredPropertyExtraData *extraData = it.ptr;
            QV4::CompiledData::RequiredPropertyExtraData *extraDataToWrite = reinterpret_cast<QV4::CompiledData::RequiredPropertyExtraData*>(requiredPropertyExtraDataPtr);
            *extraDataToWrite = *extraData;
            requiredPropertyExtraDataPtr += sizeof(QV4::CompiledData::RequiredPropertyExtraData);
        }
    }

    if (!output.javaScriptCompilationUnit.data) {
        // Combine the qml data into the general unit data.
        jsUnit = static_cast<QV4::CompiledData::Unit *>(realloc(jsUnit, jsUnit->unitSize + totalSize));
        jsUnit->offsetToQmlUnit = jsUnit->unitSize;
        jsUnit->unitSize += totalSize;
        memcpy(jsUnit->qmlUnit(), qmlUnit, totalSize);
        free(qmlUnit);
        QV4::Compiler::JSUnitGenerator::generateUnitChecksum(jsUnit);
        qmlUnit = jsUnit->qmlUnit();
    }

    static const bool showStats = qEnvironmentVariableIsSet("QML_SHOW_UNIT_STATS");
    if (showStats) {
        qDebug() << "Generated QML unit that is" << totalSize << "bytes big contains:";
        qDebug() << "    " << jsUnit->functionTableSize << "functions";
        qDebug() << "    " << jsUnit->unitSize << "for JS unit";
        qDebug() << "    " << importSize << "for imports";
        qDebug() << "    " << nextOffset - objectOffset - objectOffsetTableSize << "for" << qmlUnit->nObjects << "objects";
        quint32 totalBindingCount = 0;
        for (quint32 i = 0; i < qmlUnit->nObjects; ++i)
            totalBindingCount += qmlUnit->objectAt(i)->nBindings;
        qDebug() << "    " << totalBindingCount << "bindings";
        quint32 totalCodeSize = 0;
        for (quint32 i = 0; i < jsUnit->functionTableSize; ++i)
            totalCodeSize += jsUnit->functionAt(i)->codeSize;
        qDebug() << "    " << totalCodeSize << "bytes total byte code";
        qDebug() << "    " << jsUnit->stringTableSize << "strings";
        quint32 totalStringSize = 0;
        for (quint32 i = 0; i < jsUnit->stringTableSize; ++i)
            totalStringSize += QV4::CompiledData::String::calculateSize(jsUnit->stringAtInternal(i));
        qDebug() << "    " << totalStringSize << "bytes total strings";
    }

    output.javaScriptCompilationUnit.setUnitData(jsUnit, qmlUnit, output.jsModule.fileName,
                                                 output.jsModule.finalUrl);
}

char *QmlUnitGenerator::writeBindings(char *bindingPtr, const Object *o, BindingFilter filter) const
{
    for (const Binding *b = o->firstBinding(); b; b = b->next) {
        if (!(b->*(filter))())
            continue;
        QV4::CompiledData::Binding *bindingToWrite = reinterpret_cast<QV4::CompiledData::Binding*>(bindingPtr);
        *bindingToWrite = *b;
        if (b->type == QV4::CompiledData::Binding::Type_Script)
            bindingToWrite->value.compiledScriptIndex = o->runtimeFunctionIndices.at(b->value.compiledScriptIndex);
        bindingPtr += sizeof(QV4::CompiledData::Binding);
    }
    return bindingPtr;
}

JSCodeGen::JSCodeGen(Document *document, const QSet<QString> &globalNames)
    : QV4::Compiler::Codegen(&document->jsGenerator, /*strict mode*/false), document(document)
{
    m_globalNames = globalNames;
    _module = &document->jsModule;
    _fileNameIsUrl = true;
}

QVector<int> JSCodeGen::generateJSCodeForFunctionsAndBindings(const QList<CompiledFunctionOrExpression> &functions)
{
    auto qmlName = [&](const CompiledFunctionOrExpression &c) {
        if (c.nameIndex != 0)
            return document->stringAt(c.nameIndex);
        else
            return QStringLiteral("%qml-expression-entry");
    };
    QVector<int> runtimeFunctionIndices(functions.size());

    QV4::Compiler::ScanFunctions scan(this, document->code, QV4::Compiler::ContextType::Global);
    scan.enterGlobalEnvironment(QV4::Compiler::ContextType::Binding);
    for (const CompiledFunctionOrExpression &f : functions) {
        Q_ASSERT(f.node != document->program);
        Q_ASSERT(f.parentNode && f.parentNode != document->program);
        auto function = f.node->asFunctionDefinition();

        if (function) {
            scan.enterQmlFunction(function);
        } else {
            Q_ASSERT(f.node != f.parentNode);
            scan.enterEnvironment(f.parentNode, QV4::Compiler::ContextType::Binding, qmlName(f));
        }

        scan(function ? function->body : f.node);
        scan.leaveEnvironment();
    }
    scan.leaveEnvironment();

    if (hasError())
        return QVector<int>();

    _context = nullptr;

    for (int i = 0; i < functions.count(); ++i) {
        const CompiledFunctionOrExpression &qmlFunction = functions.at(i);
        QQmlJS::AST::Node *node = qmlFunction.node;
        Q_ASSERT(node != document->program);

        QQmlJS::AST::FunctionExpression *function = node->asFunctionDefinition();

        QString name;
        if (function)
            name = function->name.toString();
        else
            name = qmlName(qmlFunction);

        QQmlJS::AST::StatementList *body;
        if (function) {
            body = function->body;
        } else {
            // Synthesize source elements.
            QQmlJS::MemoryPool *pool = document->jsParserEngine.pool();

            QQmlJS::AST::Statement *stmt = node->statementCast();
            if (!stmt) {
                Q_ASSERT(node->expressionCast());
                QQmlJS::AST::ExpressionNode *expr = node->expressionCast();
                stmt = new (pool) QQmlJS::AST::ExpressionStatement(expr);
            }
            body = new (pool) QQmlJS::AST::StatementList(stmt);
            body = body->finish();
        }

        int idx = defineFunction(name, function ? function : qmlFunction.parentNode,
                                 function ? function->formals : nullptr,
                                 body);
        runtimeFunctionIndices[i] = idx;
    }

    return runtimeFunctionIndices;
}

bool JSCodeGen::generateCodeForComponents(const QVector<quint32> &componentRoots)
{
    for (int i = 0; i < componentRoots.count(); ++i) {
        if (!compileComponent(componentRoots.at(i)))
            return false;
    }

    return compileComponent(/*root object*/0);
}

bool JSCodeGen::compileComponent(int contextObject)
{
    const QmlIR::Object *obj = document->objects.at(contextObject);
    if (obj->flags & QV4::CompiledData::Object::IsComponent && !obj->isInlineComponent) {
        Q_ASSERT(obj->bindingCount() == 1);
        const QV4::CompiledData::Binding *componentBinding = obj->firstBinding();
        Q_ASSERT(componentBinding->type == QV4::CompiledData::Binding::Type_Object);
        contextObject = componentBinding->value.objectIndex;
    }
    for (auto it = obj->inlineComponentsBegin(); it != obj->inlineComponentsEnd(); ++it)
        compileComponent(it->objectIndex);

    return compileJavaScriptCodeInObjectsRecursively(contextObject, contextObject);
}

bool JSCodeGen::compileJavaScriptCodeInObjectsRecursively(int objectIndex, int scopeObjectIndex)
{
    QmlIR::Object *object = document->objects.at(objectIndex);
    if (object->flags & QV4::CompiledData::Object::IsComponent && !object->isInlineComponent)
        return true;

    if (object->functionsAndExpressions->count > 0) {
        QList<QmlIR::CompiledFunctionOrExpression> functionsToCompile;
        for (QmlIR::CompiledFunctionOrExpression *foe = object->functionsAndExpressions->first; foe; foe = foe->next)
            functionsToCompile << *foe;
        const QVector<int> runtimeFunctionIndices = generateJSCodeForFunctionsAndBindings(functionsToCompile);
        if (hasError())
            return false;

        object->runtimeFunctionIndices.allocate(document->jsParserEngine.pool(),
                                                runtimeFunctionIndices);
    }

    for (const QmlIR::Binding *binding = object->firstBinding(); binding; binding = binding->next) {
        if (binding->type < QV4::CompiledData::Binding::Type_Object)
            continue;

        int target = binding->value.objectIndex;
        int scope = binding->type == QV4::CompiledData::Binding::Type_Object ? target : scopeObjectIndex;

        if (!compileJavaScriptCodeInObjectsRecursively(binding->value.objectIndex, scope))
            return false;
    }

    return true;
}
