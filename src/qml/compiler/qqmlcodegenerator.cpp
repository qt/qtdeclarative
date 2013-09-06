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
#include <QCoreApplication>

QT_USE_NAMESPACE

using namespace QtQml;

void QmlObject::dump(DebugStream &out)
{
    out << inheritedTypeNameIndex << " {" << endl;
    out.indent++;

    out.indent--;
    out << "}" << endl;
}

QQmlCodeGenerator::QQmlCodeGenerator()
    : _object(0)
    , jsGenerator(0)
{
}

bool QQmlCodeGenerator::generateFromQml(const QString &code, const QUrl &url, const QString &urlString, ParsedQML *output)
{
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

    qSwap(_imports, output->imports);
    qSwap(_objects, output->objects);
    qSwap(_functions, output->functions);
    this->pool = output->jsParserEngine.pool();
    this->jsGenerator = &output->jsGenerator;

    sourceCode = code;

    accept(program->imports);

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
    qSwap(_imports, output->imports);
    qSwap(_objects, output->objects);
    qSwap(_functions, output->functions);
    return true;
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
    int idx = defineQMLObject(node);
    appendBinding(registerString(QString()), idx);
    return false;
}

bool QQmlCodeGenerator::visit(AST::UiObjectBinding *node)
{
    int idx = defineQMLObject(node->qualifiedTypeNameId, node->initializer);
    appendBinding(registerString(asString(node->qualifiedId)), idx);
    return false;
}

bool QQmlCodeGenerator::visit(AST::UiScriptBinding *node)
{
    appendBinding(registerString(asString(node->qualifiedId)), node->statement);
    return false;
}

bool QQmlCodeGenerator::visit(AST::UiArrayBinding*)
{
    return true;
}

bool QQmlCodeGenerator::visit(AST::UiImportList *list)
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

int QQmlCodeGenerator::defineQMLObject(AST::UiQualifiedId *qualifiedTypeNameId, AST::UiObjectInitializer *initializer)
{
    QmlObject *obj = New<QmlObject>();
    _objects.append(obj);
    const int objectIndex = _objects.size() - 1;
    qSwap(_object, obj);

    _object->inheritedTypeNameIndex = registerString(asString(qualifiedTypeNameId));
    _object->idIndex = registerString(QString());
    _object->indexOfDefaultProperty = -1;
    _object->properties = New<PoolList<QmlProperty> >();
    _object->qmlSignals = New<PoolList<Signal> >();
    _object->bindings = New<PoolList<Binding> >();
    _object->functions = New<PoolList<Function> >();

    accept(initializer);

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

    import->qualifierIndex = registerString(QString());

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
    }

    import->location.line = node->importIdToken.startLine;
    import->location.column = node->importIdToken.startColumn;

    import->uriIndex = registerString(uri);

    _imports.append(import);

    return false;
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
        signal->nameIndex = registerString(node->name.toString());

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
                    QHashedString::compare(memberType.constData(), t->name, t->nameLength)) {
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
                param->customTypeNameIndex = registerString(QString());
            }

            param->nameIndex = registerString(p->name.toString());
            signal->parameters->append(param);
            p = p->next;
        }

        _object->qmlSignals->append(signal);
    } else {
        const QStringRef &memberType = node->memberType;
        const QStringRef &name = node->name;

        bool typeFound = false;
        QV4::CompiledData::Property::Type type;

        if ((unsigned)memberType.length() == strlen("alias") &&
            QHashedString::compare(memberType.constData(), "alias", strlen("alias"))) {
            type = QV4::CompiledData::Property::Alias;
            typeFound = true;
        }

        for (int ii = 0; !typeFound && ii < propTypeNameToTypesCount; ++ii) {
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
                type = QV4::CompiledData::Property::Custom;
            } else if ((unsigned)typeModifier.length() == strlen("list") &&
                      QHashedString::compare(typeModifier.constData(), "list", strlen("list"))) {
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
            property->customTypeNameIndex = registerString(QString());

        property->nameIndex = registerString(name.toString());

        if (node->statement)
            appendBinding(property->nameIndex, node->statement);

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
        // ### check if this is correct?
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

QStringRef QQmlCodeGenerator::textRefAt(const AST::SourceLocation &first, const AST::SourceLocation &last) const
{
    return QStringRef(&sourceCode, first.offset, last.offset + last.length - first.offset);
}

void QQmlCodeGenerator::setBindingValue(QV4::CompiledData::Binding *binding, AST::Statement *statement)
{
    binding->value.type = QV4::CompiledData::Value::Type_Invalid;

    if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement *>(statement)) {
        AST::ExpressionNode *expr = stmt->expression;
        if (AST::StringLiteral *lit = AST::cast<AST::StringLiteral *>(expr)) {
            binding->value.type = QV4::CompiledData::Value::Type_String;
            binding->value.stringIndex = registerString(lit->value.toString());
        } else if (expr->kind == AST::Node::Kind_TrueLiteral) {
            binding->value.type = QV4::CompiledData::Value::Type_Boolean;
            binding->value.b = true;
        } else if (expr->kind == AST::Node::Kind_FalseLiteral) {
            binding->value.type = QV4::CompiledData::Value::Type_Boolean;
            binding->value.b = false;
        } else if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(expr)) {
            binding->value.type = QV4::CompiledData::Value::Type_Number;
            binding->value.d = lit->value;
        } else {

            if (AST::UnaryMinusExpression *unaryMinus = AST::cast<AST::UnaryMinusExpression *>(expr)) {
               if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(unaryMinus->expression)) {
                   binding->value.type = QV4::CompiledData::Value::Type_Number;
                   binding->value.d = -lit->value;
               }
            }
        }
    }

    // Do binding instead
    if (binding->value.type == QV4::CompiledData::Value::Type_Invalid) {
        binding->value.type = QV4::CompiledData::Value::Type_Script;
        _functions << statement;
        binding->value.compiledScriptIndex = _functions.size() - 1;
        binding->value.stringIndex = registerString(asStringRef(statement).toString());
    }
}

