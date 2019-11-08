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

#include "findunqualified.h"
#include "scopetree.h"
#include "typedescriptionreader.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qv4codegen_p.h>
#include <QtQml/private/qqmldirparser_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qscopedvaluerollback.h>

static const QString prefixedName(const QString &prefix, const QString &name)
{
    return prefix.isEmpty() ? name : (prefix  + QLatin1Char('.') + name);
}

static QQmlDirParser createQmldirParserForFile(const QString &filename)
{
    QFile f(filename);
    f.open(QFile::ReadOnly);
    QQmlDirParser parser;
    parser.parse(f.readAll());
    return parser;
}

static TypeDescriptionReader createQmltypesReaderForFile(const QString &filename)
{
    QFile f(filename);
    f.open(QFile::ReadOnly);
    TypeDescriptionReader reader { filename, f.readAll() };
    return reader;
}

void FindUnqualifiedIDVisitor::enterEnvironment(ScopeType type, QString name)
{
    m_currentScope = m_currentScope->createNewChildScope(type, std::move(name));
}

void FindUnqualifiedIDVisitor::leaveEnvironment()
{
    m_currentScope = m_currentScope->parentScope();
}

void FindUnqualifiedIDVisitor::parseHeaders(QQmlJS::AST::UiHeaderItemList *header)
{
    using namespace QQmlJS::AST;

    while (header) {
        if (auto import = cast<UiImport *>(header->headerItem)) {
            if (import->version) {
                QString path;
                auto uri = import->importUri;
                while (uri) {
                    path.append(uri->name);
                    path.append("/");
                    uri = uri->next;
                }
                path.chop(1);
                QString prefix = QLatin1String("");
                if (import->asToken.isValid()) {
                    prefix += import->importId + QLatin1Char('.');
                }
                importHelper(path, prefix, import->version->majorVersion,
                             import->version->minorVersion);
            }
        }
        header = header->next;
    }
}

void FindUnqualifiedIDVisitor::parseMembers(QQmlJS::AST::UiObjectMemberList *member,
                                            ScopeTree *scope)
{
    using namespace QQmlJS::AST;

    // member should be the sole element
    Q_ASSERT(!member->next);
    Q_ASSERT(member && member->member->kind == UiObjectMember::Kind_UiObjectDefinition);
    auto definition = cast<UiObjectDefinition *>(member->member);
    auto qualifiedId = definition->qualifiedTypeNameId;
    while (qualifiedId && qualifiedId->next) {
        qualifiedId = qualifiedId->next;
    }
    scope->setSuperclassName(qualifiedId->name.toString());
    UiObjectMemberList *initMembers = definition->initializer->members;
    while (initMembers) {
        switch (initMembers->member->kind) {
        case UiObjectMember::Kind_UiArrayBinding: {
            // nothing to do
            break;
        }
        case UiObjectMember::Kind_UiEnumDeclaration: {
            // nothing to do
            break;
        }
        case UiObjectMember::Kind_UiObjectBinding: {
            // nothing to do
            break;
        }
        case UiObjectMember::Kind_UiObjectDefinition: {
            // creates nothing accessible
            break;
        }
        case UiObjectMember::Kind_UiPublicMember: {
            auto publicMember = cast<UiPublicMember *>(initMembers->member);
            switch (publicMember->type) {
            case UiPublicMember::Signal: {
                UiParameterList *param = publicMember->parameters;
                MetaMethod method;
                method.setMethodType(MetaMethod::Signal);
                method.setMethodName(publicMember->name.toString());
                while (param) {
                    method.addParameter(param->name.toString(), param->type->name.toString());
                    param = param->next;
                }
                scope->addMethod(method);
                break;
            }
            case UiPublicMember::Property: {
                MetaProperty prop {
                    publicMember->name.toString(),
                    publicMember->typeModifier.toString(),
                    false,
                    false,
                    false,
                    0
                };
                scope->addProperty(prop);
                break;
            }
            }
            break;
        }
        case UiObjectMember::Kind_UiScriptBinding: {
            // does not create anything new, ignore
            break;
        }
        case UiObjectMember::Kind_UiSourceElement: {
            auto sourceElement = cast<UiSourceElement *>(initMembers->member);
            if (FunctionExpression *fexpr = sourceElement->sourceElement->asFunctionDefinition()) {
                MetaMethod method;
                method.setMethodName(fexpr->name.toString());
                method.setMethodType(MetaMethod::Method);
                FormalParameterList *parameters = fexpr->formals;
                while (parameters) {
                    method.addParameter(parameters->element->bindingIdentifier.toString(), "");
                    parameters = parameters->next;
                }
                scope->addMethod(method);
            } else if (ClassExpression *clexpr =
                               sourceElement->sourceElement->asClassDefinition()) {
                const MetaProperty prop { clexpr->name.toString(), "", false, false, false, 1 };
                scope->addProperty(prop);
            } else if (cast<VariableStatement *>(sourceElement->sourceElement)) {
                // nothing to do
            } else {
                const auto loc = sourceElement->firstSourceLocation();
                m_colorOut.writeUncolored(
                            "unsupportedd sourceElement at "
                            + QString::fromLatin1("%1:%2: ").arg(loc.startLine).arg(loc.startColumn)
                            + QString::number(sourceElement->sourceElement->kind));
            }
            break;
        }
        default: {
            m_colorOut.writeUncolored("unsupported element of kind "
                                      + QString::number(initMembers->member->kind));
        }
        }
        initMembers = initMembers->next;
    }
}

