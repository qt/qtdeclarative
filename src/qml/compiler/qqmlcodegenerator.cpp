/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "qqmlcodegenerator_p.h"

#include <private/qv4compileddata_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmlcompiler_p.h>
#include <QCoreApplication>

#ifdef CONST
#undef CONST
#endif

QT_USE_NAMESPACE

using namespace QtQml;

#define COMPILE_EXCEPTION(location, desc) \
    { \
        recordError(location, desc); \
        return false; \
    }

void QmlObject::dump(DebugStream &out)
{
    out << inheritedTypeNameIndex << " {" << endl;
    out.indent++;

    out.indent--;
    out << "}" << endl;
}

QStringList Signal::parameterStringList(const QStringList &stringPool) const
{
    QStringList result;
    result.reserve(parameters->count);
    for (SignalParameter *param = parameters->first; param; param = param->next)
        result << stringPool.at(param->nameIndex);
    return result;
}

QQmlCodeGenerator::QQmlCodeGenerator()
    : _object(0)
    , jsGenerator(0)
{
}

bool QQmlCodeGenerator::generateFromQml(const QString &code, const QUrl &url, const QString &urlString, ParsedQML *output)
{
    this->url = url;
    AST::UiProgram *program = 0;
    {
        QQmlJS::Lexer lexer(&output->jsParserEngine);
        lexer.setCode(code, /*line = */ 1);

        QQmlJS::Parser parser(&output->jsParserEngine);

        if (! parser.parse() || !parser.diagnosticMessages().isEmpty()) {

            // Extract errors from the parser
            foreach (const DiagnosticMessage &m, parser.diagnosticMessages()) {

                if (m.isWarning()) {
                    qWarning("%s:%d : %s", qPrintable(urlString), m.loc.startLine, qPrintable(m.message));
                    continue;
                }

                QQmlError error;
                error.setUrl(url);
                error.setDescription(m.message);
                error.setLine(m.loc.startLine);
                error.setColumn(m.loc.startColumn);
                errors << error;
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
    qSwap(_functions, output->functions);
    qSwap(_typeReferences, output->typeReferences);
    this->pool = output->jsParserEngine.pool();
    this->jsGenerator = &output->jsGenerator;

    emptyStringIndex = registerString(QString());

    sourceCode = code;

    accept(program->headers);

    if (program->members->next) {
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser", "Unexpected object definition"));
        AST::SourceLocation loc = program->members->next->firstSourceLocation();
        error.setLine(loc.startLine);
        error.setColumn(loc.startColumn);
        errors << error;
        return false;
    }

    AST::UiObjectDefinition *rootObject = AST::cast<AST::UiObjectDefinition*>(program->members->member);
    Q_ASSERT(rootObject);
    output->indexOfRootObject = defineQMLObject(rootObject);

    collectTypeReferences();

    qSwap(_imports, output->imports);
    qSwap(_pragmas, output->pragmas);
    qSwap(_objects, output->objects);
    qSwap(_functions, output->functions);
    qSwap(_typeReferences, output->typeReferences);
    return errors.isEmpty();
}

bool QQmlCodeGenerator::isSignalPropertyName(const QString &name)
{
    if (name.length() < 3) return false;
    if (!name.startsWith(QStringLiteral("on"))) return false;
    int ns = name.length();
    for (int i = 2; i < ns; ++i) {
        const QChar curr = name.at(i);
        if (curr.unicode() == '_') continue;
        if (curr.isUpper()) return true;
        return false;
    }
    return false; // consists solely of underscores - invalid.
}

bool QQmlCodeGenerator::visit(AST::UiArrayMemberList *ast)
{
    return AST::Visitor::visit(ast);
}

bool QQmlCodeGenerator::visit(AST::UiProgram *)
{
    Q_ASSERT(!"should not happen");
    return false;
}

bool QQmlCodeGenerator::visit(AST::UiObjectDefinition *node)
{
    // The grammar can't distinguish between two different definitions here:
    //     Item { ... }
    // versus
    //     font { ... }
    // The former is a new binding with no property name and "Item" as type name,
    // and the latter is a binding to the font property with no type name but
    // only initializer.

    AST::UiQualifiedId *lastId = node->qualifiedTypeNameId;
    while (lastId->next)
        lastId = lastId->next;
    bool isType = lastId->name.unicode()->isUpper();
    if (isType) {
        int idx = defineQMLObject(node);
        appendBinding(node->qualifiedTypeNameId->identifierToken, emptyStringIndex, idx);
    } else {
        int idx = defineQMLObject(/*qualfied type name id*/0, node->initializer);
        appendBinding(node->qualifiedTypeNameId, idx);
    }
    return false;
}

bool QQmlCodeGenerator::visit(AST::UiObjectBinding *node)
{
    int idx = defineQMLObject(node->qualifiedTypeNameId, node->initializer);
    appendBinding(node->qualifiedId, idx);
    return false;
}

bool QQmlCodeGenerator::visit(AST::UiScriptBinding *node)
{
    appendBinding(node->qualifiedId, node->statement);
    return false;
}

bool QQmlCodeGenerator::visit(AST::UiArrayBinding *node)
{
    QmlObject *object = 0;
    AST::UiQualifiedId *name = node->qualifiedId;
    if (!resolveQualifiedId(&name, &object))
        return false;

    qSwap(_object, object);

    AST::UiArrayMemberList *member = node->members;
    while (member) {
        AST::UiObjectDefinition *def = AST::cast<AST::UiObjectDefinition*>(member->member);

        int idx = defineQMLObject(def);
        appendBinding(name->identifierToken, registerString(name->name.toString()), idx, /*isListItem*/ true);

        member = member->next;
    }

    qSwap(_object, object);
    return false;
}

bool QQmlCodeGenerator::visit(AST::UiHeaderItemList *list)
{
    return AST::Visitor::visit(list);
}

bool QQmlCodeGenerator::visit(AST::UiObjectInitializer *ast)
{
    return AST::Visitor::visit(ast);
}

bool QQmlCodeGenerator::visit(AST::UiObjectMemberList *ast)
{
    return AST::Visitor::visit(ast);
}

bool QQmlCodeGenerator::visit(AST::UiParameterList *ast)
{
    return AST::Visitor::visit(ast);
}

bool QQmlCodeGenerator::visit(AST::UiQualifiedId *id)
{
    return AST::Visitor::visit(id);
}

void QQmlCodeGenerator::accept(AST::Node *node)
{
    AST::Node::acceptChild(node, this);
}

bool QQmlCodeGenerator::sanityCheckFunctionNames()
{
    QSet<QString> functionNames;
    for (Function *f = _object->functions->first; f; f = f->next) {
        AST::FunctionDeclaration *function = AST::cast<AST::FunctionDeclaration*>(_functions.at(f->index));
        Q_ASSERT(function);
        QString name = function->name.toString();
        if (functionNames.contains(name))
            COMPILE_EXCEPTION(function->identifierToken, tr("Duplicate method name"));
        functionNames.insert(name);
        if (_signalNames.contains(name))
            COMPILE_EXCEPTION(function->identifierToken, tr("Duplicate method name"));

        if (name.at(0).isUpper())
            COMPILE_EXCEPTION(function->identifierToken, tr("Method names cannot begin with an upper case letter"));
#if 0 // ###
        if (enginePrivate->v8engine()->illegalNames().contains(currSlot.name.toString()))
            COMPILE_EXCEPTION(&currSlot, tr("Illegal method name"));
#endif
    }
    return true;
}

int QQmlCodeGenerator::defineQMLObject(AST::UiQualifiedId *qualifiedTypeNameId, AST::UiObjectInitializer *initializer)
{
    QmlObject *obj = New<QmlObject>();
    _objects.append(obj);
    const int objectIndex = _objects.size() - 1;
    qSwap(_object, obj);

    _object->inheritedTypeNameIndex = registerString(asString(qualifiedTypeNameId));

    AST::SourceLocation loc;
    if (qualifiedTypeNameId)
        loc = qualifiedTypeNameId->firstSourceLocation();
    _object->location.line = loc.startLine;
    _object->location.column = loc.startColumn;

    _object->idIndex = emptyStringIndex;
    _object->indexOfDefaultProperty = -1;
    _object->properties = New<PoolList<QmlProperty> >();
    _object->qmlSignals = New<PoolList<Signal> >();
    _object->bindings = New<PoolList<Binding> >();
    _object->functions = New<PoolList<Function> >();

    QSet<QString> propertyNames;
    qSwap(_propertyNames, propertyNames);
    QSet<QString> signalNames;
    qSwap(_signalNames, signalNames);

    accept(initializer);

    sanityCheckFunctionNames();

    qSwap(_propertyNames, propertyNames);
    qSwap(_signalNames, signalNames);
    qSwap(_object, obj);
    return objectIndex;
}

bool QQmlCodeGenerator::visit(AST::UiImport *node)
{
    QString uri;
    QV4::CompiledData::Import *import = New<QV4::CompiledData::Import>();

    if (!node->fileName.isNull()) {
        uri = node->fileName.toString();

        if (uri.endsWith(QLatin1String(".js"))) {
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
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Invalid import qualifier ID"));
            error.setLine(node->importIdToken.startLine);
            error.setColumn(node->importIdToken.startColumn);
            errors << error;
            return false;
        }
        if (qualifier == QLatin1String("Qt")) {
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Reserved name \"Qt\" cannot be used as an qualifier"));
            error.setLine(node->importIdToken.startLine);
            error.setColumn(node->importIdToken.startColumn);
            errors << error;
            return false;
        }
        import->qualifierIndex = registerString(qualifier);

        // Check for script qualifier clashes
        bool isScript = import->type == QV4::CompiledData::Import::ImportScript;
        for (int ii = 0; ii < _imports.count(); ++ii) {
            QV4::CompiledData::Import *other = _imports.at(ii);
            bool otherIsScript = other->type == QV4::CompiledData::Import::ImportScript;

            if ((isScript || otherIsScript) && qualifier == jsGenerator->strings.at(other->qualifierIndex)) {
                QQmlError error;
                error.setDescription(QCoreApplication::translate("QQmlParser","Script import qualifiers must be unique."));
                error.setLine(node->importIdToken.startLine);
                error.setColumn(node->importIdToken.startColumn);
                errors << error;
                return false;
            }
        }

    } else if (import->type == QV4::CompiledData::Import::ImportScript) {
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","Script import requires a qualifier"));
        error.setLine(node->fileNameToken.startLine);
        error.setColumn(node->fileNameToken.startColumn);
        errors << error;
        return false;
    }

    if (node->versionToken.isValid()) {
        extractVersion(textRefAt(node->versionToken), &import->majorVersion, &import->minorVersion);
    } else if (import->type == QV4::CompiledData::Import::ImportLibrary) {
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","Library import requires a version"));
        error.setLine(node->importIdToken.startLine);
        error.setColumn(node->importIdToken.startColumn);
        errors << error;
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

bool QQmlCodeGenerator::visit(AST::UiPragma *node)
{
    Pragma *pragma = New<Pragma>();

    // For now the only valid pragma is Singleton, so lets validate the input
    if (!node->pragmaType->name.isNull())
    {
        if (QLatin1String("Singleton") == node->pragmaType->name)
        {
            pragma->type = Pragma::PragmaSingleton;
        } else {
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Pragma requires a valid qualifier"));
            error.setLine(node->pragmaToken.startLine);
            error.setColumn(node->pragmaToken.startColumn);
            errors << error;
            return false;
        }
    } else {
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","Pragma requires a valid qualifier"));
        error.setLine(node->pragmaToken.startLine);
        error.setColumn(node->pragmaToken.startColumn);
        errors << error;
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

bool QQmlCodeGenerator::visit(AST::UiPublicMember *node)
{
    static const struct TypeNameToType {
        const char *name;
        size_t nameLength;
        QV4::CompiledData::Property::Type type;
    } propTypeNameToTypes[] = {
        { "int", strlen("int"), QV4::CompiledData::Property::Int },
        { "bool", strlen("bool"), QV4::CompiledData::Property::Bool },
        { "double", strlen("double"), QV4::CompiledData::Property::Real },
        { "real", strlen("real"), QV4::CompiledData::Property::Real },
        { "string", strlen("string"), QV4::CompiledData::Property::String },
        { "url", strlen("url"), QV4::CompiledData::Property::Url },
        { "color", strlen("color"), QV4::CompiledData::Property::Color },
        // Internally QTime, QDate and QDateTime are all supported.
        // To be more consistent with JavaScript we expose only
        // QDateTime as it matches closely with the Date JS type.
        // We also call it "date" to match.
        // { "time", strlen("time"), Property::Time },
        // { "date", strlen("date"), Property::Date },
        { "date", strlen("date"), QV4::CompiledData::Property::DateTime },
        { "rect", strlen("rect"), QV4::CompiledData::Property::Rect },
        { "point", strlen("point"), QV4::CompiledData::Property::Point },
        { "size", strlen("size"), QV4::CompiledData::Property::Size },
        { "font", strlen("font"), QV4::CompiledData::Property::Font },
        { "vector2d", strlen("vector2d"), QV4::CompiledData::Property::Vector2D },
        { "vector3d", strlen("vector3d"), QV4::CompiledData::Property::Vector3D },
        { "vector4d", strlen("vector4d"), QV4::CompiledData::Property::Vector4D },
        { "quaternion", strlen("quaternion"), QV4::CompiledData::Property::Quaternion },
        { "matrix4x4", strlen("matrix4x4"), QV4::CompiledData::Property::Matrix4x4 },
        { "variant", strlen("variant"), QV4::CompiledData::Property::Variant },
        { "var", strlen("var"), QV4::CompiledData::Property::Var }
    };
    static const int propTypeNameToTypesCount = sizeof(propTypeNameToTypes) /
                                                sizeof(propTypeNameToTypes[0]);

    if (node->type == AST::UiPublicMember::Signal) {
        Signal *signal = New<Signal>();
        QString signalName = node->name.toString();
        signal->nameIndex = registerString(signalName);

        AST::SourceLocation loc = node->firstSourceLocation();
        signal->location.line = loc.startLine;
        signal->location.column = loc.startColumn;

        signal->parameters = New<PoolList<SignalParameter> >();

        AST::UiParameterList *p = node->parameters;
        while (p) {
            const QStringRef &memberType = p->type;

            if (memberType.isEmpty()) {
                QQmlError error;
                error.setDescription(QCoreApplication::translate("QQmlParser","Expected parameter type"));
                error.setLine(node->typeToken.startLine);
                error.setColumn(node->typeToken.startColumn);
                errors << error;
                return false;
            }

            const TypeNameToType *type = 0;
            for (int typeIndex = 0; typeIndex < propTypeNameToTypesCount; ++typeIndex) {
                const TypeNameToType *t = propTypeNameToTypes + typeIndex;
                if (t->nameLength == size_t(memberType.length()) &&
                    QHashedString::compare(memberType.constData(), t->name, static_cast<int>(t->nameLength))) {
                    type = t;
                    break;
                }
            }

            SignalParameter *param = New<SignalParameter>();

            if (!type) {
                if (memberType.at(0).isUpper()) {
                    // Must be a QML object type.
                    // Lazily determine type during compilation.
                    param->type = QV4::CompiledData::Property::Custom;
                    param->customTypeNameIndex = registerString(p->type.toString());
                } else {
                    QQmlError error;
                    QString errStr = QCoreApplication::translate("QQmlParser","Invalid signal parameter type: ");
                    errStr.append(memberType.toString());
                    error.setDescription(errStr);
                    error.setLine(node->typeToken.startLine);
                    error.setColumn(node->typeToken.startColumn);
                    errors << error;
                    return false;
                }
            } else {
                // the parameter is a known basic type
                param->type = type->type;
                param->customTypeNameIndex = emptyStringIndex;
            }

            param->nameIndex = registerString(p->name.toString());
            param->location.line = p->identifierToken.startLine;
            param->location.column = p->identifierToken.startColumn;
            signal->parameters->append(param);
            p = p->next;
        }

        if (_signalNames.contains(signalName))
            COMPILE_EXCEPTION(node->identifierToken, tr("Duplicate signal name"));
        _signalNames.insert(signalName);

        if (signalName.at(0).isUpper())
            COMPILE_EXCEPTION(node->identifierToken, tr("Signal names cannot begin with an upper case letter"));

#if 0 // ### cannot access identifier table from separate thread
        if (enginePrivate->v8engine()->illegalNames().contains(currSig.name.toString()))
            COMPILE_EXCEPTION(&currSig, tr("Illegal signal name"));
#endif

        _object->qmlSignals->append(signal);
    } else {
        const QStringRef &memberType = node->memberType;
        const QStringRef &name = node->name;

        bool typeFound = false;
        QV4::CompiledData::Property::Type type;

        if ((unsigned)memberType.length() == strlen("alias") &&
            QHashedString::compare(memberType.constData(), "alias", static_cast<int>(strlen("alias")))) {
            type = QV4::CompiledData::Property::Alias;
            typeFound = true;
        }

        for (int ii = 0; !typeFound && ii < propTypeNameToTypesCount; ++ii) {
            const TypeNameToType *t = propTypeNameToTypes + ii;
            if (t->nameLength == size_t(memberType.length()) &&
                QHashedString::compare(memberType.constData(), t->name, static_cast<int>(t->nameLength))) {
                type = t->type;
                typeFound = true;
            }
        }

        if (!typeFound && memberType.at(0).isUpper()) {
            const QStringRef &typeModifier = node->typeModifier;

            if (typeModifier.isEmpty()) {
                type = QV4::CompiledData::Property::Custom;
            } else if ((unsigned)typeModifier.length() == strlen("list") &&
                      QHashedString::compare(typeModifier.constData(), "list", static_cast<int>(strlen("list")))) {
                type = QV4::CompiledData::Property::CustomList;
            } else {
                QQmlError error;
                error.setDescription(QCoreApplication::translate("QQmlParser","Invalid property type modifier"));
                error.setLine(node->typeModifierToken.startLine);
                error.setColumn(node->typeModifierToken.startColumn);
                errors << error;
                return false;
            }
            typeFound = true;
        } else if (!node->typeModifier.isNull()) {
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Unexpected property type modifier"));
            error.setLine(node->typeModifierToken.startLine);
            error.setColumn(node->typeModifierToken.startColumn);
            errors << error;
            return false;
        }

        if (!typeFound) {
            QQmlError error;
            error.setDescription(QCoreApplication::translate("QQmlParser","Expected property type"));
            error.setLine(node->typeToken.startLine);
            error.setColumn(node->typeToken.startColumn);
            errors << error;
            return false;
        }

        QmlProperty *property = New<QmlProperty>();
        property->flags = 0;
        if (node->isReadonlyMember)
            property->flags |= QV4::CompiledData::Property::IsReadOnly;
        property->type = type;
        if (type >= QV4::CompiledData::Property::Custom)
            property->customTypeNameIndex = registerString(memberType.toString());
        else
            property->customTypeNameIndex = emptyStringIndex;

        property->nameIndex = registerString(name.toString());

        AST::SourceLocation loc = node->firstSourceLocation();
        property->location.line = loc.startLine;
        property->location.column = loc.startColumn;

        property->aliasPropertyValueIndex = emptyStringIndex;

        if (type == QV4::CompiledData::Property::Alias) {
            if (!node->statement && !node->binding)
                COMPILE_EXCEPTION(loc, tr("No property alias location"));

            AST::SourceLocation rhsLoc;
            if (node->binding)
                rhsLoc = node->binding->firstSourceLocation();
            else if (node->statement)
                rhsLoc = node->statement->firstSourceLocation();
            else
                rhsLoc = node->semicolonToken;
            property->aliasLocation.line = rhsLoc.startLine;
            property->aliasLocation.column = rhsLoc.startColumn;

            QStringList alias;

            if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement*>(node->statement)) {
                alias = astNodeToStringList(stmt->expression);
                if (alias.isEmpty()) {
                    if (isStatementNodeScript(node->statement)) {
                        COMPILE_EXCEPTION(rhsLoc, tr("Invalid alias reference. An alias reference must be specified as <id>, <id>.<property> or <id>.<value property>.<property>"));
                    } else {
                        COMPILE_EXCEPTION(rhsLoc, tr("Invalid alias location"));
                    }
                }
            } else {
                COMPILE_EXCEPTION(rhsLoc, tr("Invalid alias reference. An alias reference must be specified as <id>, <id>.<property> or <id>.<value property>.<property>"));
            }

            if (alias.count() < 1 || alias.count() > 3)
                COMPILE_EXCEPTION(rhsLoc, tr("Invalid alias reference. An alias reference must be specified as <id>, <id>.<property> or <id>.<value property>.<property>"));

             property->aliasIdValueIndex = registerString(alias.first());

             QString propertyValue = alias.value(1);
             if (alias.count() == 3) {
                 propertyValue += QLatin1Char('.');
                 propertyValue += alias.at(2);
             }
             property->aliasPropertyValueIndex = registerString(propertyValue);
        } else if (node->statement)
            appendBinding(node->identifierToken, property->nameIndex, node->statement);

        _object->properties->append(property);

        if (node->isDefaultMember) {
            if (_object->indexOfDefaultProperty != -1) {
                QQmlError error;
                error.setDescription(QCoreApplication::translate("QQmlParser","Duplicate default property"));
                error.setLine(node->defaultToken.startLine);
                error.setColumn(node->defaultToken.startColumn);
                errors << error;
                return false;
            }
            _object->indexOfDefaultProperty = _object->properties->count - 1;
        }

        // process QML-like initializers (e.g. property Object o: Object {})
        AST::Node::accept(node->binding, this);
    }

    return false;
}

bool QQmlCodeGenerator::visit(AST::UiSourceElement *node)
{
    if (AST::FunctionDeclaration *funDecl = AST::cast<AST::FunctionDeclaration *>(node->sourceElement)) {
        _functions << funDecl;
        Function *f = New<Function>();
        f->index = _functions.size() - 1;
        _object->functions->append(f);
    } else {
        QQmlError error;
        error.setDescription(QCoreApplication::translate("QQmlParser","JavaScript declaration outside Script element"));
        error.setLine(node->firstSourceLocation().startLine);
        error.setColumn(node->firstSourceLocation().startColumn);
        errors << error;
    }
    return false;
}

QString QQmlCodeGenerator::asString(AST::UiQualifiedId *node)
{
    QString s;

    for (AST::UiQualifiedId *it = node; it; it = it->next) {
        s.append(it->name);

        if (it->next)
            s.append(QLatin1Char('.'));
    }

    return s;
}

QStringRef QQmlCodeGenerator::asStringRef(AST::Node *node)
{
    if (!node)
        return QStringRef();

    return textRefAt(node->firstSourceLocation(), node->lastSourceLocation());
}

void QQmlCodeGenerator::extractVersion(QStringRef string, int *maj, int *min)
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

QStringRef QQmlCodeGenerator::textRefAt(const AST::SourceLocation &first, const AST::SourceLocation &last) const
{
    return QStringRef(&sourceCode, first.offset, last.offset + last.length - first.offset);
}

void QQmlCodeGenerator::setBindingValue(QV4::CompiledData::Binding *binding, AST::Statement *statement)
{
    binding->type = QV4::CompiledData::Binding::Type_Invalid;

    if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement *>(statement)) {
        AST::ExpressionNode *expr = stmt->expression;
        if (AST::StringLiteral *lit = AST::cast<AST::StringLiteral *>(expr)) {
            binding->type = QV4::CompiledData::Binding::Type_String;
            binding->stringIndex = registerString(lit->value.toString());
        } else if (expr->kind == AST::Node::Kind_TrueLiteral) {
            binding->type = QV4::CompiledData::Binding::Type_Boolean;
            binding->value.b = true;
        } else if (expr->kind == AST::Node::Kind_FalseLiteral) {
            binding->type = QV4::CompiledData::Binding::Type_Boolean;
            binding->value.b = false;
        } else if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(expr)) {
            binding->type = QV4::CompiledData::Binding::Type_Number;
            binding->value.d = lit->value;
        } else {

            if (AST::UnaryMinusExpression *unaryMinus = AST::cast<AST::UnaryMinusExpression *>(expr)) {
               if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(unaryMinus->expression)) {
                   binding->type = QV4::CompiledData::Binding::Type_Number;
                   binding->value.d = -lit->value;
               }
            }
        }
    }

    // Do binding instead
    if (binding->type == QV4::CompiledData::Binding::Type_Invalid) {
        binding->type = QV4::CompiledData::Binding::Type_Script;
        _functions << statement;
        binding->value.compiledScriptIndex = _functions.size() - 1;
        binding->stringIndex = registerString(asStringRef(statement).toString());
    }
}

void QQmlCodeGenerator::appendBinding(AST::UiQualifiedId *name, AST::Statement *value)
{
    QmlObject *object = 0;
    if (!resolveQualifiedId(&name, &object))
        return;
    qSwap(_object, object);
    appendBinding(name->identifierToken, registerString(name->name.toString()), value);
    qSwap(_object, object);
}

void QQmlCodeGenerator::appendBinding(AST::UiQualifiedId *name, int objectIndex)
{
    QmlObject *object = 0;
    if (!resolveQualifiedId(&name, &object))
        return;
    qSwap(_object, object);
    appendBinding(name->identifierToken, registerString(name->name.toString()), objectIndex);
    qSwap(_object, object);
}

void QQmlCodeGenerator::appendBinding(const AST::SourceLocation &nameLocation, int propertyNameIndex, AST::Statement *value)
{
    if (!sanityCheckPropertyName(nameLocation, propertyNameIndex))
        return;

    if (stringAt(propertyNameIndex) == QStringLiteral("id")) {
        setId(value);
        return;
    }

    Binding *binding = New<Binding>();
    binding->propertyNameIndex = propertyNameIndex;
    binding->location.line = nameLocation.startLine;
    binding->location.column = nameLocation.startColumn;
    binding->flags = 0;
    setBindingValue(binding, value);
    _object->bindings->append(binding);
}

void QQmlCodeGenerator::appendBinding(const AST::SourceLocation &nameLocation, int propertyNameIndex, int objectIndex, bool isListItem)
{
    if (!sanityCheckPropertyName(nameLocation, propertyNameIndex, isListItem))
        return;

    if (stringAt(propertyNameIndex) == QStringLiteral("id")) {
        recordError(nameLocation, tr("Invalid component id specification"));
        return;
    }

    Binding *binding = New<Binding>();
    binding->propertyNameIndex = propertyNameIndex;
    binding->location.line = nameLocation.startLine;
    binding->location.column = nameLocation.startColumn;
    binding->flags = 0;
    binding->type = QV4::CompiledData::Binding::Type_Object;
    binding->value.objectIndex = objectIndex;
    _object->bindings->append(binding);
}

bool QQmlCodeGenerator::setId(AST::Statement *value)
{
    AST::SourceLocation loc = value->firstSourceLocation();
    QStringRef str;

    AST::Node *node = value;
    if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement *>(node)) {
        if (AST::StringLiteral *lit = AST::cast<AST::StringLiteral *>(stmt->expression))
            str = lit->value;
        else
            node = stmt->expression;
    }

    if (str.isEmpty())
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

#if 0 // ###
    if (enginePrivate->v8engine()->illegalNames().contains(str))
        COMPILE_EXCEPTION(v, tr( "ID illegally masks global JavaScript property"));
#endif

    _object->idIndex = registerString(str.toString());
    _object->locationOfIdProperty.line = loc.startLine;
    _object->locationOfIdProperty.column = loc.startColumn;

    return true;
}

