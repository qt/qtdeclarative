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
#include "importedmembersvisitor.h"
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
    Q_ASSERT(!prefix.endsWith('.'));
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

void FindUnqualifiedIDVisitor::enterEnvironment(ScopeType type, const QString &name)
{
    m_currentScope = m_currentScope->createNewChildScope(type, name).get();
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
                importHelper(path,
                             import->asToken.isValid() ? import->importId.toString() : QString(),
                             import->version->majorVersion,
                             import->version->minorVersion);
            }
        }
        header = header->next;
    }
}

ScopeTree *FindUnqualifiedIDVisitor::parseProgram(QQmlJS::AST::Program *program,
                                                  const QString &name)
{
    using namespace QQmlJS::AST;
    ScopeTree *result = new ScopeTree(ScopeType::JSLexicalScope, name);
    for (auto *statement = program->statements; statement; statement = statement->next) {
        if (auto *function = cast<FunctionDeclaration *>(statement->statement)) {
            MetaMethod method(function->name.toString());
            method.setMethodType(MetaMethod::Method);
            for (auto *parameters = function->formals; parameters; parameters = parameters->next)
                method.addParameter(parameters->element->bindingIdentifier.toString(), "");
            result->addMethod(method);
        }
    }
    return result;
}

enum ImportVersion { FullyVersioned, PartiallyVersioned, Unversioned, BasePath };

QStringList completeImportPaths(const QString &uri, const QString &basePath, int vmaj, int vmin)
{
    static const QLatin1Char Slash('/');
    static const QLatin1Char Backslash('\\');

    const QVector<QStringRef> parts = uri.splitRef(QLatin1Char('.'), Qt::SkipEmptyParts);

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
        m_types[it.key()] = val;
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

            if (!m_qmltypeFiles.isEmpty())
                continue;

            Import result;

            QDirIterator it { qmltypesPath, QStringList() << QLatin1String("*.qmltypes"), QDir::Files };

            while (it.hasNext())
                readQmltypes(it.next(), result);

            processImport(prefix, result);
        }
    }

    if (!m_qmltypeFiles.isEmpty())
    {
        Import result;

        for (const auto &qmltypeFile : m_qmltypeFiles)
            readQmltypes(qmltypeFile, result);

        processImport("", result);
    }
}