void FindUnqualifiedIDVisitor::parseProgram(QQmlJS::AST::Program *program, ScopeTree *scope)
{
    using namespace QQmlJS::AST;
    for (auto *statement = program->statements; statement; statement = statement->next) {
        if (auto *function = cast<FunctionDeclaration *>(statement->statement)) {
            MetaMethod method(function->name.toString());
            method.setMethodType(MetaMethod::Method);
            for (auto *parameters = function->formals; parameters; parameters = parameters->next)
                method.addParameter(parameters->element->bindingIdentifier.toString(), "");
            scope->addMethod(method);
        }
    }
}

enum ImportVersion { FullyVersioned, PartiallyVersioned, Unversioned, BasePath };

QStringList completeImportPaths(const QString &uri, const QString &basePath, int vmaj, int vmin)
{
    static const QLatin1Char Slash('/');
    static const QLatin1Char Backslash('\\');

    const QVector<QStringRef> parts = uri.splitRef(QLatin1Char('.'), QString::SkipEmptyParts);

    QStringList qmlDirPathsPaths;
    // fully & partially versioned parts + 1 unversioned for each base path
    qmlDirPathsPaths.reserve(2 * parts.count() + 1);

    auto versionString = [](int vmaj, int vmin, ImportVersion version)
    {
        if (version == FullyVersioned) {
            // extension with fully encoded version number (eg. MyModule.3.2)
            return QString::fromLatin1(".%1.%2").arg(vmaj).arg(vmin);
        }
        if (version == PartiallyVersioned) {
            // extension with encoded version major (eg. MyModule.3)
            return QString::fromLatin1(".%1").arg(vmaj);
        }
        // else extension without version number (eg. MyModule)
        return QString();
    };
    auto joinStringRefs = [](const QVector<QStringRef> &refs, const QChar &sep) {
        QString str;
        for (auto it = refs.cbegin(); it != refs.cend(); ++it) {
            if (it != refs.cbegin())
                str += sep;
            str += *it;
        }
        return str;
    };

    const ImportVersion initial = (vmin >= 0)
            ? FullyVersioned
            : (vmaj >= 0 ? PartiallyVersioned : Unversioned);
    for (int version = initial; version <= BasePath; ++version) {
        const QString ver = versionString(vmaj, vmin, static_cast<ImportVersion>(version));

        QString dir = basePath;
        if (!dir.endsWith(Slash) && !dir.endsWith(Backslash))
            dir += Slash;

        if (version == BasePath) {
            qmlDirPathsPaths += dir;
        } else {
            // append to the end
            qmlDirPathsPaths += dir + joinStringRefs(parts, Slash) + ver;
        }

        if (version < Unversioned) {
            // insert in the middle
            for (int index = parts.count() - 2; index >= 0; --index) {
                qmlDirPathsPaths += dir + joinStringRefs(parts.mid(0, index + 1), Slash)
                        + ver + Slash
                        + joinStringRefs(parts.mid(index + 1), Slash);
            }
        }
    }
    return qmlDirPathsPaths;
}

static const QLatin1String SlashQmldir             = QLatin1String("/qmldir");
static const QLatin1String SlashAppDotQmltypes     = QLatin1String("/app.qmltypes");
static const QLatin1String SlashLibDotQmltypes     = QLatin1String("/lib.qmltypes");
static const QLatin1String SlashPluginsDotQmltypes = QLatin1String("/plugins.qmltypes");

