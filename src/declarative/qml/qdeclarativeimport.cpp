/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeimport_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qlibraryinfo.h>
#include <QtDeclarative/qdeclarativeextensioninterface.h>
#include <private/qdeclarativeglobal_p.h>
#include <private/qdeclarativetypenamecache_p.h>
#include <private/qdeclarativeengine_p.h>

#ifdef Q_OS_SYMBIAN
#include "private/qcore_symbian_p.h"
#endif

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlImportTrace, QML_IMPORT_TRACE)
DEFINE_BOOL_CONFIG_OPTION(qmlCheckTypes, QML_CHECK_TYPES)

static bool greaterThan(const QString &s1, const QString &s2)
{
    return s1 > s2;
}

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
        if (relative == QLatin1String("."))
            return url.left(url.lastIndexOf(QLatin1Char('/')) + 1);
        else if (relative.startsWith(QLatin1String("./")))
            return url.left(url.lastIndexOf(QLatin1Char('/')) + 1) + relative.mid(2);
        return url.left(url.lastIndexOf(QLatin1Char('/')) + 1) + relative;
    }
}



typedef QMap<QString, QString> StringStringMap;
Q_GLOBAL_STATIC(StringStringMap, qmlEnginePluginsWithRegisteredTypes); // stores the uri

class QDeclarativeImportedNamespace 
{
public:
    struct Data {
        QByteArray uri;
        QString url;
        int majversion;
        int minversion;
        bool isLibrary;
        QDeclarativeDirComponents qmlDirComponents;
    };
    QList<Data> imports;


    bool find_helper(QDeclarativeTypeLoader *typeLoader, const Data &data, const QByteArray& type, int *vmajor, int *vminor,
                                 QDeclarativeType** type_return, QString* url_return,
                                 QString *base = 0, bool *typeRecursionDetected = 0);
    bool find(QDeclarativeTypeLoader *typeLoader, const QByteArray& type, int *vmajor, int *vminor, QDeclarativeType** type_return,
              QString* url_return, QString *base = 0, QList<QDeclarativeError> *errors = 0);
};

class QDeclarativeImportsPrivate {
public:
    QDeclarativeImportsPrivate(QDeclarativeTypeLoader *loader);
    ~QDeclarativeImportsPrivate();

    bool importExtension(const QString &absoluteFilePath, const QString &uri, 
                         QDeclarativeImportDatabase *database, QDeclarativeDirComponents* components, 
                         QList<QDeclarativeError> *errors);

    QString resolvedUri(const QString &dir_arg, QDeclarativeImportDatabase *database);
    bool add(const QDeclarativeDirComponents &qmldircomponentsnetwork, 
             const QString& uri_arg, const QString& prefix, 
             int vmaj, int vmin, QDeclarativeScriptParser::Import::Type importType, 
             QDeclarativeImportDatabase *database, QList<QDeclarativeError> *errors);
    bool find(const QByteArray& type, int *vmajor, int *vminor, 
              QDeclarativeType** type_return, QString* url_return, QList<QDeclarativeError> *errors);

    QDeclarativeImportedNamespace *findNamespace(const QString& type);

    QUrl baseUrl;
    QString base;
    int ref;

    QSet<QString> qmlDirFilesForWhichPluginsHaveBeenLoaded;
    QDeclarativeImportedNamespace unqualifiedset;
    QHash<QString,QDeclarativeImportedNamespace* > set;
    QDeclarativeTypeLoader *typeLoader;
};

/*!
\class QDeclarativeImports
\brief The QDeclarativeImports class encapsulates one QML document's import statements.
\internal
*/
QDeclarativeImports::QDeclarativeImports(const QDeclarativeImports &copy) 
: d(copy.d)
{
    ++d->ref;
}

QDeclarativeImports &
QDeclarativeImports::operator =(const QDeclarativeImports &copy)
{
    ++copy.d->ref;
    if (--d->ref == 0)
        delete d;
    d = copy.d;
    return *this;
}

QDeclarativeImports::QDeclarativeImports(QDeclarativeTypeLoader *typeLoader)
    : d(new QDeclarativeImportsPrivate(typeLoader))
{
}

