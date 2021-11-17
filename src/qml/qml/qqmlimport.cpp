/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlimport_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtQml/qqmlfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qreadwritelock.h>
#include <QtQml/qqmlextensioninterface.h>
#include <QtQml/qqmlextensionplugin.h>
#include <private/qqmlextensionplugin_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qqmlengine_p.h>
#include <private/qfieldlist_p.h>
#include <private/qqmltypemodule_p.h>
#include <private/qqmltypeloaderqmldircontent_p.h>
#include <private/qqmlpluginimporter_p.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>
#include <QtQml/private/qqmltype_p_p.h>
#include <QtQml/private/qqmlimportresolver_p.h>

#include <algorithm>
#include <functional>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlImportTrace, QML_IMPORT_TRACE)

class QmlImportCategoryHolder
{
    Q_DISABLE_COPY_MOVE(QmlImportCategoryHolder);
public:

    QmlImportCategoryHolder() : m_category("qt.qml.import")
    {
        // We have to explicitly setEnabled() here because for categories that start with
        // "qt." it won't accept QtDebugMsg as argument. Debug messages are off by default
        // for all Qt logging categories.
        if (qmlImportTrace())
            m_category.setEnabled(QtDebugMsg, true);
    }

    ~QmlImportCategoryHolder() = default;

    const QLoggingCategory &category() const { return m_category; }

private:
    QLoggingCategory m_category;
};

const QLoggingCategory &lcQmlImport()
{
    static const QmlImportCategoryHolder holder;
    return holder.category();
}

DEFINE_BOOL_CONFIG_OPTION(qmlCheckTypes, QML_CHECK_TYPES)

static const QLatin1Char Dot('.');
static const QLatin1Char Slash('/');
static const QLatin1Char Backslash('\\');
static const QLatin1Char Colon(':');
static const QLatin1String Slash_qmldir("/qmldir");
static const QLatin1String String_qmldir("qmldir");
static const QString dotqml_string(QStringLiteral(".qml"));
static const QString dotuidotqml_string(QStringLiteral(".ui.qml"));
static bool designerSupportRequired = false;

namespace {

QTypeRevision relevantVersion(const QString &uri, QTypeRevision version)
{
    return QQmlMetaType::latestModuleVersion(uri).isValid() ? version : QTypeRevision();
}

QTypeRevision validVersion(QTypeRevision version = QTypeRevision())
{
    // If the given version is invalid, return a valid but useless version to signal "It's OK".
    return version.isValid() ? version : QTypeRevision::fromMinorVersion(0);
}

QQmlError moduleNotFoundError(const QString &uri, QTypeRevision version)
{
    QQmlError error;
    if (version.hasMajorVersion()) {
        error.setDescription(QQmlImportDatabase::tr(
                                 "module \"%1\" version %2.%3 is not installed")
                             .arg(uri).arg(version.majorVersion())
                             .arg(version.hasMinorVersion()
                                  ? QString::number(version.minorVersion())
                                  : QLatin1String("x")));
    } else {
        error.setDescription(QQmlImportDatabase::tr("module \"%1\" is not installed")
                             .arg(uri));
    }
    return error;
}

QString resolveLocalUrl(const QString &url, const QString &relative)
{
    if (relative.contains(Colon)) {
        // contains a host name
        return QUrl(url).resolved(QUrl(relative)).toString();
    } else if (relative.isEmpty()) {
        return url;
    } else if (relative.at(0) == Slash || !url.contains(Slash)) {
        return relative;
    } else {
        const QStringView baseRef = QStringView{url}.left(url.lastIndexOf(Slash) + 1);
        if (relative == QLatin1String("."))
            return baseRef.toString();

        QString base = baseRef + relative;

        // Remove any relative directory elements in the path
        int length = base.length();
        int index = 0;
        while ((index = base.indexOf(QLatin1String("/."), index)) != -1) {
            if ((length > (index + 2)) && (base.at(index + 2) == Dot) &&
                (length == (index + 3) || (base.at(index + 3) == Slash))) {
                // Either "/../" or "/..<END>"
                int previous = base.lastIndexOf(Slash, index - 1);
                if (previous == -1)
                    break;

                int removeLength = (index - previous) + 3;
                base.remove(previous + 1, removeLength);
                length -= removeLength;
                index = previous;
            } else if ((length == (index + 2)) || (base.at(index + 2) == Slash)) {
                // Either "/./" or "/.<END>"
                base.remove(index, 2);
                length -= 2;
            } else {
                ++index;
            }
        }

        return base;
    }
}

bool isPathAbsolute(const QString &path)
{
#if defined(Q_OS_UNIX)
    return (path.at(0) == Slash);
#else
    QFileInfo fi(path);
    return fi.isAbsolute();
#endif
}

} // namespace

/*!
    \internal
    \class QQmlImportInstance

    A QQmlImportType represents a single import of a document, held within a
    namespace.

    \note The uri here may not necessarily be unique (e.g. for file imports).

    \note Version numbers may be -1 for file imports: this means that no
    version was specified as part of the import. Type resolution will be
    responsible for attempting to find the "best" possible version.
*/

/*!
    \internal
    \class QQmlImportNamespace

    A QQmlImportNamespace is a way of seperating imports into a local namespace.

    Within a QML document, there is at least one namespace (the
    "unqualified set") where imports without a qualifier are placed, i.e:

        import QtQuick 2.6

    will have a single namespace (the unqualified set) containing a single import
    for QtQuick 2.6. However, there may be others if an import statement gives
    a qualifier, i.e the following will result in an additional new
    QQmlImportNamespace in the qualified set:

        import MyFoo 1.0 as Foo
*/

class QQmlImportsPrivate
{
public:
    QQmlImportsPrivate(QQmlTypeLoader *loader);
    ~QQmlImportsPrivate();

    QQmlImportNamespace *importNamespace(const QString &prefix) const;

    QTypeRevision addLibraryImport(
            const QString& uri, const QString &prefix, QTypeRevision version,
            const QString &qmldirIdentifier, const QString &qmldirUrl, uint flags,
            QQmlImportDatabase *database, QList<QQmlError> *errors);

    QTypeRevision addFileImport(
            const QString &uri, const QString &prefix, QTypeRevision version, uint flags,
            QQmlImportDatabase *database, QList<QQmlError> *errors);

    QTypeRevision updateQmldirContent(const QString &uri, const QString &prefix,
                             const QString &qmldirIdentifier, const QString& qmldirUrl,
                             QQmlImportDatabase *database,
                             QList<QQmlError> *errors);

    bool resolveType(const QHashedStringRef &type, QTypeRevision *version_return,
                     QQmlType *type_return, QList<QQmlError> *errors,
                     QQmlType::RegistrationType registrationType,
                     bool *typeRecursionDetected = nullptr);

    QUrl baseUrl;
    QString base;
    int ref;

    // storage of data related to imports without a namespace
    mutable QQmlImportNamespace unqualifiedset;

    QQmlImportNamespace *findQualifiedNamespace(const QHashedStringRef &) const;

    // storage of data related to imports with a namespace
    mutable QFieldList<QQmlImportNamespace, &QQmlImportNamespace::nextNamespace> qualifiedSets;

    QQmlTypeLoader *typeLoader;

    static QTypeRevision matchingQmldirVersion(
            const QQmlTypeLoaderQmldirContent &qmldir, const QString &uri,
            QTypeRevision version, QList<QQmlError> *errors);

    QTypeRevision importExtension(
            const QString &uri, QTypeRevision version, QQmlImportDatabase *database,
            const QQmlTypeLoaderQmldirContent *qmldir, QList<QQmlError> *errors);

    bool getQmldirContent(const QString &qmldirIdentifier, const QString &uri,
                          QQmlTypeLoaderQmldirContent *qmldir, QList<QQmlError> *errors);

    QString resolvedUri(const QString &dir_arg, QQmlImportDatabase *database);

    QQmlImportInstance *addImportToNamespace(QQmlImportNamespace *nameSpace, const QString &uri,
                                             const QString &url, QTypeRevision version,
                                             QV4::CompiledData::Import::ImportType type,
                                             QList<QQmlError> *errors, uint flags);
};