ScopeTree *FindUnqualifiedIDVisitor::localFile2ScopeTree(const QString &filePath)
{
    using namespace QQmlJS::AST;
    const QFileInfo info { filePath };
    QString baseName = info.baseName();
    const QString scopeName = baseName.endsWith(".ui") ? baseName.chopped(3) : baseName;

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        return new ScopeTree(isJavaScript ? ScopeType::JSLexicalScope : ScopeType::QMLScope,
                             scopeName);
    }

    QString code = file.readAll();
    file.close();

    lexer.setCode(code, /*line = */ 1, /*qmlMode=*/ !isJavaScript);
    QQmlJS::Parser parser(&engine);

    const bool success = isJavaScript ? (isESModule ? parser.parseModule()
                                                    : parser.parseProgram())
                                      : parser.parse();
    if (!success) {
        return new ScopeTree(isJavaScript ? ScopeType::JSLexicalScope : ScopeType::QMLScope,
                             scopeName);
    }

    if (!isJavaScript) {
        QQmlJS::AST::UiProgram *program = parser.ast();
        parseHeaders(program->headers);
        ImportedMembersVisitor membersVisitor(&m_colorOut);
        program->members->accept(&membersVisitor);
        return membersVisitor.result(scopeName);
    }

    // TODO: Anything special to do with ES modules here?
    return parseProgram(QQmlJS::AST::cast<QQmlJS::AST::Program *>(parser.rootNode()), scopeName);
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
    QList<ScopeTree::ConstPtr> scopes;
    for (;;) {
        ScopeTree::ConstPtr scope = m_exportedName2Scope.value(m_exportedName2Scope.contains(name)
                                                               ? name
                                                               : prefix + QLatin1Char('.') + name);
        if (scope) {
            if (scopes.contains(scope)) {
                QString inheritenceCycle = name;
                for (const auto &seen: qAsConst(scopes)) {
                    inheritenceCycle.append(QLatin1String(" -> "));
                    inheritenceCycle.append(seen->superclassName());
                }

                m_colorOut.write(QLatin1String("Warning: "), Warning);
                m_colorOut.write(QString::fromLatin1("%1 is part of an inheritance cycle: %2\n")
                                 .arg(name)
                                 .arg(inheritenceCycle));
                m_unknownImports.insert(name);
                m_visitFailed = true;
                break;
            }
            scopes.append(scope);
            const auto properties = scope->properties();
            for (auto property : properties) {
                property.setType(m_exportedName2Scope.value(property.typeName()).get());
                m_currentScope->insertPropertyIdentifier(property);
            }

            m_currentScope->addMethods(scope->methods());
            name = scope->superclassName();
            if (name.isEmpty() || name == QLatin1String("QObject"))
                break;
        } else {
            m_colorOut.write(QLatin1String("warning: "), Warning);
            m_colorOut.write(name + QLatin1String(" was not found."
                                                  " Did you add all import paths?\n"));
            m_unknownImports.insert(name);
            m_visitFailed = true;
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

    if (!m_qmltypeFiles.isEmpty())
    {
        for (const auto &qmltypeFile : m_qmltypeFiles) {
            auto reader = createQmltypesReaderForFile(qmltypeFile);
            auto succ = reader(&objects, &moduleApis, &dependencies);
            if (!succ)
                m_colorOut.writeUncolored(reader.errorMessage());
        }
    }

    // add builtins
    for (auto objectIt = objects.begin(); objectIt != objects.end(); ++objectIt) {
        auto val = objectIt.value();
        m_types[objectIt.key()] = val;

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
        m_qmlid2scope.insert(identexp->name.toString(), m_currentScope);
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
    MetaProperty property(
                uipm->name.toString(),
                // TODO: signals, complex types etc.
                uipm->memberType ? uipm->memberType->name.toString() : QString(),
                uipm->typeModifier == QLatin1String("list"),
                !uipm->isReadonlyMember,
                false,
                uipm->memberType ? (uipm->memberType->name == QLatin1String("alias")) : false,
                0);
    property.setType(m_exportedName2Scope.value(property.typeName()).get());
    m_currentScope->insertPropertyIdentifier(property);
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::IdentifierExpression *idexp)
{
    auto name = idexp->name;
    m_currentScope->addIdToAccessed(name.toString(), idexp->firstSourceLocation());
    m_fieldMemberBase = idexp;
    return true;
}

FindUnqualifiedIDVisitor::FindUnqualifiedIDVisitor(QStringList qmltypeDirs, QStringList qmltypeFiles, QString code,
                                                   QString fileName, bool silent)
    : m_rootScope(new ScopeTree { ScopeType::JSFunctionScope, "global" }),
      m_currentScope(m_rootScope.get()),
      m_qmltypeDirs(std::move(qmltypeDirs)),
      m_qmltypeFiles(std::move(qmltypeFiles)),
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
    return m_rootScope->recheckIdentifiers(m_code, m_qmlid2scope, m_exportedName2Scope,
                                           m_rootScope.get(), m_rootId, m_colorOut);
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
        const QString importId = import->importId.toString();
        m_qmlid2scope.insert(importId, m_exportedName2Scope.value(importId).get());
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

    MetaProperty prop(uiob->qualifiedId->name.toString(), name, false, true, true,
                      name == QLatin1String("alias"), 0);
    prop.setType(m_exportedName2Scope.value(uiob->qualifiedTypeNameId->name.toString()).get());
    m_currentScope->addProperty(prop);

    enterEnvironment(ScopeType::QMLScope, name);
    importExportedNames(prefix, name);
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::UiObjectBinding *uiob)
{
    const auto childScope = m_currentScope;
    leaveEnvironment();
    MetaProperty property(uiob->qualifiedId->name.toString(),
                          uiob->qualifiedTypeNameId->name.toString(),
                          false, true, true,
                          uiob->qualifiedTypeNameId->name == QLatin1String("alias"),
                          0);
    property.setType(childScope);
    m_currentScope->addProperty(property);
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
        const ScopeTree *targetScope;
        if (target.isEmpty()) {
            // no target set, connection comes from parentF
            ScopeTree* scope = m_currentScope;
            do {
                scope = scope->parentScope(); // TODO: rename method
            } while (scope->scopeType() != ScopeType::QMLScope);
            targetScope = m_exportedName2Scope.value(scope->name()).get();
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
    auto childScope = m_currentScope;
    leaveEnvironment();
    childScope->updateParentProperty(m_currentScope);
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::FieldMemberExpression *)
{
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::FieldMemberExpression *fieldMember)
{
    using namespace QQmlJS::AST;
    ExpressionNode *base = fieldMember->base;
    while (auto *nested = cast<NestedExpression *>(base))
        base = nested->expression;

    if (m_fieldMemberBase == base) {
        QString type;
        if (auto *binary = cast<BinaryExpression *>(base)) {
            if (binary->op == QSOperator::As) {
                // This is terrible. It's fixed in 6.0.
                if (auto *right = cast<Type *>(static_cast<Node *>(binary->right)))
                    type = right->toString();
            }
        }
        m_currentScope->accessMember(fieldMember->name.toString(),
                                     type,
                                     fieldMember->identifierToken);
        m_fieldMemberBase = fieldMember;
    } else {
        m_fieldMemberBase = nullptr;
    }
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::BinaryExpression *)
{
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::BinaryExpression *binExp)
{
    if (binExp->op == QSOperator::As && m_fieldMemberBase == binExp->left)
        m_fieldMemberBase = binExp;
    else
        m_fieldMemberBase = nullptr;
}