bool QQmlCodeGenerator::resolveQualifiedId(AST::UiQualifiedId **nameToResolve, QmlObject **object)
{
    AST::UiQualifiedId *name = *nameToResolve;

    if (name->name == QStringLiteral("id") && name->next)
        COMPILE_EXCEPTION(name->identifierToken, tr( "Invalid use of id property"));

    *object = _object;
    while (name->next) {
        Binding *binding = New<Binding>();
        binding->propertyNameIndex = registerString(name->name.toString());
        binding->location.line = name->identifierToken.startLine;
        binding->location.column = name->identifierToken.startColumn;
        binding->flags = 0;

        if (name->name.unicode()->isUpper())
            binding->type = QV4::CompiledData::Binding::Type_AttachedProperty;
        else
            binding->type = QV4::CompiledData::Binding::Type_GroupProperty;

        int objIndex = defineQMLObject(0, 0);
        binding->value.objectIndex = objIndex;

        (*object)->bindings->append(binding);
        *object = _objects[objIndex];

        name = name->next;
    }
    *nameToResolve = name;
    return true;
}

bool QQmlCodeGenerator::sanityCheckPropertyName(const AST::SourceLocation &nameLocation, int nameIndex, bool isListItem)
{
    const QString &name = jsGenerator->strings.at(nameIndex);
    if (name.isEmpty())
        return true;

    // List items are implement by multiple bindings to the same name, so allow duplicates.
    if (!isListItem) {
        if (_propertyNames.contains(name))
            COMPILE_EXCEPTION(nameLocation, tr("Duplicate property name"));

        _propertyNames.insert(name);
    }

    if (name.at(0).isUpper())
        COMPILE_EXCEPTION(nameLocation, tr("Property names cannot begin with an upper case letter"));

#if 0 // ### how to check against illegalNames when in separate thread?
    if (enginePrivate->v8engine()->illegalNames().contains(prop.name.toString())) {
        COMPILE_EXCEPTION_LOCATION(prop.nameLocation.line,
                                   prop.nameLocation.column,
                                   tr("Illegal property name"));
    }
#endif

    return true;
}

