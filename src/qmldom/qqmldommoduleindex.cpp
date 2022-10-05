// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldomtop_p.h"
#include "qqmldomelements_p.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>

#include <memory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

static ErrorGroups myVersioningErrors()
{
    static ErrorGroups res = { { DomItem::domErrorGroup, NewErrorGroup("Exports"),
                                 NewErrorGroup("Version") } };
    return res;
}

static ErrorGroups myExportErrors()
{
    static ErrorGroups res = { { DomItem::domErrorGroup, NewErrorGroup("Exports") } };
    return res;
}

bool ModuleScope::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvValueField(visitor, Fields::uri, uri);
    cont = cont && self.dvWrapField(visitor, Fields::version, version);
    cont = cont && self.dvItemField(visitor, Fields::exports, [this, &self]() {
        int minorVersion = version.minorVersion;
        return self.subMapItem(Map(
                self.pathFromOwner().field(Fields::exports),
                [minorVersion](DomItem &mapExp, QString name) -> DomItem {
                    DomItem mapExpOw = mapExp.owner();
                    QList<DomItem> exports =
                            mapExp.ownerAs<ModuleIndex>()->exportsWithNameAndMinorVersion(
                                    mapExpOw, name, minorVersion);
                    return mapExp.subListItem(List::fromQList<DomItem>(
                            mapExp.pathFromOwner().key(name), exports,
                            [](DomItem &, const PathEls::PathComponent &, DomItem &el) {
                                return el;
                            },
                            ListOptions::Normal));
                },
                [](DomItem &mapExp) {
                    DomItem mapExpOw = mapExp.owner();
                    return mapExp.ownerAs<ModuleIndex>()->exportNames(mapExpOw);
                },
                QLatin1String("List<Exports>")));
    });
    cont = cont && self.dvItemField(visitor, Fields::symbols, [&self]() {
        Path basePath = Path::Current(PathCurrent::Obj).field(Fields::exports);
        return self.subMapItem(Map(
                self.pathFromOwner().field(Fields::symbols),
                [basePath](DomItem &mapExp, QString name) -> DomItem {
                    QList<Path> symb({ basePath.key(name) });
                    return mapExp.subReferencesItem(PathEls::Key(name), symb);
                },
                [](DomItem &mapExp) {
                    DomItem mapExpOw = mapExp.owner();
                    return mapExp.ownerAs<ModuleIndex>()->exportNames(mapExpOw);
                },
                QLatin1String("List<References>")));
    });
    cont = cont && self.dvItemField(visitor, Fields::autoExports, [this, &self]() {
        return containingObject(self).field(Fields::autoExports);
    });
    return cont;
}

std::shared_ptr<OwningItem> ModuleIndex::doCopy(DomItem &) const
{
    return std::make_shared<ModuleIndex>(*this);
}

ModuleIndex::ModuleIndex(const ModuleIndex &o)
    : OwningItem(o), m_uri(o.uri()), m_majorVersion(o.majorVersion())
{
    QMap<int, ModuleScope *> scopes;
    {
        QMutexLocker l2(o.mutex());
        m_qmltypesFilesPaths += o.m_qmltypesFilesPaths;
        m_qmldirPaths += o.m_qmldirPaths;
        m_directoryPaths += o.m_directoryPaths;
        scopes = o.m_moduleScope;
    }
    auto it = scopes.begin();
    auto end = scopes.end();
    while (it != end) {
        ensureMinorVersion((*it)->version.minorVersion);
        ++it;
    }
}

ModuleIndex::~ModuleIndex()
{
    QMap<int, ModuleScope *> scopes;
    {
        QMutexLocker l(mutex());
        scopes = m_moduleScope;
        m_moduleScope.clear();
    }
    auto it = scopes.begin();
    auto end = scopes.end();
    while (it != end) {
        delete *it;
        ++it;
    }
}

