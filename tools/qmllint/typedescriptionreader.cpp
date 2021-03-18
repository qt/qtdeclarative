/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "typedescriptionreader.h"

#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsengine_p.h>

#include <QtCore/qdir.h>

using namespace QQmlJS;
using namespace QQmlJS::AST;

QString toString(const UiQualifiedId *qualifiedId, QChar delimiter = QLatin1Char('.'))
{
    QString result;

    for (const UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter != qualifiedId)
            result += delimiter;

        result += iter->name;
    }

    return result;
}

bool TypeDescriptionReader::operator()(
        QHash<QString, ScopeTree::ConstPtr> *objects,
        QList<ModuleApiInfo> *moduleApis,
        QStringList *dependencies)
{
    Engine engine;

    Lexer lexer(&engine);
    Parser parser(&engine);

    lexer.setCode(m_source, /*lineno = */ 1, /*qmlMode = */true);

    if (!parser.parse()) {
        m_errorMessage = QString::fromLatin1("%1:%2: %3").arg(
                    QString::number(parser.errorLineNumber()),
                    QString::number(parser.errorColumnNumber()),
                    parser.errorMessage());
        return false;
    }

    m_objects = objects;
    m_moduleApis = moduleApis;
    m_dependencies = dependencies;
    readDocument(parser.ast());

    return m_errorMessage.isEmpty();
}

void TypeDescriptionReader::readDocument(UiProgram *ast)
{
    if (!ast) {
        addError(SourceLocation(), tr("Could not parse document."));
        return;
    }

    if (!ast->headers || ast->headers->next || !cast<UiImport *>(ast->headers->headerItem)) {
        addError(SourceLocation(), tr("Expected a single import."));
        return;
    }

    auto *import = cast<UiImport *>(ast->headers->headerItem);
    if (toString(import->importUri) != QLatin1String("QtQuick.tooling")) {
        addError(import->importToken, tr("Expected import of QtQuick.tooling."));
        return;
    }

    if (!import->version) {
        addError(import->firstSourceLocation(), tr("Import statement without version."));
        return;
    }

    if (import->version->majorVersion != 1) {
        addError(import->version->firstSourceLocation(),
                 tr("Major version different from 1 not supported."));
        return;
    }

    if (!ast->members || !ast->members->member || ast->members->next) {
        addError(SourceLocation(), tr("Expected document to contain a single object definition."));
        return;
    }

    auto *module = cast<UiObjectDefinition *>(ast->members->member);
    if (!module) {
        addError(SourceLocation(), tr("Expected document to contain a single object definition."));
        return;
    }

    if (toString(module->qualifiedTypeNameId) != QLatin1String("Module")) {
        addError(SourceLocation(), tr("Expected document to contain a Module {} member."));
        return;
    }

    readModule(module);
}

void TypeDescriptionReader::readModule(UiObjectDefinition *ast)
{
    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        auto *component = cast<UiObjectDefinition *>(member);

        auto *script = cast<UiScriptBinding *>(member);
        if (script && (toString(script->qualifiedId) == QStringLiteral("dependencies"))) {
            readDependencies(script);
            continue;
        }

        QString typeName;
        if (component)
            typeName = toString(component->qualifiedTypeNameId);

        if (!component || (typeName != QLatin1String("Component")
                           && typeName != QLatin1String("ModuleApi"))) {
            continue;
        }

        if (typeName == QLatin1String("Component"))
            readComponent(component);
        else if (typeName == QLatin1String("ModuleApi"))
            readModuleApi(component);
    }
}

void TypeDescriptionReader::addError(const SourceLocation &loc, const QString &message)
{
    m_errorMessage += QString::fromLatin1("%1:%2:%3: %4\n").arg(
                QDir::toNativeSeparators(m_fileName),
                QString::number(loc.startLine),
                QString::number(loc.startColumn),
                message);
}

void TypeDescriptionReader::addWarning(const SourceLocation &loc, const QString &message)
{
    m_warningMessage += QString::fromLatin1("%1:%2:%3: %4\n").arg(
                QDir::toNativeSeparators(m_fileName),
                QString::number(loc.startLine),
                QString::number(loc.startColumn),
                message);
}