void QQmlCodeGenerator::recordError(const AST::SourceLocation &location, const QString &description)
{
    QQmlError error;
    error.setUrl(url);
    error.setLine(location.startLine);
    error.setColumn(location.startColumn);
    error.setDescription(description);
    errors << error;
}

void QQmlCodeGenerator::collectTypeReferences()
{
    foreach (QmlObject *obj, _objects) {
        if (!stringAt(obj->inheritedTypeNameIndex).isEmpty())
            _typeReferences.add(obj->inheritedTypeNameIndex, obj->location);

        for (QmlProperty *prop = obj->properties->first; prop; prop = prop->next) {
            if (prop->type >= QV4::CompiledData::Property::Custom)
                _typeReferences.add(prop->customTypeNameIndex, prop->location);
        }

        for (Signal *sig = obj->qmlSignals->first; sig; sig = sig->next)
            for (SignalParameter *param = sig->parameters->first; param; param = param->next)
                if (!stringAt(param->customTypeNameIndex).isEmpty())
                    _typeReferences.add(param->customTypeNameIndex, param->location);

        for (Binding *binding = obj->bindings->first; binding; binding = binding->next) {
            if (binding->type == QV4::CompiledData::Binding::Type_AttachedProperty)
                _typeReferences.add(binding->propertyNameIndex, binding->location);
        }
    }
}