void FindUnqualifiedIDVisitor::readQmltypes(const QString &filename,
                                            FindUnqualifiedIDVisitor::Import &result)
{
    auto reader = createQmltypesReaderForFile(filename);
    auto succ = reader(&result.objects, &result.moduleApis, &result.dependencies);
    if (!succ)
        m_colorOut.writeUncolored(reader.errorMessage());
}

FindUnqualifiedIDVisitor::Import FindUnqualifiedIDVisitor::readQmldir(const QString &path)
{
    Import result;
    auto reader = createQmldirParserForFile(path + SlashQmldir);
    const auto imports = reader.imports();
    for (const QString &import : imports)
        result.dependencies.append(import);

    QHash<QString, ScopeTree *> qmlComponents;
    const auto components = reader.components();
    for (auto it = components.begin(), end = components.end(); it != end; ++it) {
        const QString filePath = path + QLatin1Char('/') + it->fileName;
        if (!QFile::exists(filePath)) {
            m_colorOut.write(QLatin1String("warning: "), Warning);
            m_colorOut.write(it->fileName + QLatin1String(" is listed as component in ")
                             + path + SlashQmldir
                             + QLatin1String(" but does not exist.\n"));
            continue;
        }

        auto mo = qmlComponents.find(it.key());
        if (mo == qmlComponents.end())
            mo = qmlComponents.insert(it.key(), localFile2ScopeTree(filePath));

        (*mo)->addExport(
                    it.key(), reader.typeNamespace(),
                    ComponentVersion(it->majorVersion, it->minorVersion));
    }
    for (auto it = qmlComponents.begin(), end = qmlComponents.end(); it != end; ++it)
        result.objects.insert( it.key(), ScopeTree::ConstPtr(it.value()));

    if (!reader.plugins().isEmpty() && QFile::exists(path + SlashPluginsDotQmltypes))
        readQmltypes(path + SlashPluginsDotQmltypes, result);

    return result;
}

void FindUnqualifiedIDVisitor::processImport(const QString &prefix, const FindUnqualifiedIDVisitor::Import &import)
{
    for (auto const &dependency : qAsConst(import.dependencies)) {
        auto const split = dependency.split(" ");
        auto const &id = split.at(0);
        if (split.length() > 1) {
            const auto version = split.at(1).split('.');
            importHelper(id, QString(),
                         version.at(0).toInt(),
                         version.length() > 1 ? version.at(1).toInt() : -1);
        } else {
            importHelper(id, QString(), -1, -1);
        }


    }

    // add objects
    for (auto it = import.objects.begin(); it != import.objects.end(); ++it) {
        const auto &val = it.value();
        m_exportedName2Scope.insert(prefixedName(prefix, val->className()), val);

        const auto exports = val->exports();
        for (const auto &valExport : exports)
            m_exportedName2Scope.insert(prefixedName(prefix, valExport.type()), val);

        const auto enums = val->enums();
        for (const auto &valEnum : enums)
            m_currentScope->addEnum(valEnum);
    }
}

void FindUnqualifiedIDVisitor::importHelper(const QString &module, const QString &prefix,
                                            int major, int minor)
{
    const QString id = QString(module).replace(QLatin1Char('/'), QLatin1Char('.'));
    QPair<QString, QString> importId { id, prefix };
    if (m_alreadySeenImports.contains(importId))
        return;
    m_alreadySeenImports.insert(importId);

    for (const QString &qmltypeDir : m_qmltypeDirs) {
        auto qmltypesPaths = completeImportPaths(id, qmltypeDir, major, minor);

        for (auto const &qmltypesPath : qmltypesPaths) {
            if (QFile::exists(qmltypesPath + SlashQmldir)) {
                processImport(prefix, readQmldir(qmltypesPath));

                // break so that we don't import unversioned qml components
                // in addition to versioned ones
                break;
            }

            Import result;
            if (QFile::exists(qmltypesPath + SlashAppDotQmltypes))
                readQmltypes(qmltypesPath + SlashAppDotQmltypes, result);
            else if (QFile::exists(qmltypesPath + SlashLibDotQmltypes))
                readQmltypes(qmltypesPath + SlashLibDotQmltypes, result);
            else
                continue;
            processImport(prefix, result);
        }
    }
}

