// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomtop_p.h"
#include "qqmldomoutwriter_p.h"
#include "qqmldomcomments_p.h"
#include "qqmldommock_p.h"
#include "qqmldomelements_p.h"

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtCore/QDir>
#include <QtCore/QScopeGuard>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QQmlJS {
namespace Dom {

ExternalOwningItem::ExternalOwningItem(QString filePath, QDateTime lastDataUpdateAt, Path path,
                                       int derivedFrom, QString code)
    : OwningItem(derivedFrom, lastDataUpdateAt),
      m_canonicalFilePath(filePath),
      m_code(code),
      m_path(path)
{}

QString ExternalOwningItem::canonicalFilePath(DomItem &) const
{
    return m_canonicalFilePath;
}

QString ExternalOwningItem::canonicalFilePath() const
{
    return m_canonicalFilePath;
}

Path ExternalOwningItem::canonicalPath(DomItem &) const
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

std::shared_ptr<QmldirFile> QmldirFile::fromPathAndCode(QString path, QString code)
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
        if (imp.flags & QQmlDirParser::Import::Auto)
            qWarning() << "qmldir contains dependency with auto keyword";
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
        addErrorLocal(msg);
        if (msg.level == ErrorLevel::Error || msg.level == ErrorLevel::Fatal)
            hasErrors = true;
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

void QmldirFile::ensureInModuleIndex(DomItem &self, QString uri)
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

QCborValue pluginData(QQmlDirParser::Plugin &pl, QStringList cNames)
{
    QCborArray names;
    for (QString n : cNames)
        names.append(n);
    return QCborMap({ { QCborValue(QStringView(Fields::name)), pl.name },
                      { QStringView(Fields::path), pl.path },
                      { QStringView(Fields::classNames), names } });
}

bool QmldirFile::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
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
                [cNames](DomItem &list, const PathEls::PathComponent &p,
                         QQmlDirParser::Plugin &plugin) {
                    return list.subDataItem(p, pluginData(plugin, cNames));
                }));
    });
    // add qmlfiles as map because this way they are presented the same way as
    // the qmlfiles in a directory
    cont = cont && self.dvItemField(visitor, Fields::qmlFiles, [this, &self]() {
        const QMap<QString, QString> typeFileMap = qmlFiles();
        return self.subMapItem(Map(
                self.pathFromOwner().field(Fields::qmlFiles),
                [typeFileMap](DomItem &map, QString typeV) {
                    QString path = typeFileMap.value(typeV);
                    if (path.isEmpty())
                        return DomItem();
                    else
                        return map.subReferencesItem(
                                PathEls::Key(typeV),
                                QList<Path>({ Paths::qmlFileObjectPath(path) }));
                },
                [typeFileMap](DomItem &) {
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

std::shared_ptr<OwningItem> QmlFile::doCopy(DomItem &) const
{
    auto res = std::make_shared<QmlFile>(*this);
    return res;
}

QmlFile::QmlFile(const QmlFile &o)
    : ExternalOwningItem(o),
      m_engine(o.m_engine),
      m_ast(o.m_ast),
      m_astComments(o.m_astComments),
      m_comments(o.m_comments),
      m_fileLocationsTree(o.m_fileLocationsTree),
      m_components(o.m_components),
      m_pragmas(o.m_pragmas),
      m_imports(o.m_imports),
      m_importScope(o.m_importScope)
{
    if (m_astComments)
        m_astComments = std::make_shared<AstComments>(*m_astComments);
}

QmlFile::QmlFile(QString filePath, QString code, QDateTime lastDataUpdateAt, int derivedFrom)
    : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::qmlFilePath(filePath), derivedFrom,
                         code),
      m_engine(new QQmlJS::Engine),
      m_astComments(new AstComments(m_engine)),
      m_fileLocationsTree(FileLocations::createTree(canonicalPath()))
{
    QQmlJS::Lexer lexer(m_engine.get());
    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/true);
    QQmlJS::Parser parser(m_engine.get());
    m_isValid = parser.parse();
    for (DiagnosticMessage msg : parser.diagnosticMessages())
        addErrorLocal(myParsingErrors().errorMessage(msg).withFile(filePath).withPath(m_path));
    m_ast = parser.ast();
}

ErrorGroups QmlFile::myParsingErrors()
{
    static ErrorGroups res = { { DomItem::domErrorGroup, NewErrorGroup("QmlFile"),
                                 NewErrorGroup("Parsing") } };
    return res;
}

bool QmlFile::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvWrapField(visitor, Fields::components, m_components);
    cont = cont && self.dvWrapField(visitor, Fields::pragmas, m_pragmas);
    cont = cont && self.dvWrapField(visitor, Fields::imports, m_imports);
    cont = cont && self.dvWrapField(visitor, Fields::importScope, m_importScope);
    cont = cont && self.dvWrapField(visitor, Fields::fileLocationsTree, m_fileLocationsTree);
    cont = cont && self.dvWrapField(visitor, Fields::comments, m_comments);
    cont = cont && self.dvWrapField(visitor, Fields::astComments, m_astComments);
    return cont;
}

DomItem QmlFile::field(DomItem &self, QStringView name)
{
    if (name == Fields::components)
        return self.wrapField(Fields::components, m_components);
    return DomBase::field(self, name);
}

void QmlFile::addError(DomItem &self, ErrorMessage msg)
{
    self.containingObject().addError(msg);
}

void QmlFile::writeOut(DomItem &self, OutWriter &ow) const
{
    for (DomItem &p : self.field(Fields::pragmas).values()) {
        p.writeOut(ow);
    }
    for (auto i : self.field(Fields::imports).values()) {
        i.writeOut(ow);
    }
    ow.ensureNewline(2);
    DomItem mainC = self.field(Fields::components).key(QString()).index(0);
    mainC.writeOut(ow);
}

std::shared_ptr<OwningItem> GlobalScope::doCopy(DomItem &self) const
{
    auto res = std::make_shared<GlobalScope>(
                canonicalFilePath(self), lastDataUpdateAt(), revision());
    return res;
}

bool GlobalScope::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    return cont;
}

void QmltypesFile::ensureInModuleIndex(DomItem &self)
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

bool QmltypesFile::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvWrapField(visitor, Fields::components, m_components);
    cont = cont && self.dvWrapField(visitor, Fields::exports, m_exports);
    cont = cont && self.dvItemField(visitor, Fields::uris, [this, &self]() {
        return self.subMapItem(Map::fromMapRef<QSet<int>>(
                self.pathFromOwner().field(Fields::uris), m_uris,
                [](DomItem &map, const PathEls::PathComponent &p, QSet<int> &el) {
                    QList<int> l(el.cbegin(), el.cend());
                    std::sort(l.begin(), l.end());
                    return map.subListItem(
                            List::fromQList<int>(map.pathFromOwner().appendComponent(p), l,
                                                 [](DomItem &list, const PathEls::PathComponent &p,
                                                    int &el) { return list.subDataItem(p, el); }));
                }));
    });
    cont = cont && self.dvWrapField(visitor, Fields::imports, m_imports);
    return cont;
}