QDeclarativeImports::~QDeclarativeImports()
{
    if (--d->ref == 0)
        delete d;
}

/*!
  Sets the base URL to be used for all relative file imports added.
*/
void QDeclarativeImports::setBaseUrl(const QUrl& url)
{
    d->baseUrl = url;
    d->base = url.toString();
}

/*!
  Returns the base URL to be used for all relative file imports added.
*/
QUrl QDeclarativeImports::baseUrl() const
{
    return d->baseUrl;
}

void QDeclarativeImports::populateCache(QDeclarativeTypeNameCache *cache, QDeclarativeEngine *engine) const
{
    const QDeclarativeImportedNamespace &set = d->unqualifiedset;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QDeclarativeImportedNamespace::Data &data = set.imports.at(ii);
        QDeclarativeTypeModule *module = QDeclarativeMetaType::typeModule(data.uri, data.majversion);
        if (module)
            cache->m_anonymousImports.append(QDeclarativeTypeModuleVersion(module, data.minversion));
    }

    for (QHash<QString,QDeclarativeImportedNamespace* >::ConstIterator iter = d->set.begin();
         iter != d->set.end(); 
         ++iter) {

        const QDeclarativeImportedNamespace &set = *iter.value();
        QDeclarativeTypeNameCache::Import &import = cache->m_namedImports[iter.key()];
        for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
            const QDeclarativeImportedNamespace::Data &data = set.imports.at(ii);
            QDeclarativeTypeModule *module = QDeclarativeMetaType::typeModule(data.uri, data.majversion);
            if (module)
                import.modules.append(QDeclarativeTypeModuleVersion(module, data.minversion));

            QDeclarativeMetaType::ModuleApi moduleApi = QDeclarativeMetaType::moduleApi(data.uri, data.majversion, data.minversion);
            if (moduleApi.script || moduleApi.qobject) {
                QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);
                QDeclarativeMetaType::ModuleApiInstance *a = ep->moduleApiInstances.value(moduleApi);
                if (!a) {
                    a = new QDeclarativeMetaType::ModuleApiInstance;
                    a->scriptCallback = moduleApi.script;
                    a->qobjectCallback = moduleApi.qobject;
                    ep->moduleApiInstances.insert(moduleApi, a);
                }
                import.moduleApi = a;
            }
        }
    }


}