bool ModuleIndex::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = self.dvValueField(visitor, Fields::uri, uri());
    cont = cont && self.dvValueField(visitor, Fields::majorVersion, majorVersion());
    cont = cont && self.dvItemField(visitor, Fields::moduleScope, [this, &self]() {
        return self.subMapItem(Map(
                pathFromOwner(self).field(Fields::moduleScope),
                [](DomItem &map, QString minorVersionStr) {
                    bool ok;
                    int minorVersion = minorVersionStr.toInt(&ok);
                    if (minorVersionStr.isEmpty()
                        || minorVersionStr.compare(u"Latest", Qt::CaseInsensitive) == 0)
                        minorVersion = Version::Latest;
                    else if (!ok)
                        return DomItem();
                    return map.copy(map.ownerAs<ModuleIndex>()->ensureMinorVersion(minorVersion));
                },
                [this](DomItem &) {
                    QSet<QString> res;
                    for (int el : minorVersions())
                        if (el >= 0)
                            res.insert(QString::number(el));
                    if (!minorVersions().isEmpty())
                        res.insert(QString());
                    return res;
                },
                QLatin1String("Map<List<Exports>>")));
    });
    cont = cont && self.dvItemField(visitor, Fields::sources, [this, &self]() {
        return self.subReferencesItem(PathEls::Field(Fields::sources), sources());
    });
    cont = cont && self.dvValueLazyField(visitor, Fields::autoExports, [this, &self]() {
        return autoExports(self);
    });
    return cont;
}

QSet<QString> ModuleIndex::exportNames(DomItem &self) const
{
    QSet<QString> res;
    QList<Path> mySources = sources();
    for (int i = 0; i < mySources.size(); ++i) {
        DomItem source = self.path(mySources.at(i));
        res += source.field(Fields::exports).keys();
    }
    return res;
}

QList<DomItem> ModuleIndex::autoExports(DomItem &self) const
{
    QList<DomItem> res;
    Path selfPath = canonicalPath(self).field(Fields::autoExports);
    RefCacheEntry cached = RefCacheEntry::forPath(self, selfPath);
    QList<Path> cachedPaths;
    switch (cached.cached) {
    case RefCacheEntry::Cached::None:
    case RefCacheEntry::Cached::First:
        break;
    case RefCacheEntry::Cached::All:
        cachedPaths += cached.canonicalPaths;
        if (cachedPaths.isEmpty())
            return res;
    }
    DomItem env = self.environment();
    if (!cachedPaths.isEmpty()) {
        bool outdated = false;
        for (Path p : cachedPaths) {
            DomItem newEl = env.path(p);
            if (!newEl) {
                outdated = true;
                qWarning() << "referenceCache outdated, reference at " << selfPath
                           << " leads to invalid path " << p;
                break;
            } else {
                res.append(newEl);
            }
        }
        if (outdated) {
            res.clear();
        } else {
            return res;
        }
    }
    QList<Path> mySources = sources();
    QSet<QString> knownAutoImportUris;
    QList<ModuleAutoExport> knownExports;
    for (Path p : mySources) {
        DomItem autoExports = self.path(p).field(Fields::autoExports);
        for (DomItem i : autoExports.values()) {
            if (const ModuleAutoExport *iPtr = i.as<ModuleAutoExport>()) {
                if (!knownAutoImportUris.contains(iPtr->import.uri.toString())
                    || !knownExports.contains(*iPtr)) {
                    knownAutoImportUris.insert(iPtr->import.uri.toString());
                    knownExports.append(*iPtr);
                    res.append(i);
                    cachedPaths.append(i.canonicalPath());
                }
            }
        }
    }
    RefCacheEntry::addForPath(self, selfPath,
                              RefCacheEntry { RefCacheEntry::Cached::All, cachedPaths });
    return res;
}

QList<DomItem> ModuleIndex::exportsWithNameAndMinorVersion(DomItem &self, QString name,
                                                           int minorVersion) const
{
    Path myPath = Paths::moduleScopePath(uri(), Version(majorVersion(), minorVersion))
                          .field(Fields::exports)
                          .key(name);
    QList<Path> mySources = sources();
    QList<DomItem> res;
    QList<DomItem> undef;
    if (minorVersion < 0)
        minorVersion = std::numeric_limits<int>::max();
    int vNow = Version::Undefined;
    for (int i = 0; i < mySources.size(); ++i) {
        DomItem source = self.path(mySources.at(i));
        DomItem exports = source.field(Fields::exports).key(name);
        int nExports = exports.indexes();
        if (nExports == 0)
            continue;
        for (int j = 0; j < nExports; ++j) {
            DomItem exportItem = exports.index(j);
            if (!exportItem)
                continue;
            Version const *versionPtr = exportItem.field(Fields::version).as<Version>();
            if (versionPtr == nullptr || !versionPtr->isValid()) {
                undef.append(exportItem);
            } else {
                if (majorVersion() < 0)
                    self.addError(myVersioningErrors()
                                          .error(tr("Module %1 (unversioned) has versioned entries "
                                                    "for '%2' from %3")
                                                         .arg(uri(), name,
                                                              source.canonicalPath().toString()))
                                          .withPath(myPath));
                if ((versionPtr->majorVersion == majorVersion()
                     || versionPtr->majorVersion == Version::Undefined)
                    && versionPtr->minorVersion >= vNow
                    && versionPtr->minorVersion <= minorVersion) {
                    if (versionPtr->minorVersion > vNow)
                        res.clear();
                    res.append(exportItem);
                    vNow = versionPtr->minorVersion;
                }
            }
        }
    }
    if (!undef.isEmpty()) {
        if (!res.isEmpty()) {
            self.addError(myVersioningErrors()
                                  .error(tr("Module %1 (major version %2) has versioned and "
                                            "unversioned entries for '%3'")
                                                 .arg(uri(), QString::number(majorVersion()), name))
                                  .withPath(myPath));
            return res + undef;
        } else {
            return undef;
        }
    }
    return res;
}

