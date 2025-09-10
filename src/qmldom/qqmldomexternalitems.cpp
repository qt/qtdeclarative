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

ErrorGroups QmldirFile::myParsingErrors()
{
    static ErrorGroups res = { { DomItem::domErrorGroup, NewErrorGroup("Qmldir"),
                                 NewErrorGroup("Parsing") } };
    return res;
}

std::shared_ptr<QmldirFile> QmldirFile::fromPathAndCode(const QString &path, const QString &code)
{
    QString canonicalFilePath = QFileInfo(path).canonicalFilePath();

    QDateTime dataUpdate = QDateTime::currentDateTimeUtc();
    auto res = std::make_shared<QmldirFile>(canonicalFilePath, code, dataUpdate);

    if (canonicalFilePath.isEmpty() && !path.isEmpty())
        res->addErrorLocal(
                myParsingErrors().error(tr("QmldirFile started from invalid path '%1'").arg(path)));
    res->parse();
    return res;
}

void QmldirFile::parse()
{
    if (canonicalFilePath().isEmpty()) {
        addErrorLocal(myParsingErrors().error(tr("canonicalFilePath is empty")));
        setIsValid(false);
    } else {
        m_qmldir.parse(m_code);
        setFromQmldir();
    }
}

void QmldirFile::setFromQmldir()
{
    m_uri = QmlUri::fromUriString(m_qmldir.typeNamespace());
    if (m_uri.isValid())
        m_uri = QmlUri::fromDirectoryString(canonicalFilePath());
    Path exportsPath = Path::Field(Fields::exports);
    QDir baseDir = QFileInfo(canonicalFilePath()).dir();
    int majorVersion = Version::Undefined;
    bool ok;
    int vNr = QFileInfo(baseDir.dirName()).suffix().toInt(&ok);
    if (ok && vNr > 0) // accept 0?
        majorVersion = vNr;
    Path exportSource = canonicalPath();
    for (auto const &el : m_qmldir.components()) {
        QString exportFilePath = baseDir.filePath(el.fileName);
        QString canonicalExportFilePath = QFileInfo(exportFilePath).canonicalFilePath();
        if (canonicalExportFilePath.isEmpty()) // file does not exist (yet? assuming it might be
                                               // created where we expect it)
            canonicalExportFilePath = exportFilePath;
        Export exp;
        exp.exportSourcePath = exportSource;
        exp.isSingleton = el.singleton;
        exp.isInternal = el.internal;
        exp.version =
                Version((el.version.hasMajorVersion() ? el.version.majorVersion() : majorVersion),
                        el.version.hasMinorVersion() ? el.version.minorVersion() : 0);
        exp.typeName = el.typeName;
        exp.typePath = Paths::qmlFileObjectPath(canonicalExportFilePath);
        exp.uri = uri().toString();
        m_exports.insert(exp.typeName, exp);
        if (exp.version.majorVersion > 0)
            m_majorVersions.insert(exp.version.majorVersion);
    }
    for (auto const &el : m_qmldir.scripts()) {
        QString exportFilePath = baseDir.filePath(el.fileName);
        QString canonicalExportFilePath = QFileInfo(exportFilePath).canonicalFilePath();
        if (canonicalExportFilePath.isEmpty()) // file does not exist (yet? assuming it might be
                                               // created where we expect it)
            canonicalExportFilePath = exportFilePath;
        Export exp;
        exp.exportSourcePath = exportSource;
        exp.isSingleton = true;
        exp.isInternal = false;
        exp.version =
                Version((el.version.hasMajorVersion() ? el.version.majorVersion() : majorVersion),
                        el.version.hasMinorVersion() ? el.version.minorVersion() : 0);
        exp.typePath = Paths::jsFilePath(canonicalExportFilePath).field(Fields::rootComponent);
        exp.uri = uri().toString();
        exp.typeName = el.nameSpace;
        m_exports.insert(exp.typeName, exp);
        if (exp.version.majorVersion > 0)
            m_majorVersions.insert(exp.version.majorVersion);
    }
    for (QQmlDirParser::Import const &imp : m_qmldir.imports()) {
        QString uri = imp.module;
        bool isAutoImport = imp.flags & QQmlDirParser::Import::Auto;
        Version v;
        if (isAutoImport)
            v = Version(majorVersion, int(Version::Latest));
        else {
            v = Version((imp.version.hasMajorVersion() ? imp.version.majorVersion()
                                                       : int(Version::Latest)),
                        (imp.version.hasMinorVersion() ? imp.version.minorVersion()
                                                       : int(Version::Latest)));
        }
        m_imports.append(Import(QmlUri::fromUriString(uri), v));
        m_autoExports.append(
                ModuleAutoExport { Import(QmlUri::fromUriString(uri), v), isAutoImport });
    }
    for (QQmlDirParser::Import const &imp : m_qmldir.dependencies()) {
        QString uri = imp.module;
        if (imp.flags & QQmlDirParser::Import::Auto) {
            qCDebug(QQmlJSDomImporting) << "QmldirFile::setFromQmlDir: ignoring initial version"
                                           " 'auto' in depends command, using latest version"
                                           " instead.";
        }
        Version v = Version(
                (imp.version.hasMajorVersion() ? imp.version.majorVersion() : int(Version::Latest)),
                (imp.version.hasMinorVersion() ? imp.version.minorVersion()
                                               : int(Version::Latest)));
        m_imports.append(Import(QmlUri::fromUriString(uri), v));
    }
    bool hasInvalidTypeinfo = false;
    for (auto const &el : m_qmldir.typeInfos()) {
        QString elStr = el;
        QFileInfo elPath(elStr);
        if (elPath.isRelative())
            elPath = QFileInfo(baseDir.filePath(elStr));
        QString typeInfoPath = elPath.canonicalFilePath();
        if (typeInfoPath.isEmpty()) {
            hasInvalidTypeinfo = true;
            typeInfoPath = elPath.absoluteFilePath();
        }
        m_qmltypesFilePaths.append(Paths::qmltypesFilePath(typeInfoPath));
    }
    if (m_qmltypesFilePaths.isEmpty() || hasInvalidTypeinfo) {
        // add all type info files in the directory...
        for (QFileInfo const &entry :
             baseDir.entryInfoList(QStringList({ QLatin1String("*.qmltypes") }),
                                   QDir::Filter::Readable | QDir::Filter::Files)) {
            Path p = Paths::qmltypesFilePath(entry.canonicalFilePath());
            if (!m_qmltypesFilePaths.contains(p))
                m_qmltypesFilePaths.append(p);
        }
    }
    bool hasErrors = false;
    for (auto const &el : m_qmldir.errors(uri().toString())) {
        ErrorMessage msg = myParsingErrors().errorMessage(el);
        if (msg.level == ErrorLevel::Error || msg.level == ErrorLevel::Fatal)
            hasErrors = true;
        addErrorLocal(std::move(msg));
    }
    setIsValid(!hasErrors); // consider it valid also with errors?
    m_plugins = m_qmldir.plugins();
}