void TypeDescriptionReader::readDependencies(UiScriptBinding *ast)
{
    auto *stmt = cast<ExpressionStatement*>(ast->statement);
    if (!stmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected dependency definitions"));
        return;
    }
    auto *exp = cast<ArrayPattern *>(stmt->expression);
    if (!exp) {
        addError(stmt->expression->firstSourceLocation(), tr("Expected dependency definitions"));
        return;
    }
    for (PatternElementList *l = exp->elements; l; l = l->next) {
        auto *str = cast<StringLiteral *>(l->element->initializer);
        *m_dependencies << str->value.toString();
    }
}

void TypeDescriptionReader::readComponent(UiObjectDefinition *ast)
{
    ScopeTree::Ptr scope(new ScopeTree(ScopeType::QMLScope));

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        auto *component = cast<UiObjectDefinition *>(member);
        auto *script = cast<UiScriptBinding *>(member);
        if (component) {
            QString name = toString(component->qualifiedTypeNameId);
            if (name == QLatin1String("Property"))
                readProperty(component, scope);
            else if (name == QLatin1String("Method") || name == QLatin1String("Signal"))
                readSignalOrMethod(component, name == QLatin1String("Method"), scope);
            else if (name == QLatin1String("Enum"))
                readEnum(component, scope);
            else
                addWarning(component->firstSourceLocation(),
                           tr("Expected only Property, Method, Signal and Enum object definitions, "
                              "not \"%1\".").arg(name));
        } else if (script) {
            QString name = toString(script->qualifiedId);
            if (name == QLatin1String("name")) {
                scope->setClassName(readStringBinding(script));
            } else if (name == QLatin1String("prototype")) {
                scope->setSuperclassName(readStringBinding(script));
            } else if (name == QLatin1String("defaultProperty")) {
                scope->setDefaultPropertyName(readStringBinding(script));
            } else if (name == QLatin1String("exports")) {
                readExports(script, scope);
            } else if (name == QLatin1String("exportMetaObjectRevisions")) {
                readMetaObjectRevisions(script, scope);
            } else if (name == QLatin1String("attachedType")) {
                scope->setAttachedTypeName(readStringBinding(script));
            } else if (name == QLatin1String("isSingleton")) {
                scope->setIsSingleton(readBoolBinding(script));
            } else if (name == QLatin1String("isCreatable")) {
                scope->setIsCreatable(readBoolBinding(script));
            } else if (name == QLatin1String("isComposite")) {
                scope->setIsComposite(readBoolBinding(script));
            } else {
                addWarning(script->firstSourceLocation(),
                           tr("Expected only name, prototype, defaultProperty, attachedType, "
                              "exports, isSingleton, isCreatable, isComposite and "
                              "exportMetaObjectRevisions script bindings, not \"%1\".").arg(name));
            }
        } else {
            addWarning(member->firstSourceLocation(),
                       tr("Expected only script bindings and object definitions."));
        }
    }

    if (scope->className().isEmpty()) {
        addError(ast->firstSourceLocation(), tr("Component definition is missing a name binding."));
        return;
    }

    // ### add implicit export into the package of c++ types
    scope->addExport(scope->className(), QStringLiteral("<cpp>"), ComponentVersion());
    m_objects->insert(scope->className(), scope);
}

void TypeDescriptionReader::readModuleApi(UiObjectDefinition *ast)
{
    ModuleApiInfo apiInfo;

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        auto *script = cast<UiScriptBinding *>(member);

        if (script) {
            const QString name = toString(script->qualifiedId);
            if (name == QLatin1String("uri")) {
                apiInfo.uri = readStringBinding(script);
            } else if (name == QLatin1String("version")) {
                apiInfo.version = readNumericVersionBinding(script);
            } else if (name == QLatin1String("name")) {
                apiInfo.cppName = readStringBinding(script);
            } else {
                addWarning(script->firstSourceLocation(),
                           tr("Expected only uri, version and name script bindings."));
            }
        } else {
            addWarning(member->firstSourceLocation(), tr("Expected only script bindings."));
        }
    }

    if (!apiInfo.version.isValid()) {
        addError(ast->firstSourceLocation(),
                 tr("ModuleApi definition has no or invalid version binding."));
        return;
    }

    if (m_moduleApis)
        m_moduleApis->append(apiInfo);
}