void QQmlCodeGenerator::appendBinding(int propertyNameIndex, AST::Statement *value)
{
    Binding *binding = New<Binding>();
    binding->propertyNameIndex = propertyNameIndex;
    setBindingValue(binding, value);
    _object->bindings->append(binding);
}

void QQmlCodeGenerator::appendBinding(int propertyNameIndex, int objectIndex)
{
    Binding *binding = New<Binding>();
    binding->propertyNameIndex = propertyNameIndex;
    binding->value.type = QV4::CompiledData::Value::Type_Object;
    binding->value.objectIndex = objectIndex;
    _object->bindings->append(binding);
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

QV4::CompiledData::QmlUnit *QmlUnitGenerator::generate(ParsedQML &output)
{
    jsUnitGenerator = &output.jsGenerator;
    const QmlObject *rootObject = output.objects.at(output.indexOfRootObject);
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
            *functionsTable++ = f->index;

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

    return qmlUnit;
}

int QmlUnitGenerator::getStringId(const QString &str) const
{
    return jsUnitGenerator->getStringId(str);
}

void JSCodeGen::generateJSCodeForFunctionsAndBindings(const QString &fileName, ParsedQML *output)
{
    _module = &output->jsModule;
    _module->setFileName(fileName);

    foreach (AST::Node *node, output->functions) {
        _env = 0;

        AST::FunctionDeclaration *function = AST::cast<AST::FunctionDeclaration*>(node);

        ScanFunctions scan(this, output->code);
        scan.enterEnvironment(node);
        scan(function ? function->body : node);
        scan.leaveEnvironment();

        QString name;
        if (function)
            name = function->name.toString();
        else
            name = QStringLiteral("%qml-expression-entry");

        defineFunction(name, node,
                       function ? function->formals : 0,
                       function ? function->body->elements : node, QmlBinding);

    }

    qDeleteAll(_envMap);
    _envMap.clear();
}
