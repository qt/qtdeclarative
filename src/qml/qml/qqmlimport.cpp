/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
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
#include <QtQml/qqmlextensioninterface.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qqmlengine_p.h>
#include <private/qfieldlist_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlImportTrace, QML_IMPORT_TRACE)
DEFINE_BOOL_CONFIG_OPTION(qmlCheckTypes, QML_CHECK_TYPES)

static QString dotqml_string(QLatin1String(".qml"));

QString resolveLocalUrl(const QString &url, const QString &relative)
{
    if (relative.contains(QLatin1Char(':'))) {
        // contains a host name
        return QUrl(url).resolved(QUrl(relative)).toString();
    } else if (relative.isEmpty()) {
        return url;
    } else if (relative.at(0) == QLatin1Char('/') || !url.contains(QLatin1Char('/'))) {
        return relative;
    } else {
        QString base(url.left(url.lastIndexOf(QLatin1Char('/')) + 1));

        if (relative == QLatin1String("."))
            return base;

        base.append(relative);

        // Remove any relative directory elements in the path
        const QLatin1Char dot('.');
        const QLatin1Char slash('/');

        int length = base.length();
        int index = 0;
        while ((index = base.indexOf(QLatin1String("/."), index)) != -1) {
            if ((length > (index + 2)) && (base.at(index + 2) == dot) &&
                (length == (index + 3) || (base.at(index + 3) == slash))) {
                // Either "/../" or "/..<END>"
                int previous = base.lastIndexOf(slash, index - 1);
                if (previous == -1)
                    break;

                int removeLength = (index - previous) + 3;
                base.remove(previous + 1, removeLength);
                length -= removeLength;
                index = previous;
            } else if ((length == (index + 2)) || (base.at(index + 2) == slash)) {
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

typedef QMap<QString, QString> StringStringMap;
Q_GLOBAL_STATIC(StringStringMap, qmlEnginePluginsWithRegisteredTypes); // stores the uri

class QQmlImportNamespace
{
public:
    QQmlImportNamespace() : nextNamespace(0) {}

    struct Import {
        QString uri;
        QString url;
        int majversion;
        int minversion;
        bool isLibrary;
        QQmlDirComponents qmlDirComponents;
        QQmlDirScripts qmlDirScripts;

        bool resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef &type,
                         int *vmajor, int *vminor,
                         QQmlType** type_return, QString* url_return,
                         QString *base = 0, bool *typeRecursionDetected = 0) const;
    };
    QList<Import> imports;

    bool resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef& type,
                     int *vmajor, int *vminor,
                     QQmlType** type_return, QString* url_return,
                     QString *base = 0, QList<QQmlError> *errors = 0);

    // Prefix when used as a qualified import.  Otherwise empty.
    QHashedString prefix;

    // Used by QQmlImportsPrivate::qualifiedSets
    QQmlImportNamespace *nextNamespace;
};

class QQmlImportsPrivate
{
public:
    QQmlImportsPrivate(QQmlTypeLoader *loader);
    ~QQmlImportsPrivate();

    bool addImport(const QQmlDirComponents &qmldircomponentsnetwork,
                   const QString &importedUri, const QString& prefix,
                   int vmaj, int vmin, QQmlScript::Import::Type importType,
                   bool isImplicitImport, QQmlImportDatabase *database,
                   QString *, QList<QQmlError> *errors);

    bool resolveType(const QHashedStringRef &type, int *vmajor, int *vminor,
                     QQmlType** type_return, QString* url_return,
                     QList<QQmlError> *errors);

    QUrl baseUrl;
    QString base;
    int ref;

    QQmlImportNamespace unqualifiedset;

    QQmlImportNamespace *findQualifiedNamespace(const QHashedStringRef &);
    QFieldList<QQmlImportNamespace, &QQmlImportNamespace::nextNamespace> qualifiedSets;

    QQmlTypeLoader *typeLoader;

    static inline QString tr(const char *str) {
        return QQmlImportDatabase::tr(str);
    }

private:
    static bool locateQmldir(const QString &uri, int vmaj, int vmin,
                             QQmlImportDatabase *database,
                             QString *outQmldirFilePath, QString *outUrl);
    bool importExtension(const QString &absoluteFilePath, const QString &uri,
                         QQmlImportDatabase *database, QQmlDirComponents* components,
                         QQmlDirScripts *scripts,
                         QString *url, QList<QQmlError> *errors);
    QString resolvedUri(const QString &dir_arg, QQmlImportDatabase *database);
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

void QQmlImports::populateCache(QQmlTypeNameCache *cache, QQmlEngine *engine) const
{
    const QQmlImportNamespace &set = d->unqualifiedset;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportNamespace::Import &import = set.imports.at(ii);
        QQmlTypeModule *module = QQmlMetaType::typeModule(import.uri, import.majversion);
        if (module)
            cache->m_anonymousImports.append(QQmlTypeModuleVersion(module, import.minversion));
    }

    for (QQmlImportNamespace *ns = d->qualifiedSets.first(); ns; ns = d->qualifiedSets.next(ns)) {

        const QQmlImportNamespace &set = *ns;

        for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
            const QQmlImportNamespace::Import &import = set.imports.at(ii);
            QQmlTypeModule *module = QQmlMetaType::typeModule(import.uri, import.majversion);
            if (module) {
                QQmlTypeNameCache::Import &typeimport = cache->m_namedImports[set.prefix];
                typeimport.modules.append(QQmlTypeModuleVersion(module, import.minversion));
            }

            QQmlMetaType::ModuleApi moduleApi = QQmlMetaType::moduleApi(import.uri, import.majversion,
                                                                        import.minversion);
            if (moduleApi.script || moduleApi.qobject) {
                QQmlTypeNameCache::Import &import = cache->m_namedImports[set.prefix];
                QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
                import.moduleApi = ep->moduleApiInstance(moduleApi);
            }
        }
    }
}

QList<QQmlImports::ScriptReference> QQmlImports::resolvedScripts() const
{
    QList<QQmlImports::ScriptReference> scripts;

    const QQmlImportNamespace &set = d->unqualifiedset;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportNamespace::Import &import = set.imports.at(ii);

        foreach (const QQmlDirParser::Script &script, import.qmlDirScripts) {
            ScriptReference ref;
            ref.nameSpace = script.nameSpace;
            ref.location = QUrl(import.url).resolved(QUrl(script.fileName));
            scripts.append(ref);
        }
    }

    for (QQmlImportNamespace *ns = d->qualifiedSets.first(); ns; ns = d->qualifiedSets.next(ns)) {
        const QQmlImportNamespace &set = *ns;

        for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
            const QQmlImportNamespace::Import &import = set.imports.at(ii);

            foreach (const QQmlDirParser::Script &script, import.qmlDirScripts) {
                ScriptReference ref;
                ref.nameSpace = script.nameSpace;
                ref.qualifier = set.prefix;
                ref.location = QUrl(import.url).resolved(QUrl(script.fileName));
                scripts.append(ref);
            }
        }
    }

    return scripts;
}