QQmlScript::LocationSpan QQmlCodeGenerator::location(AST::SourceLocation start, AST::SourceLocation end)
{
    QQmlScript::LocationSpan rv;
    rv.start.line = start.startLine;
    rv.start.column = start.startColumn;
    rv.end.line = end.startLine;
    rv.end.column = end.startColumn + end.length - 1;
    rv.range.offset = start.offset;
    rv.range.length = end.offset + end.length - start.offset;
    return rv;
}

bool QQmlCodeGenerator::isStatementNodeScript(AST::Statement *statement)
{
    if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement *>(statement)) {
        AST::ExpressionNode *expr = stmt->expression;
        if (AST::cast<AST::StringLiteral *>(expr))
            return false;
        else if (expr->kind == AST::Node::Kind_TrueLiteral)
            return false;
        else if (expr->kind == AST::Node::Kind_FalseLiteral)
            return false;
        else if (AST::cast<AST::NumericLiteral *>(expr))
            return false;
        else {

            if (AST::UnaryMinusExpression *unaryMinus = AST::cast<AST::UnaryMinusExpression *>(expr)) {
               if (AST::cast<AST::NumericLiteral *>(unaryMinus->expression)) {
                   return false;
               }
            }
        }
    }

    return true;
}

QV4::CompiledData::QmlUnit *QmlUnitGenerator::generate(ParsedQML &output, const QVector<int> &runtimeFunctionIndices)
{
    jsUnitGenerator = &output.jsGenerator;
    int unitSize = 0;
    QV4::CompiledData::Unit *jsUnit = jsUnitGenerator->generateUnit(&unitSize);

    const int importSize = sizeof(QV4::CompiledData::Import) * output.imports.count();
    const int objectOffsetTableSize = output.objects.count() * sizeof(quint32);

    QHash<QmlObject*, quint32> objectOffsets;

    int objectsSize = 0;
    foreach (QmlObject *o, output.objects) {
        objectOffsets.insert(o, unitSize + importSize + objectOffsetTableSize + objectsSize);
        objectsSize += QV4::CompiledData::Object::calculateSizeExcludingSignals(o->functions->count, o->properties->count, o->qmlSignals->count, o->bindings->count);

        int signalTableSize = 0;
        for (Signal *s = o->qmlSignals->first; s; s = s->next)
            signalTableSize += QV4::CompiledData::Signal::calculateSize(s->parameters->count);

        objectsSize += signalTableSize;
    }

    const int totalSize = unitSize + importSize + objectOffsetTableSize + objectsSize;
    char *data = (char*)malloc(totalSize);
    memcpy(data, jsUnit, unitSize);
    free(jsUnit);
    jsUnit = 0;

    QV4::CompiledData::QmlUnit *qmlUnit = reinterpret_cast<QV4::CompiledData::QmlUnit *>(data);
    qmlUnit->header.flags |= QV4::CompiledData::Unit::IsQml;
    qmlUnit->offsetToImports = unitSize;
    qmlUnit->nImports = output.imports.count();
    qmlUnit->offsetToObjects = unitSize + importSize;
    qmlUnit->nObjects = output.objects.count();
    qmlUnit->indexOfRootObject = output.indexOfRootObject;

    // write imports
    char *importPtr = data + qmlUnit->offsetToImports;
    foreach (QV4::CompiledData::Import *imp, output.imports) {
        QV4::CompiledData::Import *importToWrite = reinterpret_cast<QV4::CompiledData::Import*>(importPtr);
        *importToWrite = *imp;
        importPtr += sizeof(QV4::CompiledData::Import);
    }

    // write objects
    quint32 *objectTable = reinterpret_cast<quint32*>(data + qmlUnit->offsetToObjects);
    char *objectPtr = data + qmlUnit->offsetToObjects + objectOffsetTableSize;
    foreach (QmlObject *o, output.objects) {
        *objectTable++ = objectOffsets.value(o);

        QV4::CompiledData::Object *objectToWrite = reinterpret_cast<QV4::CompiledData::Object*>(objectPtr);
        objectToWrite->inheritedTypeNameIndex = o->inheritedTypeNameIndex;
        objectToWrite->indexOfDefaultProperty = o->indexOfDefaultProperty;
        objectToWrite->idIndex = o->idIndex;
        objectToWrite->location = o->location;
        objectToWrite->locationOfIdProperty = o->locationOfIdProperty;

        quint32 nextOffset = sizeof(QV4::CompiledData::Object);

        objectToWrite->nFunctions = o->functions->count;
        objectToWrite->offsetToFunctions = nextOffset;
        nextOffset += objectToWrite->nFunctions * sizeof(quint32);

        objectToWrite->nProperties = o->properties->count;
        objectToWrite->offsetToProperties = nextOffset;
        nextOffset += objectToWrite->nProperties * sizeof(QV4::CompiledData::Property);

        objectToWrite->nSignals = o->qmlSignals->count;
        objectToWrite->offsetToSignals = nextOffset;
        nextOffset += objectToWrite->nSignals * sizeof(quint32);

        objectToWrite->nBindings = o->bindings->count;
        objectToWrite->offsetToBindings = nextOffset;
        nextOffset += objectToWrite->nBindings * sizeof(QV4::CompiledData::Binding);

        quint32 *functionsTable = reinterpret_cast<quint32*>(objectPtr + objectToWrite->offsetToFunctions);
        for (Function *f = o->functions->first; f; f = f->next)
            *functionsTable++ = runtimeFunctionIndices[f->index];

        char *propertiesPtr = objectPtr + objectToWrite->offsetToProperties;
        for (QmlProperty *p = o->properties->first; p; p = p->next) {
            QV4::CompiledData::Property *propertyToWrite = reinterpret_cast<QV4::CompiledData::Property*>(propertiesPtr);
            *propertyToWrite = *p;
            propertiesPtr += sizeof(QV4::CompiledData::Property);
        }

        char *bindingPtr = objectPtr + objectToWrite->offsetToBindings;
        for (Binding *b = o->bindings->first; b; b = b->next) {
            QV4::CompiledData::Binding *bindingToWrite = reinterpret_cast<QV4::CompiledData::Binding*>(bindingPtr);
            *bindingToWrite = *b;
            if (b->type == QV4::CompiledData::Binding::Type_Script)
                bindingToWrite->value.compiledScriptIndex = runtimeFunctionIndices[b->value.compiledScriptIndex];
            bindingPtr += sizeof(QV4::CompiledData::Binding);
        }

        quint32 *signalOffsetTable = reinterpret_cast<quint32*>(objectPtr + objectToWrite->offsetToSignals);
        quint32 signalTableSize = 0;
        char *signalPtr = objectPtr + nextOffset;
        for (Signal *s = o->qmlSignals->first; s; s = s->next) {
            *signalOffsetTable++ = signalPtr - objectPtr;
            QV4::CompiledData::Signal *signalToWrite = reinterpret_cast<QV4::CompiledData::Signal*>(signalPtr);

            signalToWrite->nameIndex = s->nameIndex;
            signalToWrite->location = s->location;
            signalToWrite->nParameters = s->parameters->count;

            QV4::CompiledData::Parameter *parameterToWrite = reinterpret_cast<QV4::CompiledData::Parameter*>(signalPtr + sizeof(*signalToWrite));
            for (SignalParameter *param = s->parameters->first; param; param = param->next, ++parameterToWrite)
                *parameterToWrite = *param;

            int size = QV4::CompiledData::Signal::calculateSize(s->parameters->count);
            signalTableSize += size;
            signalPtr += size;
        }

        objectPtr += QV4::CompiledData::Object::calculateSizeExcludingSignals(o->functions->count, o->properties->count, o->qmlSignals->count, o->bindings->count);
        objectPtr += signalTableSize;
    }

    // enable flag if we encountered pragma Singleton
    foreach (Pragma *p, output.pragmas) {
        if (p->type == Pragma::PragmaSingleton) {
            qmlUnit->header.flags |= QV4::CompiledData::Unit::IsSingleton;
            break;
        }
    }

    return qmlUnit;
}

