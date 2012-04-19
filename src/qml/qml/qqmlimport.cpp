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
#include <QtCore/qfileinfo.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qlibraryinfo.h>
#include <QtQml/qqmlextensioninterface.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlImportTrace, QML_IMPORT_TRACE)
DEFINE_BOOL_CONFIG_OPTION(qmlCheckTypes, QML_CHECK_TYPES)

static bool greaterThan(const QString &s1, const QString &s2)
{
    return s1 > s2;
}

QString resolveLocalUrl(const QString &url, const QString &relative)
{
    return QUrl(url).resolved(QUrl(relative)).toString();

    //XXX Find out why this broke with new QUrl.
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

class QQmlImportedNamespace 
{
public:
    struct Data {
        QString uri;
        QString url;
        int majversion;
        int minversion;
        bool isLibrary;
        QQmlDirComponents qmlDirComponents;
        QQmlDirScripts qmlDirScripts;
    };
    QList<Data> imports;


    bool find_helper(QQmlTypeLoader *typeLoader, const Data &data, const QString& type, int *vmajor, int *vminor,
                                 QQmlType** type_return, QString* url_return,
                                 QString *base = 0, bool *typeRecursionDetected = 0);
    bool find(QQmlTypeLoader *typeLoader, const QString& type, int *vmajor, int *vminor, QQmlType** type_return,
              QString* url_return, QString *base = 0, QList<QQmlError> *errors = 0);
};

class QQmlImportsPrivate {
public:
    QQmlImportsPrivate(QQmlTypeLoader *loader);
    ~QQmlImportsPrivate();

    bool importExtension(const QString &absoluteFilePath, const QString &uri, 
                         QQmlImportDatabase *database, QQmlDirComponents* components, 
                         QQmlDirScripts *scripts,
                         QList<QQmlError> *errors);

    QString resolvedUri(const QString &dir_arg, QQmlImportDatabase *database);
    QString add(const QQmlDirComponents &qmldircomponentsnetwork,
             const QString& uri_arg, const QString& prefix, 
             int vmaj, int vmin, QQmlScript::Import::Type importType, 
             QQmlImportDatabase *database, QList<QQmlError> *errors);
    bool find(const QString& type, int *vmajor, int *vminor,
              QQmlType** type_return, QString* url_return, QList<QQmlError> *errors);

    QQmlImportedNamespace *findNamespace(const QString& type);

    QUrl baseUrl;
    QString base;
    int ref;

    QSet<QString> qmlDirFilesForWhichPluginsHaveBeenLoaded;
    QQmlImportedNamespace unqualifiedset;
    QHash<QString,QQmlImportedNamespace* > set;
    QQmlTypeLoader *typeLoader;
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
    const QQmlImportedNamespace &set = d->unqualifiedset;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportedNamespace::Data &data = set.imports.at(ii);
        QQmlTypeModule *module = QQmlMetaType::typeModule(data.uri, data.majversion);
        if (module)
            cache->m_anonymousImports.append(QQmlTypeModuleVersion(module, data.minversion));
    }

    for (QHash<QString,QQmlImportedNamespace* >::ConstIterator iter = d->set.begin();
         iter != d->set.end(); 
         ++iter) {

        const QQmlImportedNamespace &set = *iter.value();
        for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
            const QQmlImportedNamespace::Data &data = set.imports.at(ii);
            QQmlTypeModule *module = QQmlMetaType::typeModule(data.uri, data.majversion);
            if (module) {
                QQmlTypeNameCache::Import &import = cache->m_namedImports[iter.key()];
                import.modules.append(QQmlTypeModuleVersion(module, data.minversion));
            }

            QQmlMetaType::ModuleApi moduleApi = QQmlMetaType::moduleApi(data.uri, data.majversion, data.minversion);
            if (moduleApi.script || moduleApi.qobject) {
                QQmlTypeNameCache::Import &import = cache->m_namedImports[iter.key()];
                QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
                import.moduleApi = ep->moduleApiInstance(moduleApi);
            }
        }
    }
}

