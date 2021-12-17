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

#include "qqmljstypedescriptionreader_p.h"

#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsengine_p.h>

#include <QtCore/qdir.h>

QT_BEGIN_NAMESPACE

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

bool QQmlJSTypeDescriptionReader::operator()(
        QHash<QString, QQmlJSScope::Ptr> *objects,
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
    m_dependencies = dependencies;
    readDocument(parser.ast());

    return m_errorMessage.isEmpty();
}

void QQmlJSTypeDescriptionReader::readDocument(UiProgram *ast)
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

    if (import->version->version.majorVersion() != 1) {
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

void QQmlJSTypeDescriptionReader::readModule(UiObjectDefinition *ast)
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

        if (!component || typeName != QLatin1String("Component")) {
            continue;
        }

        if (typeName == QLatin1String("Component"))
            readComponent(component);
    }
}

void QQmlJSTypeDescriptionReader::addError(const SourceLocation &loc, const QString &message)
{
    m_errorMessage += QString::fromLatin1("%1:%2:%3: %4\n").arg(
                QDir::toNativeSeparators(m_fileName),
                QString::number(loc.startLine),
                QString::number(loc.startColumn),
                message);
}

void QQmlJSTypeDescriptionReader::addWarning(const SourceLocation &loc, const QString &message)
{
    m_warningMessage += QString::fromLatin1("%1:%2:%3: %4\n").arg(
                QDir::toNativeSeparators(m_fileName),
                QString::number(loc.startLine),
                QString::number(loc.startColumn),
                message);
}

void QQmlJSTypeDescriptionReader::readDependencies(UiScriptBinding *ast)
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

void QQmlJSTypeDescriptionReader::readComponent(UiObjectDefinition *ast)
{
    QQmlJSScope::Ptr scope = QQmlJSScope::create();

    UiScriptBinding *metaObjectRevisions = nullptr;
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
            if (name == QLatin1String("file")) {
                scope->setFileName(readStringBinding(script));
            } else if (name == QLatin1String("name")) {
                scope->setInternalName(readStringBinding(script));
            } else if (name == QLatin1String("prototype")) {
                scope->setBaseTypeName(readStringBinding(script));
            } else if (name == QLatin1String("defaultProperty")) {
                scope->setDefaultPropertyName(readStringBinding(script));
            } else if (name == QLatin1String("parentProperty")) {
                scope->setParentPropertyName(readStringBinding(script));
            } else if (name == QLatin1String("exports")) {
                readExports(script, scope);
            } else if (name == QLatin1String("interfaces")) {
                readInterfaces(script, scope);
            } else if (name == QLatin1String("exportMetaObjectRevisions")) {
                metaObjectRevisions = script;
            } else if (name == QLatin1String("attachedType")) {
                scope->setOwnAttachedTypeName(readStringBinding(script));
            } else if (name == QLatin1String("valueType")) {
                scope->setValueTypeName(readStringBinding(script));
            } else if (name == QLatin1String("isSingleton")) {
                scope->setIsSingleton(readBoolBinding(script));
            } else if (name == QLatin1String("isCreatable")) {
                scope->setIsCreatable(readBoolBinding(script));
            } else if (name == QLatin1String("isComposite")) {
                scope->setIsComposite(readBoolBinding(script));
            } else if (name == QLatin1String("hasCustomParser")) {
                scope->setHasCustomParser(readBoolBinding(script));
            } else if (name == QLatin1String("accessSemantics")) {
                const QString semantics = readStringBinding(script);
                if (semantics == QLatin1String("reference")) {
                    scope->setAccessSemantics(QQmlJSScope::AccessSemantics::Reference);
                } else if (semantics == QLatin1String("value")) {
                    scope->setAccessSemantics(QQmlJSScope::AccessSemantics::Value);
                } else if (semantics == QLatin1String("none")) {
                    scope->setAccessSemantics(QQmlJSScope::AccessSemantics::None);
                } else if (semantics == QLatin1String("sequence")) {
                    scope->setAccessSemantics(QQmlJSScope::AccessSemantics::Sequence);
                } else {
                    addWarning(script->firstSourceLocation(),
                               tr("Unknown access semantics \"%1\".").arg(semantics));
                }
            } else if (name == QLatin1String("extension")) {
                scope->setExtensionTypeName(readStringBinding(script));
            } else {
                addWarning(script->firstSourceLocation(),
                           tr("Expected only name, prototype, defaultProperty, attachedType, "
                              "valueType, exports, interfaces, isSingleton, isCreatable, "
                              "isComposite, hasCustomParser and "
                              "exportMetaObjectRevisions script bindings, not \"%1\".")
                                   .arg(name));
            }
        } else {
            addWarning(member->firstSourceLocation(),
                       tr("Expected only script bindings and object definitions."));
        }
    }

    if (scope->internalName().isEmpty()) {
        addError(ast->firstSourceLocation(), tr("Component definition is missing a name binding."));
        return;
    }

    if (metaObjectRevisions)
        readMetaObjectRevisions(metaObjectRevisions, scope);
    m_objects->insert(scope->internalName(), scope);
}