int QmlUnitGenerator::getStringId(const QString &str) const
{
    return jsUnitGenerator->getStringId(str);
}

JSCodeGen::JSCodeGen(QQmlEnginePrivate *enginePrivate, const QString &fileName, const QString &sourceCode, V4IR::Module *jsModule, Engine *jsEngine, AST::UiProgram *qmlRoot, QQmlTypeNameCache *imports)
    : QQmlJS::Codegen(/*strict mode*/false)
    , engine(enginePrivate)
    , sourceCode(sourceCode)
    , jsEngine(jsEngine)
    , qmlRoot(qmlRoot)
    , imports(imports)
{
    _module = jsModule;
    _module->setFileName(fileName);
    _fileNameIsUrl = true;
}

void JSCodeGen::beginContextScope(const JSCodeGen::ObjectIdMapping &objectIds, QQmlPropertyCache *contextObject)
{
    _idObjects = objectIds;
    _contextObject = contextObject;
    _scopeObject = 0;
}

void JSCodeGen::beginObjectScope(QQmlPropertyCache *scopeObject)
{
    _scopeObject = scopeObject;
}

QVector<int> JSCodeGen::generateJSCodeForFunctionsAndBindings(const QList<AST::Node*> &functions, const QHash<int, QString> &functionNames)
{
    QVector<int> runtimeFunctionIndices(functions.size());

    ScanFunctions scan(this, sourceCode, GlobalCode);
    scan.enterEnvironment(0, QmlBinding);
    scan.enterQmlScope(qmlRoot, QStringLiteral("context scope"));
    foreach (AST::Node *node, functions) {
        Q_ASSERT(node != qmlRoot);
        AST::FunctionDeclaration *function = AST::cast<AST::FunctionDeclaration*>(node);

        if (function)
            scan.enterQmlFunction(function);
        else
            scan.enterEnvironment(node, QmlBinding);

        scan(function ? function->body : node);
        scan.leaveEnvironment();
    }
    scan.leaveEnvironment();
    scan.leaveEnvironment();

    _env = 0;
    _function = _module->functions.at(defineFunction(QStringLiteral("context scope"), qmlRoot, 0, 0));

    for (int i = 0; i < functions.count(); ++i) {
        AST::Node *node = functions.at(i);
        Q_ASSERT(node != qmlRoot);

        AST::FunctionDeclaration *function = AST::cast<AST::FunctionDeclaration*>(node);

        QString name;
        if (function)
            name = function->name.toString();
        else
            name = functionNames.value(i, QStringLiteral("%qml-expression-entry"));

        AST::SourceElements *body;
        if (function)
            body = function->body ? function->body->elements : 0;
        else {
            // Synthesize source elements.
            QQmlJS::MemoryPool *pool = jsEngine->pool();

            AST::Statement *stmt = node->statementCast();
            if (!stmt) {
                Q_ASSERT(node->expressionCast());
                AST::ExpressionNode *expr = node->expressionCast();
                stmt = new (pool) AST::ExpressionStatement(expr);
            }
            AST::SourceElement *element = new (pool) AST::StatementSourceElement(stmt);
            body = new (pool) AST::SourceElements(element);
            body = body->finish();
        }

        int idx = defineFunction(name, node,
                                 function ? function->formals : 0,
                                 body);
        runtimeFunctionIndices[i] = idx;
    }

    qDeleteAll(_envMap);
    _envMap.clear();
    return runtimeFunctionIndices;
}