/*!
\class QQmlImports
\brief The QQmlImports class encapsulates one QML document's import statements.
\internal
*/
QQmlImports::QQmlImports(const QQmlImports &copy)
: d(copy.d)
{
    ++d->ref;
}

QQmlImports &
QQmlImports::operator =(const QQmlImports &copy)
{
    ++copy.d->ref;
    if (--d->ref == 0)
        delete d;
    d = copy.d;
    return *this;
}

QQmlImports::QQmlImports(QQmlTypeLoader *typeLoader)
    : d(new QQmlImportsPrivate(typeLoader))
{
}

QQmlImports::~QQmlImports()
{
    if (--d->ref == 0)
        delete d;
}

/*!
  Sets the base URL to be used for all relative file imports added.
*/
void QQmlImports::setBaseUrl(const QUrl& url, const QString &urlString)
{
    d->baseUrl = url;

    if (urlString.isEmpty()) {
        d->base = url.toString();
    } else {
        //Q_ASSERT(url.toString() == urlString);
        d->base = urlString;
    }
}

/*!
  Returns the base URL to be used for all relative file imports added.
*/
QUrl QQmlImports::baseUrl() const
{
    return d->baseUrl;
}

/*
    \internal

    This method is responsible for populating data of all types visible in this
    document's imports into the \a cache for resolution elsewhere (e.g. in JS,
    or when loading additional types).

    \note This is for C++ types only. Composite types are handled separately,
    as they do not have a QQmlTypeModule.
*/
void QQmlImports::populateCache(QQmlTypeNameCache *cache) const
{
    const QQmlImportNamespace &set = d->unqualifiedset;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportInstance *import = set.imports.at(ii);
        QQmlTypeModule *module = QQmlMetaType::typeModule(import->uri, import->version);
        if (module) {
            cache->m_anonymousImports.append(QQmlTypeModuleVersion(module, import->version));
        }
    }

    for (QQmlImportNamespace *ns = d->qualifiedSets.first(); ns; ns = d->qualifiedSets.next(ns)) {

        const QQmlImportNamespace &set = *ns;

        // positioning is important; we must create the namespace even if there is no module.
        QQmlImportRef &typeimport = cache->m_namedImports[set.prefix];
        typeimport.m_qualifier = set.prefix;

        for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
            const QQmlImportInstance *import = set.imports.at(ii);
            QQmlTypeModule *module = QQmlMetaType::typeModule(import->uri, import->version);
            if (module) {
                QQmlImportRef &typeimport = cache->m_namedImports[set.prefix];
                typeimport.modules.append(QQmlTypeModuleVersion(module, import->version));
            }
        }
    }
}

// We need to exclude the entry for the current baseUrl. This can happen for example
// when handling qmldir files on the remote dir case and the current type is marked as
// singleton.
bool excludeBaseUrl(const QString &importUrl, const QString &fileName, const QString &baseUrl)
{
    if (importUrl.isEmpty())
        return false;

    if (baseUrl.startsWith(importUrl))
    {
        if (fileName == QStringView{baseUrl}.mid(importUrl.size()))
            return false;
    }

    return true;
}

void findCompositeSingletons(const QQmlImportNamespace &set, QList<QQmlImports::CompositeSingletonReference> &resultList, const QUrl &baseUrl)
{
    typedef QQmlDirComponents::const_iterator ConstIterator;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportInstance *import = set.imports.at(ii);

        const QQmlDirComponents &components = import->qmlDirComponents;

        const QTypeRevision importVersion = import->version;
        auto shouldSkipSingleton = [importVersion](QTypeRevision singletonVersion) -> bool {
            return importVersion.hasMajorVersion() &&
                    (singletonVersion.majorVersion() > importVersion.majorVersion()
                     || (singletonVersion.majorVersion() == importVersion.majorVersion()
                         && singletonVersion.minorVersion() > importVersion.minorVersion()));
        };

        ConstIterator cend = components.constEnd();
        for (ConstIterator cit = components.constBegin(); cit != cend; ++cit) {
            if (cit->singleton && excludeBaseUrl(import->url, cit->fileName, baseUrl.toString())) {
                if (shouldSkipSingleton(cit->version))
                    continue;
                QQmlImports::CompositeSingletonReference ref;
                ref.typeName = cit->typeName;
                ref.prefix = set.prefix;
                ref.version = cit->version;
                resultList.append(ref);
            }
        }

        if (QQmlTypeModule *module = QQmlMetaType::typeModule(import->uri, import->version)) {
            module->walkCompositeSingletons([&resultList, &set, &shouldSkipSingleton](const QQmlType &singleton) {
                if (shouldSkipSingleton(singleton.version()))
                    return;
                QQmlImports::CompositeSingletonReference ref;
                ref.typeName = singleton.elementName();
                ref.prefix = set.prefix;
                ref.version = singleton.version();
                resultList.append(ref);
            });
        }
    }
}

/*
    \internal

    Returns a list of all composite singletons present in this document's
    imports.

    This information is used by QQmlTypeLoader to ensure that composite singletons
    are marked as dependencies during type loading.
*/
QList<QQmlImports::CompositeSingletonReference> QQmlImports::resolvedCompositeSingletons() const
{
    QList<QQmlImports::CompositeSingletonReference> compositeSingletons;

    const QQmlImportNamespace &set = d->unqualifiedset;
    findCompositeSingletons(set, compositeSingletons, baseUrl());

    for (QQmlImportNamespace *ns = d->qualifiedSets.first(); ns; ns = d->qualifiedSets.next(ns)) {
        const QQmlImportNamespace &set = *ns;
        findCompositeSingletons(set, compositeSingletons, baseUrl());
    }

    std::stable_sort(compositeSingletons.begin(), compositeSingletons.end(),
                     [](const QQmlImports::CompositeSingletonReference &lhs,
                        const QQmlImports::CompositeSingletonReference &rhs) {
        if (lhs.prefix != rhs.prefix)
            return lhs.prefix < rhs.prefix;

        if (lhs.typeName != rhs.typeName)
            return lhs.typeName < rhs.typeName;

        return lhs.version.majorVersion() != rhs.version.majorVersion()
            ? lhs.version.majorVersion() < rhs.version.majorVersion()
            : lhs.version.minorVersion() < rhs.version.minorVersion();
    });

    return compositeSingletons;
}

/*
    \internal

    Returns a list of scripts imported by this document. This is used by
    QQmlTypeLoader to properly handle dependencies on imported scripts.
*/
QList<QQmlImports::ScriptReference> QQmlImports::resolvedScripts() const
{
    QList<QQmlImports::ScriptReference> scripts;

    const QQmlImportNamespace &set = d->unqualifiedset;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportInstance *import = set.imports.at(ii);

        for (const QQmlDirParser::Script &script : import->qmlDirScripts) {
            ScriptReference ref;
            ref.nameSpace = script.nameSpace;
            ref.location = QUrl(import->url).resolved(QUrl(script.fileName));
            scripts.append(ref);
        }
    }

    for (QQmlImportNamespace *ns = d->qualifiedSets.first(); ns; ns = d->qualifiedSets.next(ns)) {
        const QQmlImportNamespace &set = *ns;

        for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
            const QQmlImportInstance *import = set.imports.at(ii);

            for (const QQmlDirParser::Script &script : import->qmlDirScripts) {
                ScriptReference ref;
                ref.nameSpace = script.nameSpace;
                ref.qualifier = set.prefix;
                ref.location = QUrl(import->url).resolved(QUrl(script.fileName));
                scripts.append(ref);
            }
        }
    }

    return scripts;
}

/*!
    Forms complete paths to a qmldir file, from a base URL, a module URI and version specification.

    For example, QtQml.Models 2.0:
    - base/QtQml/Models.2.0/qmldir
    - base/QtQml.2.0/Models/qmldir
    - base/QtQml/Models.2/qmldir
    - base/QtQml.2/Models/qmldir
    - base/QtQml/Models/qmldir
*/
QStringList QQmlImports::completeQmldirPaths(const QString &uri, const QStringList &basePaths,
                                             QTypeRevision version)
{
    QStringList paths = qQmlResolveImportPaths(uri, basePaths, version);
    for (QString &path : paths)
        path += Slash_qmldir;
    return paths;
}