void QQmlJSTypeDescriptionReader::readSignalOrMethod(UiObjectDefinition *ast, bool isMethod,
                                               const QQmlJSScope::Ptr &scope)
{
    QQmlJSMetaMethod metaMethod;
    // ### confusion between Method and Slot. Method should be removed.
    if (isMethod)
        metaMethod.setMethodType(QQmlJSMetaMethod::Slot);
    else
        metaMethod.setMethodType(QQmlJSMetaMethod::Signal);

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
                           tr("Expected only Parameter in object definitions."));
            }
        } else if (script) {
            QString name = toString(script->qualifiedId);
            if (name == QLatin1String("name")) {
                metaMethod.setMethodName(readStringBinding(script));
            } else if (name == QLatin1String("type")) {
                metaMethod.setReturnTypeName(readStringBinding(script));
            } else if (name == QLatin1String("revision")) {
                metaMethod.setRevision(readIntBinding(script));
            } else if (name == QLatin1String("isConstructor")) {
                metaMethod.setIsConstructor(true);
            } else if (name == QLatin1String("isList")) {
                // TODO: Theoretically this can happen. QQmlJSMetaMethod should store it.
            } else if (name == QLatin1String("isPointer")) {
                // TODO: We don't need this information. We can probably drop all isPointer members
                //       once we make sure that the type information is always complete. The
                //       description of the type being referenced has access semantics after all.
            } else {
                addWarning(script->firstSourceLocation(),
                           tr("Expected only name, type, revision, isPointer, isList, and "
                              "isConstructor in script bindings."));
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

    scope->addOwnMethod(metaMethod);
}

void QQmlJSTypeDescriptionReader::readProperty(UiObjectDefinition *ast, const QQmlJSScope::Ptr &scope)
{
    QQmlJSMetaProperty property;
    property.setIsWritable(true); // default is writable
    bool isRequired = false;

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        auto *script = cast<UiScriptBinding *>(member);
        if (!script) {
            addWarning(member->firstSourceLocation(), tr("Expected script binding."));
            continue;
        }

        QString id = toString(script->qualifiedId);
        if (id == QLatin1String("name")) {
            property.setPropertyName(readStringBinding(script));
        } else if (id == QLatin1String("type")) {
            property.setTypeName(readStringBinding(script));
        } else if (id == QLatin1String("isPointer")) {
            property.setIsPointer(readBoolBinding(script));
        } else if (id == QLatin1String("isReadonly")) {
            property.setIsWritable(!readBoolBinding(script));
        } else if (id == QLatin1String("isRequired")) {
            isRequired = readBoolBinding(script);
        } else if (id == QLatin1String("isList")) {
            property.setIsList(readBoolBinding(script));
        } else if (id == QLatin1String("isFinal")) {
            property.setIsFinal(readBoolBinding(script));
        } else if (id == QLatin1String("revision")) {
            property.setRevision(readIntBinding(script));
        } else if (id == QLatin1String("bindable")) {
            property.setBindable(readStringBinding(script));
        } else if (id == QLatin1String("read")) {
            property.setRead(readStringBinding(script));
        } else if (id == QLatin1String("write")) {
            property.setWrite(readStringBinding(script));
        } else if (id == QLatin1String("notify")) {
            property.setNotify(readStringBinding(script));
        } else if (id == QLatin1String("index")) {
            property.setIndex(readIntBinding(script));
        } else {
            addWarning(script->firstSourceLocation(),
                       tr("Expected only type, name, revision, isPointer, isReadonly, isRequired, "
                          "isFinal, bindable, read, write, notify, index and isList script "
                          "bindings."));
        }
    }

    if (property.propertyName().isEmpty()) {
        addError(ast->firstSourceLocation(),
                 tr("Property object is missing a name script binding."));
        return;
    }

    scope->addOwnProperty(property);
    if (isRequired)
        scope->setPropertyLocallyRequired(property.propertyName(), true);
}

void QQmlJSTypeDescriptionReader::readEnum(UiObjectDefinition *ast, const QQmlJSScope::Ptr &scope)
{
    QQmlJSMetaEnum metaEnum;

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

    scope->addOwnEnumeration(metaEnum);
}

void QQmlJSTypeDescriptionReader::readParameter(UiObjectDefinition *ast, QQmlJSMetaMethod *metaMethod)
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

QString QQmlJSTypeDescriptionReader::readStringBinding(UiScriptBinding *ast)
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

bool QQmlJSTypeDescriptionReader::readBoolBinding(UiScriptBinding *ast)
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

double QQmlJSTypeDescriptionReader::readNumericBinding(UiScriptBinding *ast)
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