void TypeDescriptionReader::readSignalOrMethod(UiObjectDefinition *ast, bool isMethod,
                                               const ScopeTree::Ptr &scope)
{
    MetaMethod metaMethod;
    // ### confusion between Method and Slot. Method should be removed.
    if (isMethod)
        metaMethod.setMethodType(MetaMethod::Slot);
    else
        metaMethod.setMethodType(MetaMethod::Signal);

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        auto *component = cast<UiObjectDefinition *>(member);
        auto *script = cast<UiScriptBinding *>(member);
        if (component) {
            QString name = toString(component->qualifiedTypeNameId);
            if (name == QLatin1String("Parameter")) {
                readParameter(component, &metaMethod);
            } else {
                addWarning(component->firstSourceLocation(),
                           tr("Expected only Parameter object definitions."));
            }
        } else if (script) {
            QString name = toString(script->qualifiedId);
            if (name == QLatin1String("name")) {
                metaMethod.setMethodName(readStringBinding(script));
            } else if (name == QLatin1String("type")) {
                metaMethod.setReturnType(readStringBinding(script));
            } else if (name == QLatin1String("revision")) {
                metaMethod.setRevision(readIntBinding(script));
            } else {
                addWarning(script->firstSourceLocation(),
                           tr("Expected only name and type script bindings."));
            }
        } else {
            addWarning(member->firstSourceLocation(),
                       tr("Expected only script bindings and object definitions."));
        }
    }

    if (metaMethod.methodName().isEmpty()) {
        addError(ast->firstSourceLocation(),
                 tr("Method or signal is missing a name script binding."));
        return;
    }

    scope->addMethod(metaMethod);
}

void TypeDescriptionReader::readProperty(UiObjectDefinition *ast, const ScopeTree::Ptr &scope)
{
    QString name;
    QString type;
    bool isPointer = false;
    bool isReadonly = false;
    bool isList = false;
    int revision = 0;

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        auto *script = cast<UiScriptBinding *>(member);
        if (!script) {
            addWarning(member->firstSourceLocation(), tr("Expected script binding."));
            continue;
        }

        QString id = toString(script->qualifiedId);
        if (id == QLatin1String("name")) {
            name = readStringBinding(script);
        } else if (id == QLatin1String("type")) {
            type = readStringBinding(script);
        } else if (id == QLatin1String("isPointer")) {
            isPointer = readBoolBinding(script);
        } else if (id == QLatin1String("isReadonly")) {
            isReadonly = readBoolBinding(script);
        } else if (id == QLatin1String("isList")) {
            isList = readBoolBinding(script);
        } else if (id == QLatin1String("revision")) {
            revision = readIntBinding(script);
        } else {
            addWarning(script->firstSourceLocation(),
                       tr("Expected only type, name, revision, isPointer, isReadonly and"
                          " isList script bindings."));
        }
    }

    if (name.isEmpty() || type.isEmpty()) {
        addError(ast->firstSourceLocation(),
                 tr("Property object is missing a name or type script binding."));
        return;
    }

    scope->addProperty(MetaProperty(name, type, isList, !isReadonly, isPointer, false, revision));
}

void TypeDescriptionReader::readEnum(UiObjectDefinition *ast, const ScopeTree::Ptr &scope)
{
    MetaEnum metaEnum;

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        auto *script = cast<UiScriptBinding *>(member);
        if (!script) {
            addWarning(member->firstSourceLocation(), tr("Expected script binding."));
            continue;
        }

        QString name = toString(script->qualifiedId);
        if (name == QLatin1String("name")) {
            metaEnum.setName(readStringBinding(script));
        } else if (name == QLatin1String("alias")) {
            metaEnum.setAlias(readStringBinding(script));
        } else if (name == QLatin1String("isFlag")) {
            metaEnum.setIsFlag(readBoolBinding(script));
        } else if (name == QLatin1String("values")) {
            readEnumValues(script, &metaEnum);
        } else {
            addWarning(script->firstSourceLocation(),
                       tr("Expected only name and values script bindings."));
        }
    }

    scope->addEnum(metaEnum);
}