QList<Path> ModuleIndex::sources() const
{
    QList<Path> res;
    QMutexLocker l(mutex());
    res += m_qmltypesFilesPaths;
    if (!m_qmldirPaths.isEmpty())
        res += m_qmldirPaths.first();
    else if (!m_directoryPaths.isEmpty())
        res += m_directoryPaths.first();
    return res;
}

ModuleScope *ModuleIndex::ensureMinorVersion(int minorVersion)
{
    if (minorVersion < 0)
        minorVersion = Version::Latest;
    {
        QMutexLocker l(mutex());
        auto it = m_moduleScope.find(minorVersion);
        if (it != m_moduleScope.end())
            return *it;
    }
    ModuleScope *res = nullptr;
    ModuleScope *newScope = new ModuleScope(m_uri, Version(majorVersion(), minorVersion));
    auto cleanup = qScopeGuard([&newScope] { delete newScope; });
    {
        QMutexLocker l(mutex());
        auto it = m_moduleScope.find(minorVersion);
        if (it != m_moduleScope.end()) {
            res = *it;
        } else {
            res = newScope;
            newScope = nullptr;
            m_moduleScope.insert(minorVersion, res);
        }
    }
    return res;
}

void ModuleIndex::mergeWith(std::shared_ptr<ModuleIndex> o)
{
    if (o) {
        QList<Path> qmltypesPaths;
        QMap<int, ModuleScope *> scopes;
        {
            QMutexLocker l2(o->mutex());
            qmltypesPaths = o->m_qmltypesFilesPaths;
            scopes = o->m_moduleScope;
        }
        {
            QMutexLocker l(mutex());
            for (Path qttPath : qmltypesPaths) {
                if (!m_qmltypesFilesPaths.contains((qttPath)))
                    m_qmltypesFilesPaths.append(qttPath);
            }
        }
        auto it = scopes.begin();
        auto end = scopes.end();
        while (it != end) {
            ensureMinorVersion((*it)->version.minorVersion);
            ++it;
        }
    }
}

QList<Path> ModuleIndex::qmldirsToLoad(DomItem &self)
{
    // this always checks the filesystem to the qmldir file to load
    DomItem env = self.environment();
    std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>();
    QStringList subPathComponents = uri().split(u'.');
    QString subPath = subPathComponents.join(u'/');
    QString logicalPath;
    QString subPathV = subPath + QChar::fromLatin1('.') + QString::number(majorVersion())
            + QLatin1String("/qmldir");
    QString dirPath;
    if (majorVersion() >= 0) {
        for (QString path : envPtr->loadPaths()) {
            QDir dir(path);
            QFileInfo fInfo(dir.filePath(subPathV));
            if (fInfo.isFile()) {
                logicalPath = subPathV;
                dirPath = fInfo.canonicalFilePath();
                break;
            }
        }
    }
    if (dirPath.isEmpty()) {
        for (QString path : envPtr->loadPaths()) {
            QDir dir(path);
            QFileInfo fInfo(dir.filePath(subPath + QLatin1String("/qmldir")));
            if (fInfo.isFile()) {
                logicalPath = subPath + QLatin1String("/qmldir");
                dirPath = fInfo.canonicalFilePath();
                break;
            }
        }
    }
    if (!dirPath.isEmpty()) {
        QMutexLocker l(mutex());
        m_qmldirPaths = QList<Path>({ Paths::qmldirFilePath(dirPath) });
    } else if (uri() != u"QML") {
        addErrorLocal(myExportErrors()
                              .warning(tr("Failed to find main qmldir file for %1 %2")
                                               .arg(uri(), QString::number(majorVersion())))
                              .handle());
    }
    return qmldirPaths();
}

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