/*!
  \internal

  The given (namespace qualified) \a type is resolved to either
  \list
  \li a QQmlImportNamespace stored at \a ns_return,
  \li a QQmlType stored at \a type_return, or
  \li a component located at \a url_return.
  \endlist

  If any return pointer is 0, the corresponding search is not done.

  \sa addImport()
*/
bool QQmlImports::resolveType(const QHashedStringRef &type,
                              QQmlType** type_return, QString* url_return, int *vmaj, int *vmin,
                              QQmlImportNamespace** ns_return, QList<QQmlError> *errors) const
{
    QQmlImportNamespace* ns = d->findQualifiedNamespace(type);
    if (ns) {
        if (ns_return)
            *ns_return = ns;
        return true;
    }
    if (type_return || url_return) {
        if (d->resolveType(type,vmaj,vmin,type_return,url_return, errors)) {
            if (qmlImportTrace()) {
#define RESOLVE_TYPE_DEBUG qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) \
                                              << ')' << "::resolveType: " << type.toString() << " => "

                if (type_return && *type_return && url_return && !url_return->isEmpty())
                    RESOLVE_TYPE_DEBUG << (*type_return)->typeName() << ' ' << *url_return;
                if (type_return && *type_return)
                    RESOLVE_TYPE_DEBUG << (*type_return)->typeName();
                if (url_return && !url_return->isEmpty())
                    RESOLVE_TYPE_DEBUG << *url_return;

#undef RESOLVE_TYPE_DEBUG
            }
            return true;
        }
    }
    return false;
}

/*!
  \internal

  Searching \e only in the namespace \a ns (previously returned in a call to
  resolveType(), \a type is found and returned to either
  a QQmlType stored at \a type_return, or
  a component located at \a url_return.

  If either return pointer is 0, the corresponding search is not done.
*/
bool QQmlImports::resolveType(QQmlImportNamespace* ns, const QHashedStringRef &type,
                              QQmlType** type_return, QString* url_return,
                              int *vmaj, int *vmin) const
{
    return ns->resolveType(d->typeLoader,type,vmaj,vmin,type_return,url_return);
}

bool QQmlImportNamespace::Import::resolveType(QQmlTypeLoader *typeLoader,
                                              const QHashedStringRef& type, int *vmajor, int *vminor,
                                              QQmlType** type_return, QString* url_return,
                                              QString *base, bool *typeRecursionDetected) const
{
    if (majversion >= 0 && minversion >= 0) {
        QQmlType *t = QQmlMetaType::qmlType(type, uri, majversion, minversion);
        if (t) {
            if (vmajor) *vmajor = majversion;
            if (vminor) *vminor = minversion;
            if (type_return)
                *type_return = t;
            return true;
        }
    }

    bool typeWasDeclaredInQmldir = false;
    if (!qmlDirComponents.isEmpty()) {
        QQmlDirComponents::ConstIterator it = qmlDirComponents.find(type);
        if (it != qmlDirComponents.end()) {
            typeWasDeclaredInQmldir = true;
            // first found is last inserted - process in reverse
            QQmlDirComponents::ConstIterator begin = it;
            while (++it != qmlDirComponents.end() && it.key() == type) {}
            do {
                --it;
                const QQmlDirParser::Component &c = *it;

                // importing version -1 means import ALL versions
                if ((majversion == -1) || (c.majorVersion == majversion &&
                                           minversion >= c.minorVersion)) {

                    QString candidate = resolveLocalUrl(QString(url + c.typeName + dotqml_string), c.fileName);
                    if (c.internal && base) {
                        if (resolveLocalUrl(*base, c.fileName) != candidate)
                            continue; // failed attempt to access an internal type
                    }
                    if (base && *base == candidate) {
                        if (typeRecursionDetected)
                            *typeRecursionDetected = true;
                        continue; // no recursion
                    }
                    if (url_return)
                        *url_return = candidate;
                    return true;
                }
            } while (it != begin);
        }
    }

    if (!typeWasDeclaredInQmldir && !isLibrary) {
        QString qmlUrl = url + QString::fromRawData(type.constData(), type.length()) + dotqml_string;

        bool exists = false;

        if (QQmlFile::isBundle(qmlUrl)) {
            exists = QQmlFile::bundleFileExists(qmlUrl, typeLoader->engine());
        } else {
            exists = !typeLoader->absoluteFilePath(QQmlFile::urlToLocalFileOrQrc(qmlUrl)).isEmpty();
        }

        if (exists) {
            if (base && *base == qmlUrl) { // no recursion
                if (typeRecursionDetected)
                    *typeRecursionDetected = true;
            } else {
                if (url_return)
                    *url_return = qmlUrl;
                return true;
            }
        }
    }

    return false;
}