static QQmlPropertyData *lookupQmlCompliantProperty(QQmlPropertyCache *cache, const QString &name, bool *propertyExistsButForceNameLookup = 0)
{
    if (propertyExistsButForceNameLookup)
        *propertyExistsButForceNameLookup = false;
    QQmlPropertyData *pd = cache->property(name, /*object*/0, /*context*/0);

    // Q_INVOKABLEs can't be FINAL, so we have to look them up at run-time
    if (pd && pd->isFunction()) {
        if (propertyExistsButForceNameLookup)
            *propertyExistsButForceNameLookup = true;
        pd = 0;
    }

    if (pd && !cache->isAllowedInRevision(pd))
        pd = 0;

    return pd;
}

V4IR::Expr *JSCodeGen::member(V4IR::Expr *base, const QString *name)
{
    V4IR::Member *baseAsMember = base->asMember();
    if (baseAsMember) {
        QQmlPropertyCache *cache = 0;

        if (baseAsMember->type == V4IR::Member::MemberOfQObject
            && baseAsMember->property->isQObject()) {

            bool propertySuitable = baseAsMember->property->isFinal();

            if (!propertySuitable) {
                // Properties of the scope or context object do not need to be final, as we
                // intend to find the version of a property available at compile time, not at run-time.
                if (V4IR::Name *baseName = baseAsMember->base->asName())
                    propertySuitable = baseName->builtin == V4IR::Name::builtin_qml_scope_object || baseName->builtin == V4IR::Name::builtin_qml_context_object;
            }

            // Check if it's suitable for caching
            if (propertySuitable)
                cache = engine->propertyCacheForType(baseAsMember->property->propType);
        } else if (baseAsMember->type == V4IR::Member::MemberOfQmlContext) {
            // Similarly, properties of an id referenced object also don't need to be final, because
            // we intend to find the version of a property available at compile time, not at run-time.
            foreach (const IdMapping &mapping, _idObjects) {
                if (baseAsMember->memberIndex == mapping.idIndex) {
                    cache = mapping.type;
                    break;
                }
            }
        }

        if (cache) {
            if (QQmlPropertyData *pd = lookupQmlCompliantProperty(cache, *name)) {
                const unsigned baseTemp = _block->newTemp();
                move(_block->TEMP(baseTemp), base);
                return _block->QML_QOBJECT_PROPERTY(_block->TEMP(baseTemp), name, pd);
            }
        }
    }
    return QQmlJS::Codegen::member(base, name);
}