QString QQmlImports::versionString(QTypeRevision version, ImportVersion versionMode)
{
    if (versionMode == QQmlImports::FullyVersioned) {
        // extension with fully encoded version number (eg. MyModule.3.2)
        return QString::asprintf(".%d.%d", version.majorVersion(), version.minorVersion());
    } else if (versionMode == QQmlImports::PartiallyVersioned) {
        // extension with encoded version major (eg. MyModule.3)
        return QString::asprintf(".%d", version.majorVersion());
    } // else extension without version number (eg. MyModule)
    return QString();
}

/*!
  \internal

  The given (namespace qualified) \a type is resolved to either
  \list
  \li a QQmlImportNamespace stored at \a ns_return, or
  \li a QQmlType stored at \a type_return,
  \endlist

  If any return pointer is 0, the corresponding search is not done.

  \sa addFileImport(), addLibraryImport
*/
bool QQmlImports::resolveType(const QHashedStringRef &type,
                              QQmlType *type_return, QTypeRevision *version_return,
                              QQmlImportNamespace** ns_return, QList<QQmlError> *errors,
                              QQmlType::RegistrationType registrationType,
                              bool *typeRecursionDetected) const
{
    QQmlImportNamespace* ns = d->findQualifiedNamespace(type);
    if (ns) {
        if (ns_return)
            *ns_return = ns;
        return true;
    }
    if (type_return) {
        if (d->resolveType(type, version_return, type_return, errors, registrationType,
                           typeRecursionDetected)) {
            if (lcQmlImport().isDebugEnabled()) {
#define RESOLVE_TYPE_DEBUG qCDebug(lcQmlImport) \
                << "resolveType:" << qPrintable(baseUrl().toString()) << type.toString() << " => "

                if (type_return && type_return->isValid()) {
                    if (type_return->isCompositeSingleton())
                        RESOLVE_TYPE_DEBUG << type_return->typeName() << ' ' << type_return->sourceUrl() << " TYPE/URL-SINGLETON";
                    else if (type_return->isComposite())
                        RESOLVE_TYPE_DEBUG << type_return->typeName() << ' ' << type_return->sourceUrl() << " TYPE/URL";
                    else if (type_return->isInlineComponentType())
                        RESOLVE_TYPE_DEBUG << type_return->typeName() << ' ' << type_return->sourceUrl() << " TYPE(INLINECOMPONENT)";
                    else
                        RESOLVE_TYPE_DEBUG << type_return->typeName() << " TYPE";
                }
#undef RESOLVE_TYPE_DEBUG
            }
            return true;
        }
    }
    return false;
}

bool QQmlImportInstance::setQmldirContent(const QString &resolvedUrl,
                                          const QQmlTypeLoaderQmldirContent &qmldir,
                                          QQmlImportNamespace *nameSpace, QList<QQmlError> *errors)
{

    const QString preferredPath = qmldir.preferredPath();
    if (preferredPath.isEmpty()) {
        Q_ASSERT(resolvedUrl.endsWith(Slash));
        url = resolvedUrl;
    } else {
        Q_ASSERT(preferredPath.endsWith(Slash));
        if (preferredPath.startsWith(u':'))
            url = QStringLiteral("qrc") + preferredPath;
        else
            url = QUrl::fromLocalFile(preferredPath).toString();
    }

    qmlDirComponents = qmldir.components();

    const QQmlDirScripts &scripts = qmldir.scripts();
    if (!scripts.isEmpty()) {
        // Verify that we haven't imported these scripts already
        for (QList<QQmlImportInstance *>::const_iterator it = nameSpace->imports.constBegin();
             it != nameSpace->imports.constEnd(); ++it) {
            if ((*it != this) && ((*it)->uri == uri)) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("\"%1\" is ambiguous. Found in %2 and in %3").arg(uri).arg(url).arg((*it)->url));
                errors->prepend(error);
                return false;
            }
        }

        qmlDirScripts = getVersionedScripts(scripts, version);
    }

    return true;
}

QQmlDirScripts QQmlImportInstance::getVersionedScripts(const QQmlDirScripts &qmldirscripts,
                                                       QTypeRevision version)
{
    QMap<QString, QQmlDirParser::Script> versioned;

    for (QList<QQmlDirParser::Script>::const_iterator sit = qmldirscripts.constBegin();
         sit != qmldirscripts.constEnd(); ++sit) {
        // Only include scripts that match our requested version
        if ((!version.hasMajorVersion() || (sit->version.majorVersion() == version.majorVersion()))
                && (!version.hasMinorVersion()
                    || (sit->version.minorVersion() <= version.minorVersion()))) {
            // Load the highest version that matches
            QMap<QString, QQmlDirParser::Script>::iterator vit = versioned.find(sit->nameSpace);
            if (vit == versioned.end()
                    || (vit->version.minorVersion() < sit->version.minorVersion())) {
                versioned.insert(sit->nameSpace, *sit);
            }
        }
    }

    return versioned.values();
}

/*!
  \internal

  Searching \e only in the namespace \a ns (previously returned in a call to
  resolveType(), \a type is found and returned to
  a QQmlType stored at \a type_return. If the type is from a QML file, the returned
  type will be a CompositeType.

  If the return pointer is 0, the corresponding search is not done.
*/
bool QQmlImports::resolveType(QQmlImportNamespace *ns, const QHashedStringRef &type,
                              QQmlType *type_return, QTypeRevision *version_return,
                              QQmlType::RegistrationType registrationType) const
{
    return ns->resolveType(d->typeLoader, type, version_return, type_return, nullptr, nullptr,
                           registrationType);
}