bool QQmlImportsPrivate::resolveType(const QHashedStringRef& type, int *vmajor, int *vminor,
                                     QQmlType** type_return, QString* url_return,
                                     QList<QQmlError> *errors)
{
    QQmlImportNamespace *s = 0;
    int dot = type.indexOf(QLatin1Char('.'));
    if (dot >= 0) {
        QHashedStringRef namespaceName(type.constData(), dot);
        s = findQualifiedNamespace(namespaceName);
        if (!s) {
            if (errors) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("- %1 is not a namespace").arg(namespaceName.toString()));
                errors->prepend(error);
            }
            return false;
        }
        int ndot = type.indexOf(QLatin1Char('.'),dot+1);
        if (ndot > 0) {
            if (errors) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("- nested namespaces not allowed"));
                errors->prepend(error);
            }
            return false;
        }
    } else {
        s = &unqualifiedset;
    }
    QHashedStringRef unqualifiedtype = dot < 0 ? type : QHashedStringRef(type.constData()+dot+1, type.length()-dot-1);
    if (s) {
        if (s->resolveType(typeLoader,unqualifiedtype,vmajor,vminor,type_return,url_return, &base, errors))
            return true;
        if (s->imports.count() == 1 && !s->imports.at(0).isLibrary && url_return && s != &unqualifiedset) {
            // qualified, and only 1 url
            *url_return = resolveLocalUrl(s->imports.at(0).url, unqualifiedtype.toString() + QLatin1String(".qml"));
            return true;
        }
    }

    return false;
}