QList<ModuleAutoExport> QmldirFile::autoExports() const
{
    return m_autoExports;
}

void QmldirFile::setAutoExports(const QList<ModuleAutoExport> &autoExport)
{
    m_autoExports = autoExport;
}

void QmldirFile::ensureInModuleIndex(const DomItem &self, const QString &uri) const
{
    // ModuleIndex keeps the various sources of types from a given module uri import
    // this method ensures that all major versions that are contained in this qmldir
    // file actually have a ModuleIndex. This is required so that when importing the
    // latest version the correct "lastest major version" is found, for example for
    // qml only modules (qmltypes files also register their versions)
    DomItem env = self.environment();
    if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>()) {
        for (int majorV : m_majorVersions) {
            auto mIndex = envPtr->moduleIndexWithUri(env, uri, majorV, EnvLookup::Normal,
                                                     Changeable::Writable);
        }
    }
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

bool QmldirFile::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvValueField(visitor, Fields::uri, uri().toString());
    cont = cont && self.dvValueField(visitor, Fields::designerSupported, designerSupported());
    cont = cont && self.dvReferencesField(visitor, Fields::qmltypesFiles, m_qmltypesFilePaths);
    cont = cont && self.dvWrapField(visitor, Fields::exports, m_exports);
    cont = cont && self.dvWrapField(visitor, Fields::imports, m_imports);
    cont = cont && self.dvItemField(visitor, Fields::plugins, [this, &self]() {
        QStringList cNames = classNames();
        return self.subListItem(List::fromQListRef<QQmlDirParser::Plugin>(
                self.pathFromOwner().field(Fields::plugins), m_plugins,
                [cNames](const DomItem &list, const PathEls::PathComponent &p,
                         const QQmlDirParser::Plugin &plugin) {
                    return list.subDataItem(p, pluginData(plugin, cNames));
                }));
    });
    // add qmlfiles as map because this way they are presented the same way as
    // the qmlfiles in a directory
    cont = cont && self.dvItemField(visitor, Fields::qmlFiles, [this, &self]() {
        const QMap<QString, QString> typeFileMap = qmlFiles();
        return self.subMapItem(Map(
                self.pathFromOwner().field(Fields::qmlFiles),
                [typeFileMap](const DomItem &map, const QString &typeV) {
                    QString path = typeFileMap.value(typeV);
                    if (path.isEmpty())
                        return DomItem();
                    else
                        return map.subReferencesItem(
                                PathEls::Key(typeV),
                                QList<Path>({ Paths::qmlFileObjectPath(path) }));
                },
                [typeFileMap](const DomItem &) {
                    return QSet<QString>(typeFileMap.keyBegin(), typeFileMap.keyEnd());
                },
                QStringLiteral(u"QList<Reference>")));
    });
    cont = cont && self.dvWrapField(visitor, Fields::autoExports, m_autoExports);
    return cont;
}