bool QQmlImportInstance::resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef& type,
                                     QTypeRevision *version_return, QQmlType *type_return,
                                     QString *base, bool *typeRecursionDetected,
                                     QQmlType::RegistrationType registrationType,
                                     QQmlImport::RecursionRestriction recursionRestriction,
                                     QList<QQmlError> *errors) const
{
    QQmlType t = QQmlMetaType::qmlType(type, uri, version);
    if (t.isValid()) {
        if (version_return)
            *version_return = version;
        if (type_return)
            *type_return = t;
        return true;
    }

    const QString typeStr = type.toString();
    if (isInlineComponent) {
        Q_ASSERT(type_return);
        bool ret = uri == typeStr;
        if (ret) {
            Q_ASSERT(!type_return->isValid());
            auto createICType = [&]() {
                auto typePriv = new QQmlTypePrivate {QQmlType::RegistrationType::InlineComponentType};
                bool ok = false;
                typePriv->extraData.id->objectId = QUrl(this->url).fragment().toInt(&ok);
                Q_ASSERT(ok);
                typePriv->extraData.id->url = QUrl(this->url);
                auto icType = QQmlType(typePriv);
                typePriv->release();
                return icType;
            };
            if (containingType.isValid()) {
                // we currently cannot reference a Singleton inside itself
                // in that case, containingType is still invalid
                if (int icID = containingType.lookupInlineComponentIdByName(typeStr); icID != -1) {
                    *type_return = containingType.lookupInlineComponentById(icID);
                } else {
                    auto icType = createICType();
                    int placeholderId = containingType.generatePlaceHolderICId();
                    const_cast<QQmlImportInstance*>(this)->containingType.associateInlineComponent(typeStr, placeholderId, CompositeMetaTypeIds {}, icType);
                    *type_return = QQmlType(icType);
                }
            } else  {
                *type_return = createICType();
            }
        }
        return ret;
    }
    QQmlDirComponents::ConstIterator it = qmlDirComponents.find(typeStr), end = qmlDirComponents.end();
    if (it != end) {
        QString componentUrl;
        bool isCompositeSingleton = false;
        QQmlDirComponents::ConstIterator candidate = end;
        for ( ; it != end && it.key() == typeStr; ++it) {
            const QQmlDirParser::Component &c = *it;
            switch (registrationType) {
            case QQmlType::AnyRegistrationType:
                break;
            case QQmlType::CompositeSingletonType:
                if (!c.singleton)
                    continue;
                break;
            default:
                if (c.singleton)
                    continue;
                break;
            }

            // importing invalid version means import ALL versions
            if (!version.hasMajorVersion() || (implicitlyImported && c.internal)
                    // allow the implicit import of internal types
                    || (c.version.majorVersion() == version.majorVersion()
                        && c.version.minorVersion() <= version.minorVersion())) {
                // Is this better than the previous candidate?
                if ((candidate == end)
                        || (c.version.majorVersion() > candidate->version.majorVersion())
                        || ((c.version.majorVersion() == candidate->version.majorVersion())
                            && (c.version.minorVersion() > candidate->version.minorVersion()))) {
                    if (base) {
                        componentUrl = resolveLocalUrl(QString(url + c.typeName + dotqml_string), c.fileName);
                        if (c.internal) {
                            if (resolveLocalUrl(*base, c.fileName) != componentUrl)
                                continue; // failed attempt to access an internal type
                        }

                        const bool recursion = *base == componentUrl;
                        if (typeRecursionDetected)
                            *typeRecursionDetected = recursion;

                        if (recursionRestriction == QQmlImport::PreventRecursion && recursion) {
                            continue; // no recursion
                        }
                    }

                    // This is our best candidate so far
                    candidate = it;
                    isCompositeSingleton = c.singleton;
                }
            }
        }

        if (candidate != end) {
            if (!base) // ensure we have a componentUrl
                componentUrl = resolveLocalUrl(QString(url + candidate->typeName + dotqml_string), candidate->fileName);
            QQmlType returnType = QQmlMetaType::typeForUrl(componentUrl, type, isCompositeSingleton,
                                                           nullptr, candidate->version);
            if (version_return)
                *version_return = candidate->version;
            if (type_return)
                *type_return = returnType;
            return returnType.isValid();
        }
    } else if (!isLibrary) {
        // the base path of the import if it's a local file
        const QString localDirectoryPath = QQmlFile::urlToLocalFileOrQrc(url);
        if (localDirectoryPath.isEmpty())
            return false;

        QString qmlUrl;
        bool exists = false;

        const QString urlsToTry[2] = {
            typeStr + dotqml_string, // Type -> Type.qml
            typeStr + dotuidotqml_string // Type -> Type.ui.qml
        };
        for (const QString &urlToTry : urlsToTry) {
            exists = typeLoader->fileExists(localDirectoryPath, urlToTry);
            if (exists) {
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
                // don't let function.qml confuse the use of "new Function(...)" for example.
                if (!QQml_isFileCaseCorrect(localDirectoryPath + urlToTry)) {
                    exists = false;
                    if (errors) {
                        QQmlError caseError;
                        caseError.setDescription(QLatin1String("File name case mismatch"));
                        errors->append(caseError);
                    }
                    break;
                }
#else
                Q_UNUSED(errors);
#endif
                qmlUrl = url + urlToTry;
                break;
            }
        }

        if (exists) {
            const bool recursion = base && *base == qmlUrl;
            if (typeRecursionDetected)
                *typeRecursionDetected = recursion;
            if (recursionRestriction == QQmlImport::AllowRecursion || !recursion) {
                QQmlType returnType = QQmlMetaType::typeForUrl(
                        qmlUrl, type, registrationType == QQmlType::CompositeSingletonType, errors);
                if (type_return)
                    *type_return = returnType;
                return returnType.isValid();
            }
        }
    }

    return false;
}

bool QQmlImportsPrivate::resolveType(const QHashedStringRef& type, QTypeRevision *version_return,
                                     QQmlType *type_return, QList<QQmlError> *errors,
                                     QQmlType::RegistrationType registrationType,
                                     bool *typeRecursionDetected)
{
    const QVector<QHashedStringRef> splitName = type.split(Dot);
    auto resolveTypeInNamespace = [&](QHashedStringRef unqualifiedtype, QQmlImportNamespace *nameSpace, QList<QQmlError> *errors) -> bool {
        if (nameSpace->resolveType(typeLoader, unqualifiedtype,  version_return, type_return, &base, errors,
                           registrationType, typeRecursionDetected))
            return true;
        if (nameSpace->imports.count() == 1 && !nameSpace->imports.at(0)->isLibrary && type_return && nameSpace != &unqualifiedset) {
            // qualified, and only 1 url
            *type_return = QQmlMetaType::typeForUrl(
                    resolveLocalUrl(nameSpace->imports.at(0)->url,
                                    unqualifiedtype.toString() + QLatin1String(".qml")),
                    type, false, errors);
            return type_return->isValid();
        }
        return false;
    };
    switch (splitName.size()) {
    case 1: {
        // must be a simple type
        return resolveTypeInNamespace(type, &unqualifiedset, errors);
    }
    case 2: {
        // either namespace + simple type OR simple type + inline component
        QQmlImportNamespace *s = findQualifiedNamespace(splitName.at(0));
        if (s) {
            // namespace + simple type
            return resolveTypeInNamespace(splitName.at(1), s, errors);
        } else {
            if (resolveTypeInNamespace(splitName.at(0), &unqualifiedset, nullptr)) {
                // either simple type + inline component
                auto const icName = splitName.at(1).toString();
                auto objectIndex = type_return->lookupInlineComponentIdByName(icName);
                if (objectIndex != -1) {
                    *type_return = type_return->lookupInlineComponentById(objectIndex);
                } else {
                    auto icTypePriv = new QQmlTypePrivate(QQmlType::RegistrationType::InlineComponentType);
                    icTypePriv->setContainingType(type_return);
                    icTypePriv->extraData.id->url = type_return->sourceUrl();
                    int placeholderId = type_return->generatePlaceHolderICId();
                    icTypePriv->extraData.id->url.setFragment(QString::number(placeholderId));
                    auto icType = QQmlType(icTypePriv);
                    icTypePriv->release();
                    type_return->associateInlineComponent(icName, placeholderId, CompositeMetaTypeIds {}, icType);
                    *type_return = icType;
                }
                Q_ASSERT(type_return->containingType().isValid());
                type_return->setPendingResolutionName(icName);
                return true;
            } else {
                // or a failure
                if (errors) {
                    QQmlError error;
                    error.setDescription(QQmlImportDatabase::tr("- %1 is neither a type nor a namespace").arg(splitName.at(0).toString()));
                    errors->prepend(error);
                }
                return false;
            }
        }
    }
    case 3: {
        // must be namespace + simple type + inline component
        QQmlImportNamespace *s = findQualifiedNamespace(splitName.at(0));
        QQmlError error;
        if (!s) {
            error.setDescription(QQmlImportDatabase::tr("- %1 is not a namespace").arg(splitName.at(0).toString()));
        } else {
            if (resolveTypeInNamespace(splitName.at(1), s, nullptr)) {
                auto const icName = splitName.at(2).toString();
                auto objectIndex = type_return->lookupInlineComponentIdByName(icName);
                if (objectIndex != -1)
                    *type_return = type_return->lookupInlineComponentById(objectIndex);
                else {
                    auto icTypePriv = new QQmlTypePrivate(QQmlType::RegistrationType::InlineComponentType);
                    icTypePriv->setContainingType(type_return);
                    icTypePriv->extraData.id->url = type_return->sourceUrl();
                    int placeholderId = type_return->generatePlaceHolderICId();
                    icTypePriv->extraData.id->url.setFragment(QString::number(placeholderId));
                    auto icType = QQmlType(icTypePriv);
                    icTypePriv->release();
                    type_return->associateInlineComponent(icName, placeholderId, CompositeMetaTypeIds {}, icType);
                    *type_return = icType;
                }
                type_return->setPendingResolutionName(icName);
                return true;
            } else {
                error.setDescription(QQmlImportDatabase::tr("- %1 is not a type").arg(splitName.at(1).toString()));
            }
        }
        if (errors) {
            errors->prepend(error);
        }
        return false;
    }
    default: {
        // all other numbers suggest a user error
        if (errors) {
            QQmlError error;
            error.setDescription(QQmlImportDatabase::tr("- nested namespaces not allowed"));
            errors->prepend(error);
        }
        return false;
    }
    }
    Q_UNREACHABLE();
}

QQmlImportInstance *QQmlImportNamespace::findImport(const QString &uri) const
{
    for (QQmlImportInstance *import : imports) {
        if (import->uri == uri)
            return import;
    }
    return nullptr;
}