ScopeTree *FindUnqualifiedIDVisitor::localFile2ScopeTree(const QString &filePath)
{
    using namespace QQmlJS::AST;
    auto scope = new ScopeTree(ScopeType::QMLScope);
    const QFileInfo info { filePath };
    QString baseName = info.baseName();
    scope->setClassName(baseName.endsWith(".ui") ? baseName.chopped(3) : baseName);
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
        return scope;

    QString code = file.readAll();
    file.close();

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    const QString lowerSuffix = info.suffix().toLower();
    const bool isJavaScript = (lowerSuffix == QLatin1String("js") || lowerSuffix == QLatin1String("mjs"));
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    lexer.setCode(code, /*line = */ 1, /*qmlMode=*/ !isJavaScript);
    QQmlJS::Parser parser(&engine);

    const bool success = isJavaScript ? (isESModule ? parser.parseModule()
                                                    : parser.parseProgram())
                                      : parser.parse();
    if (!success)
        return scope;

    if (!isJavaScript) {
        QQmlJS::AST::UiProgram *program = parser.ast();
        parseHeaders(program->headers);
        parseMembers(program->members, scope);
    } else {
        // TODO: Anything special to do with ES modules here?
        parseProgram(QQmlJS::AST::cast<QQmlJS::AST::Program *>(parser.rootNode()), scope);
    }

    return scope;
}

void FindUnqualifiedIDVisitor::importFileOrDirectory(const QString &fileOrDirectory,
                                                     const QString &prefix)
{
    QString name = fileOrDirectory;

    if (QFileInfo(name).isRelative())
        name = QDir(QFileInfo { m_filePath }.path()).filePath(name);

    if (QFileInfo(name).isFile()) {
        m_exportedName2Scope.insert(prefix, ScopeTree::ConstPtr(localFile2ScopeTree(name)));
        return;
    }

    QDirIterator it { name, QStringList() << QLatin1String("*.qml"), QDir::NoFilter };
    while (it.hasNext()) {
        ScopeTree::ConstPtr scope(localFile2ScopeTree(it.next()));
        if (!scope->className().isEmpty())
            m_exportedName2Scope.insert(prefixedName(prefix, scope->className()), scope);
    }
}

void FindUnqualifiedIDVisitor::importExportedNames(const QStringRef &prefix, QString name)
{
    for (;;) {
        ScopeTree::ConstPtr scope = m_exportedName2Scope.value(m_exportedName2Scope.contains(name)
                                                               ? name
                                                               : prefix + QLatin1Char('.') + name);
        if (scope) {
            const auto properties = scope->properties();
            for (const auto &property : properties)
                m_currentScope->insertPropertyIdentifier(property);

            m_currentScope->addMethods(scope->methods());
            name = scope->superclassName();
            if (name.isEmpty() || name == QLatin1String("QObject"))
                break;
        } else {
            m_colorOut.write(QLatin1String("warning: "), Warning);
            m_colorOut.write(name + QLatin1String(" was not found."
                                                  " Did you add all import paths?\n"));
            m_unknownImports.insert(name);
            break;
        }
    }
}