V4IR::Expr *JSCodeGen::fallbackNameLookup(const QString &name, int line, int col)
{
    // Implement QML lookup semantics in the current file context.
    //
    // Note: We do not check if properties of the qml scope object or context object
    // are final. That's because QML tries to get as close as possible to lexical scoping,
    // which means in terms of properties that only those visible at compile time are chosen.
    // I.e. access to a "foo" property declared within the same QML component as "property int foo"
    // will always access that instance and as integer. If a sub-type implements its own property string foo,
    // then that one is not chosen for accesses from within this file, because it wasn't visible at compile
    // time. This corresponds to the logic in QQmlPropertyCache::findProperty to find the property associated
    // with the correct QML context.

    // Look for IDs first.
    foreach (const IdMapping &mapping, _idObjects)
        if (name == mapping.name) {
            _function->idObjectDependencies.insert(mapping.idIndex);
            return _block->QML_CONTEXT_MEMBER(_block->NAME(V4IR::Name::builtin_qml_id_scope, line, col),
                                              _function->newString(mapping.name), mapping.idIndex);
        }

    {
        QQmlTypeNameCache::Result r = imports->query(name);
        if (r.isValid()) {
            if (r.scriptIndex != -1)
                return subscript(_block->NAME(V4IR::Name::builtin_qml_imported_scripts_object, line, col), _block->CONST(V4IR::NumberType, r.scriptIndex));
            else
                return 0; // TODO: We can't do fast lookup for these yet.
        }
    }

    if (_scopeObject) {
        bool propertyExistsButForceNameLookup = false;
        QQmlPropertyData *pd = lookupQmlCompliantProperty(_scopeObject, name, &propertyExistsButForceNameLookup);
        if (propertyExistsButForceNameLookup)
            return 0;
        if (pd) {
            if (!pd->isConstant())
                _function->scopeObjectDependencies.insert(pd);
            int base = _block->newTemp();
            move(_block->TEMP(base), _block->NAME(V4IR::Name::builtin_qml_scope_object, line, col));
            return  _block->QML_QOBJECT_PROPERTY(_block->TEMP(base), _function->newString(name), pd);
        }
    }

    if (_contextObject) {
        bool propertyExistsButForceNameLookup = false;
        QQmlPropertyData *pd = lookupQmlCompliantProperty(_contextObject, name, &propertyExistsButForceNameLookup);
        if (propertyExistsButForceNameLookup)
            return 0;
        if (pd) {
            if (!pd->isConstant())
                _function->contextObjectDependencies.insert(pd);
            int base = _block->newTemp();
            move(_block->TEMP(base), _block->NAME(V4IR::Name::builtin_qml_context_object, line, col));
            return _block->QML_QOBJECT_PROPERTY(_block->TEMP(base), _function->newString(name), pd);
        }
    }

    // fall back to name lookup at run-time.
    return 0;
}

SignalHandlerConverter::SignalHandlerConverter(QQmlEnginePrivate *enginePrivate, ParsedQML *parsedQML,
                                               QQmlCompiledData *unit)
    : enginePrivate(enginePrivate)
    , parsedQML(parsedQML)
    , unit(unit)
{
}

bool SignalHandlerConverter::convertSignalHandlerExpressionsToFunctionDeclarations()
{
    foreach (QmlObject *obj, parsedQML->objects) {
        QString elementName = stringAt(obj->inheritedTypeNameIndex);
        if (elementName.isEmpty())
            continue;
        QQmlPropertyCache *cache = unit->resolvedTypes[obj->inheritedTypeNameIndex].createPropertyCache(QQmlEnginePrivate::get(enginePrivate));
        if (!convertSignalHandlerExpressionsToFunctionDeclarations(obj, elementName, cache))
            return false;
    }
    return true;
}