bool QQmlImportNamespace::resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef &type,
                                      QTypeRevision *version_return, QQmlType *type_return,
                                      QString *base, QList<QQmlError> *errors,
                                      QQmlType::RegistrationType registrationType,
                                      bool *typeRecursionDetected)
{
    QQmlImport::RecursionRestriction recursionRestriction =
            typeRecursionDetected ? QQmlImport::AllowRecursion : QQmlImport::PreventRecursion;

    bool localTypeRecursionDetected = false;
    if (!typeRecursionDetected)
        typeRecursionDetected = &localTypeRecursionDetected;

    if (needsSorting()) {
        std::stable_partition(imports.begin(), imports.end(), [](QQmlImportInstance *import) {
            return import->isInlineComponent;
        });
        setNeedsSorting(false);
    }
    for (int i=0; i<imports.count(); ++i) {
        const QQmlImportInstance *import = imports.at(i);
        if (import->resolveType(typeLoader, type, version_return, type_return, base,
                                typeRecursionDetected, registrationType, recursionRestriction, errors)) {
            if (qmlCheckTypes()) {
                // check for type clashes
                for (int j = i+1; j<imports.count(); ++j) {
                    const QQmlImportInstance *import2 = imports.at(j);
                    if (import2->resolveType(typeLoader, type, version_return, nullptr, base,
                                             nullptr, registrationType)) {
                        if (errors) {
                            QString u1 = import->url;
                            QString u2 = import2->url;
                            if (base) {
                                QStringView b(*base);
                                int dot = b.lastIndexOf(Dot);
                                if (dot >= 0) {
                                    b = b.left(dot+1);
                                    QStringView l = b.left(dot);
                                    if (u1.startsWith(b))
                                        u1 = u1.mid(b.size());
                                    else if (u1 == l)
                                        u1 = QQmlImportDatabase::tr("local directory");
                                    if (u2.startsWith(b))
                                        u2 = u2.mid(b.size());
                                    else if (u2 == l)
                                        u2 = QQmlImportDatabase::tr("local directory");
                                }
                            }

                            QQmlError error;
                            if (u1 != u2) {
                                error.setDescription(QQmlImportDatabase::tr("is ambiguous. Found in %1 and in %2").arg(u1).arg(u2));
                            } else {
                                error.setDescription(QQmlImportDatabase::tr("is ambiguous. Found in %1 in version %2.%3 and %4.%5")
                                                     .arg(u1)
                                                     .arg(import->version.majorVersion())
                                                     .arg(import->version.minorVersion())
                                                     .arg(import2->version.majorVersion())
                                                     .arg(import2->version.minorVersion()));
                            }
                            errors->prepend(error);
                        }
                        return false;
                    }
                }
            }
            return true;
        }
    }
    if (errors) {
        QQmlError error;
        if (*typeRecursionDetected)
            error.setDescription(QQmlImportDatabase::tr("is instantiated recursively"));
        else
            error.setDescription(QQmlImportDatabase::tr("is not a type"));
        errors->prepend(error);
    }
    return false;
}

bool QQmlImportNamespace::needsSorting() const
{
    return nextNamespace == this;
}

void QQmlImportNamespace::setNeedsSorting(bool needsSorting)
{
    Q_ASSERT(nextNamespace == this || nextNamespace == nullptr);
    nextNamespace = needsSorting ? this : nullptr;
}

QQmlImportsPrivate::QQmlImportsPrivate(QQmlTypeLoader *loader)
: ref(1), typeLoader(loader) {
}

QQmlImportsPrivate::~QQmlImportsPrivate()
{
    while (QQmlImportNamespace *ns = qualifiedSets.takeFirst())
        delete ns;
}

QQmlImportNamespace *QQmlImportsPrivate::findQualifiedNamespace(const QHashedStringRef &prefix) const
{
    for (QQmlImportNamespace *ns = qualifiedSets.first(); ns; ns = qualifiedSets.next(ns)) {
        if (prefix == ns->prefix)
            return ns;
    }
    return nullptr;
}

/*
Import an extension defined by a qmldir file.
*/
QTypeRevision QQmlImportsPrivate::importExtension(
        const QString &uri, QTypeRevision version, QQmlImportDatabase *database,
        const QQmlTypeLoaderQmldirContent *qmldir, QList<QQmlError> *errors)
{
    Q_ASSERT(qmldir->hasContent());

    qCDebug(lcQmlImport)
            << "importExtension:" << qPrintable(base) << "loaded" << qmldir->qmldirLocation();

    if (designerSupportRequired && !qmldir->designerSupported()) {
        if (errors) {
            QQmlError error;
            error.setDescription(
                        QQmlImportDatabase::tr("module does not support the designer \"%1\"")
                        .arg(qmldir->typeNamespace()));
            error.setUrl(QUrl::fromLocalFile(qmldir->qmldirLocation()));
            errors->prepend(error);
        }
        return QTypeRevision();
    }

    if (qmldir->plugins().isEmpty())
        return validVersion(version);

    QQmlPluginImporter importer(uri, version, database, qmldir, typeLoader, errors);
    return importer.importPlugins();
}

bool QQmlImportsPrivate::getQmldirContent(const QString &qmldirIdentifier, const QString &uri,
                                          QQmlTypeLoaderQmldirContent *qmldir, QList<QQmlError> *errors)
{
    Q_ASSERT(errors);
    Q_ASSERT(qmldir);

    *qmldir = typeLoader->qmldirContent(qmldirIdentifier);
    if ((*qmldir).hasContent()) {
        // Ensure that parsing was successful
        if ((*qmldir).hasError()) {
            QUrl url = QUrl::fromLocalFile(qmldirIdentifier);
            const QList<QQmlError> qmldirErrors = (*qmldir).errors(uri);
            for (int i = 0; i < qmldirErrors.size(); ++i) {
                QQmlError error = qmldirErrors.at(i);
                error.setUrl(url);
                errors->append(error);
            }
            return false;
        }
    }

    return true;
}

QString QQmlImportsPrivate::resolvedUri(const QString &dir_arg, QQmlImportDatabase *database)
{
    QString dir = dir_arg;
    if (dir.endsWith(Slash) || dir.endsWith(Backslash))
        dir.chop(1);

    QStringList paths = database->fileImportPath;
    if (!paths.isEmpty())
        std::sort(paths.begin(), paths.end(), std::greater<QString>()); // Ensure subdirs preceed their parents.

    QString stableRelativePath = dir;
    for (const QString &path : qAsConst(paths)) {
        if (dir.startsWith(path)) {
            stableRelativePath = dir.mid(path.length()+1);
            break;
        }
    }

    stableRelativePath.replace(Backslash, Slash);

    // remove optional versioning in dot notation from uri
    int versionDot = stableRelativePath.lastIndexOf(Dot);
    if (versionDot >= 0) {
        int nextSlash = stableRelativePath.indexOf(Slash, versionDot);
        if (nextSlash >= 0)
            stableRelativePath.remove(versionDot, nextSlash - versionDot);
        else
            stableRelativePath = stableRelativePath.left(versionDot);
    }

    stableRelativePath.replace(Slash, Dot);

    return stableRelativePath;
}

/*!
  \internal

  \fn template<typename Callback>
      QQmlImportDatabase::LocalQmldirResult QQmlImportDatabase::locateLocalQmldir(
            const QString &uri, QTypeRevision version, const Callback &callback)

  Locates the qmldir files for \a uri version \a version. For each one, calls
  the \a callback. If the \a callback returns \c true, returns QmldirFound.

  If at least one callback invocation returned \c false and there are no qmldir
  files left to check, returns QmldirRejected.

  Otherwise, if interception redirects a previously local qmldir URL to a remote
  one, returns QmldirInterceptedToRemote. Otherwise, returns QmldirNotFound.
*/

