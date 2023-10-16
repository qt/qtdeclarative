// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlirbuilder_p.h"

#include <private/qv4staticvalue_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qv4compilerscanfunctions_p.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <cmath>

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;

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
            it->setIsRequired(true);
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

bool Parameter::initType(
        QV4::CompiledData::ParameterType *paramType,
        const QString &typeName, int typeNameIndex,
        QV4::CompiledData::ParameterType::Flag listFlag)
{
    auto builtinType = stringToBuiltinType(typeName);
    if (builtinType == QV4::CompiledData::CommonType::Invalid) {
        if (typeName.isEmpty()) {
            paramType->set(
                    listFlag | QV4::CompiledData::ParameterType::Common, quint32(builtinType));
            return false;
        }
        Q_ASSERT(quint32(typeNameIndex) < (1u << 31));
        paramType->set(listFlag, typeNameIndex);
    } else {
        Q_ASSERT(quint32(builtinType) < (1u << 31));
        paramType->set(listFlag | QV4::CompiledData::ParameterType::Common,
                       static_cast<quint32>(builtinType));
    }
    return true;
}

QV4::CompiledData::CommonType Parameter::stringToBuiltinType(const QString &typeName)
{
    static const struct TypeNameToType {
        const char *name;
        size_t nameLength;
        QV4::CompiledData::CommonType type;
    } propTypeNameToTypes[] = {
        { "void", strlen("void"), QV4::CompiledData::CommonType::Void },
        { "int", strlen("int"), QV4::CompiledData::CommonType::Int },
        { "bool", strlen("bool"), QV4::CompiledData::CommonType::Bool },
        { "double", strlen("double"), QV4::CompiledData::CommonType::Real },
        { "real", strlen("real"), QV4::CompiledData::CommonType::Real },
        { "string", strlen("string"), QV4::CompiledData::CommonType::String },
        { "url", strlen("url"), QV4::CompiledData::CommonType::Url },
        { "date", strlen("date"), QV4::CompiledData::CommonType::DateTime },
        { "regexp", strlen("regexp"), QV4::CompiledData::CommonType::RegExp },
        { "rect", strlen("rect"), QV4::CompiledData::CommonType::Rect },
        { "point", strlen("point"), QV4::CompiledData::CommonType::Point },
        { "size", strlen("size"), QV4::CompiledData::CommonType::Size },
        { "variant", strlen("variant"), QV4::CompiledData::CommonType::Var },
        { "var", strlen("var"), QV4::CompiledData::CommonType::Var }
    };
    static const int propTypeNameToTypesCount = sizeof(propTypeNameToTypes) /
                                                sizeof(propTypeNameToTypes[0]);

    for (int typeIndex = 0; typeIndex < propTypeNameToTypesCount; ++typeIndex) {
        const TypeNameToType *t = propTypeNameToTypes + typeIndex;
        if (typeName == QLatin1String(t->name, static_cast<int>(t->nameLength))) {
            return t->type;
        }
    }
    return QV4::CompiledData::CommonType::Invalid;
}