void FindUnqualifiedIDVisitor::throwRecursionDepthError()
{
    m_colorOut.write(QStringLiteral("Error"), Error);
    m_colorOut.write(QStringLiteral("Maximum statement or expression depth exceeded"), Error);
    m_visitFailed = true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiProgram *)
{
    enterEnvironment(ScopeType::QMLScope, "program");
    QHash<QString, ScopeTree::ConstPtr> objects;
    QList<ModuleApiInfo> moduleApis;
    QStringList dependencies;
    for (auto const &dir : m_qmltypeDirs) {
        QDirIterator it { dir, QStringList() << QLatin1String("builtins.qmltypes"), QDir::NoFilter,
                          QDirIterator::Subdirectories };
        while (it.hasNext()) {
            auto reader = createQmltypesReaderForFile(it.next());
            auto succ = reader(&objects, &moduleApis, &dependencies);
            if (!succ)
                m_colorOut.writeUncolored(reader.errorMessage());
        }
    }
    // add builtins
    for (auto ob_it = objects.begin(); ob_it != objects.end(); ++ob_it) {
        auto val = ob_it.value();

        const auto exports = val->exports();
        for (const auto &valExport : exports)
            m_exportedName2Scope.insert(valExport.type(), val);

        const auto enums = val->enums();
        for (const auto &valEnum : enums)
            m_currentScope->addEnum(valEnum);
    }
    // add "self" (as we only ever check the first part of a qualified identifier, we get away with
    // using an empty ScopeTree
    m_exportedName2Scope.insert(QFileInfo { m_filePath }.baseName(), {});

    importFileOrDirectory(".", QString());
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::UiProgram *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::ClassExpression *ast)
{
    enterEnvironment(ScopeType::JSFunctionScope, ast->name.toString());
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::ClassExpression *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::ClassDeclaration *ast)
{
    enterEnvironment(ScopeType::JSFunctionScope, ast->name.toString());
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::ClassDeclaration *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::ForStatement *)
{
    enterEnvironment(ScopeType::JSLexicalScope, "forloop");
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::ForStatement *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::ForEachStatement *)
{
    enterEnvironment(ScopeType::JSLexicalScope, "foreachloop");
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::ForEachStatement *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::Block *)
{
    enterEnvironment(ScopeType::JSLexicalScope, "block");
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::Block *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::CaseBlock *)
{
    enterEnvironment(ScopeType::JSLexicalScope, "case");
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::CaseBlock *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::Catch *catchStatement)
{
    enterEnvironment(ScopeType::JSLexicalScope, "catch");
    m_currentScope->insertJSIdentifier(catchStatement->patternElement->bindingIdentifier.toString(),
                                       QQmlJS::AST::VariableScope::Let);
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::Catch *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::WithStatement *withStatement)
{
    m_colorOut.write(QString::fromLatin1("Warning: "), Warning);
    m_colorOut.write(QString::fromLatin1(
                         "%1:%2: with statements are strongly discouraged in QML "
                         "and might cause false positives when analysing unqalified identifiers\n")
                     .arg(withStatement->firstSourceLocation().startLine)
                     .arg(withStatement->firstSourceLocation().startColumn),
                     Normal);
    enterEnvironment(ScopeType::JSLexicalScope, "with");
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::WithStatement *)
{
    leaveEnvironment();
}

static QString signalName(const QStringRef &handlerName)
{
    if (handlerName.startsWith("on") && handlerName.size() > 2) {
        QString signal = handlerName.mid(2).toString();
        for (int i = 0; i < signal.length(); ++i) {
            QCharRef ch = signal[i];
            if (ch.isLower())
                return QString();
            if (ch.isUpper()) {
                ch = ch.toLower();
                return signal;
            }
        }
    }
    return QString();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiScriptBinding *uisb)
{
    using namespace QQmlJS::AST;
    auto name = uisb->qualifiedId->name;
    if (name == QLatin1String("id")) {
        // found id
        auto expstat = cast<ExpressionStatement *>(uisb->statement);
        auto identexp = cast<IdentifierExpression *>(expstat->expression);
        QString elementName = m_currentScope->name();
        m_qmlid2scope.insert(identexp->name.toString(), m_exportedName2Scope.value(elementName));
        if (m_currentScope->isVisualRootScope())
            m_rootId = identexp->name.toString();
    } else {
        const QString signal = signalName(name);
        if (signal.isEmpty())
            return true;

        if (!m_currentScope->methods().contains(signal)) {
            m_currentScope->addUnmatchedSignalHandler(name.toString(), uisb->firstSourceLocation());
            return true;
        }

        const auto statement = uisb->statement;
        if (statement->kind == Node::Kind::Kind_ExpressionStatement) {
            if (cast<ExpressionStatement *>(statement)->expression->asFunctionDefinition()) {
                // functions are already handled
                // they do not get names inserted according to the signal, but access their formal
                // parameters
                return true;
            }
        }

        auto method = m_currentScope->methods()[signal];
        for (auto const &param : method.parameterNames()) {
            const auto firstSourceLocation = statement->firstSourceLocation();
            bool hasMultilineStatementBody
                    = statement->lastSourceLocation().startLine > firstSourceLocation.startLine;
            m_currentScope->insertSignalIdentifier(param, method, firstSourceLocation,
                                                   hasMultilineStatementBody);
        }
        return true;
    }
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiPublicMember *uipm)
{
    // property bool inactive: !active
    // extract name inactive
    m_currentScope->insertPropertyIdentifier(MetaProperty(
                uipm->name.toString(),
                // TODO: signals, complex types etc.
                uipm->memberType ? uipm->memberType->name.toString() : QString(),
                uipm->typeModifier == QLatin1String("list"),
                !uipm->isReadonlyMember,
                false, 0));
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::IdentifierExpression *idexp)
{
    auto name = idexp->name;
    if (!m_exportedName2Scope.contains(name.toString())) {
        m_currentScope->addIdToAccssedIfNotInParentScopes(
                { name.toString(), idexp->firstSourceLocation() }, m_unknownImports);
    }
    return true;
}

