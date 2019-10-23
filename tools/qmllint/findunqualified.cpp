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

#include "qmljstypedescriptionreader.h"

#include <QFile>
#include <QDirIterator>
#include <QScopedValueRollback>

#include <private/qqmljsast_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qv4codegen_p.h>

QDebug operator<<(QDebug dbg, const QQmlJS::AST::SourceLocation &loc);

static QQmlJS::TypeDescriptionReader createReaderForFile(QString const &filename)
{
    QFile f(filename);
    f.open(QFile::ReadOnly);
    QQmlJS::TypeDescriptionReader reader { filename, f.readAll() };
    return reader;
}

void FindUnqualifiedIDVisitor::enterEnvironment(ScopeType type, QString name)
{
    m_currentScope = m_currentScope->createNewChildScope(type, name);
}

void FindUnqualifiedIDVisitor::leaveEnvironment()
{
    m_currentScope = m_currentScope->parentScope();
}

enum ImportVersion { FullyVersioned, PartiallyVersioned, Unversioned };

QStringList completeQmltypesPaths(const QString &uri, const QStringList &basePaths, int vmaj, int vmin)
{
    static const QLatin1Char Slash('/');
    static const QLatin1Char Backslash('\\');
    static const QLatin1String SlashPluginsDotQmltypes("/plugins.qmltypes");

    const QVector<QStringRef> parts = uri.splitRef(QLatin1Char('.'), QString::SkipEmptyParts);

    QStringList qmlDirPathsPaths;
    // fully & partially versioned parts + 1 unversioned for each base path
    qmlDirPathsPaths.reserve(basePaths.count() * (2 * parts.count() + 1));

    auto versionString = [](int vmaj, int vmin, ImportVersion version)
    {
        if (version == FullyVersioned) {
            // extension with fully encoded version number (eg. MyModule.3.2)
            return QString::asprintf(".%d.%d", vmaj, vmin);
        } else if (version == PartiallyVersioned) {
            // extension with encoded version major (eg. MyModule.3)
            return QString::asprintf(".%d", vmaj);
        } // else extension without version number (eg. MyModule)
        return QString();
    };
    auto joinStringRefs = [](const QVector<QStringRef> &refs, const QChar &sep)
    {
        QString str;
        for (auto it = refs.cbegin(); it != refs.cend(); ++it) {
            if (it != refs.cbegin())
                str += sep;
            str += *it;
        }
        return str;
    };

    for (int version = FullyVersioned; version <= Unversioned; ++version) {
        const QString ver = versionString(vmaj, vmin, static_cast<ImportVersion>(version));

        for (const QString &path : basePaths) {
            QString dir = path;
            if (!dir.endsWith(Slash) && !dir.endsWith(Backslash))
                dir += Slash;

            // append to the end
            qmlDirPathsPaths += dir + joinStringRefs(parts, Slash) + ver + SlashPluginsDotQmltypes;

            if (version != Unversioned) {
                // insert in the middle
                for (int index = parts.count() - 2; index >= 0; --index) {
                    qmlDirPathsPaths += dir + joinStringRefs(parts.mid(0, index + 1), Slash)
                            + ver + Slash
                            + joinStringRefs(parts.mid(index + 1), Slash) + SlashPluginsDotQmltypes;
                }
            }
        }
    }

    return qmlDirPathsPaths;
}