void Object::init(QQmlJS::MemoryPool *pool, int typeNameIndex, int idIndex,
                  const QV4::CompiledData::Location &loc)
{
    Q_ASSERT(loc.line() > 0 && loc.column() > 0);
    inheritedTypeNameIndex = typeNameIndex;
    location = loc;
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
        errorLocation->startLine = f->location.line();
        errorLocation->startColumn = f->location.column();
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

    for (Alias *a = target->aliases->first; a; a = a->next)
        if (a->nameIndex() == prop->nameIndex)
            return tr("Property duplicates alias name");

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

    const auto aliasWithSameName = std::find_if(target->aliases->begin(), target->aliases->end(), [&alias](const Alias &targetAlias){
        return targetAlias.nameIndex() == alias->nameIndex();
    });
    if (aliasWithSameName != target->aliases->end())
        return tr("Duplicate alias name");

    const auto aliasSameAsProperty = std::find_if(target->properties->begin(), target->properties->end(), [&alias](const Property &targetProp){
        return targetProp.nameIndex == alias->nameIndex();
    });

    if (aliasSameAsProperty != target->properties->end())
        return tr("Alias has same name as existing property");

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
    // Unlike properties, a function definition inside a grouped property does not go into
    // the surrounding object. It's been broken since the Qt 5 era, and the semantics
    // seems super confusing, so it wouldn't make sense to support that.
    Q_ASSERT(!declarationsOverride);
    functions->append(f);
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
    if (!isListBinding
            && !bindingToDefaultProperty
            && b->type() != QV4::CompiledData::Binding::Type_GroupProperty
            && b->type() != QV4::CompiledData::Binding::Type_AttachedProperty
            && !b->hasFlag(QV4::CompiledData::Binding::IsOnAssignment)) {
        Binding *existing = findBinding(b->propertyNameIndex);
        if (existing
                && existing->isValueBinding() == b->isValueBinding()
                && !existing->hasFlag(QV4::CompiledData::Binding::IsOnAssignment)) {
            return tr("Property value set multiple times");
        }
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
    import->location.set(lineNumber, column);
    document->imports << import;
}

void ScriptDirectivesCollector::importModule(const QString &uri, const QString &version, const QString &module, int lineNumber, int column)
{
    QV4::CompiledData::Import *import = engine->pool()->New<QV4::CompiledData::Import>();
    import->type = QV4::CompiledData::Import::ImportLibrary;
    import->uriIndex = jsGenerator->registerString(uri);
    import->version = IRBuilder::extractVersion(version);
    import->qualifierIndex = jsGenerator->registerString(module);
    import->location.set(lineNumber, column);
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

            if (!errors.isEmpty() || !parseResult)
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
    if (name.size() < 3) return false;
    if (!name.startsWith(QLatin1String("on"))) return false;
    int ns = name.size();
    for (int i = 2; i < ns; ++i) {
        const QChar curr = name.at(i);
        if (curr.unicode() == '_') continue;
        if (curr.isUpper()) return true;
        return false;
    }
    return false; // consists solely of underscores - invalid.
}

QString IRBuilder::signalNameFromSignalPropertyName(const QString &signalPropertyName)
{
    Q_ASSERT(signalPropertyName.startsWith(QLatin1String("on")));
    QString signalNameCandidate = signalPropertyName;
    signalNameCandidate.remove(0, 2);

    // Note that the property name could start with any alpha or '_' or '$' character,
    // so we need to do the lower-casing of the first alpha character.
    for (int firstAlphaIndex = 0; firstAlphaIndex < signalNameCandidate.size(); ++firstAlphaIndex) {
        if (signalNameCandidate.at(firstAlphaIndex).isUpper()) {
            signalNameCandidate[firstAlphaIndex] = signalNameCandidate.at(firstAlphaIndex).toLower();
            return signalNameCandidate;
        }
    }

    Q_UNREACHABLE_RETURN(QString());
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
    bool isType = lastId->name.data()->isUpper();
    if (isType) {
        int idx = 0;
        if (!defineQMLObject(&idx, node))
            return false;
        const QQmlJS::SourceLocation nameLocation = node->qualifiedTypeNameId->identifierToken;
        appendBinding(nameLocation, nameLocation, emptyStringIndex, idx);
    } else {
        int idx = 0;
        const QQmlJS::SourceLocation location = node->qualifiedTypeNameId->firstSourceLocation();
        if (!defineQMLObject(
                    &idx, /*qualfied type name id*/nullptr,
                    { location.startLine, location.startColumn }, node->initializer,
                    /*declarations should go here*/_object)) {
            return false;
        }
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
    definedObject->flags |= QV4::CompiledData::Object::IsPartOfInlineComponent;
    auto inlineComponent = New<InlineComponent>();
    inlineComponent->nameIndex = registerString(ast->name.toString());
    inlineComponent->objectIndex = idx;
    auto location = ast->firstSourceLocation();
    inlineComponent->location.set(location.startLine, location.startColumn);
    _object->appendInlineComponent(inlineComponent);
    return false;
}

bool IRBuilder::visit(QQmlJS::AST::UiObjectBinding *node)
{
    int idx = 0;
    const QQmlJS::SourceLocation location = node->qualifiedTypeNameId->firstSourceLocation();
    if (!defineQMLObject(&idx, node->qualifiedTypeNameId,
                         { location.startLine, location.startColumn }, node->initializer)) {
        return false;
    }
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
    for (int i = memberList.size() - 1; i >= 0; --i) {
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

bool IRBuilder::defineQMLObject(
        int *objectIndex, QQmlJS::AST::UiQualifiedId *qualifiedTypeNameId,
        const QV4::CompiledData::Location &location, QQmlJS::AST::UiObjectInitializer *initializer,
        Object *declarationsOverride)
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
        _object->flags |= QV4::CompiledData::Object::IsPartOfInlineComponent;
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
        for (int ii = 0; ii < _imports.size(); ++ii) {
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
        import->version = node->version->version;
    } else {
        // Otherwise initialize the major and minor version to invalid to signal "latest".
        import->version = QTypeRevision();
    }

    import->location.set(node->importToken.startLine, node->importToken.startColumn);

    import->uriIndex = registerString(uri);

    _imports.append(import);

    return false;
}


template<typename Argument>
struct PragmaParser
{
    static bool run(IRBuilder *builder, QQmlJS::AST::UiPragma *node, Pragma *pragma)
    {
        Q_ASSERT(builder);
        Q_ASSERT(node);
        Q_ASSERT(pragma);

        if (!isUnique(builder)) {
            builder->recordError(
                        node->pragmaToken, QCoreApplication::translate(
                            "QQmlParser", "Multiple %1 pragmas found").arg(name()));
            return false;
        }

        pragma->type = type();

        if (QQmlJS::AST::UiPragmaValueList *bad = assign(pragma, node->values)) {
            builder->recordError(
                        node->pragmaToken, QCoreApplication::translate(
                            "QQmlParser", "Unknown %1 '%2' in pragma").arg(name(), bad->value));
            return false;
        }

        return true;
    }

private:
    static constexpr Pragma::PragmaType type()
    {
        if constexpr (std::is_same_v<Argument, Pragma::ComponentBehaviorValue>) {
            return Pragma::ComponentBehavior;
        } else if constexpr (std::is_same_v<Argument, Pragma::ListPropertyAssignBehaviorValue>) {
            return Pragma::ListPropertyAssignBehavior;
        } else if constexpr (std::is_same_v<Argument, Pragma::FunctionSignatureBehaviorValue>) {
            return Pragma::FunctionSignatureBehavior;
        } else if constexpr (std::is_same_v<Argument, Pragma::NativeMethodBehaviorValue>) {
            return Pragma::NativeMethodBehavior;
        } else if constexpr (std::is_same_v<Argument, Pragma::ValueTypeBehaviorValue>) {
            return Pragma::ValueTypeBehavior;
        }

        Q_UNREACHABLE_RETURN(Pragma::PragmaType(-1));
    }

    template<typename F>
    static QQmlJS::AST::UiPragmaValueList *iterateValues(
            QQmlJS::AST::UiPragmaValueList *input, F &&process)
    {
        for (QQmlJS::AST::UiPragmaValueList *i = input; i; i = i->next) {
            if (!process(i->value))
                return i;
        }
        return nullptr;
    }

    static QQmlJS::AST::UiPragmaValueList *assign(
            Pragma *pragma, QQmlJS::AST::UiPragmaValueList *values)
    {
        // We could use QMetaEnum here to make the code more compact,
        // but it's probably more expensive.

        if constexpr (std::is_same_v<Argument, Pragma::ComponentBehaviorValue>) {
            return iterateValues(values, [pragma](QStringView value) {
                if (value == "Unbound"_L1) {
                    pragma->componentBehavior = Pragma::Unbound;
                    return true;
                }
                if (value == "Bound"_L1) {
                    pragma->componentBehavior = Pragma::Bound;
                    return true;
                }
                return false;
            });
        } else if constexpr (std::is_same_v<Argument, Pragma::ListPropertyAssignBehaviorValue>) {
            return iterateValues(values, [pragma](QStringView value) {
                if (value == "Append"_L1) {
                    pragma->listPropertyAssignBehavior = Pragma::Append;
                    return true;
                }
                if (value == "Replace"_L1) {
                    pragma->listPropertyAssignBehavior = Pragma::Replace;
                    return true;
                }
                if (value == "ReplaceIfNotDefault"_L1) {
                    pragma->listPropertyAssignBehavior = Pragma::ReplaceIfNotDefault;
                    return true;
                }
                return false;
            });
        } else if constexpr (std::is_same_v<Argument, Pragma::FunctionSignatureBehaviorValue>) {
            return iterateValues(values, [pragma](QStringView value) {
                if (value == "Ignored"_L1) {
                    pragma->functionSignatureBehavior = Pragma::Ignored;
                    return true;
                }
                if (value == "Enforced"_L1) {
                    pragma->functionSignatureBehavior = Pragma::Enforced;
                    return true;
                }
                return false;
            });
        } else if constexpr (std::is_same_v<Argument, Pragma::NativeMethodBehaviorValue>) {
            return iterateValues(values, [pragma](QStringView value) {
                if (value == "AcceptThisObject"_L1) {
                    pragma->nativeMethodBehavior = Pragma::AcceptThisObject;
                    return true;
                }
                if (value == "RejectThisObject"_L1) {
                    pragma->nativeMethodBehavior = Pragma::RejectThisObject;
                    return true;
                }
                return false;
            });
        } else if constexpr (std::is_same_v<Argument, Pragma::ValueTypeBehaviorValue>) {
            pragma->valueTypeBehavior = Pragma::ValueTypeBehaviorValues().toInt();
            return iterateValues(values, [pragma](QStringView value) {
                const auto setFlag = [pragma](Pragma::ValueTypeBehaviorValue flag, bool value) {
                    pragma->valueTypeBehavior
                            = Pragma::ValueTypeBehaviorValues(pragma->valueTypeBehavior)
                                .setFlag(flag, value).toInt();
                };

                if (value == "Reference"_L1) {
                    setFlag(Pragma::Copy, false);
                    return true;
                }
                if (value == "Copy"_L1) {
                    setFlag(Pragma::Copy, true);
                    return true;
                }

                if (value == "Inaddressable"_L1) {
                    setFlag(Pragma::Addressable, false);
                    return true;
                }
                if (value == "Addressable"_L1) {
                    setFlag(Pragma::Addressable, true);
                    return true;
                }

                return false;
            });
        }

        Q_UNREACHABLE_RETURN(nullptr);
    }

    static bool isUnique(IRBuilder *builder)
    {
        for (const Pragma *prev : builder->_pragmas) {
            if (prev->type == type())
                return false;
        }
        return true;
    };

    static QLatin1StringView name()
    {
        switch (type()) {
        case Pragma::ListPropertyAssignBehavior:
            return "list property assign behavior"_L1;
        case Pragma::ComponentBehavior:
            return "component behavior"_L1;
        case Pragma::FunctionSignatureBehavior:
            return "function signature behavior"_L1;
        case Pragma::NativeMethodBehavior:
            return "native method behavior"_L1;
        case Pragma::ValueTypeBehavior:
            return "value type behavior"_L1;
        default:
            break;
        }
        Q_UNREACHABLE_RETURN(QLatin1StringView());
    }
};

bool IRBuilder::visit(QQmlJS::AST::UiPragma *node)
{
    Pragma *pragma = New<Pragma>();

    if (!node->name.isNull()) {
        if (node->name == "Singleton"_L1) {
            pragma->type = Pragma::Singleton;
        } else if (node->name == "Strict"_L1) {
            pragma->type = Pragma::Strict;
        } else if (node->name == "ComponentBehavior"_L1) {
            if (!PragmaParser<Pragma::ComponentBehaviorValue>::run(this, node, pragma))
                return false;
        } else if (node->name == "ListPropertyAssignBehavior"_L1) {
            if (!PragmaParser<Pragma::ListPropertyAssignBehaviorValue>::run(this, node, pragma))
                return false;
        } else if (node->name == "FunctionSignatureBehavior"_L1) {
            if (!PragmaParser<Pragma::FunctionSignatureBehaviorValue>::run(this, node, pragma))
                return false;
        } else if (node->name == "NativeMethodBehavior"_L1) {
            if (!PragmaParser<Pragma::NativeMethodBehaviorValue>::run(this, node, pragma))
                return false;
        } else if (node->name == "ValueTypeBehavior"_L1) {
            if (!PragmaParser<Pragma::ValueTypeBehaviorValue>::run(this, node, pragma))
                return false;
        } else {
            recordError(node->pragmaToken, QCoreApplication::translate(
                            "QQmlParser", "Unknown pragma '%1'").arg(node->name));
            return false;
        }
    } else {
        recordError(node->pragmaToken, QCoreApplication::translate(
                        "QQmlParser", "Empty pragma found"));
        return false;
    }

    pragma->location.set(node->pragmaToken.startLine, node->pragmaToken.startColumn);
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

    enumeration->location.set(node->enumToken.startLine, node->enumToken.startColumn);

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

        enumValue->location.set(e->memberToken.startLine, e->memberToken.startColumn);
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
        signal->location.set(loc.startLine, loc.startColumn);

        signal->parameters = New<PoolList<Parameter> >();

        QQmlJS::AST::UiParameterList *p = node->parameters;
        while (p) {
            if (!p->type) {
                recordError(node->typeToken, QCoreApplication::translate("QQmlParser","Expected parameter type"));
                return false;
            }

            Parameter *param = New<Parameter>();
            param->nameIndex = registerString(p->name.toString());
            if (!Parameter::initType(
                        &param->type, [this](const QString &str) { return registerString(str); },
                        p->type)) {
                QString errStr = QCoreApplication::translate("QQmlParser","Invalid signal parameter type: ");
                errStr.append(p->type->toString());
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
            QStringView name = node->name;

            Property *property = New<Property>();
            property->setIsReadOnly(node->isReadonly());
            property->setIsRequired(node->isRequired());

            const QV4::CompiledData::CommonType builtinPropertyType
                    = Parameter::stringToBuiltinType(memberType);
            if (builtinPropertyType != QV4::CompiledData::CommonType::Invalid)
                property->setCommonType(builtinPropertyType);
            else
                property->setTypeNameIndex(registerString(memberType));

            QStringView typeModifier = node->typeModifier;
            if (typeModifier == QLatin1String("list")) {
                property->setIsList(true);
            } else if (!typeModifier.isEmpty()) {
                recordError(node->typeModifierToken, QCoreApplication::translate("QQmlParser","Invalid property type modifier"));
                return false;
            }

            const QString propName = name.toString();
            property->nameIndex = registerString(propName);

            QQmlJS::SourceLocation loc = node->firstSourceLocation();
            property->location.set(loc.startLine, loc.startColumn);

            QQmlJS::SourceLocation errorLocation;
            QString error;

            if (illegalNames.contains(propName))
                error = tr("Illegal property name");
            else
                error = _object->appendProperty(property, propName, node->isDefaultMember(), node->defaultToken(), &errorLocation);

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
        if (_object->declarationsOverride) {
            // See Object::appendFunction() for why.
            recordError(node->firstSourceLocation(),
                        QCoreApplication::translate(
                                "QQmlParser", "Function declaration inside grouped property"));
            return false;
        }

        CompiledFunctionOrExpression *foe = New<CompiledFunctionOrExpression>();
        foe->node = funDecl;
        foe->parentNode = funDecl;
        foe->nameIndex = registerString(funDecl->name.toString());
        const int index = _object->functionsAndExpressions->append(foe);

        Function *f = New<Function>();
        QQmlJS::SourceLocation loc = funDecl->identifierToken;
        f->location.set(loc.startLine, loc.startColumn);
        f->index = index;
        f->nameIndex = registerString(funDecl->name.toString());

        const auto idGenerator = [this](const QString &str) { return registerString(str); };

        Parameter::initType(
                    &f->returnType, idGenerator,
                    funDecl->typeAnnotation ? funDecl->typeAnnotation->type : nullptr);

        const QQmlJS::AST::BoundNames formals = funDecl->formals ? funDecl->formals->formals() : QQmlJS::AST::BoundNames();
        int formalsCount = formals.size();
        f->formals.allocate(pool, formalsCount);

        int i = 0;
        for (const auto &arg : formals) {
            Parameter *functionParameter = &f->formals[i];
            functionParameter->nameIndex = registerString(arg.id);
            Parameter::initType(
                        &functionParameter->type, idGenerator,
                        arg.typeAnnotation.isNull() ? nullptr : arg.typeAnnotation->type);
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

QStringView IRBuilder::asStringRef(QQmlJS::AST::Node *node)
{
    if (!node)
        return QStringView();

    return textRefAt(node->firstSourceLocation(), node->lastSourceLocation());
}

QTypeRevision IRBuilder::extractVersion(QStringView string)
{
    if (string.isEmpty())
        return QTypeRevision();

    const int dot = string.indexOf(QLatin1Char('.'));
    return (dot < 0)
        ? QTypeRevision::fromMajorVersion(string.toInt())
        : QTypeRevision::fromVersion(string.left(dot).toInt(), string.mid(dot + 1).toInt());
}

QStringView IRBuilder::textRefAt(const QQmlJS::SourceLocation &first, const QQmlJS::SourceLocation &last) const
{
    return QStringView(sourceCode).mid(first.offset, last.offset + last.length - first.offset);
}

void IRBuilder::setBindingValue(QV4::CompiledData::Binding *binding, QQmlJS::AST::Statement *statement, QQmlJS::AST::Node *parentNode)
{
    QQmlJS::SourceLocation loc = statement->firstSourceLocation();
    binding->valueLocation.set(loc.startLine, loc.startColumn);
    binding->setType(QV4::CompiledData::Binding::Type_Invalid);
    if (_propertyDeclaration && _propertyDeclaration->isReadOnly())
        binding->setFlag(QV4::CompiledData::Binding::InitializerForReadOnlyDeclaration);

    QQmlJS::AST::ExpressionStatement *exprStmt = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement *>(statement);
    if (exprStmt) {
        QQmlJS::AST::ExpressionNode * const expr = exprStmt->expression;
        if (QQmlJS::AST::StringLiteral *lit = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(expr)) {
            binding->setType(QV4::CompiledData::Binding::Type_String);
            binding->stringIndex = registerString(lit->value.toString());
        } else if (QQmlJS::AST::TemplateLiteral *templateLit = QQmlJS::AST::cast<QQmlJS::AST::TemplateLiteral *>(expr);
                   templateLit && templateLit->hasNoSubstitution) {
            // A template literal without substitution is just a string.
            // With substitution, it could however be an arbitrarily complex expression
            binding->setType(QV4::CompiledData::Binding::Type_String);
            binding->stringIndex = registerString(templateLit->value.toString());
        } else if (expr->kind == QQmlJS::AST::Node::Kind_TrueLiteral) {
            binding->setType(QV4::CompiledData::Binding::Type_Boolean);
            binding->value.b = true;
        } else if (expr->kind == QQmlJS::AST::Node::Kind_FalseLiteral) {
            binding->setType(QV4::CompiledData::Binding::Type_Boolean);
            binding->value.b = false;
        } else if (QQmlJS::AST::NumericLiteral *lit = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(expr)) {
            binding->setType(QV4::CompiledData::Binding::Type_Number);
            binding->value.constantValueIndex = jsGenerator->registerConstant(QV4::Encode(lit->value));
        } else if (QQmlJS::AST::CallExpression *call = QQmlJS::AST::cast<QQmlJS::AST::CallExpression *>(expr)) {
            if (QQmlJS::AST::IdentifierExpression *base = QQmlJS::AST::cast<QQmlJS::AST::IdentifierExpression *>(call->base)) {
                tryGeneratingTranslationBinding(base->name, call->arguments, binding);
                // If it wasn't a translation binding, a normal script binding will be generated
                // below.
            }
        } else if (QQmlJS::AST::cast<QQmlJS::AST::FunctionExpression *>(expr)) {
            binding->setFlag(QV4::CompiledData::Binding::IsFunctionExpression);
        } else if (QQmlJS::AST::UnaryMinusExpression *unaryMinus = QQmlJS::AST::cast<QQmlJS::AST::UnaryMinusExpression *>(expr)) {
            if (QQmlJS::AST::NumericLiteral *lit = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(unaryMinus->expression)) {
                binding->setType(QV4::CompiledData::Binding::Type_Number);
                binding->value.constantValueIndex = jsGenerator->registerConstant(QV4::Encode(-lit->value));
            }
        } else if (QQmlJS::AST::cast<QQmlJS::AST::NullExpression *>(expr)) {
            binding->setType(QV4::CompiledData::Binding::Type_Null);
            binding->value.nullMarker = 0;
        }
    }

    // Do binding instead
    if (binding->type() == QV4::CompiledData::Binding::Type_Invalid) {
        binding->setType(QV4::CompiledData::Binding::Type_Script);

        CompiledFunctionOrExpression *expr = New<CompiledFunctionOrExpression>();
        expr->node = statement;
        expr->parentNode = parentNode;
        expr->nameIndex = registerString(QLatin1String("expression for ")
                                         + stringAt(binding->propertyNameIndex));
        const int index = bindingsTarget()->functionsAndExpressions->append(expr);
        binding->value.compiledScriptIndex = index;
        // We don't need to store the binding script as string, except for script strings
        // and types with custom parsers. Those will be added later in the compilation phase.
        // Except that we cannot recover the string when cachegen runs; we need to therefore retain
        // "undefined". Any other "special" strings (for the various literals) are already handled above
        QQmlJS::AST::Node *nodeForString = statement;
        if (exprStmt)
            nodeForString = exprStmt->expression;
        if (asStringRef(nodeForString) == u"undefined")
            binding->stringIndex = registerString(u"undefined"_s);
        else
            binding->stringIndex = emptyStringIndex;
    }
}

void IRBuilder::tryGeneratingTranslationBinding(QStringView base, AST::ArgumentList *args, QV4::CompiledData::Binding *binding)
{
    const auto registerString = [&](QStringView string) {
        return jsGenerator->registerString(string.toString()) ;
    };

    const auto finalizeTranslationData = [&](
            QV4::CompiledData::Binding::Type type,
            QV4::CompiledData::TranslationData translationData) {
        binding->setType(type);
        if (type == QV4::CompiledData::Binding::Type_Translation
                || type == QV4::CompiledData::Binding::Type_TranslationById) {
            binding->value.translationDataIndex = jsGenerator->registerTranslation(translationData);
        } else if (type == QV4::CompiledData::Binding::Type_String) {
            binding->stringIndex = translationData.number;
        }
    };

    tryGeneratingTranslationBindingBase(
                base, args,
                registerString, registerString, registerString, finalizeTranslationData);
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
    binding->location.set(nameLocation.startLine, nameLocation.startColumn);
    binding->clearFlags();
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
    binding->location.set(nameLocation.startLine, nameLocation.startColumn);

    const Object *obj = _objects.at(objectIndex);
    binding->valueLocation = obj->location;

    binding->clearFlags();

    if (_propertyDeclaration && _propertyDeclaration->isReadOnly())
        binding->setFlag(Binding::InitializerForReadOnlyDeclaration);

    // No type name on the initializer means it must be a group property
    if (_objects.at(objectIndex)->inheritedTypeNameIndex == emptyStringIndex)
        binding->setType(Binding::Type_GroupProperty);
    else
        binding->setType(Binding::Type_Object);

    if (isOnAssignment)
        binding->setFlag(Binding::IsOnAssignment);
    if (isListItem)
        binding->setFlag(Binding::IsListItem);

    binding->value.objectIndex = objectIndex;
    QString error = bindingsTarget()->appendBinding(binding, isListItem);
    if (!error.isEmpty()) {
        recordError(qualifiedNameLocation, error);
    }
}

bool IRBuilder::appendAlias(QQmlJS::AST::UiPublicMember *node)
{
    Alias *alias = New<Alias>();
    alias->clearFlags();
    if (node->isReadonly())
        alias->setFlag(QV4::CompiledData::Alias::IsReadOnly);

    const QString propName = node->name.toString();
    alias->setNameIndex(registerString(propName));

    QQmlJS::SourceLocation loc = node->firstSourceLocation();
    alias->location.set(loc.startLine, loc.startColumn);

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
    alias->referenceLocation.set(rhsLoc.startLine, rhsLoc.startColumn);

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

    if (aliasReference.size() < 1 || aliasReference.size() > 3)
        COMPILE_EXCEPTION(rhsLoc, tr("Invalid alias reference. An alias reference must be specified as <id>, <id>.<property> or <id>.<value property>.<property>"));

     alias->setIdIndex(registerString(aliasReference.first()));

     QString propertyValue = aliasReference.value(1);
     if (aliasReference.size() == 3)
         propertyValue += QLatin1Char('.') + aliasReference.at(2);
     alias->propertyNameIndex = registerString(propertyValue);

     QQmlJS::SourceLocation errorLocation;
     QString error;

     if (illegalNames.contains(propName))
         error = tr("Illegal property name");
     else
         error = _object->appendAlias(alias, propName, node->isDefaultMember(), node->defaultToken(), &errorLocation);

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
    QStringView str;

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

    for (int ii = 1; ii < str.size(); ++ii) {
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
    _object->locationOfIdProperty.set(idLocation.startLine, idLocation.startColumn);

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
        for (const QV4::CompiledData::Import* import : std::as_const(_imports))
            if (import->qualifierIndex != emptyStringIndex
                && stringAt(import->qualifierIndex) == currentName) {
                qualifiedIdElement = qualifiedIdElement->next;
                currentName += QLatin1Char('.') + qualifiedIdElement->name;

                if (!qualifiedIdElement->name.data()->isUpper())
                    COMPILE_EXCEPTION(qualifiedIdElement->firstSourceLocation(), tr("Expected type name"));

                break;
            }
    }

    *object = _object;
    while (qualifiedIdElement->next) {
        const quint32 propertyNameIndex = registerString(currentName);
        const bool isAttachedProperty = qualifiedIdElement->name.data()->isUpper();

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
            binding->location.set(qualifiedIdElement->identifierToken.startLine,
                                  qualifiedIdElement->identifierToken.startColumn);
            binding->valueLocation.set(qualifiedIdElement->next->identifierToken.startLine,
                                       qualifiedIdElement->next->identifierToken.startColumn);
            binding->clearFlags();

            if (onAssignment)
                binding->setFlag(QV4::CompiledData::Binding::IsOnAssignment);

            if (isAttachedProperty)
                binding->setType(QV4::CompiledData::Binding::Type_AttachedProperty);
            else
                binding->setType(QV4::CompiledData::Binding::Type_GroupProperty);

            int objIndex = 0;
            if (!defineQMLObject(&objIndex, nullptr, binding->location, nullptr, nullptr))
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
    if (property->isCommonType() || property->isList())
        return false;
    QQmlJS::AST::ExpressionStatement *exprStmt = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement *>(statement);
    if (!exprStmt)
        return false;
    QQmlJS::AST::ExpressionNode * const expr = exprStmt->expression;
    return QQmlJS::AST::cast<QQmlJS::AST::NullExpression *>(expr);
}

void QmlUnitGenerator::generate(Document &output, const QV4::CompiledData::DependentTypesHasher &dependencyHasher)
{
    using namespace QV4::CompiledData;

    output.jsGenerator.stringTable.registerString(output.jsModule.fileName);
    output.jsGenerator.stringTable.registerString(output.jsModule.finalUrl);

    Unit *jsUnit = nullptr;

    // We may already have unit data if we're loading an ahead-of-time generated cache file.
    if (output.javaScriptCompilationUnit.data) {
        jsUnit = const_cast<Unit *>(output.javaScriptCompilationUnit.data);
        output.javaScriptCompilationUnit.dynamicStrings = output.jsGenerator.stringTable.allStrings();
    } else {
        Unit *createdUnit;
        jsUnit = createdUnit = output.jsGenerator.generateUnit();

        // enable flag if we encountered pragma Singleton
        for (Pragma *p : std::as_const(output.pragmas)) {
            switch (p->type) {
            case Pragma::Singleton:
                createdUnit->flags |= Unit::IsSingleton;
                break;
            case Pragma::Strict:
                createdUnit->flags |= Unit::IsStrict;
                break;
            case Pragma::ComponentBehavior:
                // ### Qt7: Change the default to Bound by reverting the meaning of the flag.
                switch (p->componentBehavior) {
                case Pragma::Bound:
                    createdUnit->flags |= Unit::ComponentsBound;
                    break;
                case Pragma::Unbound:
                    // this is the default
                    break;
                }
                break;
            case Pragma::ListPropertyAssignBehavior:
                switch (p->listPropertyAssignBehavior) {
                case Pragma::Replace:
                    createdUnit->flags |= Unit::ListPropertyAssignReplace;
                    break;
                case Pragma::ReplaceIfNotDefault:
                    createdUnit->flags |= Unit::ListPropertyAssignReplaceIfNotDefault;
                    break;
                case Pragma::Append:
                    // this is the default
                    break;
                }
                break;
            case Pragma::FunctionSignatureBehavior:
                switch (p->functionSignatureBehavior) {
                case Pragma::Enforced:
                    createdUnit->flags |= Unit::FunctionSignaturesEnforced;
                    break;
                case Pragma::Ignored:
                    //this is the default;
                    break;
                }
                break;
            case Pragma::NativeMethodBehavior:
                switch (p->nativeMethodBehavior) {
                case Pragma::AcceptThisObject:
                    createdUnit->flags |= Unit::NativeMethodsAcceptThisObject;
                    break;
                case Pragma::RejectThisObject:
                    // this is the default;
                    break;
                }
                break;
            case Pragma::ValueTypeBehavior:
                if (Pragma::ValueTypeBehaviorValues(p->valueTypeBehavior)
                        .testFlag(Pragma::Copy)) {
                    createdUnit->flags |= Unit::ValueTypesCopied;
                }
                if (Pragma::ValueTypeBehaviorValues(p->valueTypeBehavior)
                        .testFlag(Pragma::Addressable)) {
                    createdUnit->flags |= Unit::ValueTypesAddressable;
                }
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

    const uint importSize = uint(sizeof(QV4::CompiledData::Import)) * output.imports.size();
    const uint objectOffsetTableSize = output.objects.size() * uint(sizeof(quint32));

    QHash<const Object*, quint32> objectOffsets;

    const unsigned int objectOffset = sizeof(QV4::CompiledData::QmlUnit) + importSize;
    uint nextOffset = objectOffset + objectOffsetTableSize;
    for (Object *o : std::as_const(output.objects)) {
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
    qmlUnit->nImports = output.imports.size();
    qmlUnit->offsetToObjects = objectOffset;
    qmlUnit->nObjects = output.objects.size();

    // write imports
    char *importPtr = data + qmlUnit->offsetToImports;
    for (const QV4::CompiledData::Import *imp : std::as_const(output.imports)) {
        QV4::CompiledData::Import *importToWrite = reinterpret_cast<QV4::CompiledData::Import*>(importPtr);
        *importToWrite = *imp;
        importPtr += sizeof(QV4::CompiledData::Import);
    }

    // write objects
    quint32_le *objectTable = reinterpret_cast<quint32_le*>(data + qmlUnit->offsetToObjects);
    for (int i = 0; i < output.objects.size(); ++i) {
        const Object *o = output.objects.at(i);
        char * const objectPtr = data + objectOffsets.value(o);
        *objectTable++ = objectOffsets.value(o);

        QV4::CompiledData::Object *objectToWrite = reinterpret_cast<QV4::CompiledData::Object*>(objectPtr);
        objectToWrite->inheritedTypeNameIndex = o->inheritedTypeNameIndex;
        objectToWrite->indexOfDefaultPropertyOrAlias = o->indexOfDefaultPropertyOrAlias;
        objectToWrite->setHasAliasAsDefaultProperty(o->defaultPropertyIsAlias);
        objectToWrite->setFlags(QV4::CompiledData::Object::Flags(o->flags));
        objectToWrite->idNameIndex = o->idNameIndex;
        objectToWrite->setObjectId(o->id);
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
        if (b->type() == QV4::CompiledData::Binding::Type_Script)
            bindingToWrite->value.compiledScriptIndex = o->runtimeFunctionIndices.at(b->value.compiledScriptIndex);
        bindingPtr += sizeof(QV4::CompiledData::Binding);
    }
    return bindingPtr;
}

JSCodeGen::JSCodeGen(Document *document, const QSet<QString> &globalNames,
                     QV4::Compiler::CodegenWarningInterface *iface,
                     bool storeSourceLocations)
    : QV4::Compiler::Codegen(&document->jsGenerator, /*strict mode*/ false, iface,
                             storeSourceLocations),
      document(document)
{
    m_globalNames = globalNames;
    _module = &document->jsModule;
    _fileNameIsUrl = true;
}

QVector<int> JSCodeGen::generateJSCodeForFunctionsAndBindings(
        const QList<CompiledFunctionOrExpression> &functions)
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

        /* We do not want to visit the whole function, as we already  called enterQmlFunction
           However, there might be a function defined as a default argument of the function.
           That needs to be considered, too, so we call handleTopLevelFunctionFormals to
           deal with them.
         */
        scan.handleTopLevelFunctionFormals(function);
        scan(function ? function->body : f.node);
        scan.leaveEnvironment();
    }
    scan.leaveEnvironment();

    if (hasError())
        return QVector<int>();

    _context = nullptr;

    for (int i = 0; i < functions.size(); ++i) {
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
                                 function ? function->formals : nullptr, body);
        runtimeFunctionIndices[i] = idx;
    }

    return runtimeFunctionIndices;
}

bool JSCodeGen::generateRuntimeFunctions(QmlIR::Object *object)
{
    if (object->functionsAndExpressions->count == 0)
        return true;

    QList<QmlIR::CompiledFunctionOrExpression> functionsToCompile;
    functionsToCompile.reserve(object->functionsAndExpressions->count);
    for (QmlIR::CompiledFunctionOrExpression *foe = object->functionsAndExpressions->first; foe;
         foe = foe->next) {
        functionsToCompile << *foe;
    }

    const auto runtimeFunctionIndices = generateJSCodeForFunctionsAndBindings(functionsToCompile);
    if (hasError())
        return false;

    object->runtimeFunctionIndices.allocate(document->jsParserEngine.pool(),
                                            runtimeFunctionIndices);
    return true;
}
