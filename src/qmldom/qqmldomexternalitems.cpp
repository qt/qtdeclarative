// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomtop_p.h"
#include "qqmldomoutwriter_p.h"
#include "qqmldomcomments_p.h"
#include "qqmldommock_p.h"
#include "qqmldomelements_p.h"
#include "qqmldom_utils_p.h"

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtCore/QDir>
#include <QtCore/QScopeGuard>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpressionMatch>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QQmlJS {
namespace Dom {

ExternalOwningItem::ExternalOwningItem(
        const QString &filePath, const QDateTime &lastDataUpdateAt, const Path &path,
        int derivedFrom, const QString &code)
    : OwningItem(derivedFrom, lastDataUpdateAt),
      m_canonicalFilePath(filePath),
      m_code(code),
      m_path(path)
{}

QString ExternalOwningItem::canonicalFilePath(const DomItem &) const
{
    return m_canonicalFilePath;
}

QString ExternalOwningItem::canonicalFilePath() const
{
    return m_canonicalFilePath;
}

Path ExternalOwningItem::canonicalPath(const DomItem &) const
{
    return m_path;
}

Path ExternalOwningItem::canonicalPath() const
{
    return m_path;
}

QCborValue pluginData(const QQmlDirParser::Plugin &pl, const QStringList &cNames)
{
    QCborArray names;
    for (const QString &n : cNames)
        names.append(n);
    return QCborMap({ { QCborValue(QStringView(Fields::name)), pl.name },
                      { QStringView(Fields::path), pl.path },
                      { QStringView(Fields::classNames), names } });
}

JsFile::JsFile(
        const QString &filePath, const QString &code, const QDateTime &lastDataUpdateAt,
        int derivedFrom)
    : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::qmlFilePath(filePath), derivedFrom,
                         code)
{
    m_engine = std::make_shared<QQmlJS::Engine>();
    LegacyDirectivesCollector directivesCollector(*this);
    m_engine->setDirectives(&directivesCollector);

    QQmlJS::Lexer lexer(m_engine.get());
    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/false);
    QQmlJS::Parser parser(m_engine.get());

    bool isESM = filePath.endsWith(u".mjs", Qt::CaseInsensitive);
    bool isValid = isESM ? parser.parseModule() : parser.parseProgram();
    setIsValid(isValid);

    const auto diagnostics = parser.diagnosticMessages();
    for (const DiagnosticMessage &msg : diagnostics) {
        addErrorLocal(
                std::move(myParsingErrors().errorMessage(msg).withFile(filePath).withPath(m_path)));
    }

    auto astComments = std::make_shared<AstComments>(m_engine);

    CommentCollector collector;
    collector.collectComments(m_engine, parser.rootNode(), astComments);
    m_script = std::make_shared<ScriptExpression>(code, m_engine, parser.rootNode(), astComments,
                                                  isESM ? ScriptExpression::ExpressionType::ESMCode
                                                        : ScriptExpression::ExpressionType::JSCode);
}

ErrorGroups JsFile::myParsingErrors()
{
    static ErrorGroups res = { { DomItem::domErrorGroup, NewErrorGroup("JsFile"),
                                 NewErrorGroup("Parsing") } };
    return res;
}

bool JsFile::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.invokeVisitorOnField(visitor, Fields::fileLocationsTree, m_fileLocationsTree);
    if (m_script)
        cont = cont && visitor(PathEls::Field(Fields::expression), [this, &self]() {
                   return self.subOwnerItem(PathEls::Field(Fields::expression), m_script);
               });
    return cont;
}

void JsFile::writeOut(const DomItem &self, OutWriter &ow) const
{
    writeOutDirectives(self, ow);
    ow.ensureNewline(2);
    if (DomItem script = self.field(Fields::expression)) {
        ow.ensureNewline();
        script.writeOut(ow);
    }
}

void JsFile::addFileImport(const QString &jsfile, const QString &module)
{
    LegacyImport import;
    import.fileName = jsfile;
    import.asIdentifier = module;
    m_imports.append(std::move(import));
}

void JsFile::addModuleImport(const QString &uri, const QString &version, const QString &module)
{
    LegacyImport import;
    import.uri = uri;
    import.version = version;
    import.asIdentifier = module;
    m_imports.append(std::move(import));
}