void FindUnqualifiedIDVisitor::importHelper(QString id, QString prefix, int major, int minor)
{
    QPair<QString, QString> importId { id, prefix };
    if (m_alreadySeenImports.contains(importId)) {
        return;
    } else {
        m_alreadySeenImports.insert(importId);
    }
    id = id.replace(QLatin1String("/"), QLatin1String("."));
    auto qmltypesPaths = completeQmltypesPaths(id, m_qmltypeDirs, major, minor);

    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> objects;
    QList<QQmlJS::ModuleApiInfo> moduleApis;
    QStringList dependencies;
    for (auto const &qmltypesPath : qmltypesPaths) {
        if (QFile::exists(qmltypesPath)) {
            auto reader = createReaderForFile(qmltypesPath);
            auto succ = reader(&objects, &moduleApis, &dependencies);
            if (!succ) {
                qDebug() << reader.errorMessage();
            }
            break;
        }
    }
    for (auto const &dependency : qAsConst(dependencies)) {
        auto const split = dependency.split(" ");
        auto const id = split.at(0);
        auto const major = split.at(1).split('.').at(0).toInt();
        auto const minor = split.at(1).split('.').at(1).toInt();
        importHelper(id, QString(), major, minor);
    }
    // add objects
    for (auto ob_it = objects.begin(); ob_it != objects.end(); ++ob_it) {
        auto val = ob_it.value();
        m_exportedName2MetaObject[prefix + val->className()] = val;
        for (auto export_ : val->exports()) {
            m_exportedName2MetaObject[prefix + export_.type] = val;
        }
        for (auto enumCount = 0; enumCount < val->enumeratorCount(); ++enumCount) {
            m_currentScope->insertQMLIdentifier(val->enumerator(enumCount).name());
        }
    }
}

LanguageUtils::FakeMetaObject *
FindUnqualifiedIDVisitor::localQmlFile2FakeMetaObject(QString filePath)
{
    using namespace QQmlJS::AST;
    auto fake = new LanguageUtils::FakeMetaObject;
    fake->setClassName(QFileInfo { filePath }.baseName());
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        return fake;
    }
    QString code = file.readAll();
    file.close();

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    lexer.setCode(code, 1, true);
    QQmlJS::Parser parser(&engine);
    if (!parser.parse()) {
        return fake;
    }
    QQmlJS::AST::UiProgram *program = parser.ast();
    auto header = program->headers;
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
                importHelper(path, prefix, import->version->majorVersion, import->version->minorVersion);
            }
        }
        header = header->next;
    }
    auto member = program->members;
    // member should be the sole element
    Q_ASSERT(!member->next);
    Q_ASSERT(member && member->member->kind == UiObjectMember::Kind_UiObjectDefinition);
    auto definition = static_cast<UiObjectDefinition *>(member->member);
    auto qualifiedId = definition->qualifiedTypeNameId;
    while (qualifiedId && qualifiedId->next) {
        qualifiedId = qualifiedId->next;
    }
    fake->setSuperclassName(qualifiedId->name.toString());
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
            auto publicMember = static_cast<UiPublicMember *>(initMembers->member);
            switch (publicMember->type) {
            case UiPublicMember::Signal: {
                UiParameterList *param = publicMember->parameters;
                LanguageUtils::FakeMetaMethod method;
                method.setMethodType(LanguageUtils::FakeMetaMethod::Signal);
                method.setMethodName(publicMember->name.toString());
                while (param) {
                    method.addParameter(param->name.toString(), param->type->name.toString());
                    param = param->next;
                }
                fake->addMethod(method);
                break;
            }
            case UiPublicMember::Property: {
                LanguageUtils::FakeMetaProperty fakeprop { publicMember->name.toString(),
                                                           publicMember->typeModifier.toString(),
                                                           false,
                                                           false,
                                                           false,
                                                           0 };
                fake->addProperty(fakeprop);
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
            auto sourceElement = static_cast<UiSourceElement *>(initMembers->member);
            if (FunctionExpression *fexpr = sourceElement->sourceElement->asFunctionDefinition()) {
                LanguageUtils::FakeMetaMethod method;
                method.setMethodType(LanguageUtils::FakeMetaMethod::Method);
                FormalParameterList *parameters = fexpr->formals;
                while (parameters) {
                    method.addParameter(parameters->element->bindingIdentifier.toString(),
                                        "");
                    parameters = parameters->next;
                }
                fake->addMethod(method);
            } else if (ClassExpression *clexpr =
                               sourceElement->sourceElement->asClassDefinition()) {
                LanguageUtils::FakeMetaProperty prop {
                    clexpr->name.toString(), "", false, false, false, 1
                };
                fake->addProperty(prop);
            } else if (cast<VariableStatement *>(sourceElement->sourceElement)) {
                // nothing to do
            } else {
                qDebug() << "unsupportedd sourceElement at" << sourceElement->firstSourceLocation()
                         << sourceElement->sourceElement->kind;
            }
            break;
        }
        default: {
            qDebug() << "unsupported element of kind" << initMembers->member->kind;
        }
        }
        initMembers = initMembers->next;
    }
    return fake;
}