QMap<QString, QString> QmldirFile::qmlFiles() const
{
    // add qmlfiles as map because this way they are presented the same way as
    // the qmlfiles in a directory which gives them as fileName->list of references to files
    // this is done only to ensure that they are loaded as dependencies
    QMap<QString, QString> res;
    for (const auto &e : m_exports)
        res.insert(e.typeName + QStringLiteral(u"-") + e.version.stringValue(),
                   e.typePath[2].headName());
    return res;
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
    cont = cont && self.dvWrapField(visitor, Fields::fileLocationsTree, m_fileLocationsTree);
    if (m_script)
        cont = cont && self.dvItemField(visitor, Fields::expression, [this, &self]() {
            return self.subOwnerItem(PathEls::Field(Fields::expression), m_script);
        });
    return cont;
}

void JsFile::writeOut(const DomItem &self, OutWriter &ow) const
{
    writeOutDirectives(ow);
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

void JsFile::LegacyPragmaLibrary::writeOut(OutWriter &lw) const
{
    lw.write(u".pragma").ensureSpace().write(u"library").ensureNewline();
}

void JsFile::LegacyImport::writeOut(OutWriter &lw) const
{
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
    lw.writeRegion(AsTokenRegion).ensureSpace().write(asIdentifier);

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
void JsFile::writeOutDirectives(OutWriter &ow) const
{
    if (m_pragmaLibrary.has_value()) {
        m_pragmaLibrary->writeOut(ow);
    }
    for (const auto &import : m_imports) {
        import.writeOut(ow);
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
    cont = cont && self.dvWrapField(visitor, Fields::components, members.m_components);
    cont = cont && self.dvWrapField(visitor, Fields::pragmas, members.m_pragmas);
    cont = cont && self.dvWrapField(visitor, Fields::imports, members.m_imports);
    cont = cont && self.dvWrapField(visitor, Fields::importScope, members.m_importScope);
    cont = cont
            && self.dvWrapField(visitor, Fields::fileLocationsTree, members.m_fileLocationsTree);
    cont = cont && self.dvWrapField(visitor, Fields::comments, members.m_comments);
    cont = cont && self.dvWrapField(visitor, Fields::astComments, members.m_astComments);
    return cont;
}

DomItem QmlFile::field(const DomItem &self, QStringView name) const
{
    ensurePopulated();
    if (name == Fields::components)
        return self.wrapField(Fields::components, lazyMembers().m_components);
    return DomBase::field(self, name);
}

void QmlFile::addError(const DomItem &self, ErrorMessage &&msg)
{
    self.containingObject().addError(std::move(msg));
}

void QmlFile::writeOut(const DomItem &self, OutWriter &ow) const
{
    ensurePopulated();
    for (const DomItem &p : self.field(Fields::pragmas).values()) {
        p.writeOut(ow);
    }
    for (auto i : self.field(Fields::imports).values()) {
        i.writeOut(ow);
    }
    ow.ensureNewline(2);
    DomItem mainC = self.field(Fields::components).key(QString()).index(0);
    mainC.writeOut(ow);
}

std::shared_ptr<OwningItem> GlobalScope::doCopy(const DomItem &self) const
{
    auto res = std::make_shared<GlobalScope>(
                canonicalFilePath(self), lastDataUpdateAt(), revision());
    return res;
}

bool GlobalScope::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    return cont;
}

void QmltypesFile::ensureInModuleIndex(const DomItem &self) const
{
    auto it = m_uris.begin();
    auto end = m_uris.end();
    DomItem env = self.environment();
    if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>()) {
        while (it != end) {
            QString uri = it.key();
            for (int majorV : it.value()) {
                auto mIndex = envPtr->moduleIndexWithUri(env, uri, majorV, EnvLookup::Normal,
                                                         Changeable::Writable);
                mIndex->addQmltypeFilePath(self.canonicalPath());
            }
            ++it;
        }
    }
}

bool QmltypesFile::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvWrapField(visitor, Fields::components, m_components);
    cont = cont && self.dvWrapField(visitor, Fields::exports, m_exports);
    cont = cont && self.dvItemField(visitor, Fields::uris, [this, &self]() {
        return self.subMapItem(Map::fromMapRef<QSet<int>>(
                self.pathFromOwner().field(Fields::uris), m_uris,
                [](const DomItem &map, const PathEls::PathComponent &p, const QSet<int> &el) {
                    QList<int> l(el.cbegin(), el.cend());
                    std::sort(l.begin(), l.end());
                    return map.subListItem(
                            List::fromQList<int>(map.pathFromOwner().appendComponent(p), l,
                                                 [](const DomItem &list, const PathEls::PathComponent &p,
                                                    int el) { return list.subDataItem(p, el); }));
                }));
    });
    cont = cont && self.dvWrapField(visitor, Fields::imports, m_imports);
    return cont;
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
    cont = cont && self.dvWrapField(visitor, Fields::exports, m_exports);
    cont = cont && self.dvItemField(visitor, Fields::qmlFiles, [this, &self]() -> DomItem {
        QDir baseDir(canonicalFilePath());
        return self.subMapItem(Map(
                self.pathFromOwner().field(Fields::qmlFiles),
                [this, baseDir](const DomItem &map, const QString &key) -> DomItem {
                    QList<Path> res;
                    auto it = m_qmlFiles.find(key);
                    while (it != m_qmlFiles.end() && it.key() == key) {
                        res.append(Paths::qmlFilePath(
                                QFileInfo(baseDir.filePath(it.value())).canonicalFilePath()));
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