FindUnqualifiedIDVisitor::FindUnqualifiedIDVisitor(QStringList qmltypeDirs, QString code,
                                                   QString fileName, bool silent)
    : m_rootScope(new ScopeTree { ScopeType::JSFunctionScope, "global" }),
      m_currentScope(m_rootScope.get()),
      m_qmltypeDirs(std::move(qmltypeDirs)),
      m_code(std::move(code)),
      m_rootId(QLatin1String("<id>")),
      m_filePath(std::move(fileName)),
      m_colorOut(silent)
{
    // setup color output
    m_colorOut.insertMapping(Error, ColorOutput::RedForeground);
    m_colorOut.insertMapping(Warning, ColorOutput::PurpleForeground);
    m_colorOut.insertMapping(Info, ColorOutput::BlueForeground);
    m_colorOut.insertMapping(Normal, ColorOutput::DefaultColor);
    m_colorOut.insertMapping(Hint, ColorOutput::GreenForeground);
    QLatin1String jsGlobVars[] = {
        /* Not listed on the MDN page; browser and QML extensions: */
        // console/debug api
        QLatin1String("console"), QLatin1String("print"),
        // garbage collector
        QLatin1String("gc"),
        // i18n
        QLatin1String("qsTr"), QLatin1String("qsTrId"), QLatin1String("QT_TR_NOOP"),
        QLatin1String("QT_TRANSLATE_NOOP"), QLatin1String("QT_TRID_NOOP"),
        // XMLHttpRequest
        QLatin1String("XMLHttpRequest")
    };
    for (const char **globalName = QV4::Compiler::Codegen::s_globalNames;
         *globalName != nullptr;
         ++globalName) {
        m_currentScope->insertJSIdentifier(QString::fromLatin1(*globalName),
                                           QQmlJS::AST::VariableScope::Const);
    }
    for (const auto& jsGlobVar: jsGlobVars)
        m_currentScope->insertJSIdentifier(jsGlobVar, QQmlJS::AST::VariableScope::Const);
}

bool FindUnqualifiedIDVisitor::check()
{
    if (m_visitFailed)
        return false;

    // now that all ids are known, revisit any Connections whose target were perviously unknown
    for (auto const &outstandingConnection: m_outstandingConnections) {
        auto targetScope = m_qmlid2scope[outstandingConnection.targetName];
        if (outstandingConnection.scope)
            outstandingConnection.scope->addMethods(targetScope->methods());
        QScopedValueRollback<ScopeTree*> rollback(m_currentScope, outstandingConnection.scope);
        outstandingConnection.uiod->initializer->accept(this);
    }
    return m_rootScope->recheckIdentifiers(m_code, m_qmlid2scope, m_rootScope.get(), m_rootId,
                                           m_colorOut);
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::VariableDeclarationList *vdl)
{
    while (vdl) {
        m_currentScope->insertJSIdentifier(vdl->declaration->bindingIdentifier.toString(),
                                           vdl->declaration->scope);
        vdl = vdl->next;
    }
    return true;
}