void FindUnqualifiedIDVisitor::importExportedNames(QStringRef prefix, QString name)
{
    for (;;) {
        auto metaObject = m_exportedName2MetaObject[m_exportedName2MetaObject.contains(name)
                                                            ? name
                                                            : prefix + QLatin1Char('.') + name];
        if (metaObject) {
            auto propertyCount = metaObject->propertyCount();
            for (auto i = 0; i < propertyCount; ++i) {
                m_currentScope->insertPropertyIdentifier(metaObject->property(i).name());
            }

            m_currentScope->addMethodsFromMetaObject(metaObject);

            name = metaObject->superclassName();
            if (name.isEmpty() || name == QLatin1String("QObject")) {
                break;
            }
        } else {
            m_colorOut.write(QLatin1String("warning: "), Warning);
            m_colorOut.write(name + QLatin1String(" was not found. Did you add all import paths?\n"));
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
    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> objects;
    QList<QQmlJS::ModuleApiInfo> moduleApis;
    QStringList dependencies;
    for (auto const &dir : m_qmltypeDirs) {
        QDirIterator it { dir, QStringList() << QLatin1String("builtins.qmltypes"), QDir::NoFilter,
                          QDirIterator::Subdirectories };
        while (it.hasNext()) {
            auto reader = createReaderForFile(it.next());
            auto succ = reader(&objects, &moduleApis, &dependencies);
            if (!succ) {
                qDebug() << reader.errorMessage();
            }
        }
    }
    // add builtins
    for (auto ob_it = objects.begin(); ob_it != objects.end(); ++ob_it) {
        auto val = ob_it.value();
        for (auto export_ : val->exports()) {
            m_exportedName2MetaObject[export_.type] = val;
        }
        for (auto enumCount = 0; enumCount < val->enumeratorCount(); ++enumCount) {
            m_currentScope->insertQMLIdentifier(val->enumerator(enumCount).name());
        }
    }
    // add "self" (as we only ever check the first part of a qualified identifier, we get away with
    // using an empty FakeMetaObject
    m_exportedName2MetaObject[QFileInfo { m_filePath }.baseName()] = {};

    // add QML builtins
    m_exportedName2MetaObject["QtObject"] = {}; // QtObject contains nothing of interest

    LanguageUtils::FakeMetaObject *meta = new LanguageUtils::FakeMetaObject{};
    meta->addProperty(LanguageUtils::FakeMetaProperty {"enabled", "bool", false, false, false, 0});
    meta->addProperty(LanguageUtils::FakeMetaProperty {"ignoreUnknownSignals", "bool", false, false, false, 0});
    meta->addProperty(LanguageUtils::FakeMetaProperty {"target", "QObject", false, false, false, 0});
    m_exportedName2MetaObject["Connections"] = LanguageUtils::FakeMetaObject::ConstPtr { meta };
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
    m_currentScope->insertJSIdentifier(catchStatement->patternElement->bindingIdentifier.toString(), QQmlJS::AST::VariableScope::Let);
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::Catch *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::WithStatement *withStatement)
{
    m_colorOut.write(QString::asprintf("Warning: "), Warning);
    m_colorOut.write(QString::asprintf("%d:%d: with statements are strongly discouraged in QML and might cause false positives when analysing unqalified identifiers\n", withStatement->firstSourceLocation().startLine, withStatement->firstSourceLocation().startColumn), Normal);
    enterEnvironment(ScopeType::JSLexicalScope, "with");
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::WithStatement *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiScriptBinding *uisb)
{
    using namespace QQmlJS::AST;
    auto name = uisb->qualifiedId->name;
    if (name == QLatin1String("id")) {
        // found id
        auto expstat = static_cast<ExpressionStatement *>(uisb->statement);
        auto identexp = static_cast<IdentifierExpression *>(expstat->expression);
        QString elementName = m_currentScope->name();
        m_qmlid2meta.insert(identexp->name.toString(), m_exportedName2MetaObject[elementName]);
        if (m_currentScope->isVisualRootScope()) {
            m_rootId = identexp->name.toString();
        }
    } else if (name.startsWith("on") && name.size() > 2 && name.at(2).isUpper()) {
        auto statement = uisb->statement;
        if (statement->kind == Node::Kind::Kind_ExpressionStatement) {
            if (static_cast<ExpressionStatement *>(statement)->expression->asFunctionDefinition()) {
                // functions are already handled
                // they do not get names inserted according to the signal, but access their formal
                // parameters
                return true;
            }
        }
        QString signal = name.mid(2).toString();
        signal[0] = signal[0].toLower();
        if (!m_currentScope->methods().contains(signal)) {
            qDebug() << "Info file does not contain signal" << signal;
        } else {
            auto method = m_currentScope->methods()[signal];
            for (auto const &param : method.parameterNames()) {
                auto firstSourceLocation = uisb->statement->firstSourceLocation();
                bool hasMultilineStatementBody = uisb->statement->lastSourceLocation().startLine > firstSourceLocation.startLine;
                m_currentScope->insertSignalIdentifier(param, method, firstSourceLocation, hasMultilineStatementBody);
            }
        }
        return true;
    }
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiPublicMember *uipm)
{
    // property bool inactive: !active
    // extract name inactive
    m_currentScope->insertPropertyIdentifier(uipm->name.toString());
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::IdentifierExpression *idexp)
{
    auto name = idexp->name;
    if (!m_exportedName2MetaObject.contains(name.toString())) {
        m_currentScope->addIdToAccssedIfNotInParentScopes(
                { name.toString(), idexp->firstSourceLocation() }, m_unknownImports);
    }
    return true;
}

FindUnqualifiedIDVisitor::FindUnqualifiedIDVisitor(QStringList const &qmltypeDirs,
                                                   const QString &code, const QString &fileName)
    : m_rootScope(new ScopeTree { ScopeType::JSFunctionScope, "global" }),
      m_currentScope(m_rootScope.get()),
      m_qmltypeDirs(qmltypeDirs),
      m_code(code),
      m_rootId(QLatin1String("<id>")),
      m_filePath(fileName)
{
    // setup color output
    m_colorOut.insertColorMapping(Error, ColorOutput::RedForeground);
    m_colorOut.insertColorMapping(Warning, ColorOutput::PurpleForeground);
    m_colorOut.insertColorMapping(Info, ColorOutput::BlueForeground);
    m_colorOut.insertColorMapping(Normal, ColorOutput::DefaultColor);
    m_colorOut.insertColorMapping(Hint, ColorOutput::GreenForeground);
    QLatin1String jsGlobVars[] = {
        /* Not listed on the MDN page; browser and QML extensions: */
        // console/debug api
        QLatin1String("console"), QLatin1String("print"),
        // garbage collector
        QLatin1String("gc"),
        // i18n
        QLatin1String("qsTr"), QLatin1String("qsTrId"), QLatin1String("QT_TR_NOOP"), QLatin1String("QT_TRANSLATE_NOOP"), QLatin1String("QT_TRID_NOOP"),
        // XMLHttpRequest
        QLatin1String("XMLHttpRequest")
    };
    for (const char **globalName = QV4::Compiler::Codegen::s_globalNames; *globalName != nullptr; ++globalName) {
        m_currentScope->insertJSIdentifier(QString::fromLatin1(*globalName), QQmlJS::AST::VariableScope::Const);
    }
    for (const auto& jsGlobVar: jsGlobVars)
        m_currentScope->insertJSIdentifier(jsGlobVar, QQmlJS::AST::VariableScope::Const);
}

FindUnqualifiedIDVisitor::~FindUnqualifiedIDVisitor() = default;

bool FindUnqualifiedIDVisitor::check()
{
    if (m_visitFailed)
        return false;

    // now that all ids are known, revisit any Connections whose target were perviously unknown
    for (auto const& outstandingConnection: m_outstandingConnections) {
        auto metaObject = m_qmlid2meta[outstandingConnection.targetName];
        outstandingConnection.scope->addMethodsFromMetaObject(metaObject);
        QScopedValueRollback<ScopeTree*> rollback(m_currentScope, outstandingConnection.scope);
        outstandingConnection.uiod->initializer->accept(this);
    }
    return m_rootScope->recheckIdentifiers(m_code, m_qmlid2meta, m_rootScope.get(), m_rootId, m_colorOut);
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
    if (!fexpr->name.isEmpty()) {
        auto name = fexpr->name.toString();
        if (m_currentScope->scopeType() == ScopeType::QMLScope) {
            m_currentScope->insertQMLIdentifier(name);
        } else {
            m_currentScope->insertJSIdentifier(name, VariableScope::Const);
        }
    }
    QString name = fexpr->name.toString();
    if (name.isEmpty())
        name = "<anon>";
    enterEnvironment(ScopeType::JSFunctionScope, name);
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
        prefix += import->importId + QLatin1Char('.');
    }
    auto dirname = import->fileName.toString();
    if (!dirname.isEmpty()) {
        QFileInfo info { dirname };
        if (info.isRelative()) {
            dirname = QDir(QFileInfo { m_filePath }.path()).filePath(dirname);
        }
        QDirIterator it { dirname, QStringList() << QLatin1String("*.qml"), QDir::NoFilter };
        while (it.hasNext()) {
            LanguageUtils::FakeMetaObject *fake = localQmlFile2FakeMetaObject(it.next());
            m_exportedName2MetaObject.insert(
                    fake->className(), QSharedPointer<const LanguageUtils::FakeMetaObject>(fake));
        }
    }
    QString path {};
    if (!import->importId.isEmpty()) {
        m_qmlid2meta.insert(import->importId.toString(), {}); // TODO: do not put imported ids into the same space as qml IDs
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
    m_currentScope->insertQMLIdentifier(uied->name.toString());
    return true;
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiObjectBinding *uiob)
{
    // property QtObject __styleData: QtObject {...}
    m_currentScope->insertPropertyIdentifier(uiob->qualifiedId->name.toString());
    QString name {};
    auto id = uiob->qualifiedTypeNameId;
    QStringRef prefix = uiob->qualifiedTypeNameId->name;
    while (id) {
        name += id->name.toString() + QLatin1Char('.');
        id = id->next;
    }
    name.chop(1);
    enterEnvironment(ScopeType::QMLScope, name);
    if (name == QLatin1String("Component") || name == QLatin1String("QtObject")) // there is no typeinfo for Component and QtObject, but they also have no interesting properties
        return true;
    importExportedNames(prefix, name);
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::UiObjectBinding *)
{
    leaveEnvironment();
}

bool FindUnqualifiedIDVisitor::visit(QQmlJS::AST::UiObjectDefinition *uiod)
{
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
    if (name == QLatin1String("Component") || name == QLatin1String("QtObject")) // there is no typeinfo for Component
        return true;
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
        LanguageUtils::FakeMetaObject::ConstPtr metaObject {};
        if (target.isEmpty()) {
            // no target set, connection comes from parentF
            ScopeTree* scope = m_currentScope;
            do {
                scope = scope->parentScope(); // TODO: rename method
            } while (scope->scopeType() != ScopeType::QMLScope);
            auto metaObject = m_exportedName2MetaObject[scope->name()];
        } else {
            // there was a target, check if we already can find it
            auto metaObjectIt =  m_qmlid2meta.find(target);
            if (metaObjectIt != m_qmlid2meta.end()) {
                metaObject = *metaObjectIt;
            } else {
                m_outstandingConnections.push_back({target, m_currentScope, uiod});
                return false; // visit children later once target is known
            }
        }
        m_currentScope->addMethodsFromMetaObject(metaObject);
    }
    return true;
}

void FindUnqualifiedIDVisitor::endVisit(QQmlJS::AST::UiObjectDefinition *)
{
    leaveEnvironment();
}

QDebug operator<<(QDebug dbg, const QQmlJS::AST::SourceLocation &loc)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << loc.startLine;
    dbg.nospace() << ":";
    dbg.nospace() << loc.startColumn;
    return dbg.maybeSpace();
}