QTypeRevision QQmlImportsPrivate::matchingQmldirVersion(
        const QQmlTypeLoaderQmldirContent &qmldir, const QString &uri, QTypeRevision version,
        QList<QQmlError> *errors)
{
    int bestMajorVersion = -1;
    quint8 lowestMinorVersion = std::numeric_limits<quint8>::max();
    quint8 highestMinorVersion = 0;

    auto addVersion = [&](QTypeRevision newVersion) {
        if (!newVersion.hasMajorVersion())
            return;
        if (!version.hasMajorVersion() || version.majorVersion() == newVersion.majorVersion()) {
            if (newVersion.majorVersion() > bestMajorVersion) {
                bestMajorVersion = newVersion.majorVersion();
                if (newVersion.hasMinorVersion()) {
                    lowestMinorVersion = newVersion.minorVersion();
                    highestMinorVersion = newVersion.minorVersion();
                }
            } else if (newVersion.majorVersion() == bestMajorVersion
                       && newVersion.hasMinorVersion()) {
                lowestMinorVersion = qMin(lowestMinorVersion, newVersion.minorVersion());
                highestMinorVersion = qMax(highestMinorVersion, newVersion.minorVersion());
            }
        }
    };

    typedef QQmlDirComponents::const_iterator ConstIterator;
    const QQmlDirComponents &components = qmldir.components();

    ConstIterator cend = components.constEnd();
    for (ConstIterator cit = components.constBegin(); cit != cend; ++cit) {
        for (ConstIterator cit2 = components.constBegin(); cit2 != cit; ++cit2) {
            if (cit2->typeName == cit->typeName && cit2->version == cit->version) {
                // This entry clashes with a predecessor
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("\"%1\" version %2.%3 is defined more than once in module \"%4\"")
                                     .arg(cit->typeName).arg(cit->version.majorVersion())
                                     .arg(cit->version.minorVersion()).arg(uri));
                errors->prepend(error);
                return QTypeRevision();
            }
        }

        addVersion(cit->version);
    }

    typedef QList<QQmlDirParser::Script>::const_iterator SConstIterator;
    const QQmlDirScripts &scripts = qmldir.scripts();

    SConstIterator send = scripts.constEnd();
    for (SConstIterator sit = scripts.constBegin(); sit != send; ++sit) {
        for (SConstIterator sit2 = scripts.constBegin(); sit2 != sit; ++sit2) {
            if (sit2->nameSpace == sit->nameSpace && sit2->version == sit->version) {
                // This entry clashes with a predecessor
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("\"%1\" version %2.%3 is defined more than once in module \"%4\"")
                                     .arg(sit->nameSpace).arg(sit->version.majorVersion())
                                     .arg(sit->version.minorVersion()).arg(uri));
                errors->prepend(error);
                return QTypeRevision();
            }
        }

        addVersion(sit->version);
    }

    // Failure to find a match is only an error if we were asking for a specific version ...
    if (version.hasMajorVersion()
            && (bestMajorVersion < 0
                || (version.hasMinorVersion()
                    && (lowestMinorVersion > version.minorVersion()
                        || highestMinorVersion < version.minorVersion())))) {
        errors->prepend(moduleNotFoundError(uri, version));
        return QTypeRevision();
    }

    // ... otherwise, anything is valid.
    if (bestMajorVersion < 0)
        return validVersion();

    return QTypeRevision::fromVersion(
                bestMajorVersion,
                (version.hasMajorVersion() && version.hasMinorVersion())
                        ? version.minorVersion()
                        : highestMinorVersion);
}

QQmlImportNamespace *QQmlImportsPrivate::importNamespace(const QString &prefix) const
{
    QQmlImportNamespace *nameSpace = nullptr;

    if (prefix.isEmpty()) {
        nameSpace = &unqualifiedset;
    } else {
        nameSpace = findQualifiedNamespace(prefix);

        if (!nameSpace) {
            nameSpace = new QQmlImportNamespace;
            nameSpace->prefix = prefix;
            qualifiedSets.append(nameSpace);
        }
    }

    return nameSpace;
}

QQmlImportInstance *QQmlImportsPrivate::addImportToNamespace(
        QQmlImportNamespace *nameSpace, const QString &uri, const QString &url, QTypeRevision version,
        QV4::CompiledData::Import::ImportType type, QList<QQmlError> *errors, uint flags)
{
    Q_ASSERT(nameSpace);
    Q_ASSERT(errors);
    Q_UNUSED(errors);
    Q_ASSERT(url.isEmpty() || url.endsWith(Slash));

    QQmlImportInstance *import = new QQmlImportInstance;
    import->uri = uri;
    import->url = url;
    import->version = version;
    import->isLibrary = (type == QV4::CompiledData::Import::ImportLibrary);
    if (flags & QQmlImports::ImportImplicit) {
        import->implicitlyImported = true;
        nameSpace->imports.append(import);
    } else if (flags & QQmlImports::ImportLowPrecedence) {
        if (nameSpace->imports.isEmpty()) {
            nameSpace->imports.append(import);
        } else {
            for (auto it = nameSpace->imports.rbegin(), end = nameSpace->imports.rend();
                 it != end; ++it) {

                if (!(*it)->implicitlyImported) {
                    nameSpace->imports.insert(it.base(), import);
                    break;
                }
            }
        }
    } else {
        nameSpace->imports.prepend(import);
    }

    return import;
}

QTypeRevision QQmlImportsPrivate::addLibraryImport(
        const QString &uri, const QString &prefix, QTypeRevision version,
        const QString &qmldirIdentifier, const QString &qmldirUrl, uint flags,
        QQmlImportDatabase *database, QList<QQmlError> *errors)
{
    Q_ASSERT(database);
    Q_ASSERT(errors);

    QQmlImportNamespace *nameSpace = importNamespace(prefix);
    Q_ASSERT(nameSpace);

    QQmlImportInstance *inserted = addImportToNamespace(
                nameSpace, uri, qmldirUrl, version,
                QV4::CompiledData::Import::ImportLibrary, errors,
                flags);
    Q_ASSERT(inserted);

    if (!(flags & QQmlImports::ImportIncomplete)) {
        QQmlTypeLoaderQmldirContent qmldir;

        if (!qmldirIdentifier.isEmpty()) {
            if (!getQmldirContent(qmldirIdentifier, uri, &qmldir, errors))
                return QTypeRevision();

            if (qmldir.hasContent()) {
                version = importExtension(uri, version, database, &qmldir, errors);
                if (!version.isValid())
                    return QTypeRevision();

                if (!inserted->setQmldirContent(qmldirUrl, qmldir, nameSpace, errors))
                    return QTypeRevision();
            }
        }

        // Ensure that we are actually providing something
        const QTypeRevision matchingVersion = QQmlMetaType::matchingModuleVersion(uri, version);
        if (matchingVersion.isValid())
            return matchingVersion;

        if (inserted->qmlDirComponents.isEmpty() && inserted->qmlDirScripts.isEmpty()) {
            if (qmldir.plugins().isEmpty()) {
                if (!qmldir.imports().isEmpty())
                    return validVersion(); // This is a pure redirection
                if (qmldir.hasTypeInfo())
                    return validVersion(); // A pure C++ module without plugin
            }
            errors->prepend(moduleNotFoundError(uri, relevantVersion(uri, version)));
            return QTypeRevision();
        } else if (qmldir.hasContent()) {
            // Verify that the qmldir content is valid for this version
            version = matchingQmldirVersion(qmldir, uri, version, errors);
            if (!version.isValid())
                return QTypeRevision();
        }
    }

    return validVersion(version);
}