void TypeDescriptionReader::readParameter(UiObjectDefinition *ast, MetaMethod *metaMethod)
{
    QString name;
    QString type;

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        auto *script = cast<UiScriptBinding *>(member);
        if (!script) {
            addWarning(member->firstSourceLocation(), tr("Expected script binding."));
            continue;
        }

        const QString id = toString(script->qualifiedId);
        if (id == QLatin1String("name")) {
            name = readStringBinding(script);
        } else if (id == QLatin1String("type")) {
            type = readStringBinding(script);
        } else if (id == QLatin1String("isPointer")) {
            // ### unhandled
        } else if (id == QLatin1String("isReadonly")) {
            // ### unhandled
        } else if (id == QLatin1String("isList")) {
            // ### unhandled
        } else {
            addWarning(script->firstSourceLocation(),
                       tr("Expected only name and type script bindings."));
        }
    }

    metaMethod->addParameter(name, type);
}

QString TypeDescriptionReader::readStringBinding(UiScriptBinding *ast)
{
    Q_ASSERT(ast);

    if (!ast->statement) {
        addError(ast->colonToken, tr("Expected string after colon."));
        return QString();
    }

    auto *expStmt = cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected string after colon."));
        return QString();
    }

    auto *stringLit = cast<StringLiteral *>(expStmt->expression);
    if (!stringLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected string after colon."));
        return QString();
    }

    return stringLit->value.toString();
}

bool TypeDescriptionReader::readBoolBinding(UiScriptBinding *ast)
{
    Q_ASSERT(ast);

    if (!ast->statement) {
        addError(ast->colonToken, tr("Expected boolean after colon."));
        return false;
    }

    auto *expStmt = cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected boolean after colon."));
        return false;
    }

    auto *trueLit = cast<TrueLiteral *>(expStmt->expression);
    auto *falseLit = cast<FalseLiteral *>(expStmt->expression);
    if (!trueLit && !falseLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected true or false after colon."));
        return false;
    }

    return trueLit;
}

double TypeDescriptionReader::readNumericBinding(UiScriptBinding *ast)
{
    Q_ASSERT(ast);

    if (!ast->statement) {
        addError(ast->colonToken, tr("Expected numeric literal after colon."));
        return 0;
    }

    auto *expStmt = cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(),
                 tr("Expected numeric literal after colon."));
        return 0;
    }

    auto *numericLit = cast<NumericLiteral *>(expStmt->expression);
    if (!numericLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected numeric literal after colon."));
        return 0;
    }

    return numericLit->value;
}

ComponentVersion TypeDescriptionReader::readNumericVersionBinding(UiScriptBinding *ast)
{
    ComponentVersion invalidVersion;

    if (!ast || !ast->statement) {
        addError((ast ? ast->colonToken : SourceLocation()),
                 tr("Expected numeric literal after colon."));
        return invalidVersion;
    }

    auto *expStmt = cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(),
                 tr("Expected numeric literal after colon."));
        return invalidVersion;
    }

    auto *numericLit = cast<NumericLiteral *>(expStmt->expression);
    if (!numericLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected numeric literal after colon."));
        return invalidVersion;
    }

    return ComponentVersion(m_source.mid(numericLit->literalToken.begin(),
                                         numericLit->literalToken.length));
}

int TypeDescriptionReader::readIntBinding(UiScriptBinding *ast)
{
    double v = readNumericBinding(ast);
    int i = static_cast<int>(v);

    if (i != v) {
        addError(ast->firstSourceLocation(), tr("Expected integer after colon."));
        return 0;
    }

    return i;
}