void JsFile::LegacyPragmaLibrary::writeOut(const DomItem &self, OutWriter &lw) const
{
    Q_UNUSED(self);
    lw.write(u".pragma").ensureSpace().write(u"library").ensureNewline();
}

void JsFile::LegacyImport::writeOut(const DomItem &self, OutWriter &lw) const
{
    const auto fLoc = FileLocations::treeOf(self);
    // either filename or module uri must be present
    Q_ASSERT(!fileName.isEmpty() || !uri.isEmpty());

    lw.write(u".import").ensureSpace();
    if (!uri.isEmpty()) {
        lw.write(uri).ensureSpace();
        if (!version.isEmpty()) {
            lw.write(version).ensureSpace();
        }
    } else {
        lw.write(u"\"").write(fileName).write(u"\"").ensureSpace();
    }
    lw.writeRegion(fLoc, AsTokenRegion).ensureSpace().write(asIdentifier);

    lw.ensureNewline();
}

/*!
 * \internal JsFile::writeOutDirectives
 * \brief Performs writeOut of the .js Directives (.import, .pragma)
 *
 * Watch out!
 * Currently directives in .js files do not have representative AST::Node-s (see QTBUG-119770),
 * which makes it hard to preserve attached comments during the WriteOut process,
 * because currently they are being attached to the first AST::Node.
 * In case when the first AST::Node is absent, they are not collected, hence lost.
 */
void JsFile::writeOutDirectives(const DomItem &self, OutWriter &ow) const
{
    if (m_pragmaLibrary.has_value()) {
        m_pragmaLibrary->writeOut(self, ow);
    }
    for (const auto &import : m_imports) {
        import.writeOut(self, ow);
    }
}

std::shared_ptr<OwningItem> QmlFile::doCopy(const DomItem &) const
{
    auto res = std::make_shared<QmlFile>(*this);
    return res;
}

/*!
    \class QmlFile

   A QmlFile, when loaded in a DomEnvironment that has the DomCreationOption::WithSemanticAnalysis,
   will be lazily constructed. That means that its member m_lazyMembers is uninitialized, and will
   only be populated when it is accessed (through a getter, a setter or the DomItem interface).

   The reason for the laziness is that the qqmljsscopes are created lazily and at the same time as
   the Dom QmlFile representations. So instead of eagerly generating all qqmljsscopes when
   constructing the Dom, the QmlFile itself becomes lazy and will only be populated on demand at
   the same time as the corresponding qqmljsscopes.

   The QDeferredFactory<QQmlJSScope> will, when the qqmljsscope is populated, take care of
   populating all fields of the QmlFile.
   Therefore, population of the QmlFile is done by populating the qqmljsscope.

*/

QmlFile::QmlFile(
        const QString &filePath, const QString &code, const QDateTime &lastDataUpdateAt,
        int derivedFrom, RecoveryOption option)
    : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::qmlFilePath(filePath), derivedFrom,
                         code),
      m_engine(new QQmlJS::Engine)
{
    QQmlJS::Lexer lexer(m_engine.get());
    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/true);
    QQmlJS::Parser parser(m_engine.get());
    if (option == EnableParserRecovery) {
        parser.setIdentifierInsertionEnabled(true);
        parser.setIncompleteBindingsEnabled(true);
    }
    m_isValid = parser.parse();
    const auto diagnostics = parser.diagnosticMessages();
    for (const DiagnosticMessage &msg : diagnostics) {
        addErrorLocal(
                std::move(myParsingErrors().errorMessage(msg).withFile(filePath).withPath(m_path)));
    }
    m_ast = parser.ast();
}

ErrorGroups QmlFile::myParsingErrors()
{
    static ErrorGroups res = { { DomItem::domErrorGroup, NewErrorGroup("QmlFile"),
                                 NewErrorGroup("Parsing") } };
    return res;
}