void FindUnqualifiedIDVisitor::visitFunctionExpressionHelper(QQmlJS::AST::FunctionExpression *fexpr)
{
    using namespace QQmlJS::AST;
    auto name = fexpr->name.toString();
    if (!name.isEmpty()) {
        if (m_currentScope->scopeType() == ScopeType::QMLScope)
            m_currentScope->addMethod(MetaMethod(name, QLatin1String("void")));
        else
            m_currentScope->insertJSIdentifier(name, VariableScope::Const);
        enterEnvironment(ScopeType::JSFunctionScope, name);
    } else {
        enterEnvironment(ScopeType::JSFunctionScope, QLatin1String("<anon>"));
    }
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::FunctionExpression *fexpr)
{
    visitFunctionExpressionHelper(fexpr);
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::FunctionExpression *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::FunctionDeclaration *fdecl)
{
    visitFunctionExpressionHelper(fdecl);
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::FunctionDeclaration *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::FormalParameterList *fpl)
{
    for (auto const &boundName : fpl->boundNames()) {
        m_currentScope->insertJSIdentifier(boundName.id, QQmlJS::AST::VariableScope::Const);
    }
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiImport *import)
{
    // construct path
    QString prefix = QLatin1String("");
    if (import->asToken.isValid()) {
        prefix += import->importId;
    }
    auto dirname = import->fileName.toString();
    if (!dirname.isEmpty())
        importFileOrDirectory(dirname, prefix);

    QString path {};
    if (!import->importId.isEmpty()) {
        // TODO: do not put imported ids into the same space as qml IDs
        m_qmlid2scope.insert(import->importId.toString(), {});
    }
    if (import->version) {
        auto uri = import->importUri;
        while (uri) {
            path.append(uri->name);
            path.append("/");
            uri = uri->next;
        }
        path.chop(1);

        importHelper(path, prefix, import->version->majorVersion, import->version->minorVersion);
    }
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiEnumDeclaration *uied)
{
    MetaEnum qmlEnum(uied->name.toString());
    for (const auto *member = uied->members; member; member = member->next)
        qmlEnum.addKey(member->member.toString());
    m_currentScope->addEnum(qmlEnum);
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiObjectBinding *uiob)
{
    // property QtObject __styleData: QtObject {...}

    QString name {};
    auto id = uiob->qualifiedTypeNameId;
    QStringRef prefix = uiob->qualifiedTypeNameId->name;
    while (id) {
        name += id->name.toString() + QLatin1Char('.');
        id = id->next;
    }
    name.chop(1);

    const MetaProperty prop(uiob->qualifiedId->name.toString(), name, false, true, true, 0);
    m_currentScope->addProperty(prop);

    enterEnvironment(ScopeType::QMLScope, name);
    importExportedNames(prefix, name);
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::UiObjectBinding *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiObjectDefinition *uiod)
{
    using namespace QQmlJS::AST;

    QString name {};
    auto id = uiod->qualifiedTypeNameId;
    QStringRef prefix = uiod->qualifiedTypeNameId->name;
    while (id) {
        name += id->name.toString() + QLatin1Char('.');
        id = id->next;
    }
    name.chop(1);
    enterEnvironment(ScopeType::QMLScope, name);
    if (name.isLower())
        return false; // Ignore grouped properties for now

    importExportedNames(prefix, name);
    if (name.endsWith("Connections")) {
        QString target;
        auto member = uiod->initializer->members;
        while (member) {
            if (member->member->kind == QQmlJS::AST::Node::Kind_UiScriptBinding) {
                auto asBinding = static_cast<QQmlJS::AST::UiScriptBinding*>(member->member);
                if (asBinding->qualifiedId->name == QLatin1String("target")) {
                    if (asBinding->statement->kind == QQmlJS::AST::Node::Kind_ExpressionStatement) {
                        auto expr = static_cast<QQmlJS::AST::ExpressionStatement*>(asBinding->statement)->expression;
                        if (auto idexpr = QQmlJS::AST::cast<QQmlJS::AST::IdentifierExpression*>(expr)) {
                            target = idexpr->name.toString();
                        } else {
                            // more complex expressions are not supported
                        }
                    }
                    break;
                }
            }
            member = member->next;
        }
        ScopeTree::ConstPtr targetScope;
        if (target.isEmpty()) {
            // no target set, connection comes from parentF
            ScopeTree* scope = m_currentScope;
            do {
                scope = scope->parentScope(); // TODO: rename method
            } while (scope->scopeType() != ScopeType::QMLScope);
            targetScope = m_exportedName2Scope.value(scope->name());
        } else {
            // there was a target, check if we already can find it
            auto scopeIt =  m_qmlid2scope.find(target);
            if (scopeIt != m_qmlid2scope.end()) {
                targetScope = *scopeIt;
            } else {
                m_outstandingConnections.push_back({target, m_currentScope, uiod});
                return false; // visit children later once target is known
            }
        }
        if (targetScope)
            m_currentScope->addMethods(targetScope->methods());
    }
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::PatternElement *element)
{
    if (element->isVariableDeclaration()) {
        QQmlJS::AST::BoundNames names;
        element->boundNames(&names);
        for (const auto &name : names)
            m_currentScope->insertJSIdentifier(name.id, element->scope);
    }

    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::UiObjectDefinition *)
{
    leaveEnvironment();
}