/*!
  \internal

  The given (namespace qualified) \a type is resolved to either
  \list
  \o a QDeclarativeImportedNamespace stored at \a ns_return,
  \o a QDeclarativeType stored at \a type_return, or
  \o a component located at \a url_return.
  \endlist

  If any return pointer is 0, the corresponding search is not done.

  \sa addImport()
*/
bool QDeclarativeImports::resolveType(const QByteArray& type,
                                      QDeclarativeType** type_return, QString* url_return, int *vmaj, int *vmin,
                                      QDeclarativeImportedNamespace** ns_return, QList<QDeclarativeError> *errors) const
{
    QDeclarativeImportedNamespace* ns = d->findNamespace(QString::fromUtf8(type));
    if (ns) {
        if (ns_return)
            *ns_return = ns;
        return true;
    }
    if (type_return || url_return) {
        if (d->find(type,vmaj,vmin,type_return,url_return, errors)) {
            if (qmlImportTrace()) {
                if (type_return && *type_return && url_return && !url_return->isEmpty())
                    qDebug().nospace() << "QDeclarativeImports(" << qPrintable(baseUrl().toString()) << ")" << "::resolveType: " 
                                       << type << " => " << (*type_return)->typeName() << " " << *url_return;
                if (type_return && *type_return)
                    qDebug().nospace() << "QDeclarativeImports(" << qPrintable(baseUrl().toString()) << ")" << "::resolveType: " 
                                       << type << " => " << (*type_return)->typeName();
                if (url_return && !url_return->isEmpty())
                    qDebug().nospace() << "QDeclarativeImports(" << qPrintable(baseUrl().toString()) << ")" << "::resolveType: " 
                                       << type << " => " << *url_return;
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
  a QDeclarativeType stored at \a type_return, or
  a component located at \a url_return.

  If either return pointer is 0, the corresponding search is not done.
*/
bool QDeclarativeImports::resolveType(QDeclarativeImportedNamespace* ns, const QByteArray& type, 
                                      QDeclarativeType** type_return, QString* url_return,
                                      int *vmaj, int *vmin) const
{
    return ns->find(d->typeLoader,type,vmaj,vmin,type_return,url_return);
}

bool QDeclarativeImportedNamespace::find_helper(QDeclarativeTypeLoader *typeLoader, const Data &data, const QByteArray& type, int *vmajor, int *vminor,
                                 QDeclarativeType** type_return, QString* url_return,
                                 QString *base, bool *typeRecursionDetected)
{
    if (vmaj >= 0 && vmin >= 0) {
        QString qt = data.uri + QLatin1Char('/') + type;
        QDeclarativeType *t = QDeclarativeMetaType::qmlType(qt,vmaj,vmin);
        if (t) {
            if (vmajor) *vmajor = vmaj;
            if (vminor) *vminor = vmin;
            if (type_return)
                *type_return = t;
            return true;
        }
    }

    const QDeclarativeDirComponents &qmldircomponents = data.qmlDirComponents;
    bool typeWasDeclaredInQmldir = false;
    if (!qmldircomponents.isEmpty()) {
        foreach (const QDeclarativeDirParser::Component &c, qmldircomponents) {
            if (c.typeName == type) {
                typeWasDeclaredInQmldir = true;
                // importing version -1 means import ALL versions
                if ((vmaj == -1) || (c.majorVersion == vmaj && vmin >= c.minorVersion)) {
                    QString url(data.url + type + QLatin1String(".qml"));
                    QString candidate = resolveLocalUrl(url, c.fileName);
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
            }
        }
    }

    if (!typeWasDeclaredInQmldir && !data.isLibrary) {
        // XXX search non-files too! (eg. zip files, see QT-524)
        QString url(data.url + QString::fromUtf8(type) + QLatin1String(".qml"));
        QString file = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url);
        if (!typeLoader->absoluteFilePath(file).isEmpty()) {
            if (base && *base == url) { // no recursion
                if (typeRecursionDetected)
                    *typeRecursionDetected = true;
            } else {
                if (url_return)
                    *url_return = url;
                return true;
            }
        }
    }
    return false;
}

QDeclarativeImportsPrivate::QDeclarativeImportsPrivate(QDeclarativeTypeLoader *loader)
    : ref(1), typeLoader(loader)
{
}

QDeclarativeImportsPrivate::~QDeclarativeImportsPrivate()
{
    foreach (QDeclarativeImportedNamespace* s, set.values())
        delete s;
}

bool QDeclarativeImportsPrivate::importExtension(const QString &absoluteFilePath, const QString &uri, 
                                                 QDeclarativeImportDatabase *database, 
                                                 QDeclarativeDirComponents* components, QList<QDeclarativeError> *errors)
{
    const QDeclarativeDirParser *qmldirParser = typeLoader->qmlDirParser(absoluteFilePath);
    if (qmldirParser->hasError()) {
        if (errors) {
            for (int i = 0; i < qmldirParser->errors().size(); ++i)
                errors->prepend(qmldirParser->errors().at(i));
        }
        return false;
    }

    if (qmlImportTrace())
        qDebug().nospace() << "QDeclarativeImports(" << qPrintable(base) << "::importExtension: "
                           << "loaded " << absoluteFilePath;

    if (! qmlDirFilesForWhichPluginsHaveBeenLoaded.contains(absoluteFilePath)) {
        qmlDirFilesForWhichPluginsHaveBeenLoaded.insert(absoluteFilePath);

        QString qmldirPath = absoluteFilePath;
        int slash = absoluteFilePath.lastIndexOf(QLatin1Char('/'));
        if (slash > 0)
            qmldirPath.truncate(slash);
        foreach (const QDeclarativeDirParser::Plugin &plugin, qmldirParser->plugins()) {

            QString resolvedFilePath = database->resolvePlugin(typeLoader, qmldirPath, plugin.path, plugin.name);
#if defined(QT_LIBINFIX) && defined(Q_OS_SYMBIAN)
            if (resolvedFilePath.isEmpty()) {
                // In case of libinfixed build, attempt to load libinfixed version, too.
                QString infixedPluginName = plugin.name + QLatin1String(QT_LIBINFIX);
                resolvedFilePath = database->resolvePlugin(dir, plugin.path, infixedPluginName);
            }
#endif
            if (!resolvedFilePath.isEmpty()) {
                if (!database->importPlugin(resolvedFilePath, uri, errors)) {
                    if (errors) {
                        // XXX TODO: should we leave the import plugin error alone?
                        // Here, we pop it off the top and coalesce it into this error's message.
                        // The reason is that the lower level may add url and line/column numbering information.
                        QDeclarativeError poppedError = errors->takeFirst();
                        QDeclarativeError error;
                        error.setDescription(QDeclarativeImportDatabase::tr("plugin cannot be loaded for module \"%1\": %2").arg(uri).arg(poppedError.description()));
                        error.setUrl(QUrl::fromLocalFile(absoluteFilePath));
                        errors->prepend(error);
                    }
                    return false;
                }
            } else {
                if (errors) {
                    QDeclarativeError error;
                    error.setDescription(QDeclarativeImportDatabase::tr("module \"%1\" plugin \"%2\" not found").arg(uri).arg(plugin.name));
                    error.setUrl(QUrl::fromLocalFile(absoluteFilePath));
                    errors->prepend(error);
                }
                return false;
            }
        }
    }

    if (components)
        *components = qmldirParser->components();

    return true;
}

QString QDeclarativeImportsPrivate::resolvedUri(const QString &dir_arg, QDeclarativeImportDatabase *database)
{
    QString dir = dir_arg;
    if (dir.endsWith(QLatin1Char('/')) || dir.endsWith(QLatin1Char('\\')))
        dir.chop(1);

    QStringList paths = database->fileImportPath;
    qSort(paths.begin(), paths.end(), greaterThan); // Ensure subdirs preceed their parents.

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

bool QDeclarativeImportsPrivate::add(const QDeclarativeDirComponents &qmldircomponentsnetwork, 
                                     const QString& uri_arg, const QString& prefix, int vmaj, int vmin, 
                                     QDeclarativeScriptParser::Import::Type importType, 
                                     QDeclarativeImportDatabase *database, QList<QDeclarativeError> *errors)
{
    static QLatin1String Slash_qmldir("/qmldir");
    static QLatin1Char Slash('/');

    QDeclarativeDirComponents qmldircomponents = qmldircomponentsnetwork;
    QString uri = uri_arg;
    QDeclarativeImportedNamespace *s;
    if (prefix.isEmpty()) {
        s = &unqualifiedset;
    } else {
        s = set.value(prefix);
        if (!s)
            set.insert(prefix,(s=new QDeclarativeImportedNamespace));
    }
    QString url = uri;
    bool versionFound = false;
    if (importType == QDeclarativeScriptParser::Import::Library) {

        Q_ASSERT(vmaj >= 0 && vmin >= 0); // Versions are always specified for libraries

        url.replace(QLatin1Char('.'), Slash);
        bool found = false;
        QString dir;
        QString qmldir;

        // step 1: search for extension with fully encoded version number
        foreach (const QString &p, database->fileImportPath) {
            dir = p+Slash+url;

            QFileInfo fi(dir+QString(QLatin1String(".%1.%2")).arg(vmaj).arg(vmin)+QLatin1String("/qmldir"));
            const QString absoluteFilePath = fi.absoluteFilePath();

            if (fi.isFile()) {
                found = true;

                url = QUrl::fromLocalFile(fi.absolutePath()).toString();
                uri = resolvedUri(dir, database);
                if (!importExtension(absoluteFilePath, uri, database, &qmldircomponents, errors))
                    return false;
                break;
            }
        }

        // step 2: search for extension with encoded version major
        foreach (const QString &p, database->fileImportPath) {
            dir = p+Slash+url;

            QFileInfo fi(dir+QString(QLatin1String(".%1")).arg(vmaj)+QLatin1String("/qmldir"));
            const QString absoluteFilePath = fi.absoluteFilePath();

            if (fi.isFile()) {
                found = true;

                url = QUrl::fromLocalFile(fi.absolutePath()).toString();
                uri = resolvedUri(dir, database);
                if (!importExtension(absoluteFilePath, uri, database, &qmldircomponents, errors))
                    return false;
                break;
            }
        }

        if (!found) {
            // step 3: search for extension without version number

            foreach (const QString &p, database->fileImportPath) {
                dir = p+Slash+url;
                qmldir = dir+Slash_qmldir;

                QString absoluteFilePath = typeLoader->absoluteFilePath(qmldir);
                if (!absoluteFilePath.isEmpty()) {
                    found = true;
                    QString absolutePath = absoluteFilePath.left(absoluteFilePath.lastIndexOf(Slash)+1);
                    url = QLatin1String("file://") + absolutePath;
                    uri = resolvedUri(dir, database);
                    if (!importExtension(absoluteFilePath, uri, database, &qmldircomponents, errors))
                        return false;
                    break;
                }
            }
        }

        if (QDeclarativeMetaType::isModule(uri.toUtf8(), vmaj, vmin)) {
            versionFound = true;
        }

        if (!versionFound && qmldircomponents.isEmpty()) {
            if (errors) {
                QDeclarativeError error; // we don't set the url or line or column as these will be set by the loader.
                if (QDeclarativeMetaType::isAnyModule(uri.toUtf8()))
                    error.setDescription(QDeclarativeImportDatabase::tr("module \"%1\" version %2.%3 is not installed").arg(uri_arg).arg(vmaj).arg(vmin));
                else
                    error.setDescription(QDeclarativeImportDatabase::tr("module \"%1\" is not installed").arg(uri_arg));
                errors->prepend(error);
            }
            return false;
        }
    } else {
        if (importType == QDeclarativeScriptParser::Import::File && qmldircomponents.isEmpty()) {
            QString importUrl = resolveLocalUrl(base, uri + Slash_qmldir);
            QString localFileOrQrc = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(importUrl);
            if (!localFileOrQrc.isEmpty()) {
                QString dir = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(resolveLocalUrl(base, uri));
                if (!typeLoader->directoryExists(dir)) {
                    if (errors) {
                        QDeclarativeError error; // we don't set the line or column as these will be set by the loader.
                        error.setDescription(QDeclarativeImportDatabase::tr("\"%1\": no such directory").arg(uri_arg));
                        error.setUrl(QUrl(importUrl));
                        errors->prepend(error);
                    }
                    return false; // local import dirs must exist
                }
                uri = resolvedUri(dir, database);
                if (uri.endsWith(Slash))
                    uri.chop(1);
                if (!typeLoader->absoluteFilePath(localFileOrQrc).isEmpty()) {
                    if (!importExtension(localFileOrQrc,uri,database,&qmldircomponents,errors))
                        return false;
                }
            } else {
                if (prefix.isEmpty()) {
                    // directory must at least exist for valid import
                    QString localFileOrQrc = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(resolveLocalUrl(base, uri));
                    if (!typeLoader->directoryExists(localFileOrQrc)) {
                        if (errors) {
                            QDeclarativeError error; // we don't set the line or column as these will be set by the loader.
                            if (localFileOrQrc.isEmpty())
                                error.setDescription(QDeclarativeImportDatabase::tr("import \"%1\" has no qmldir and no namespace").arg(uri));
                            else
                                error.setDescription(QDeclarativeImportDatabase::tr("\"%1\": no such directory").arg(uri));
                            error.setUrl(QUrl(importUrl));
                            errors->prepend(error);
                        }
                        return false;
                    }
                }
            }
        }

        url = resolveLocalUrl(base, url);
    }

    if (!versionFound && vmaj > -1 && vmin > -1 && !qmldircomponents.isEmpty()) {
        QList<QDeclarativeDirParser::Component>::ConstIterator it = qmldircomponents.begin();
        int lowest_min = INT_MAX;
        int highest_min = INT_MIN;
        for (; it != qmldircomponents.end(); ++it) {
            if (it->majorVersion == vmaj) {
                lowest_min = qMin(lowest_min, it->minorVersion);
                highest_min = qMax(highest_min, it->minorVersion);
            }
        }
        if (lowest_min > vmin || highest_min < vmin) {
            if (errors) {
                QDeclarativeError error; // we don't set the url or line or column information, as these will be set by the loader.
                error.setDescription(QDeclarativeImportDatabase::tr("module \"%1\" version %2.%3 is not installed").arg(uri_arg).arg(vmaj).arg(vmin));
                errors->prepend(error);
            }
            return false;
        }
    }

    if (!url.endsWith(Slash))
        url += Slash;

    QDeclarativeImportedNamespace::Data data;
    data.uri = uri.toUtf8();
    data.url = url;
    data.majversion = vmaj;
    data.minversion = vmin;
    data.isLibrary = importType == QDeclarativeScriptParser::Import::Library;
    data.qmlDirComponents = qmldircomponents;
    s->imports.prepend(data);

    return true;
}

bool QDeclarativeImportsPrivate::find(const QByteArray& type, int *vmajor, int *vminor, QDeclarativeType** type_return,
                                      QString* url_return, QList<QDeclarativeError> *errors)
{
    QDeclarativeImportedNamespace *s = 0;
    int slash = type.indexOf('/');
    if (slash >= 0) {
        QString namespaceName = QString::fromUtf8(type.left(slash));
        s = set.value(namespaceName);
        if (!s) {
            if (errors) {
                QDeclarativeError error;
                error.setDescription(QDeclarativeImportDatabase::tr("- %1 is not a namespace").arg(namespaceName));
                errors->prepend(error);
            }
            return false;
        }
        int nslash = type.indexOf('/',slash+1);
        if (nslash > 0) {
            if (errors) {
                QDeclarativeError error;
                error.setDescription(QDeclarativeImportDatabase::tr("- nested namespaces not allowed"));
                errors->prepend(error);
            }
            return false;
        }
    } else {
        s = &unqualifiedset;
    }
    QByteArray unqualifiedtype = slash < 0 ? type : type.mid(slash+1); // common-case opt (QString::mid works fine, but slower)
    if (s) {
        if (s->find(typeLoader,unqualifiedtype,vmajor,vminor,type_return,url_return, &base, errors))
            return true;
        if (s->imports.count() == 1 && !s->imports.at(0).isLibrary && url_return && s != &unqualifiedset) {
            // qualified, and only 1 url
            *url_return = resolveLocalUrl(s->imports.at(0).url, QString::fromUtf8(unqualifiedtype) + QLatin1String(".qml"));
            return true;
        }
    }

    return false;
}

QDeclarativeImportedNamespace *QDeclarativeImportsPrivate::findNamespace(const QString& type)
{
    return set.value(type);
}

bool QDeclarativeImportedNamespace::find(QDeclarativeTypeLoader *typeLoader, const QByteArray& type, int *vmajor, int *vminor, QDeclarativeType** type_return,
          QString* url_return, QString *base, QList<QDeclarativeError> *errors)
{
    bool typeRecursionDetected = false;
    for (int i=0; i<imports.count(); ++i) {
        if (find_helper(typeLoader, imports.at(i), type, vmajor, vminor, type_return, url_return, base, &typeRecursionDetected)) {
            if (qmlCheckTypes()) {
                // check for type clashes
                for (int j = i+1; j<imports.count(); ++j) {
                    if (find_helper(typeLoader, imports.at(j), type, vmajor, vminor, 0, 0, base)) {
                        if (errors) {
                            QString u1 = imports.at(i).url;
                            QString u2 = imports.at(j).url;
                            if (base) {
                                QString b = *base;
                                int slash = b.lastIndexOf(QLatin1Char('/'));
                                if (slash >= 0) {
                                    b = b.left(slash+1);
                                    QString l = b.left(slash);
                                    if (u1.startsWith(b))
                                        u1 = u1.mid(b.count());
                                    else if (u1 == l)
                                        u1 = QDeclarativeImportDatabase::tr("local directory");
                                    if (u2.startsWith(b))
                                        u2 = u2.mid(b.count());
                                    else if (u2 == l)
                                        u2 = QDeclarativeImportDatabase::tr("local directory");
                                }
                            }

                            QDeclarativeError error;
                            if (u1 != u2) {
                                error.setDescription(QDeclarativeImportDatabase::tr("is ambiguous. Found in %1 and in %2").arg(u1).arg(u2));
                            } else {
                                error.setDescription(QDeclarativeImportDatabase::tr("is ambiguous. Found in %1 in version %2.%3 and %4.%5")
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
        QDeclarativeError error;
        if (typeRecursionDetected)
            error.setDescription(QDeclarativeImportDatabase::tr("is instantiated recursively"));
        else
            error.setDescription(QDeclarativeImportDatabase::tr("is not a type"));
        errors->prepend(error);
    }
    return false;
}

/*!
\class QDeclarativeImportDatabase
\brief The QDeclarativeImportDatabase class manages the QML imports for a QDeclarativeEngine.
\internal
*/
QDeclarativeImportDatabase::QDeclarativeImportDatabase(QDeclarativeEngine *e)
: engine(e)
{
    filePluginPath << QLatin1String(".");

    // Search order is applicationDirPath(), $QML_IMPORT_PATH, QLibraryInfo::ImportsPath

#ifndef QT_NO_SETTINGS
    QString installImportsPath =  QLibraryInfo::location(QLibraryInfo::ImportsPath);

#if defined(Q_OS_SYMBIAN)
    // Append imports path for all available drives in Symbian
    if (installImportsPath.at(1) != QChar(QLatin1Char(':'))) {
        QString tempPath = installImportsPath;
        if (tempPath.at(tempPath.length() - 1) != QDir::separator()) {
            tempPath += QDir::separator();
        }
        RFs& fs = qt_s60GetRFs();
        TPtrC tempPathPtr(reinterpret_cast<const TText*> (tempPath.constData()));
        TFindFile finder(fs);
        TInt err = finder.FindByDir(tempPathPtr, tempPathPtr);
        while (err == KErrNone) {
            QString foundDir(reinterpret_cast<const QChar *>(finder.File().Ptr()),
                             finder.File().Length());
            foundDir = QDir(foundDir).canonicalPath();
            addImportPath(foundDir);
            err = finder.Find();
        }
    } else {
        addImportPath(installImportsPath);
    }
#else
    addImportPath(installImportsPath);
#endif

#endif // QT_NO_SETTINGS

    // env import paths
    QByteArray envImportPath = qgetenv("QML_IMPORT_PATH");
    if (!envImportPath.isEmpty()) {
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
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

QDeclarativeImportDatabase::~QDeclarativeImportDatabase()
{
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
*/
bool QDeclarativeImports::addImport(QDeclarativeImportDatabase *importDb, 
                                    const QString& uri, const QString& prefix, int vmaj, int vmin, 
                                    QDeclarativeScriptParser::Import::Type importType, 
                                    const QDeclarativeDirComponents &qmldircomponentsnetwork, 
                                    QList<QDeclarativeError> *errors)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QDeclarativeImports(" << qPrintable(baseUrl().toString()) << ")" << "::addImport: " 
                           << uri << " " << vmaj << '.' << vmin << " " 
                           << (importType==QDeclarativeScriptParser::Import::Library? "Library" : "File") 
                           << " as " << prefix;

    return d->add(qmldircomponentsnetwork, uri, prefix, vmaj, vmin, importType, importDb, errors);
}

/*!
  \internal

  Returns the result of the merge of \a baseName with \a path, \a suffixes, and \a prefix.
  The \a prefix must contain the dot.

  \a qmldirPath is the location of the qmldir file.
 */
QString QDeclarativeImportDatabase::resolvePlugin(QDeclarativeTypeLoader *typeLoader,
                                                  const QString &qmldirPath, const QString &qmldirPluginPath,
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
        qDebug() << "QDeclarativeImportDatabase::resolvePlugin: Could not resolve plugin" << baseName 
                 << "in" << qmldirPath;

    return QString();
}

/*!
  \internal

  Returns the result of the merge of \a baseName with \a dir and the platform suffix.

  \table
  \header \i Platform \i Valid suffixes
  \row \i Windows     \i \c .dll
  \row \i Unix/Linux  \i \c .so
  \row \i AIX  \i \c .a
  \row \i HP-UX       \i \c .sl, \c .so (HP-UXi)
  \row \i Mac OS X    \i \c .dylib, \c .bundle, \c .so
  \row \i Symbian     \i \c .dll
  \endtable

  Version number on unix are ignored.
*/
QString QDeclarativeImportDatabase::resolvePlugin(QDeclarativeTypeLoader *typeLoader,
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
#elif defined(Q_OS_SYMBIAN)
    return resolvePlugin(typeLoader, qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
                         << QLatin1String(".dll")
                         << QLatin1String(".qtplugin"));
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
QStringList QDeclarativeImportDatabase::pluginPathList() const
{
    return filePluginPath;
}

/*!
    \internal
*/
void QDeclarativeImportDatabase::setPluginPathList(const QStringList &paths)
{
    filePluginPath = paths;
}

/*!
    \internal
*/
void QDeclarativeImportDatabase::addPluginPath(const QString& path)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QDeclarativeImportDatabase::addPluginPath: " << path;

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
void QDeclarativeImportDatabase::addImportPath(const QString& path)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QDeclarativeImportDatabase::addImportPath: " << path;

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
QStringList QDeclarativeImportDatabase::importPathList() const
{
    return fileImportPath;
}

/*!
    \internal
*/
void QDeclarativeImportDatabase::setImportPathList(const QStringList &paths)
{
    fileImportPath = paths;
}

/*!
    \internal
*/
bool QDeclarativeImportDatabase::importPlugin(const QString &filePath, const QString &uri, QList<QDeclarativeError> *errors)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QDeclarativeImportDatabase::importPlugin: " << uri << " from " << filePath;

#ifndef QT_NO_LIBRARY
    QFileInfo fileInfo(filePath);
    const QString absoluteFilePath = fileInfo.absoluteFilePath();

    bool engineInitialized = initializedPlugins.contains(absoluteFilePath);
    bool typesRegistered = qmlEnginePluginsWithRegisteredTypes()->contains(absoluteFilePath);

    if (typesRegistered) {
        Q_ASSERT_X(qmlEnginePluginsWithRegisteredTypes()->value(absoluteFilePath) == uri,
                   "QDeclarativeImportDatabase::importExtension",
                   "Internal error: Plugin imported previously with different uri");
    }

    if (!engineInitialized || !typesRegistered) {
        if (!QDeclarative_isFileCaseCorrect(absoluteFilePath)) {
            if (errors) {
                QDeclarativeError error;
                error.setDescription(tr("File name case mismatch for \"%2\"").arg(absoluteFilePath));
                errors->prepend(error);
            }
            return false;
        }
        QPluginLoader loader(absoluteFilePath);

        if (!loader.load()) {
            if (errors) {
                QDeclarativeError error;
                error.setDescription(loader.errorString());
                errors->prepend(error);
            }
            return false;
        }

        if (QDeclarativeExtensionInterface *iface = qobject_cast<QDeclarativeExtensionInterface *>(loader.instance())) {

            const QByteArray bytes = uri.toUtf8();
            const char *moduleId = bytes.constData();
            if (!typesRegistered) {

                // ### this code should probably be protected with a mutex.
                qmlEnginePluginsWithRegisteredTypes()->insert(absoluteFilePath, uri);
                iface->registerTypes(moduleId);
            }
            if (!engineInitialized) {
                // things on the engine (eg. adding new global objects) have to be done for every engine.

                // protect against double initialization
                initializedPlugins.insert(absoluteFilePath);
                iface->initializeEngine(engine, moduleId);
            }
        } else {
            if (errors) {
                QDeclarativeError error;
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