QTypeRevision QQmlImportsPrivate::addFileImport(
        const QString& uri, const QString &prefix, QTypeRevision version, uint flags,
        QQmlImportDatabase *database, QList<QQmlError> *errors)
{
    Q_ASSERT(errors);

    QQmlImportNamespace *nameSpace = importNamespace(prefix);
    Q_ASSERT(nameSpace);

    // The uri for this import.  For library imports this is the same as uri
    // specified by the user, but it may be different in the case of file imports.
    QString importUri = uri;
    QString qmldirUrl = resolveLocalUrl(base, importUri + (importUri.endsWith(Slash)
                                                           ? String_qmldir
                                                           : Slash_qmldir));
    qmldirUrl = typeLoader->engine()->interceptUrl(
                QUrl(qmldirUrl), QQmlAbstractUrlInterceptor::QmldirFile).toString();
    QString qmldirIdentifier;

    if (QQmlFile::isLocalFile(qmldirUrl)) {

        QString localFileOrQrc = QQmlFile::urlToLocalFileOrQrc(qmldirUrl);
        Q_ASSERT(!localFileOrQrc.isEmpty());

        const QString dir = localFileOrQrc.left(localFileOrQrc.lastIndexOf(Slash) + 1);
        if (!typeLoader->directoryExists(dir)) {
            if (!(flags & QQmlImports::ImportImplicit)) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("\"%1\": no such directory").arg(uri));
                error.setUrl(QUrl(qmldirUrl));
                errors->prepend(error);
            }
            return QTypeRevision();
        }

        // Transforms the (possible relative) uri into our best guess relative to the
        // import paths.
        importUri = resolvedUri(dir, database);
        if (importUri.endsWith(Slash))
            importUri.chop(1);

        if (!typeLoader->absoluteFilePath(localFileOrQrc).isEmpty())
            qmldirIdentifier = localFileOrQrc;

    } else if (nameSpace->prefix.isEmpty() && !(flags & QQmlImports::ImportIncomplete)) {

        if (!(flags & QQmlImports::ImportImplicit)) {
            QQmlError error;
            error.setDescription(QQmlImportDatabase::tr("import \"%1\" has no qmldir and no namespace").arg(importUri));
            error.setUrl(QUrl(qmldirUrl));
            errors->prepend(error);
        }

        return QTypeRevision();

    }

    // The url for the path containing files for this import
    QString url = resolveLocalUrl(base, uri);
    if (!url.endsWith(Slash) && !url.endsWith(Backslash))
        url += Slash;

    // ### For enum support, we are now adding the implicit import always (and earlier). Bail early
    //     if the implicit import has already been explicitly added, otherwise we can run into issues
    //     with duplicate imports. However remember that we attempted to add this as implicit import, to
    //     allow for the loading of internal types.
    if (flags & QQmlImports::ImportImplicit) {
        for (QList<QQmlImportInstance *>::const_iterator it = nameSpace->imports.constBegin();
             it != nameSpace->imports.constEnd(); ++it) {
            if ((*it)->url == url) {
                (*it)->implicitlyImported = true;
                return validVersion(version);
            }
        }
    }

    QQmlImportInstance *inserted = addImportToNamespace(
                nameSpace, importUri, url, version, QV4::CompiledData::Import::ImportFile,
                errors, flags);
    Q_ASSERT(inserted);
    if (flags & QQmlImports::ImportImplicit)
        inserted->implicitlyImported = true;

    if (!(flags & QQmlImports::ImportIncomplete) && !qmldirIdentifier.isEmpty()) {
        QQmlTypeLoaderQmldirContent qmldir;
        if (!getQmldirContent(qmldirIdentifier, importUri, &qmldir, errors))
            return QTypeRevision();

        if (qmldir.hasContent()) {
            if (uri == QStringLiteral(".")) {
                // If this is an implicit import, prefer the qmldir URI. Unless it doesn't exist.
                const QString qmldirUri = qmldir.typeNamespace();
                if (!qmldirUri.isEmpty())
                    importUri = qmldirUri;
            }

            version = importExtension(importUri, version, database, &qmldir, errors);
            if (!version.isValid())
                return QTypeRevision();

            if (!inserted->setQmldirContent(url, qmldir, nameSpace, errors))
                return QTypeRevision();
        }
    }

    return validVersion(version);
}

QTypeRevision QQmlImportsPrivate::updateQmldirContent(const QString &uri, const QString &prefix,
                                             const QString &qmldirIdentifier, const QString& qmldirUrl,
                                             QQmlImportDatabase *database, QList<QQmlError> *errors)
{
    QQmlImportNamespace *nameSpace = importNamespace(prefix);
    Q_ASSERT(nameSpace);

    if (QQmlImportInstance *import = nameSpace->findImport(uri)) {
        QQmlTypeLoaderQmldirContent qmldir;
        if (!getQmldirContent(qmldirIdentifier, uri, &qmldir, errors))
            return QTypeRevision();

        if (qmldir.hasContent()) {
            QTypeRevision version = importExtension(uri, import->version, database, &qmldir,
                                                    errors);
            if (!version.isValid())
                return QTypeRevision();

            if (import->setQmldirContent(qmldirUrl, qmldir, nameSpace, errors)) {
                if (import->qmlDirComponents.isEmpty() && import->qmlDirScripts.isEmpty()) {
                    // The implicit import qmldir can be empty, and plugins have no extra versions
                    if (uri != QLatin1String(".") && !QQmlMetaType::matchingModuleVersion(uri, version).isValid()) {
                        errors->prepend(moduleNotFoundError(uri, relevantVersion(uri, version)));
                        return QTypeRevision();
                    }
                } else {
                    // Verify that the qmldir content is valid for this version
                    version = matchingQmldirVersion(qmldir, uri, version, errors);
                    if (!version.isValid())
                        return QTypeRevision();
                }
                return validVersion(version);
            }
        }
    }

    if (errors->isEmpty()) {
        QQmlError error;
        error.setDescription(QQmlTypeLoader::tr("Cannot update qmldir content for '%1'").arg(uri));
        errors->prepend(error);
    }

    return QTypeRevision();
}

/*!
  \internal

  Adds an implicit "." file import.  This is equivalent to calling addFileImport(), but error
  messages related to the path or qmldir file not existing are suppressed.

  Additionally, this will add the import with lowest instead of highest precedence.
*/
QTypeRevision QQmlImports::addImplicitImport(QQmlImportDatabase *importDb, QList<QQmlError> *errors)
{
    Q_ASSERT(errors);

    qCDebug(lcQmlImport) << "addImplicitImport:" << qPrintable(baseUrl().toString());

    uint flags = ImportImplicit | (!isLocal(baseUrl()) ? ImportIncomplete : 0);
    return d->addFileImport(QLatin1String("."), QString(), QTypeRevision(), flags,
                            importDb, errors);
}

/*!
 \internal
 */
bool QQmlImports::addInlineComponentImport(QQmlImportInstance *const importInstance, const QString &name, const QUrl importUrl, QQmlType containingType)
{
    importInstance->url = importUrl.toString();
    importInstance->uri = name;
    importInstance->isInlineComponent = true;
    importInstance->version = QTypeRevision::zero();
    importInstance->containingType = containingType;
    d->unqualifiedset.imports.push_back(importInstance);
    d->unqualifiedset.setNeedsSorting(true);
    return true;
}

/*!
  \internal

  Adds information to \a imports such that subsequent calls to resolveType()
  will resolve types qualified by \a prefix by considering types found at the given \a uri.

  The uri is either a directory (if importType is FileImport), or a URI resolved using paths
  added via addImportPath() (if importType is LibraryImport).

  The \a prefix may be empty, in which case the import location is considered for
  unqualified types.

  The base URL must already have been set with Import::setBaseUrl().

  Optionally, the url the import resolved to can be returned by providing the url parameter.
  Not all imports will result in an output url being generated, in which case the url will
  be set to an empty string.

  Returns true on success, and false on failure.  In case of failure, the errors array will
  filled appropriately.
*/
QTypeRevision QQmlImports::addFileImport(
        QQmlImportDatabase *importDb, const QString& uri, const QString& prefix,
        QTypeRevision version, uint flags, QList<QQmlError> *errors)
{
    Q_ASSERT(importDb);
    Q_ASSERT(errors);

    qCDebug(lcQmlImport)
            << "addFileImport:" << qPrintable(baseUrl().toString())
            << uri << version << "as" << prefix;

    return d->addFileImport(uri, prefix, version, flags, importDb, errors);
}

QTypeRevision QQmlImports::addLibraryImport(
        QQmlImportDatabase *importDb, const QString &uri, const QString &prefix,
        QTypeRevision version, const QString &qmldirIdentifier, const QString& qmldirUrl,
        uint flags, QList<QQmlError> *errors)
{
    Q_ASSERT(importDb);
    Q_ASSERT(errors);

    qCDebug(lcQmlImport)
            << "addLibraryImport:" << qPrintable(baseUrl().toString())
            << uri << version << "as" << prefix;

    return d->addLibraryImport(uri, prefix, version, qmldirIdentifier, qmldirUrl, flags,
                               importDb, errors);
}

QTypeRevision QQmlImports::updateQmldirContent(
        QQmlImportDatabase *importDb, const QString &uri, const QString &prefix,
        const QString &qmldirIdentifier, const QString& qmldirUrl, QList<QQmlError> *errors)
{
    Q_ASSERT(importDb);
    Q_ASSERT(errors);

    qDebug(lcQmlImport)
            << "updateQmldirContent:" << qPrintable(baseUrl().toString())
            << uri << "to" << qmldirUrl << "as" << prefix;

    return d->updateQmldirContent(uri, prefix, qmldirIdentifier, qmldirUrl, importDb, errors);
}