bool SignalHandlerConverter::convertSignalHandlerExpressionsToFunctionDeclarations(QmlObject *obj, const QString &typeName, QQmlPropertyCache *propertyCache)
{
    // map from signal name defined in qml itself to list of parameters
    QHash<QString, QStringList> customSignals;

    for (Binding *binding = obj->bindings->first; binding; binding = binding->next) {
        QString propertyName = stringAt(binding->propertyNameIndex);
        // Attached property?
        if (binding->type == QV4::CompiledData::Binding::Type_AttachedProperty) {
            QmlObject *attachedObj = parsedQML->objects[binding->value.objectIndex];
            QQmlType *type = unit->resolvedTypes.value(binding->propertyNameIndex).type;
            const QMetaObject *attachedType = type ? type->attachedPropertiesType() : 0;
            if (!attachedType)
                COMPILE_EXCEPTION(binding->location, tr("Non-existent attached object"));
            QQmlPropertyCache *cache = enginePrivate->cache(attachedType);
            if (!convertSignalHandlerExpressionsToFunctionDeclarations(attachedObj, propertyName, cache))
                return false;
            continue;
        }

        if (!QQmlCodeGenerator::isSignalPropertyName(propertyName))
            continue;

        if (binding->type != QV4::CompiledData::Binding::Type_Script) {
            COMPILE_EXCEPTION(binding->location, tr("Incorrectly specified signal assignment"));
        }

        PropertyResolver resolver(propertyCache);

        Q_ASSERT(propertyName.startsWith(QStringLiteral("on")));
        propertyName.remove(0, 2);

        // Note that the property name could start with any alpha or '_' or '$' character,
        // so we need to do the lower-casing of the first alpha character.
        for (int firstAlphaIndex = 0; firstAlphaIndex < propertyName.size(); ++firstAlphaIndex) {
            if (propertyName.at(firstAlphaIndex).isUpper()) {
                propertyName[firstAlphaIndex] = propertyName.at(firstAlphaIndex).toLower();
                break;
            }
        }

        QList<QString> parameters;

        bool notInRevision = false;
        QQmlPropertyData *signal = resolver.signal(propertyName, &notInRevision);
        if (signal) {
            int sigIndex = propertyCache->methodIndexToSignalIndex(signal->coreIndex);
            foreach (const QByteArray &param, propertyCache->signalParameterNames(sigIndex))
                parameters << QString::fromUtf8(param);
        } else {
            if (notInRevision) {
                // Try assinging it as a property later
                if (resolver.property(propertyName, /*notInRevision ptr*/0))
                    continue;

                const QString &originalPropertyName = stringAt(binding->propertyNameIndex);

                const QQmlType *type = unit->resolvedTypes.value(obj->inheritedTypeNameIndex).type;
                if (type) {
                    COMPILE_EXCEPTION(binding->location, tr("\"%1.%2\" is not available in %3 %4.%5.").arg(typeName).arg(originalPropertyName).arg(type->module()).arg(type->majorVersion()).arg(type->minorVersion()));
                } else {
                    COMPILE_EXCEPTION(binding->location, tr("\"%1.%2\" is not available due to component versioning.").arg(typeName).arg(originalPropertyName));
                }
            }

            // Try to look up the signal parameter names in the object itself

            // build cache if necessary
            if (customSignals.isEmpty()) {
                for (Signal *signal = obj->qmlSignals->first; signal; signal = signal->next) {
                    const QString &signalName = stringAt(signal->nameIndex);
                    customSignals.insert(signalName, signal->parameterStringList(parsedQML->jsGenerator.strings));
                }

                for (QmlProperty *property = obj->properties->first; property; property = property->next) {
                    const QString propName = stringAt(property->nameIndex);
                    customSignals.insert(propName, QStringList());
                }
            }

            QHash<QString, QStringList>::ConstIterator entry = customSignals.find(propertyName);
            if (entry == customSignals.constEnd() && propertyName.endsWith(QStringLiteral("Changed"))) {
                QString alternateName = propertyName.mid(0, propertyName.length() - static_cast<int>(strlen("Changed")));
                entry = customSignals.find(alternateName);
            }

            if (entry == customSignals.constEnd()) {
                // Can't find even a custom signal, then just don't do anything and try
                // keeping the binding as a regular property assignment.
                continue;
            }

            parameters = entry.value();
        }

        QQmlJS::Engine &jsEngine = parsedQML->jsParserEngine;
        QQmlJS::MemoryPool *pool = jsEngine.pool();

        AST::FormalParameterList *paramList = 0;
        foreach (const QString &param, parameters) {
            QStringRef paramNameRef = jsEngine.newStringRef(param);

            if (paramList)
                paramList = new (pool) AST::FormalParameterList(paramList, paramNameRef);
            else
                paramList = new (pool) AST::FormalParameterList(paramNameRef);
        }

        if (paramList)
            paramList = paramList->finish();

        AST::Statement *statement = static_cast<AST::Statement*>(parsedQML->functions[binding->value.compiledScriptIndex]);
        AST::SourceElement *sourceElement = new (pool) AST::StatementSourceElement(statement);
        AST::SourceElements *elements = new (pool) AST::SourceElements(sourceElement);
        elements = elements->finish();

        AST::FunctionBody *body = new (pool) AST::FunctionBody(elements);

        AST::FunctionDeclaration *functionDeclaration = new (pool) AST::FunctionDeclaration(jsEngine.newStringRef(propertyName), paramList, body);

        parsedQML->functions[binding->value.compiledScriptIndex] = functionDeclaration;
        binding->flags |= QV4::CompiledData::Binding::IsSignalHandlerExpression;
        binding->propertyNameIndex = parsedQML->jsGenerator.registerString(propertyName);
    }
    return true;
}

void SignalHandlerConverter::recordError(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setUrl(unit->url);
    error.setLine(location.line);
    error.setColumn(location.column);
    error.setDescription(description);
    errors << error;
}

QQmlPropertyData *PropertyResolver::property(const QString &name, bool *notInRevision)
{
    if (notInRevision) *notInRevision = false;

    QQmlPropertyData *d = cache->property(name, 0, 0);

    // Find the first property
    while (d && d->isFunction())
        d = cache->overrideData(d);

    if (d && !cache->isAllowedInRevision(d)) {
        if (notInRevision) *notInRevision = true;
        return 0;
    } else {
        return d;
    }
}


QQmlPropertyData *PropertyResolver::signal(const QString &name, bool *notInRevision)
{
    if (notInRevision) *notInRevision = false;

    QQmlPropertyData *d = cache->property(name, 0, 0);
    if (notInRevision) *notInRevision = false;

    while (d && !(d->isFunction()))
        d = cache->overrideData(d);

    if (d && !cache->isAllowedInRevision(d)) {
        if (notInRevision) *notInRevision = true;
        return 0;
    } else if (d && d->isSignal()) {
        return d;
    }

    if (name.endsWith(QStringLiteral("Changed"))) {
        QString propName = name.mid(0, name.length() - static_cast<int>(strlen("Changed")));

        d = property(propName, notInRevision);
        if (d)
            return cache->signal(d->notifyIndex);
    }

    return 0;
}