QList<QQmlImports::ScriptReference> QQmlImports::resolvedScripts() const
{
    QList<QQmlImports::ScriptReference> scripts;

    const QQmlImportedNamespace &set = d->unqualifiedset;

    for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
        const QQmlImportedNamespace::Data &data = set.imports.at(ii);

        foreach (const QQmlDirParser::Script &script, data.qmlDirScripts) {
            ScriptReference ref;
            ref.nameSpace = script.nameSpace;
            ref.location = QUrl(data.url).resolved(QUrl(script.fileName));
            scripts.append(ref);
        }
    }

    for (QHash<QString,QQmlImportedNamespace* >::ConstIterator iter = d->set.constBegin();
         iter != d->set.constEnd();
         ++iter) {
        const QQmlImportedNamespace &set = *iter.value();

        for (int ii = set.imports.count() - 1; ii >= 0; --ii) {
            const QQmlImportedNamespace::Data &data = set.imports.at(ii);

            foreach (const QQmlDirParser::Script &script, data.qmlDirScripts) {
                ScriptReference ref;
                ref.nameSpace = script.nameSpace;
                ref.qualifier = iter.key();
                ref.location = QUrl(data.url).resolved(QUrl(script.fileName));
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
  \li a QQmlImportedNamespace stored at \a ns_return,
  \li a QQmlType stored at \a type_return, or
  \li a component located at \a url_return.
  \endlist

  If any return pointer is 0, the corresponding search is not done.

  \sa addImport()
*/
bool QQmlImports::resolveType(const QString& type,
                                      QQmlType** type_return, QString* url_return, int *vmaj, int *vmin,
                                      QQmlImportedNamespace** ns_return, QList<QQmlError> *errors) const
{
    QQmlImportedNamespace* ns = d->findNamespace(type);
    if (ns) {
        if (ns_return)
            *ns_return = ns;
        return true;
    }
    if (type_return || url_return) {
        if (d->find(type,vmaj,vmin,type_return,url_return, errors)) {
            if (qmlImportTrace()) {
                if (type_return && *type_return && url_return && !url_return->isEmpty())
                    qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) << ")" << "::resolveType: " 
                                       << type << " => " << (*type_return)->typeName() << " " << *url_return;
                if (type_return && *type_return)
                    qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) << ")" << "::resolveType: " 
                                       << type << " => " << (*type_return)->typeName();
                if (url_return && !url_return->isEmpty())
                    qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) << ")" << "::resolveType: " 
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
  a QQmlType stored at \a type_return, or
  a component located at \a url_return.

  If either return pointer is 0, the corresponding search is not done.
*/
bool QQmlImports::resolveType(QQmlImportedNamespace* ns, const QString& type,
                                      QQmlType** type_return, QString* url_return,
                                      int *vmaj, int *vmin) const
{
    return ns->find(d->typeLoader,type,vmaj,vmin,type_return,url_return);
}

bool QQmlImportedNamespace::find_helper(QQmlTypeLoader *typeLoader, const Data &data, const QString& type, int *vmajor, int *vminor,
                                 QQmlType** type_return, QString* url_return,
                                 QString *base, bool *typeRecursionDetected)
{
    int vmaj = data.majversion;
    int vmin = data.minversion;

    if (vmaj >= 0 && vmin >= 0) {
        QString qt = data.uri + QLatin1Char('/') + type;
        QQmlType *t = QQmlMetaType::qmlType(qt,vmaj,vmin);
        if (t) {
            if (vmajor) *vmajor = vmaj;
            if (vminor) *vminor = vmin;
            if (type_return)
                *type_return = t;
            return true;
        }
    }

    const QQmlDirComponents &qmldircomponents = data.qmlDirComponents;
    bool typeWasDeclaredInQmldir = false;
    if (!qmldircomponents.isEmpty()) {
        foreach (const QQmlDirParser::Component &c, qmldircomponents) {
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
        QString url(data.url + type + QLatin1String(".qml"));
        QString file = QQmlEnginePrivate::urlToLocalFileOrQrc(url);
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

QQmlImportsPrivate::QQmlImportsPrivate(QQmlTypeLoader *loader)
    : ref(1), typeLoader(loader)
{
}

QQmlImportsPrivate::~QQmlImportsPrivate()
{
    foreach (QQmlImportedNamespace* s, set.values())
        delete s;
}

bool QQmlImportsPrivate::importExtension(const QString &absoluteFilePath, const QString &uri,
                                                 QQmlImportDatabase *database,
                                                 QQmlDirComponents* components,
                                                 QQmlDirScripts* scripts,
                                                 QList<QQmlError> *errors)
{
    const QQmlDirParser *qmldirParser = typeLoader->qmlDirParser(absoluteFilePath);
    if (qmldirParser->hasError()) {
        if (errors) {
            const QList<QQmlError> qmldirErrors = qmldirParser->errors(uri);
            for (int i = 0; i < qmldirErrors.size(); ++i)
                errors->prepend(qmldirErrors.at(i));
        }
        return false;
    }

    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(base) << "::importExtension: "
                           << "loaded " << absoluteFilePath;

    if (! qmlDirFilesForWhichPluginsHaveBeenLoaded.contains(absoluteFilePath)) {
        qmlDirFilesForWhichPluginsHaveBeenLoaded.insert(absoluteFilePath);

        QString qmldirPath = absoluteFilePath;
        int slash = absoluteFilePath.lastIndexOf(QLatin1Char('/'));
        if (slash > 0)
            qmldirPath.truncate(slash);
        foreach (const QQmlDirParser::Plugin &plugin, qmldirParser->plugins()) {

            QString resolvedFilePath = database->resolvePlugin(typeLoader, qmldirPath, plugin.path, plugin.name);
            if (!resolvedFilePath.isEmpty()) {
                if (!database->importPlugin(resolvedFilePath, uri, errors)) {
                    if (errors) {
                        // XXX TODO: should we leave the import plugin error alone?
                        // Here, we pop it off the top and coalesce it into this error's message.
                        // The reason is that the lower level may add url and line/column numbering information.
                        QQmlError poppedError = errors->takeFirst();
                        QQmlError error;
                        error.setDescription(QQmlImportDatabase::tr("plugin cannot be loaded for module \"%1\": %2").arg(uri).arg(poppedError.description()));
                        error.setUrl(QUrl::fromLocalFile(absoluteFilePath));
                        errors->prepend(error);
                    }
                    return false;
                }
            } else {
                if (errors) {
                    QQmlError error;
                    error.setDescription(QQmlImportDatabase::tr("module \"%1\" plugin \"%2\" not found").arg(uri).arg(plugin.name));
                    error.setUrl(QUrl::fromLocalFile(absoluteFilePath));
                    errors->prepend(error);
                }
                return false;
            }
        }
    }

    if (components)
        *components = qmldirParser->components();
    if (scripts)
        *scripts = qmldirParser->scripts();

    return true;
}

QString QQmlImportsPrivate::resolvedUri(const QString &dir_arg, QQmlImportDatabase *database)
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

QString QQmlImportsPrivate::add(const QQmlDirComponents &qmldircomponentsnetwork,
                                     const QString& uri_arg, const QString& prefix, int vmaj, int vmin, 
                                     QQmlScript::Import::Type importType, 
                                     QQmlImportDatabase *database, QList<QQmlError> *errors)
{
    static QLatin1String Slash_qmldir("/qmldir");
    static QLatin1Char Slash('/');

    QQmlDirComponents qmldircomponents = qmldircomponentsnetwork;
    QQmlDirScripts qmldirscripts;
    QString uri = uri_arg;
    QQmlImportedNamespace *s;
    if (prefix.isEmpty()) {
        s = &unqualifiedset;
    } else {
        s = set.value(prefix);
        if (!s)
            set.insert(prefix,(s=new QQmlImportedNamespace));
    }
    QString url = uri;
    bool versionFound = false;
    if (importType == QQmlScript::Import::Library) {

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

                const QString absolutePath = fi.absolutePath();
                if (absolutePath.at(0) == QLatin1Char(':'))
                    url = QLatin1String("qrc://") + absolutePath.mid(1);
                else
                    url = QUrl::fromLocalFile(fi.absolutePath()).toString();
                uri = resolvedUri(dir, database);
                if (!importExtension(absoluteFilePath, uri, database, &qmldircomponents, &qmldirscripts, errors))
                    return QString();
                break;
            }
        }

        if (!found) {
            // step 2: search for extension with encoded version major
            foreach (const QString &p, database->fileImportPath) {
                dir = p+Slash+url;

                QFileInfo fi(dir+QString(QLatin1String(".%1")).arg(vmaj)+QLatin1String("/qmldir"));
                const QString absoluteFilePath = fi.absoluteFilePath();

                if (fi.isFile()) {
                    found = true;

                    const QString absolutePath = fi.absolutePath();
                    if (absolutePath.at(0) == QLatin1Char(':'))
                        url = QLatin1String("qrc://") + absolutePath.mid(1);
                    else
                        url = QUrl::fromLocalFile(fi.absolutePath()).toString();
                    uri = resolvedUri(dir, database);
                    if (!importExtension(absoluteFilePath, uri, database, &qmldircomponents, &qmldirscripts, errors))
                        return QString();
                    break;
                }
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
                    if (absolutePath.at(0) == QLatin1Char(':'))
                        url = QLatin1String("qrc://") + absolutePath.mid(1);
                    else
                        url = QUrl::fromLocalFile(absolutePath).toString();
                    uri = resolvedUri(dir, database);
                    if (!importExtension(absoluteFilePath, uri, database, &qmldircomponents, &qmldirscripts, errors))
                        return QString();
                    break;
                }
            }
        }

        if (QQmlMetaType::isModule(uri, vmaj, vmin))
            versionFound = true;

        if (!versionFound && qmldircomponents.isEmpty() && qmldirscripts.isEmpty()) {
            if (errors) {
                QQmlError error; // we don't set the url or line or column as these will be set by the loader.
                if (QQmlMetaType::isAnyModule(uri))
                    error.setDescription(QQmlImportDatabase::tr("module \"%1\" version %2.%3 is not installed").arg(uri_arg).arg(vmaj).arg(vmin));
                else
                    error.setDescription(QQmlImportDatabase::tr("module \"%1\" is not installed").arg(uri_arg));
                errors->prepend(error);
            }
            return QString();
        }
    } else {
        if (importType == QQmlScript::Import::File && qmldircomponents.isEmpty()) {
            QString importUrl = resolveLocalUrl(base, uri + Slash_qmldir);
            QString localFileOrQrc = QQmlEnginePrivate::urlToLocalFileOrQrc(importUrl);
            if (!localFileOrQrc.isEmpty()) {
                QString dir = QQmlEnginePrivate::urlToLocalFileOrQrc(resolveLocalUrl(base, uri));
                if (!typeLoader->directoryExists(dir)) {
                    if (errors) {
                        QQmlError error; // we don't set the line or column as these will be set by the loader.
                        error.setDescription(QQmlImportDatabase::tr("\"%1\": no such directory").arg(uri_arg));
                        error.setUrl(QUrl(importUrl));
                        errors->prepend(error);
                    }
                    return QString(); // local import dirs must exist
                }
                uri = resolvedUri(dir, database);
                if (uri.endsWith(Slash))
                    uri.chop(1);
                if (!typeLoader->absoluteFilePath(localFileOrQrc).isEmpty()) {
                    if (!importExtension(localFileOrQrc,uri,database,&qmldircomponents,&qmldirscripts,errors))
                        return QString();
                }
            } else {
                if (prefix.isEmpty()) {
                    // directory must at least exist for valid import
                    QString localFileOrQrc = QQmlEnginePrivate::urlToLocalFileOrQrc(resolveLocalUrl(base, uri));
                    if (!typeLoader->directoryExists(localFileOrQrc)) {
                        if (errors) {
                            QQmlError error; // we don't set the line or column as these will be set by the loader.
                            if (localFileOrQrc.isEmpty())
                                error.setDescription(QQmlImportDatabase::tr("import \"%1\" has no qmldir and no namespace").arg(uri));
                            else
                                error.setDescription(QQmlImportDatabase::tr("\"%1\": no such directory").arg(uri));
                            error.setUrl(QUrl(importUrl));
                            errors->prepend(error);
                        }
                        return QString();
                    }
                }
            }
        }

        url = resolveLocalUrl(base, url);
    }

    if (!versionFound && (vmaj > -1) && (vmin > -1) && !qmldircomponents.isEmpty()) {
        int lowest_min = INT_MAX;
        int highest_min = INT_MIN;

        QList<QQmlDirParser::Component>::const_iterator cend = qmldircomponents.constEnd();
        for (QList<QQmlDirParser::Component>::const_iterator cit = qmldircomponents.constBegin(); cit != cend; ++cit) {
            if (cit->majorVersion == vmaj) {
                lowest_min = qMin(lowest_min, cit->minorVersion);
                highest_min = qMax(highest_min, cit->minorVersion);
            }
        }

        if (lowest_min > vmin || highest_min < vmin) {
            if (errors) {
                QQmlError error; // we don't set the url or line or column information, as these will be set by the loader.
                error.setDescription(QQmlImportDatabase::tr("module \"%1\" version %2.%3 is not installed").arg(uri_arg).arg(vmaj).arg(vmin));
                errors->prepend(error);
            }
            return QString();
        }
    }

    if (!url.endsWith(Slash))
        url += Slash;

    QMap<QString, QQmlDirParser::Script> scripts;

    if (!qmldirscripts.isEmpty()) {
        // Verify that we haven't imported these scripts already
        QList<QQmlImportedNamespace::Data>::const_iterator end = s->imports.constEnd();
        for (QList<QQmlImportedNamespace::Data>::const_iterator it = s->imports.constBegin(); it != end; ++it) {
            if (it->uri == uri) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("\"%1\" is ambiguous. Found in %2 and in %3").arg(uri).arg(url).arg(it->url));
                errors->prepend(error);
                return QString();
            }
        }

        QList<QQmlDirParser::Script>::const_iterator send = qmldirscripts.constEnd();
        for (QList<QQmlDirParser::Script>::const_iterator sit = qmldirscripts.constBegin(); sit != send; ++sit) {
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

    QQmlImportedNamespace::Data data;
    data.uri = uri;
    data.url = url;
    data.majversion = vmaj;
    data.minversion = vmin;
    data.isLibrary = importType == QQmlScript::Import::Library;
    data.qmlDirComponents = qmldircomponents;
    data.qmlDirScripts = scripts.values();

    s->imports.prepend(data);

    return data.url;
}

bool QQmlImportsPrivate::find(const QString& type, int *vmajor, int *vminor, QQmlType** type_return,
                                      QString* url_return, QList<QQmlError> *errors)
{
    QQmlImportedNamespace *s = 0;
    int slash = type.indexOf(QLatin1Char('/'));
    if (slash >= 0) {
        QString namespaceName = type.left(slash);
        s = set.value(namespaceName);
        if (!s) {
            if (errors) {
                QQmlError error;
                error.setDescription(QQmlImportDatabase::tr("- %1 is not a namespace").arg(namespaceName));
                errors->prepend(error);
            }
            return false;
        }
        int nslash = type.indexOf(QLatin1Char('/'),slash+1);
        if (nslash > 0) {
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
    QString unqualifiedtype = slash < 0 ? type : type.mid(slash+1); // common-case opt (QString::mid works fine, but slower)
    if (s) {
        if (s->find(typeLoader,unqualifiedtype,vmajor,vminor,type_return,url_return, &base, errors))
            return true;
        if (s->imports.count() == 1 && !s->imports.at(0).isLibrary && url_return && s != &unqualifiedset) {
            // qualified, and only 1 url
            *url_return = resolveLocalUrl(s->imports.at(0).url, unqualifiedtype + QLatin1String(".qml"));
            return true;
        }
    }

    return false;
}

QQmlImportedNamespace *QQmlImportsPrivate::findNamespace(const QString& type)
{
    return set.value(type);
}

bool QQmlImportedNamespace::find(QQmlTypeLoader *typeLoader, const QString& type, int *vmajor, int *vminor, QQmlType** type_return,
          QString* url_return, QString *base, QList<QQmlError> *errors)
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
}

/*!
  \internal

  Adds information to \a imports such that subsequent calls to resolveType()
  will resolve types qualified by \a prefix by considering types found at the given \a uri.

  The uri is either a directory (if importType is FileImport), or a URI resolved using paths
  added via addImportPath() (if importType is LibraryImport).

  The \a prefix may be empty, in which case the import location is considered for
  unqualified types.

  Returns the resolved URL of the import on success.

  The base URL must already have been set with Import::setBaseUrl().
*/
QString QQmlImports::addImport(QQmlImportDatabase *importDb,
                                    const QString& uri, const QString& prefix, int vmaj, int vmin, 
                                    QQmlScript::Import::Type importType, 
                                    const QQmlDirComponents &qmldircomponentsnetwork, 
                                    QList<QQmlError> *errors)
{
    if (qmlImportTrace())
        qDebug().nospace() << "QQmlImports(" << qPrintable(baseUrl().toString()) << ")" << "::addImport: " 
                           << uri << " " << vmaj << '.' << vmin << " " 
                           << (importType==QQmlScript::Import::Library? "Library" : "File") 
                           << " as " << prefix;

    return d->add(qmldircomponentsnetwork, uri, prefix, vmaj, vmin, importType, importDb, errors);
}

/*!
  \internal

  Returns the result of the merge of \a baseName with \a path, \a suffixes, and \a prefix.
  The \a prefix must contain the dot.

  \a qmldirPath is the location of the qmldir file.
 */
QString QQmlImportDatabase::resolvePlugin(QQmlTypeLoader *typeLoader,
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