bool QQmlImports::isLocal(const QString &url)
{
    return !QQmlFile::urlToLocalFileOrQrc(url).isEmpty();
}

bool QQmlImports::isLocal(const QUrl &url)
{
    return !QQmlFile::urlToLocalFileOrQrc(url).isEmpty();
}

QUrl QQmlImports::urlFromLocalFileOrQrcOrUrl(const QString &file)
{
    QUrl url(QLatin1String(file.at(0) == Colon ? "qrc" : "") + file);

    // We don't support single character schemes as those conflict with windows drive letters.
    if (url.scheme().length() < 2)
        return QUrl::fromLocalFile(file);
    return url;
}

void QQmlImports::setDesignerSupportRequired(bool b)
{
    designerSupportRequired = b;
}

static QStringList parseEnvImportPath(const QString &envImportPath)
{
    if (QDir::listSeparator() == u':') {
        // Double colons are interpreted as separator + resource path.
        QStringList paths = envImportPath.split(u':');
        bool wasEmpty = false;
        for (auto it = paths.begin(); it != paths.end();) {
            if (it->isEmpty()) {
                wasEmpty = true;
                it = paths.erase(it);
            } else {
                if (wasEmpty) {
                    it->prepend(u':');
                    wasEmpty = false;
                }
                ++it;
            }
        }
        return paths;
    } else {
        return envImportPath.split(QDir::listSeparator(), Qt::SkipEmptyParts);
    }
}

/*!
\class QQmlImportDatabase
\brief The QQmlImportDatabase class manages the QML imports for a QQmlEngine.
\internal
*/
QQmlImportDatabase::QQmlImportDatabase(QQmlEngine *e)
: engine(e)
{
    filePluginPath << QLatin1String(".");
    // Search order is applicationDirPath(), qrc:/qt-project.org/imports, $QML_IMPORT_PATH, $QML2_IMPORT_PATH, QLibraryInfo::QmlImportsPath

    QString installImportsPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    addImportPath(installImportsPath);

    auto addEnvImportPath = [this](const char *var) {
        if (Q_UNLIKELY(!qEnvironmentVariableIsEmpty(var))) {
            const QStringList paths = parseEnvImportPath(qEnvironmentVariable(var));
            for (int ii = paths.count() - 1; ii >= 0; --ii)
                addImportPath(paths.at(ii));
        }
    };

    // env import paths
    addEnvImportPath("QML_IMPORT_PATH");
    addEnvImportPath("QML2_IMPORT_PATH");

    addImportPath(QStringLiteral("qrc:/qt-project.org/imports"));
    addImportPath(QCoreApplication::applicationDirPath());
#if defined(Q_OS_ANDROID)
    addImportPath(QStringLiteral("qrc:/android_rcc_bundle/qml"));
    if (Q_UNLIKELY(!qEnvironmentVariableIsEmpty("QT_BUNDLED_LIBS_PATH"))) {
        const QString envImportPath = qEnvironmentVariable("QT_BUNDLED_LIBS_PATH");
        QLatin1Char pathSep(':');
        QStringList paths = envImportPath.split(pathSep, Qt::SkipEmptyParts);
        for (int ii = paths.count() - 1; ii >= 0; --ii)
            addPluginPath(paths.at(ii));
    }
#endif
}

QQmlImportDatabase::~QQmlImportDatabase()
{
    clearDirCache();
}

/*!
    \internal
*/
QStringList QQmlImportDatabase::pluginPathList() const
{
    return filePluginPath;
}

/*!
    \internal
*/
void QQmlImportDatabase::setPluginPathList(const QStringList &paths)
{
    qCDebug(lcQmlImport) << "setPluginPathList:" << paths;
    filePluginPath = paths;
}

/*!
    \internal
*/
void QQmlImportDatabase::addPluginPath(const QString& path)
{
    qCDebug(lcQmlImport) << "addPluginPath:" << path;

    QUrl url = QUrl(path);
    if (url.isRelative() || url.scheme() == QLatin1String("file")
            || (url.scheme().length() == 1 && QFile::exists(path)) ) {  // windows path
        QDir dir = QDir(path);
        filePluginPath.prepend(dir.canonicalPath());
    } else {
        filePluginPath.prepend(path);
    }
}

QString QQmlImportDatabase::absoluteFilePath(const QString &path) const
{
    return QQmlEnginePrivate::get(engine)->typeLoader.absoluteFilePath(path);
}

/*!
    \internal
*/
void QQmlImportDatabase::addImportPath(const QString& path)
{
    qCDebug(lcQmlImport) << "addImportPath:" << path;

    if (path.isEmpty())
        return;

    QUrl url = QUrl(path);
    QString cPath;

    if (url.scheme() == QLatin1String("file")) {
        cPath = QQmlFile::urlToLocalFileOrQrc(url);
    } else if (path.startsWith(QLatin1Char(':'))) {
        // qrc directory, e.g. :/foo
        // need to convert to a qrc url, e.g. qrc:/foo
        cPath = QLatin1String("qrc") + path;
        cPath.replace(Backslash, Slash);
    } else if (url.isRelative() ||
               (url.scheme().length() == 1 && QFile::exists(path)) ) {  // windows path
        QDir dir = QDir(path);
        cPath = dir.canonicalPath();
    } else {
        cPath = path;
        cPath.replace(Backslash, Slash);
    }

    if (!cPath.isEmpty()) {
        if (fileImportPath.contains(cPath))
            fileImportPath.move(fileImportPath.indexOf(cPath), 0);
        else
            fileImportPath.prepend(cPath);
    }
}

/*!
    \internal
*/
QStringList QQmlImportDatabase::importPathList(PathType type) const
{
    if (type == LocalOrRemote)
        return fileImportPath;

    QStringList list;
    for (const QString &path : fileImportPath) {
        bool localPath = isPathAbsolute(path) || QQmlFile::isLocalFile(path);
        if (localPath == (type == Local))
            list.append(path);
    }

    return list;
}

/*!
    \internal
*/
void QQmlImportDatabase::setImportPathList(const QStringList &paths)
{
    qCDebug(lcQmlImport) << "setImportPathList:" << paths;

    fileImportPath.clear();
    for (auto it = paths.crbegin(); it != paths.crend(); ++it)
        addImportPath(*it);

    // Our existing cached paths may have been invalidated
    clearDirCache();
}

/*!
    \internal
*/
QTypeRevision QQmlImportDatabase::lockModule(const QString &uri, const QString &typeNamespace,
                                             QTypeRevision version, QList<QQmlError> *errors)
{
    if (!version.hasMajorVersion()) {
        version = QQmlMetaType::latestModuleVersion(uri);
        if (!version.isValid())
            errors->prepend(moduleNotFoundError(uri, version));
    }
    if (version.hasMajorVersion() && !typeNamespace.isEmpty()
            && !QQmlMetaType::protectModule(uri, version, true)) {
        // Not being able to protect the module means there are not types registered for it,
        // means the plugin we loaded didn't provide any, means we didn't find the module.
        // We output the generic error message as depending on the load order of imports we may
        // hit this path or another one that only figures "plugin is already loaded but module
        // unavailable" and doesn't try to protect it anymore.
        errors->prepend(moduleNotFoundError(uri, version));
        return QTypeRevision();
    }

    return version;
}

bool QQmlImportDatabase::removeDynamicPlugin(const QString &pluginId)
{
    return QQmlPluginImporter::removePlugin(pluginId);
}

QStringList QQmlImportDatabase::dynamicPlugins() const
{
    return QQmlPluginImporter::plugins();
}

void QQmlImportDatabase::clearDirCache()
{
    QStringHash<QmldirCache *>::ConstIterator itr = qmldirCache.constBegin();
    while (itr != qmldirCache.constEnd()) {
        QmldirCache *cache = *itr;
        do {
            QmldirCache *nextCache = cache->next;
            delete cache;
            cache = nextCache;
        } while (cache);

        ++itr;
    }
    qmldirCache.clear();
}

QT_END_NAMESPACE