void TypeDescriptionReader::readExports(UiScriptBinding *ast, const ScopeTree::Ptr &scope)
{
    Q_ASSERT(ast);

    if (!ast->statement) {
        addError(ast->colonToken, tr("Expected array of strings after colon."));
        return;
    }

    auto *expStmt = cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(),
                 tr("Expected array of strings after colon."));
        return;
    }

    auto *arrayLit = cast<ArrayPattern *>(expStmt->expression);
    if (!arrayLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected array of strings after colon."));
        return;
    }

    for (PatternElementList *it = arrayLit->elements; it; it = it->next) {
        auto *stringLit = cast<StringLiteral *>(it->element->initializer);
        if (!stringLit) {
            addError(arrayLit->firstSourceLocation(),
                     tr("Expected array literal with only string literal members."));
            return;
        }
        QString exp = stringLit->value.toString();
        int slashIdx = exp.indexOf(QLatin1Char('/'));
        int spaceIdx = exp.indexOf(QLatin1Char(' '));
        ComponentVersion version(exp.mid(spaceIdx + 1));

        if (spaceIdx == -1 || !version.isValid()) {
            addError(stringLit->firstSourceLocation(),
                     tr("Expected string literal to contain 'Package/Name major.minor' "
                        "or 'Name major.minor'."));
            continue;
        }
        QString package;
        if (slashIdx != -1)
            package = exp.left(slashIdx);
        QString name = exp.mid(slashIdx + 1, spaceIdx - (slashIdx+1));

        // ### relocatable exports where package is empty?
        scope->addExport(name, package, version);
    }
}

void TypeDescriptionReader::readMetaObjectRevisions(UiScriptBinding *ast,
                                                    const ScopeTree::Ptr &scope)
{
    Q_ASSERT(ast);

    if (!ast->statement) {
        addError(ast->colonToken, tr("Expected array of numbers after colon."));
        return;
    }

    auto *expStmt = cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(),
                 tr("Expected array of numbers after colon."));
        return;
    }

    auto *arrayLit = cast<ArrayPattern *>(expStmt->expression);
    if (!arrayLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected array of numbers after colon."));
        return;
    }

    int exportIndex = 0;
    const int exportCount = scope->exports().size();
    for (PatternElementList *it = arrayLit->elements; it; it = it->next, ++exportIndex) {
        auto *numberLit = cast<NumericLiteral *>(it->element->initializer);
        if (!numberLit) {
            addError(arrayLit->firstSourceLocation(),
                     tr("Expected array literal with only number literal members."));
            return;
        }

        if (exportIndex >= exportCount) {
            addError(numberLit->firstSourceLocation(),
                     tr("Meta object revision without matching export."));
            return;
        }

        const double v = numberLit->value;
        const int metaObjectRevision = static_cast<int>(v);
        if (metaObjectRevision != v) {
            addError(numberLit->firstSourceLocation(), tr("Expected integer."));
            return;
        }

        scope->setExportMetaObjectRevision(exportIndex, metaObjectRevision);
    }
}

void TypeDescriptionReader::readEnumValues(UiScriptBinding *ast, MetaEnum *metaEnum)
{
    if (!ast)
        return;
    if (!ast->statement) {
        addError(ast->colonToken, tr("Expected object literal after colon."));
        return;
    }

    auto *expStmt = cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected expression after colon."));
        return;
    }

    if (auto *objectLit = cast<ObjectPattern *>(expStmt->expression)) {
        for (PatternPropertyList *it = objectLit->properties; it; it = it->next) {
            if (PatternProperty *assignement = it->property) {
                if (auto *name = cast<StringLiteralPropertyName *>(assignement->name)) {
                    metaEnum->addKey(name->id.toString());
                    continue;
                }
            }
            addError(it->firstSourceLocation(), tr("Expected strings as enum keys."));
        }
    } else if (auto *arrayLit = cast<ArrayPattern *>(expStmt->expression)) {
        for (PatternElementList *it = arrayLit->elements; it; it = it->next) {
            if (PatternElement *element = it->element) {
                if (auto *name = cast<StringLiteral *>(element->initializer)) {
                    metaEnum->addKey(name->value.toString());
                    continue;
                }
            }
            addError(it->firstSourceLocation(), tr("Expected strings as enum keys."));
        }
    } else {
        addError(ast->statement->firstSourceLocation(),
                 tr("Expected either array or object literal as enum definition."));
    }
}