bool QQmlImportNamespace::resolveType(QQmlTypeLoader *typeLoader, const QHashedStringRef &type,
                                      int *vmajor, int *vminor, QQmlType** type_return,
                                      QString* url_return, QString *base, QList<QQmlError> *errors)
{
    bool typeRecursionDetected = false;
    for (int i=0; i<imports.count(); ++i) {
        const Import &import = imports.at(i);
        if (import.resolveType(typeLoader, type, vmajor, vminor, type_return, url_return,
                               base, &typeRecursionDetected)) {
            if (qmlCheckTypes()) {
                // check for type clashes
                for (int j = i+1; j<imports.count(); ++j) {
                    const Import &import2 = imports.at(j);
                    if (import2.resolveType(typeLoader, type, vmajor, vminor, 0, 0, base)) {
                        if (errors) {
                            QString u1 = imports.at(i).url;
                            QString u2 = imports.at(j).url;
                            if (base) {
                                QString b = *base;
                                int dot = b.lastIndexOf(QLatin1Char('.'));
                                if (dot >= 0) {
                                    b = b.left(dot+1);
                                    QString l = b.left(dot);
                                    if (u1.startsWith(b))
                                        u1 = u1.mid(b.count());
                                    else if (u1 == l)
                                        u1 = QQmlImportDatabase::tr("local directory");
                                    if (u2.startsWith(b))
                                        u2 = u2.mid(b.count());
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
                                                        .arg(imports.at(i).majversion).arg(imports.at(i).minversion)
                                                        .arg(imports.at(j).majversion).arg(imports.at(j).minversion));
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
        if (typeRecursionDetected)
            error.setDescription(QQmlImportDatabase::tr("is instantiated recursively"));
        else
            error.setDescription(QQmlImportDatabase::tr("is not a type"));
        errors->prepend(error);
    }
    return false;
}

QQmlImportsPrivate::QQmlImportsPrivate(QQmlTypeLoader *loader)
: ref(1), typeLoader(loader) {
}

QQmlImportsPrivate::~QQmlImportsPrivate()
{
    while (QQmlImportNamespace *ns = qualifiedSets.takeFirst())
        delete ns;
}

QQmlImportNamespace *QQmlImportsPrivate::findQualifiedNamespace(const QHashedStringRef &prefix)
{
    for (QQmlImportNamespace *ns = qualifiedSets.first(); ns; ns = qualifiedSets.next(ns)) {
        if (prefix == ns->prefix)
            return ns;
    }
    return 0;
}

/*!
Import an extension defined by a qmldir file.

\a qmldirFilePath is either a raw file path, or a bundle url.

This call will modify the \a url parameter if importing the extension redirects to
a bundle path.
*/
bool QQmlImportsPrivate::importExtension(const QString &qmldirFilePath,
                                         const QString &uri,
                                         QQmlImportDatabase *database,
                                         QQmlDirComponents* components,
                                         QQmlDirScripts* scripts,
                                         QString *url,
                                         QList<QQmlError> *errors)
{
    // As qmldirFilePath is always local, this method can always return synchronously
    const QQmlDirParser *qmldirParser = typeLoader->qmlDirParser(qmldirFilePath, uri, url);
    if (qmldirParser->hasError()) {
        if (errors) {
            QUrl url;

            if (QQmlFile::isBundle(qmldirFilePath))
                url = QUrl(qmldirFilePath);
            else
                url = QUrl::fromLocalFile(qmldirFilePath);

            const QList<QQmlError> qmldirErrors = qmldirParser->errors(uri);
            for (int i = 0; i < qmldirErrors.size(); ++i) {
                QQmlError error = qmldirErrors.at(i);
                error.setUrl(url);
                errors->append(error);
            }
        }
        return false;
    }

    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(base) << "::importExtension: "
                           << "loaded " << qmldirFilePath;

    if (!database->qmlDirFilesForWhichPluginsHaveBeenLoaded.contains(qmldirFilePath)) {

        QString qmldirPath = qmldirFilePath;

        if (QQmlFile::isBundle(*url))
            qmldirPath = QQmlFile::bundleFileName(*url, typeLoader->engine());

        int slash = qmldirFilePath.lastIndexOf(QLatin1Char('/'));
        if (slash > 0)
            qmldirPath.truncate(slash);

        foreach (const QQmlDirParser::Plugin &plugin, qmldirParser->plugins()) {
            QString resolvedFilePath = database->resolvePlugin(typeLoader, qmldirPath,
                                                               plugin.path, plugin.name);
            if (!resolvedFilePath.isEmpty()) {
                if (!database->importPlugin(resolvedFilePath, uri, errors)) {
                    if (errors) {
                        // XXX TODO: should we leave the import plugin error alone?
                        // Here, we pop it off the top and coalesce it into this error's message.
                        // The reason is that the lower level may add url and line/column numbering information.
                        QQmlError poppedError = errors->takeFirst();
                        QQmlError error;
                        error.setDescription(tr("plugin cannot be loaded for module \"%1\": %2").arg(uri).arg(poppedError.description()));
                        error.setUrl(QUrl::fromLocalFile(qmldirFilePath));
                        errors->prepend(error);
                    }
                    return false;
                }
            } else {
                if (errors) {
                    QQmlError error;
                    error.setDescription(tr("module \"%1\" plugin \"%2\" not found").arg(uri).arg(plugin.name));
                    error.setUrl(QUrl::fromLocalFile(qmldirFilePath));
                    errors->prepend(error);
                }
                return false;
            }
        }

        database->qmlDirFilesForWhichPluginsHaveBeenLoaded.insert(qmldirFilePath);
    }

    if (components)
        *components = qmldirParser->components();
    if (scripts)
        *scripts = qmldirParser->scripts();

    return true;
}

QString QQmlImportsPrivate::resolvedUri(const QString &dir_arg, QQmlImportDatabase *database)
{
    struct I { static bool greaterThan(const QString &s1, const QString &s2) {
        return s1 > s2;
    } };

    QString dir = dir_arg;
    if (dir.endsWith(QLatin1Char('/')) || dir.endsWith(QLatin1Char('\\')))
        dir.chop(1);

    QStringList paths = database->fileImportPath;
    qSort(paths.begin(), paths.end(), I::greaterThan); // Ensure subdirs preceed their parents.

    QString stableRelativePath = dir;
    foreach(const QString &path, paths) {
        if (dir.startsWith(path)) {
            stableRelativePath = dir.mid(path.length()+1);
            break;
        }
    }

    stableRelativePath.replace(QLatin1Char('\\'), QLatin1Char('/'));

    // remove optional versioning in dot notation from uri
    int lastSlash = stableRelativePath.lastIndexOf(QLatin1Char('/'));
    if (lastSlash >= 0) {
        int versionDot = stableRelativePath.indexOf(QLatin1Char('.'), lastSlash);
        if (versionDot >= 0)
            stableRelativePath = stableRelativePath.left(versionDot);
    }

    stableRelativePath.replace(QLatin1Char('/'), QLatin1Char('.'));

    return stableRelativePath;
}

/*
Locates the qmldir file for \a uri version \a vmaj.vmin.  Returns true if found,
and fills in outQmldirFilePath and outQmldirUrl appropriately.  Otherwise returns
false.
*/
bool QQmlImportsPrivate::locateQmldir(const QString &uri, int vmaj, int vmin,
                                      QQmlImportDatabase *database,
                                      QString *outQmldirFilePath,
                                      QString *outQmldirPathUrl)
{
    Q_ASSERT(vmaj >= 0 && vmin >= 0); // Versions are always specified for libraries

    // Check cache first

    QQmlImportDatabase::QmldirCache *cacheHead = 0;
    {
    QQmlImportDatabase::QmldirCache **cachePtr = database->qmldirCache.value(uri);
    if (cachePtr) {
        cacheHead = *cachePtr;
        QQmlImportDatabase::QmldirCache *cache = cacheHead;
        while (cache) {
            if (cache->versionMajor == vmaj && cache->versionMinor == vmin) {
                *outQmldirFilePath = cache->qmldirFilePath;
                *outQmldirPathUrl = cache->qmldirPathUrl;
                return !cache->qmldirFilePath.isEmpty();
            }
            cache = cache->next;
        }
    }
    }

    static QLatin1Char Slash('/');
    static QLatin1String Slash_qmldir("/qmldir");

    QString url = uri;
    url.replace(QLatin1Char('.'), Slash);

    // step 0: search for extension with fully encoded version number (eg. MyModule.3.2)
    // step 1: search for extension with encoded version major (eg. MyModule.3)
    // step 2: search for extension without version number (eg. MyModule)
    for (int step = 0; step <= 2; ++step) {
        foreach (const QString &p, database->fileImportPath) {
            QString dir = p + Slash + url;

            QString qmldirFile = dir;
            if (step == 0) qmldirFile += QString(QLatin1String(".%1.%2")).arg(vmaj).arg(vmin);
            else if (step == 1) qmldirFile += QString(QLatin1String(".%1")).arg(vmaj);
            qmldirFile += Slash_qmldir;

            QQmlTypeLoader &typeLoader = QQmlEnginePrivate::get(database->engine)->typeLoader;
            QString absoluteFilePath = typeLoader.absoluteFilePath(qmldirFile);
            if (!absoluteFilePath.isEmpty()) {
                QString absolutePath = absoluteFilePath.left(absoluteFilePath.lastIndexOf(Slash)+1);
                if (absolutePath.at(0) == QLatin1Char(':'))
                    url = QLatin1String("qrc://") + absolutePath.mid(1);
                else
                    url = QUrl::fromLocalFile(absolutePath).toString();

                QQmlImportDatabase::QmldirCache *cache = new QQmlImportDatabase::QmldirCache;
                cache->versionMajor = vmaj;
                cache->versionMinor = vmin;
                cache->qmldirFilePath = absoluteFilePath;
                cache->qmldirPathUrl = url;
                cache->next = cacheHead;
                database->qmldirCache.insert(uri, cache);

                *outQmldirFilePath = absoluteFilePath;
                *outQmldirPathUrl = url;

                return true;
            }
        }
    }

    QQmlImportDatabase::QmldirCache *cache = new QQmlImportDatabase::QmldirCache;
    cache->versionMajor = vmaj;
    cache->versionMinor = vmin;
    cache->next = cacheHead;
    database->qmldirCache.insert(uri, cache);

    return false;
}

bool QQmlImportsPrivate::addImport(const QQmlDirComponents &qmldircomponentsnetwork,
                                   const QString& importedUri, const QString& prefix,
                                   int vmaj, int vmin, QQmlScript::Import::Type importType,
                                   bool isImplicitImport, QQmlImportDatabase *database,
                                   QString *outUrl, QList<QQmlError> *errors)
{
    Q_ASSERT(errors);
    Q_ASSERT(importType == QQmlScript::Import::File || importType == QQmlScript::Import::Library);

    static QLatin1String String_qmldir("qmldir");
    static QLatin1String Slash_qmldir("/qmldir");
    static QLatin1Char Slash('/');

    // The list of components defined by a qmldir file for this import.
    QQmlDirComponents qmldircomponents;
    // The list of scripts defined by a qmldir file for this import.
    QQmlDirScripts qmldirscripts;
    // The namespace that this import affects.
    QQmlImportNamespace *importSet = 0;
    // The uri for this import.  For library imports this is the same as importedUri
    // specified by the user, but it may be different in the case of file imports.
    QString uri;
    // The url for the path containing files for this import.
    QString url;

    qmldircomponents = qmldircomponentsnetwork;
    uri = importedUri;
    if (prefix.isEmpty()) {
        importSet = &unqualifiedset;
    } else {
        importSet = findQualifiedNamespace(prefix);

        if (!importSet) {
            importSet = new QQmlImportNamespace;
            importSet->prefix = prefix;
            qualifiedSets.append(importSet);
        }
    }

    if (importType == QQmlScript::Import::Library) {
        Q_ASSERT(vmaj >= 0 && vmin >= 0);

        QString qmldirFilePath;

        if (locateQmldir(uri, vmaj, vmin, database, &qmldirFilePath, &url)) {

            if (!importExtension(qmldirFilePath, uri, database, &qmldircomponents,
                                 &qmldirscripts, &url, errors))
                return false;

        }

        if (!QQmlMetaType::isModule(uri, vmaj, vmin)) {

            if (qmldircomponents.isEmpty() && qmldirscripts.isEmpty()) {
                QQmlError error;
                if (QQmlMetaType::isAnyModule(uri))
                    error.setDescription(tr("module \"%1\" version %2.%3 is not installed").arg(importedUri).arg(vmaj).arg(vmin));
                else
                    error.setDescription(tr("module \"%1\" is not installed").arg(importedUri));
                errors->prepend(error);
                return false;
            } else {
                int lowest_min = INT_MAX;
                int highest_min = INT_MIN;
                typedef QQmlDirComponents::const_iterator ConstIterator;
                typedef QList<QQmlDirParser::Script>::const_iterator SConstIterator;

                ConstIterator cend = qmldircomponents.end();
                for (ConstIterator cit = qmldircomponents.begin(); cit != cend; ++cit) {
                    if (cit->majorVersion == vmaj) {
                        lowest_min = qMin(lowest_min, cit->minorVersion);
                        highest_min = qMax(highest_min, cit->minorVersion);
                    }
                }

                for (SConstIterator cit = qmldirscripts.constBegin();
                     cit != qmldirscripts.constEnd(); ++cit) {
                    if (cit->majorVersion == vmaj) {
                        lowest_min = qMin(lowest_min, cit->minorVersion);
                        highest_min = qMax(highest_min, cit->minorVersion);
                    }
                }

                if (lowest_min > vmin || highest_min < vmin) {
                    QQmlError error;
                    error.setDescription(tr("module \"%1\" version %2.%3 is not installed").arg(importedUri).arg(vmaj).arg(vmin));
                    errors->prepend(error);
                    return false;
                }
            }

        }
    } else {

        Q_ASSERT(importType == QQmlScript::Import::File);

        if (qmldircomponents.isEmpty()) {

            QString qmldirPath = uri;
            if (uri.endsWith(Slash)) qmldirPath += String_qmldir;
            else qmldirPath += Slash_qmldir;
            QString qmldirUrl = resolveLocalUrl(base, qmldirPath);

            if (QQmlFile::isBundle(qmldirUrl)) {

                QString dir = resolveLocalUrl(base, uri);
                Q_ASSERT(QQmlFile::isBundle(dir));
                if (!QQmlFile::bundleDirectoryExists(dir, typeLoader->engine())) {
                    if (!isImplicitImport) {
                        QQmlError error;
                        error.setDescription(tr("\"%1\": no such directory").arg(importedUri));
                        error.setUrl(QUrl(qmldirUrl));
                        errors->prepend(error);
                    }
                    return false;
                }

                // Transforms the (possible relative) uri into our best guess relative to the
                // import paths.
                uri = resolvedUri(dir, database);

                if (uri.endsWith(Slash))
                    uri.chop(1);
                if (QQmlFile::bundleFileExists(qmldirUrl, typeLoader->engine())) {
                    if (!importExtension(qmldirUrl, uri, database, &qmldircomponents,
                                         &qmldirscripts, &url, errors))
                        return false;
                }

            } else if (QQmlFile::isLocalFile(qmldirUrl)) {

                QString localFileOrQrc = QQmlFile::urlToLocalFileOrQrc(qmldirUrl);
                Q_ASSERT(!localFileOrQrc.isEmpty());

                QString dir = QQmlFile::urlToLocalFileOrQrc(resolveLocalUrl(base, uri));
                if (!typeLoader->directoryExists(dir)) {
                    if (!isImplicitImport) {
                        QQmlError error;
                        error.setDescription(tr("\"%1\": no such directory").arg(importedUri));
                        error.setUrl(QUrl(qmldirUrl));
                        errors->prepend(error);
                    }
                    return false;
                }

                // Transforms the (possible relative) uri into our best guess relative to the
                // import paths.
                uri = resolvedUri(dir, database);

                if (uri.endsWith(Slash))
                    uri.chop(1);
                if (!typeLoader->absoluteFilePath(localFileOrQrc).isEmpty()) {
                    if (!importExtension(localFileOrQrc, uri, database, &qmldircomponents,
                                         &qmldirscripts, &url, errors))
                        return false;
                }

            } else if (prefix.isEmpty()) {

                if (!isImplicitImport) {
                    QQmlError error;
                    error.setDescription(tr("import \"%1\" has no qmldir and no namespace").arg(uri));
                    error.setUrl(QUrl(qmldirUrl));
                    errors->prepend(error);
                }

                return false;

            }
        }

        url = resolveLocalUrl(base, importedUri);
        if (!url.endsWith(Slash))
            url += Slash;
    }

    Q_ASSERT(url.isEmpty() || url.endsWith(Slash));

    QMap<QString, QQmlDirParser::Script> scripts;
    if (!qmldirscripts.isEmpty()) {
        // Verify that we haven't imported these scripts already
        for (QList<QQmlImportNamespace::Import>::const_iterator it = importSet->imports.constBegin();
             it != importSet->imports.constEnd(); ++it) {
            if (it->uri == uri) {
                QQmlError error;
                error.setDescription(tr("\"%1\" is ambiguous. Found in %2 and in %3").arg(uri).arg(url).arg(it->url));
                errors->prepend(error);
                return false;
            }
        }

        for (QList<QQmlDirParser::Script>::const_iterator sit = qmldirscripts.constBegin();
             sit != qmldirscripts.constEnd(); ++sit) {
            // Only include scripts that match our requested version
            if (((vmaj == -1) || (sit->majorVersion == vmaj)) &&
                ((vmin == -1) || (sit->minorVersion <= vmin))) {

                // Load the highest version that matches
                QMap<QString, QQmlDirParser::Script>::iterator it = scripts.find(sit->nameSpace);
                if (it == scripts.end() || (it->minorVersion < sit->minorVersion)) {
                    scripts.insert(sit->nameSpace, *sit);
                }
            }
        }
    }

    QQmlImportNamespace::Import import;
    import.uri = uri;
    import.url = url;
    import.majversion = vmaj;
    import.minversion = vmin;
    import.isLibrary = importType == QQmlScript::Import::Library;
    import.qmlDirComponents = qmldircomponents;
    import.qmlDirScripts = scripts.values();

    importSet->imports.prepend(import);

    if (outUrl) *outUrl = url;

    return true;
}

/*!
  \internal

  Adds an implicit "." file import.  This is equivalent to calling addImport(), but error
  messages related to the path or qmldir file not existing are suppressed.
*/
bool QQmlImports::addImplicitImport(QQmlImportDatabase *importDb,
                                    const QQmlDirComponents &qmldircomponentsnetwork,
                                    QList<QQmlError> *errors)
{
    Q_ASSERT(errors);

    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString())
                           << ")::addImplicitImport";


    return d->addImport(qmldircomponentsnetwork, QLatin1String("."), QString(), -1, -1,
                        QQmlScript::Import::File, true, importDb, 0, errors);
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
bool QQmlImports::addImport(QQmlImportDatabase *importDb,
                            const QString& uri, const QString& prefix, int vmaj, int vmin,
                            QQmlScript::Import::Type importType,
                            const QQmlDirComponents &qmldircomponentsnetwork,
                            QString *url, QList<QQmlError> *errors)
{
    Q_ASSERT(errors);

    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) << ')' << "::addImport: "
                           << uri << ' ' << vmaj << '.' << vmin << ' '
                           << (importType==QQmlScript::Import::Library? "Library" : "File")
                           << " as " << prefix;

    return d->addImport(qmldircomponentsnetwork, uri, prefix, vmaj, vmin, importType, false,
                        importDb, url, errors);
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

    // Search order is applicationDirPath(), $QML_IMPORT_PATH, QLibraryInfo::ImportsPath

#ifndef QT_NO_SETTINGS
    QString installImportsPath =  QLibraryInfo::location(QLibraryInfo::ImportsPath);
    addImportPath(installImportsPath);
#endif // QT_NO_SETTINGS

    // env import paths
    QByteArray envImportPath = qgetenv("QML_IMPORT_PATH");
    if (!envImportPath.isEmpty()) {
#if defined(Q_OS_WIN)
        QLatin1Char pathSep(';');
#else
        QLatin1Char pathSep(':');
#endif
        QStringList paths = QString::fromLatin1(envImportPath).split(pathSep, QString::SkipEmptyParts);
        for (int ii = paths.count() - 1; ii >= 0; --ii)
            addImportPath(paths.at(ii));
    }

    addImportPath(QCoreApplication::applicationDirPath());
}

QQmlImportDatabase::~QQmlImportDatabase()
{
    for (QStringHash<QmldirCache *>::ConstIterator iter = qmldirCache.begin();
         iter != qmldirCache.end(); ++iter) {

        QmldirCache *c = *iter;
        while (c) {
            QmldirCache *n = c->next;
            delete c;
            c = n;
        }
    }
}

/*!
  \internal

  Returns the result of the merge of \a baseName with \a path, \a suffixes, and \a prefix.
  The \a prefix must contain the dot.

  \a qmldirPath is the location of the qmldir file.
 */
QString QQmlImportDatabase::resolvePlugin(QQmlTypeLoader *typeLoader,
                                          const QString &qmldirPath,
                                          const QString &qmldirPluginPath,
                                          const QString &baseName, const QStringList &suffixes,
                                          const QString &prefix)
{
    QStringList searchPaths = filePluginPath;
    bool qmldirPluginPathIsRelative = QDir::isRelativePath(qmldirPluginPath);
    if (!qmldirPluginPathIsRelative)
        searchPaths.prepend(qmldirPluginPath);

    foreach (const QString &pluginPath, searchPaths) {

        QString resolvedPath;
        if (pluginPath == QLatin1String(".")) {
            if (qmldirPluginPathIsRelative && !qmldirPluginPath.isEmpty() && qmldirPluginPath != QLatin1String("."))
                resolvedPath = QDir::cleanPath(qmldirPath + QLatin1Char('/') + qmldirPluginPath);
            else
                resolvedPath = qmldirPath;
        } else {
            if (QDir::isRelativePath(pluginPath))
                resolvedPath = QDir::cleanPath(qmldirPath + QLatin1Char('/') + pluginPath);
            else
                resolvedPath = pluginPath;
        }

        // hack for resources, should probably go away
        if (resolvedPath.startsWith(QLatin1Char(':')))
            resolvedPath = QCoreApplication::applicationDirPath();

        if (!resolvedPath.endsWith(QLatin1Char('/')))
            resolvedPath += QLatin1Char('/');

        foreach (const QString &suffix, suffixes) {
            QString pluginFileName = prefix;

            pluginFileName += baseName;
            pluginFileName += suffix;

            QString absolutePath = typeLoader->absoluteFilePath(resolvedPath + pluginFileName);
            if (!absolutePath.isEmpty())
                return absolutePath;
        }
    }

    if (qmlImportTrace())
        qDebug() << "QQmlImportDatabase::resolvePlugin: Could not resolve plugin" << baseName 
                 << "in" << qmldirPath;

    return QString();
}

/*!
  \internal

  Returns the result of the merge of \a baseName with \a dir and the platform suffix.

  \table
  \header \li Platform \li Valid suffixes
  \row \li Windows     \li \c .dll
  \row \li Unix/Linux  \li \c .so
  \row \li AIX  \li \c .a
  \row \li HP-UX       \li \c .sl, \c .so (HP-UXi)
  \row \li Mac OS X    \li \c .dylib, \c .bundle, \c .so
  \endtable

  Version number on unix are ignored.
*/
QString QQmlImportDatabase::resolvePlugin(QQmlTypeLoader *typeLoader,
                                                  const QString &qmldirPath, const QString &qmldirPluginPath,
                                                  const QString &baseName)
{
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    return resolvePlugin(typeLoader, qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
# ifdef QT_DEBUG
                         << QLatin1String("d.dll") // try a qmake-style debug build first
# endif
                         << QLatin1String(".dll"));
#else

# if defined(Q_OS_DARWIN)

    return resolvePlugin(typeLoader, qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
# ifdef QT_DEBUG
                         << QLatin1String("_debug.dylib") // try a qmake-style debug build first
                         << QLatin1String(".dylib")
# else
                         << QLatin1String(".dylib")
                         << QLatin1String("_debug.dylib") // try a qmake-style debug build after
# endif
                         << QLatin1String(".so")
                         << QLatin1String(".bundle"),
                         QLatin1String("lib"));
# else  // Generic Unix
    QStringList validSuffixList;

#  if defined(Q_OS_HPUX)
/*
    See "HP-UX Linker and Libraries User's Guide", section "Link-time Differences between PA-RISC and IPF":
    "In PA-RISC (PA-32 and PA-64) shared libraries are suffixed with .sl. In IPF (32-bit and 64-bit),
    the shared libraries are suffixed with .so. For compatibility, the IPF linker also supports the .sl suffix."
 */
    validSuffixList << QLatin1String(".sl");
#   if defined __ia64
    validSuffixList << QLatin1String(".so");
#   endif
#  elif defined(Q_OS_AIX)
    validSuffixList << QLatin1String(".a") << QLatin1String(".so");
#  elif defined(Q_OS_UNIX)
    validSuffixList << QLatin1String(".so");
#  endif

    // Examples of valid library names:
    //  libfoo.so

    return resolvePlugin(typeLoader, qmldirPath, qmldirPluginPath, baseName, validSuffixList, QLatin1String("lib"));
# endif

#endif
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
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::setPluginPathList: " << paths;

    filePluginPath = paths;
}

/*!
    \internal
*/
void QQmlImportDatabase::addPluginPath(const QString& path)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::addPluginPath: " << path;

    QUrl url = QUrl(path);
    if (url.isRelative() || url.scheme() == QLatin1String("file")
            || (url.scheme().length() == 1 && QFile::exists(path)) ) {  // windows path
        QDir dir = QDir(path);
        filePluginPath.prepend(dir.canonicalPath());
    } else {
        filePluginPath.prepend(path);
    }
}

/*!
    \internal
*/
void QQmlImportDatabase::addImportPath(const QString& path)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::addImportPath: " << path;

    if (path.isEmpty())
        return;

    QUrl url = QUrl(path);
    QString cPath;

    if (url.isRelative() || url.scheme() == QLatin1String("file")
            || (url.scheme().length() == 1 && QFile::exists(path)) ) {  // windows path
        QDir dir = QDir(path);
        cPath = dir.canonicalPath();
    } else {
        cPath = path;
        cPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
    }

    if (!cPath.isEmpty()
        && !fileImportPath.contains(cPath))
        fileImportPath.prepend(cPath);
}

/*!
    \internal
*/
QStringList QQmlImportDatabase::importPathList() const
{
    return fileImportPath;
}

/*!
    \internal
*/
void QQmlImportDatabase::setImportPathList(const QStringList &paths)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::setImportPathList: " << paths;

    fileImportPath = paths;
}

/*!
    \internal
*/
bool QQmlImportDatabase::importPlugin(const QString &filePath, const QString &uri, QList<QQmlError> *errors)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImportDatabase::importPlugin: " << uri << " from " << filePath;

#ifndef QT_NO_LIBRARY
    QFileInfo fileInfo(filePath);
    const QString absoluteFilePath = fileInfo.absoluteFilePath();

    bool engineInitialized = initializedPlugins.contains(absoluteFilePath);
    bool typesRegistered = qmlEnginePluginsWithRegisteredTypes()->contains(absoluteFilePath);

    if (typesRegistered) {
        Q_ASSERT_X(qmlEnginePluginsWithRegisteredTypes()->value(absoluteFilePath) == uri,
                   "QQmlImportDatabase::importExtension",
                   "Internal error: Plugin imported previously with different uri");
    }

    if (!engineInitialized || !typesRegistered) {
        if (!QQml_isFileCaseCorrect(absoluteFilePath)) {
            if (errors) {
                QQmlError error;
                error.setDescription(tr("File name case mismatch for \"%1\"").arg(absoluteFilePath));
                errors->prepend(error);
            }
            return false;
        }
        QPluginLoader loader(absoluteFilePath);

        if (!loader.load()) {
            if (errors) {
                QQmlError error;
                error.setDescription(loader.errorString());
                errors->prepend(error);
            }
            return false;
        }

        QObject *instance = loader.instance();
        if (QQmlTypesExtensionInterface *iface = qobject_cast<QQmlExtensionInterface *>(instance)) {

            const QByteArray bytes = uri.toUtf8();
            const char *moduleId = bytes.constData();
            if (!typesRegistered) {

                // XXX thread this code should probably be protected with a mutex.
                qmlEnginePluginsWithRegisteredTypes()->insert(absoluteFilePath, uri);
                iface->registerTypes(moduleId);
            }
            if (!engineInitialized) {
                // things on the engine (eg. adding new global objects) have to be done for every 
                // engine.  
                // XXX protect against double initialization
                initializedPlugins.insert(absoluteFilePath);

                QQmlExtensionInterface *eiface = 
                    qobject_cast<QQmlExtensionInterface *>(instance);
                if (eiface) {
                    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
                    ep->typeLoader.initializeEngine(eiface, moduleId);
                }
            }
        } else {
            if (errors) {
                QQmlError error;
                error.setDescription(loader.errorString());
                errors->prepend(error);
            }
            return false;
        }
    }

    return true;
#else
    return false;
#endif
}

QT_END_NAMESPACE