QmlDirectory::QmlDirectory(QString filePath, QStringList dirList, QDateTime lastDataUpdateAt,
                           int derivedFrom)
    : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::qmlDirectoryPath(filePath), derivedFrom,
                         dirList.join(QLatin1Char('\n')))
{
    for (QString f : dirList) {
        addQmlFilePath(f);
    }
}

bool QmlDirectory::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = ExternalOwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvWrapField(visitor, Fields::exports, m_exports);
    cont = cont && self.dvItemField(visitor, Fields::qmlFiles, [this, &self]() -> DomItem {
        QDir baseDir(canonicalFilePath());
        return self.subMapItem(Map(
                self.pathFromOwner().field(Fields::qmlFiles),
                [this, baseDir](DomItem &map, QString key) -> DomItem {
                    QList<Path> res;
                    auto it = m_qmlFiles.find(key);
                    while (it != m_qmlFiles.end() && it.key() == key) {
                        res.append(Paths::qmlFilePath(
                                QFileInfo(baseDir.filePath(it.value())).canonicalFilePath()));
                        ++it;
                    }
                    return map.subReferencesItem(PathEls::Key(key), res);
                },
                [this](DomItem &) {
                    auto keys = m_qmlFiles.keys();
                    return QSet<QString>(keys.begin(), keys.end());
                },
                u"List<Reference>"_s));
    });
    return cont;
}

bool QmlDirectory::addQmlFilePath(QString relativePath)
{
    QRegularExpression qmlFileRe(QRegularExpression::anchoredPattern(
            uR"((?<compName>[a-zA-z0-9_]+)\.(?:qml|qmlannotation))"));
    QRegularExpressionMatch m = qmlFileRe.match(relativePath);
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