static QTypeRevision parseVersion(const QString &versionString)
{
    const int dotIdx = versionString.indexOf(QLatin1Char('.'));
    if (dotIdx == -1)
        return QTypeRevision();
    bool ok = false;
    const int maybeMajor = QStringView{versionString}.left(dotIdx).toInt(&ok);
    if (!ok)
        return QTypeRevision();
    const int maybeMinor = QStringView{versionString}.mid(dotIdx + 1).toInt(&ok);
    if (!ok)
        return QTypeRevision();
    return QTypeRevision::fromVersion(maybeMajor, maybeMinor);
}

QTypeRevision QQmlJSTypeDescriptionReader::readNumericVersionBinding(UiScriptBinding *ast)
{
    QTypeRevision invalidVersion;

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

    return parseVersion(m_source.mid(numericLit->literalToken.begin(),
                                     numericLit->literalToken.length));
}

int QQmlJSTypeDescriptionReader::readIntBinding(UiScriptBinding *ast)
{
    double v = readNumericBinding(ast);
    int i = static_cast<int>(v);

    if (i != v) {
        addError(ast->firstSourceLocation(), tr("Expected integer after colon."));
        return 0;
    }

    return i;
}

ArrayPattern* QQmlJSTypeDescriptionReader::getArray(UiScriptBinding *ast)
{
    Q_ASSERT(ast);

    if (!ast->statement) {
        addError(ast->colonToken, tr("Expected array of strings after colon."));
        return nullptr;
    }

    auto *expStmt = cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(),
                 tr("Expected array of strings after colon."));
        return nullptr;
    }

    auto *arrayLit = cast<ArrayPattern *>(expStmt->expression);
    if (!arrayLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected array of strings after colon."));
        return nullptr;
    }

    return arrayLit;
}

void QQmlJSTypeDescriptionReader::readExports(UiScriptBinding *ast, const QQmlJSScope::Ptr &scope)
{
    auto *arrayLit = getArray(ast);

    if (!arrayLit)
        return;

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
        const QTypeRevision version = parseVersion(exp.mid(spaceIdx + 1));

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
        scope->addExport(name, package, version, version);
    }
}

void QQmlJSTypeDescriptionReader::readInterfaces(UiScriptBinding *ast, const QQmlJSScope::Ptr &scope)
{
    auto *arrayLit = getArray(ast);

    if (!arrayLit)
        return;

    QStringList list;

    for (PatternElementList *it = arrayLit->elements; it; it = it->next) {
        auto *stringLit = cast<StringLiteral *>(it->element->initializer);
        if (!stringLit) {
            addError(arrayLit->firstSourceLocation(),
                     tr("Expected array literal with only string literal members."));
            return;
        }

        list << stringLit->value.toString();
    }

    scope->setInterfaceNames(list);
}

void QQmlJSTypeDescriptionReader::readMetaObjectRevisions(
        UiScriptBinding *ast, const QQmlJSScope::Ptr &scope)
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
    QList<QQmlJSScope::Export> exports = scope->exports();
    const int exportCount = exports.size();
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

        const QTypeRevision metaObjectVersion
                = QTypeRevision::fromEncodedVersion(metaObjectRevision);
        const QQmlJSScope::Export &entry = exports.at(exportIndex);
        const QTypeRevision exportVersion = entry.version();
        if (metaObjectVersion != exportVersion) {
            addWarning(numberLit->firstSourceLocation(),
                       tr("Meta object revision and export version differ.\n"
                          "Revision %1 corresponds to version %2.%3; it should be %4.%5.")
                       .arg(metaObjectRevision)
                       .arg(metaObjectVersion.majorVersion()).arg(metaObjectVersion.minorVersion())
                       .arg(exportVersion.majorVersion()).arg(exportVersion.minorVersion()));
            exports[exportIndex] = QQmlJSScope::Export(
                        entry.package(), entry.type(), exportVersion, metaObjectVersion);
        }
    }
    scope->setExports(exports);
}

void QQmlJSTypeDescriptionReader::readEnumValues(UiScriptBinding *ast, QQmlJSMetaEnum *metaEnum)
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
        int currentValue = -1;
        for (PatternPropertyList *it = objectLit->properties; it; it = it->next) {
            if (PatternProperty *assignement = it->property) {
                if (auto *name = cast<StringLiteralPropertyName *>(assignement->name)) {
                    metaEnum->addKey(name->id.toString());

                    if (auto *value = AST::cast<NumericLiteral *>(assignement->initializer)) {
                        currentValue = int(value->value);
                    } else if (auto *minus = AST::cast<UnaryMinusExpression *>(
                                   assignement->initializer)) {
                        if (auto *value = AST::cast<NumericLiteral *>(minus->expression))
                            currentValue = -int(value->value);
                        else
                            ++currentValue;
                    } else {
                        ++currentValue;
                    }

                    metaEnum->addValue(currentValue);
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

QT_END_NAMESPACE