bool QmlFile::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    auto &members = lazyMembers();
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.invokeVisitorOnField(visitor, Fields::components, members.m_components);
    cont = cont && self.invokeVisitorOnField(visitor, Fields::pragmas, members.m_pragmas);
    cont = cont && self.invokeVisitorOnField(visitor, Fields::imports, members.m_imports);
    cont = cont && self.invokeVisitorOnField(visitor, Fields::fileLocationsTree,
                                             members.m_fileLocationsTree);
    cont = cont && self.invokeVisitorOnField(visitor, Fields::comments, members.m_comments);
    cont = cont && self.invokeVisitorOnField(visitor, Fields::astComments, members.m_astComments);
    return cont;
}

DomItem QmlFile::field(const DomItem &self, QStringView name) const
{
    ensurePopulated();
    if (name == Fields::components)
        return self.wrap(PathEls::Field(Fields::components), lazyMembers().m_components);
    return DomBase::field(self, name);
}

void QmlFile::addError(const DomItem &self, ErrorMessage &&msg)
{
    m_isValid = false;
    self.containingObject().addError(std::move(msg));
}

void QmlFile::writeOut(const DomItem &self, OutWriter &ow) const
{
    ensurePopulated();
    for (const DomItem &p : self.field(Fields::pragmas).values()) {
        p.writeOut(ow);
    }
    auto imports = self.field(Fields::imports).values();

    if (ow.lineWriter.options().sortImports) {
        std::stable_sort (imports.begin(), imports.end(),
                         [](const DomItem& item1, const DomItem& item2) {
                             const auto uri1 = item1[Fields::uri].toString();
                             const auto uri2 = item2[Fields::uri].toString();
                             return uri1 < uri2;
                         });
    }

    for (const DomItem& i : imports) {
        i.writeOut(ow);
    }
    ow.ensureNewline(2);
    DomItem mainC = self.field(Fields::components).key(QString()).index(0);
    mainC.writeOut(ow);
}

QmlDirectory::QmlDirectory(
        const QString &filePath, const QStringList &dirList, const QDateTime &lastDataUpdateAt,
        int derivedFrom)
    : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::qmlDirectoryPath(filePath), derivedFrom,
                         dirList.join(QLatin1Char('\n')))
{
    for (const QString &f : dirList) {
        addQmlFilePath(f);
    }
}

bool QmlDirectory::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.invokeVisitorOnField(visitor, Fields::exports, m_exports);
    cont = cont && visitor(PathEls::Field(Fields::qmlFiles), [this, &self]() -> DomItem {
                   QDir baseDir(canonicalFilePath());
                   return self.subMapItem(Map(
                           self.pathFromOwner().withField(Fields::qmlFiles),
                           [this, baseDir](const DomItem &map, const QString &key) -> DomItem {
                               QList<Path> res;
                               auto it = m_qmlFiles.find(key);
                               while (it != m_qmlFiles.end() && it.key() == key) {
                                   res.append(Paths::qmlFilePath(
                                           QFileInfo(baseDir.filePath(it.value()))
                                                   .canonicalFilePath()));
                                   ++it;
                               }
                               return map.subReferencesItem(PathEls::Key(key), res);
                           },
                           [this](const DomItem &) {
                               auto keys = m_qmlFiles.keys();
                               return QSet<QString>(keys.begin(), keys.end());
                           },
                           u"List<Reference>"_s));
               });
    return cont;
}

bool QmlDirectory::addQmlFilePath(const QString &relativePath)
{
    static const QRegularExpression qmlFileRegularExpression{
        QRegularExpression::anchoredPattern(
                uR"((?<compName>[a-zA-z0-9_]+)\.(?:qml|qmlannotation|ui\.qml))")
    };
    QRegularExpressionMatch m = qmlFileRegularExpression.match(relativePath);
    if (m.hasMatch() && !m_qmlFiles.values(m.captured(u"compName")).contains(relativePath)) {
        m_qmlFiles.insert(m.captured(u"compName"), relativePath);
        Export e;
        QDir dir(canonicalFilePath());
        QFileInfo fInfo(dir.filePath(relativePath));
        e.exportSourcePath = canonicalPath();
        e.typeName = m.captured(u"compName");
        e.typePath = Paths::qmlFileObjectPath(fInfo.canonicalFilePath());
        e.uri = QLatin1String("file://") + canonicalFilePath();
        m_exports.insert(e.typeName, e);
        return true;
    }
    return false;
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
