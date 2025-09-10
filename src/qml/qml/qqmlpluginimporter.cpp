// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpluginimporter_p.h"
#include "qqmlimport_p.h"

#include <private/qqmlextensionplugin_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlglobal_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qdir.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qjsonarray.h>

#include <unordered_map>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQmlImport)

struct QmlPlugin {
    std::unique_ptr<QPluginLoader> loader;
};

class PluginMap
{
    Q_DISABLE_COPY_MOVE(PluginMap)
public:
    PluginMap() = default;
    ~PluginMap() = default;

    // This is a std::unordered_map because QHash cannot handle move-only types.
    using Container = std::unordered_map<QString, QmlPlugin>;

private:
    QBasicMutex mutex;
    Container plugins;
    friend class PluginMapPtr;
};

class PluginMapPtr
{
    Q_DISABLE_COPY_MOVE(PluginMapPtr)
public:
    PluginMapPtr(PluginMap *map) : map(map), locker(&map->mutex) {}
    ~PluginMapPtr() = default;

    PluginMap::Container &operator*() { return map->plugins; }
    const PluginMap::Container &operator*() const { return map->plugins; }

    PluginMap::Container *operator->() { return &map->plugins; }
    const PluginMap::Container *operator->() const { return &map->plugins; }

private:
    PluginMap *map;
    QMutexLocker<QBasicMutex> locker;
};

Q_GLOBAL_STATIC(PluginMap, qmlPluginsById); // stores the uri and the PluginLoaders

static QVector<QStaticPlugin> makePlugins()
{
    QVector<QStaticPlugin> plugins;
    // To avoid traversing all static plugins for all imports, we cut down
    // the list the first time called to only contain QML plugins:
    const auto staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &plugin : staticPlugins) {
        const QString iid = plugin.metaData().value(QLatin1String("IID")).toString();
        if (iid == QLatin1String(QQmlEngineExtensionInterface_iid)
                || iid == QLatin1String(QQmlExtensionInterface_iid)
                || iid == QLatin1String(QQmlExtensionInterface_iid_old)) {
            if (Q_UNLIKELY(iid == QLatin1String(QQmlExtensionInterface_iid_old))) {
                qWarning()
                        << "Found plugin with old IID, this will be unsupported in upcoming Qt releases:"
                        << plugin.metaData();
            }
            plugins.append(plugin);
        }
    }
    return plugins;
}

/*
    Returns the list of possible versioned URI combinations. For example, if \a uri is
    QtQml.Models, \a vmaj is 2, and \a vmin is 0, this method returns the following:
    [QtQml.Models.2.0, QtQml.2.0.Models, QtQml.Models.2, QtQml.2.Models, QtQml.Models]
 */
static QStringList versionUriList(const QString &uri, QTypeRevision version)
{
    QStringList result;
    for (int mode = QQmlImports::FullyVersioned; mode <= QQmlImports::Unversioned; ++mode) {
        int index = uri.size();
        do {
            QString versionUri = uri;
            versionUri.insert(index, QQmlImports::versionString(
                                  version, QQmlImports::ImportVersion(mode)));
            result += versionUri;

            index = uri.lastIndexOf(u'.', index - 1);
        } while (index > 0 && mode != QQmlImports::Unversioned);
    }
    return result;
}

static bool unloadPlugin(const std::pair<const QString, QmlPlugin> &plugin)
{
    const auto &loader = plugin.second.loader;
    if (!loader)
        return false;

#if QT_CONFIG(library)
    if (auto extensionPlugin = qobject_cast<QQmlExtensionPlugin *>(loader->instance()))
        extensionPlugin->unregisterTypes();

# ifndef Q_OS_MACOS
    if (!loader->unload()) {
        qWarning("Unloading %s failed: %s", qPrintable(plugin.first),
                 qPrintable(loader->errorString()));
        return false;
    }
# endif
#endif

    return true;
}

void qmlClearEnginePlugins()
{
    PluginMapPtr plugins(qmlPluginsById());
    for (const auto &plugin : std::as_const(*plugins))
        unloadPlugin(plugin);
    plugins->clear();
}

bool QQmlPluginImporter::removePlugin(const QString &pluginId)
{
    PluginMapPtr plugins(qmlPluginsById());

    auto it = plugins->find(pluginId);
    if (it == plugins->end())
        return false;

    const bool success = unloadPlugin(*it);

    plugins->erase(it);
    return success;
}

QStringList QQmlPluginImporter::plugins()
{
    PluginMapPtr plugins(qmlPluginsById());
    QStringList results;
    for (auto it = plugins->cbegin(), end = plugins->cend(); it != end; ++it) {
        if (it->second.loader != nullptr)
            results.append(it->first);
    }
    return results;
}

QString QQmlPluginImporter::truncateToDirectory(const QString &qmldirFilePath)
{
    const int slash = qmldirFilePath.lastIndexOf(u'/');
    return slash > 0 ? qmldirFilePath.left(slash) : qmldirFilePath;
}

void QQmlPluginImporter::finalizePlugin(QObject *instance, const QString &pluginId) {
    // The plugin's per-engine initialization does not need lock protection, as this function is
    // only called from the engine specific loader thread and importDynamicPlugin as well as
    // importStaticPlugin are the only places of access.

    database->initializedPlugins.insert(pluginId);
    if (auto *extensionIface = qobject_cast<QQmlExtensionInterface *>(instance))
        typeLoader->initializeEngine(extensionIface, uri.toUtf8().constData());
    else if (auto *engineIface = qobject_cast<QQmlEngineExtensionInterface *>(instance))
        typeLoader->initializeEngine(engineIface, uri.toUtf8().constData());
}

QTypeRevision QQmlPluginImporter::importStaticPlugin(QObject *instance, const QString &pluginId) {
    // Dynamic plugins are differentiated by their filepath. For static plugins we
    // don't have that information so we use their address as key instead.
    QTypeRevision importVersion = version;
    {
        PluginMapPtr plugins(qmlPluginsById());

        // Plugin types are global across all engines and should only be
        // registered once. But each engine still needs to be initialized.
        bool typesRegistered = plugins->find(pluginId) != plugins->end();

        if (!typesRegistered) {
            plugins->insert(std::make_pair(pluginId, QmlPlugin()));
            if (QQmlMetaType::registerPluginTypes(
                        instance, QFileInfo(qmldirPath).absoluteFilePath(), uri,
                        qmldir->typeNamespace(), importVersion, errors)
                    == QQmlMetaType::RegistrationResult::Failure) {
                return QTypeRevision();
            }

            importVersion = QQmlImportDatabase::lockModule(
                        uri, qmldir->typeNamespace(), importVersion, errors);
            if (!importVersion.isValid())
                return QTypeRevision();
        }

        // Release the lock on plugins early as we're done with the global part. Releasing the lock
        // also allows other QML loader threads to acquire the lock while this thread is blocking
        // in the initializeEngine call to the gui thread (which in turn may be busy waiting for
        // other QML loader threads and thus not process the initializeEngine call).
    }

    if (!database->initializedPlugins.contains(pluginId))
        finalizePlugin(instance, pluginId);

    return QQmlImports::validVersion(importVersion);
}

QTypeRevision QQmlPluginImporter::importDynamicPlugin(
        const QString &filePath, const QString &pluginId, bool optional)
{
    QObject *instance = nullptr;
    QTypeRevision importVersion = version;

    const bool engineInitialized = database->initializedPlugins.contains(pluginId);
    {
        PluginMapPtr plugins(qmlPluginsById());
        bool typesRegistered = plugins->find(pluginId) != plugins->end();

        if (!engineInitialized || !typesRegistered) {
            const QFileInfo fileInfo(filePath);
            if (!typesRegistered && optional) {
                switch (QQmlMetaType::registerPluginTypes(
                            nullptr, fileInfo.absolutePath(), uri, qmldir->typeNamespace(),
                            importVersion, errors)) {
                case QQmlMetaType::RegistrationResult::NoRegistrationFunction:
                    // try again with plugin
                    break;
                case QQmlMetaType::RegistrationResult::Success:
                    importVersion = QQmlImportDatabase::lockModule(
                                uri, qmldir->typeNamespace(), importVersion, errors);
                    if (!importVersion.isValid())
                        return QTypeRevision();
                    // instance and loader intentionally left at nullptr
                    plugins->insert(std::make_pair(pluginId, QmlPlugin()));
                    // Not calling initializeEngine with null instance
                    database->initializedPlugins.insert(pluginId);
                    return importVersion;
                case QQmlMetaType::RegistrationResult::Failure:
                    return QTypeRevision();
                }
            }

#if QT_CONFIG(library)
            if (!typesRegistered) {

                // Check original filePath. If that one is empty, not being able
                // to load the plugin is not an error. We were just checking if
                // the types are already available. absoluteFilePath can still be
                // empty if filePath is not.
                if (filePath.isEmpty())
                    return QTypeRevision();

                const QString absoluteFilePath = fileInfo.absoluteFilePath();
                if (!QQml_isFileCaseCorrect(absoluteFilePath)) {
                    if (errors) {
                        QQmlError error;
                        error.setDescription(
                                    QQmlImportDatabase::tr("File name case mismatch for \"%1\"")
                                    .arg(absoluteFilePath));
                        errors->prepend(error);
                    }
                    return QTypeRevision();
                }

                QmlPlugin plugin;
                plugin.loader = std::make_unique<QPluginLoader>(absoluteFilePath);
                if (!plugin.loader->load()) {
                    if (errors) {
                        QQmlError error;
                        error.setDescription(plugin.loader->errorString());
                        errors->prepend(error);
                    }
                    return QTypeRevision();
                }

                instance = plugin.loader->instance();
                plugins->insert(std::make_pair(pluginId, std::move(plugin)));

                // Continue with shared code path for dynamic and static plugins:
                if (QQmlMetaType::registerPluginTypes(
                            instance, fileInfo.absolutePath(), uri, qmldir->typeNamespace(),
                            importVersion, errors)
                        == QQmlMetaType::RegistrationResult::Failure) {
                    return QTypeRevision();
                }

                importVersion = QQmlImportDatabase::lockModule(
                            uri, qmldir->typeNamespace(), importVersion, errors);
                if (!importVersion.isValid())
                    return QTypeRevision();
            } else {
                auto it = plugins->find(pluginId);
                if (it != plugins->end() && it->second.loader)
                    instance = it->second.loader->instance();
            }
#else
            // Here plugin is not optional and NOT QT_CONFIG(library)
            // Cannot finalize such plugin and return valid, because no types are registered.
            // Just return invalid.
            if (!optional)
                return QTypeRevision();
#endif // QT_CONFIG(library)
        }

        // Release the lock on plugins early as we're done with the global part. Releasing the lock
        // also allows other QML loader threads to acquire the lock while this thread is blocking
        // in the initializeEngine call to the gui thread (which in turn may be busy waiting for
        // other QML loader threads and thus not process the initializeEngine call).
    }

    if (!engineInitialized)
        finalizePlugin(instance, pluginId);

    return QQmlImports::validVersion(importVersion);
}

/*!
  \internal

  Searches for a plugin called \a baseName in \a qmldirPluginPath, taking the
  path of the qmldir file itself, and the plugin paths of the QQmlImportDatabase
  into account.

  The baseName is amended with a platform-dependent prefix and suffix to
  construct the final plugin file name:

  \table
  \header \li Platform   \li Prefix \li Valid suffixes
  \row    \li Windows    \li        \li \c .dll, \c .d.dll
  \row    \li Unix/Linux \li lib    \li \c .so
  \row    \li \macos     \li lib    \li \c .dylib, \c _debug.dylib \c .bundle, \c .so
  \row    \li Android    \li lib    \li \c .so, \c _<ABI>.so
  \endtable

  If the \a qmldirPluginPath is absolute, it is searched first. Then each of the
  filePluginPath entries in the QQmlImportDatabase is checked in turn. If the
  entry is relative, it is resolved on top of the path of the qmldir file,
  otherwise it is taken verbatim. If a "." is found in the filePluginPath, and
  \a qmldirPluginPath is relative, then \a qmldirPluginPath is used in its
  place.

  TODO: Document the android special casing.

  TODO: The above paragraph, as well as the code implementing it makes very
        little sense and is mostly here for backwards compatibility.
 */
QString QQmlPluginImporter::resolvePlugin(const QString &qmldirPluginPath, const QString &baseName)
{
#if defined(Q_OS_WIN)
    static const QString prefix;
    static const QStringList suffixes = {
    # ifdef QT_DEBUG
        QLatin1String("d.dll"), // try a qmake-style debug build first
        QLatin1String(".dll")
    #else
        QLatin1String(".dll"),
        QLatin1String("d.dll") // try a qmake-style debug build after
    # endif
    };
#elif defined(Q_OS_DARWIN)
    static const QString prefix = QLatin1String("lib");
    static const QStringList suffixes = {
    # ifdef QT_DEBUG
        QLatin1String("_debug.dylib"), // try a qmake-style debug build first
        QLatin1String(".dylib"),
    # else
        QLatin1String(".dylib"),
        QLatin1String("_debug.dylib"), // try a qmake-style debug build after
    # endif
        QLatin1String(".so"),
        QLatin1String(".bundle")
    };
#else  // Unix
    static const QString prefix = QLatin1String("lib");
    static const QStringList suffixes = {
    # if defined(Q_OS_ANDROID)
        QStringLiteral(LIBS_SUFFIX),
    # endif
        QLatin1String(".so")
    };
#endif

    QStringList searchPaths = database->filePluginPath;
    bool qmldirPluginPathIsRelative = QDir::isRelativePath(qmldirPluginPath);
    if (!qmldirPluginPathIsRelative)
        searchPaths.prepend(qmldirPluginPath);

    for (const QString &pluginPath : std::as_const(searchPaths)) {
        QString resolvedBasePath;
        if (pluginPath == QLatin1String(".")) {
            if (qmldirPluginPathIsRelative && !qmldirPluginPath.isEmpty()
                    && qmldirPluginPath != QLatin1String(".")) {
                resolvedBasePath = QDir::cleanPath(qmldirPath + u'/' + qmldirPluginPath);
            } else {
                resolvedBasePath = qmldirPath;
            }
        } else {
            if (QDir::isRelativePath(pluginPath))
                resolvedBasePath = QDir::cleanPath(qmldirPath + u'/' + pluginPath);
            else
                resolvedBasePath = pluginPath;
        }

        // hack for resources, should probably go away
        if (resolvedBasePath.startsWith(u':'))
            resolvedBasePath = QCoreApplication::applicationDirPath();

        if (!resolvedBasePath.endsWith(u'/'))
            resolvedBasePath += u'/';

        QString resolvedPath = resolvedBasePath + prefix + baseName;
        for (const QString &suffix : suffixes) {
            QString absolutePath = typeLoader->absoluteFilePath(resolvedPath + suffix);
            if (!absolutePath.isEmpty())
                return absolutePath;
        }

#if defined(Q_OS_ANDROID)
        if (qmldirPath.size() > 25 && qmldirPath.at(0) == QLatin1Char(':')
            && qmldirPath.at(1) == QLatin1Char('/')
            && qmldirPath.startsWith(QStringLiteral(":/android_rcc_bundle/qml/"),
                                     Qt::CaseInsensitive)) {
            QString pluginName = qmldirPath.mid(21) + u'/' + baseName;
            pluginName.replace(QLatin1Char('/'), QLatin1Char('_'));
            QString bundledPath = resolvedBasePath + QLatin1String("lib") + pluginName;
            for (const QString &suffix : suffixes) {
                const QString absolutePath = typeLoader->absoluteFilePath(bundledPath + suffix);
                if (!absolutePath.isEmpty()) {
                    qWarning("The implicit resolving of Qml plugin locations using the URI "
                             "embedded in the filename has been deprecated. Please use the "
                             "modern CMake API to create QML modules or set the name of "
                             "QML plugin in qmldir file, that matches the name of plugin "
                             "on file system. The correct plugin name is '%s'.",
                             qPrintable(pluginName));
                    return absolutePath;
                }
            }
        }
#endif
    }

    qCDebug(lcQmlImport) << "resolvePlugin" << "Could not resolve dynamic plugin with base name"
                         << baseName << "in" << qmldirPath
                         << " file does not exist";

    return QString();
}

/*
    Get all static plugins that are QML plugins and has a meta data URI that matches with one of
    \a versionUris, which is a list of all possible versioned URI combinations - see versionUriList()
    above.
 */
bool QQmlPluginImporter::populatePluginDataVector(QVector<StaticPluginData> &result, const QStringList &versionUris)
{
    static const QVector<QStaticPlugin> plugins = makePlugins();
    for (const QStaticPlugin &plugin : plugins) {
        // Since a module can list more than one plugin,
        // we keep iterating even after we found a match.
        QObject *instance = plugin.instance();
        if (qobject_cast<QQmlEngineExtensionPlugin *>(instance)
                || qobject_cast<QQmlExtensionPlugin *>(instance)) {
            const QJsonArray metaTagsUriList =
                    plugin.metaData().value(QStringLiteral("uri")).toArray();
            if (metaTagsUriList.isEmpty()) {
                if (errors) {
                    QQmlError error;
                    error.setDescription(QQmlImportDatabase::tr(
                                             "static plugin for module \"%1\" with name \"%2\" "
                                             "has no metadata URI")
                                         .arg(uri, QString::fromUtf8(
                                                  instance->metaObject()->className())));
                    error.setUrl(QUrl::fromLocalFile(qmldir->qmldirLocation()));
                    errors->prepend(error);
                }
                return false;
            }
            // A plugin can be set up to handle multiple URIs, so go through the list:
            for (const QJsonValueConstRef metaTagUri : metaTagsUriList) {
                if (versionUris.contains(metaTagUri.toString())) {
                    result.append({ plugin, metaTagsUriList });
                    break;
                }
            }
        }
    }
    return true;
}

QTypeRevision QQmlPluginImporter::importPlugins() {
    const auto qmldirPlugins = qmldir->plugins();
    const int qmldirPluginCount = qmldirPlugins.size();
    QTypeRevision importVersion = version;

    // If the path contains a version marker or if we have more than one plugin,
    // we need to use paths. In that case we cannot fall back to other instances
    // of the same module if a qmldir is rejected. However, as we don't generate
    // such modules, it shouldn't be a problem.
    const bool canUseUris = qmldirPluginCount == 1
            && qmldirPath.endsWith(u'/' + QString(uri).replace(u'.', u'/'));
    const QString moduleId = canUseUris ? uri : qmldir->qmldirLocation();

    if (!database->modulesForWhichPluginsHaveBeenLoaded.contains(moduleId)) {
        // First search for listed qmldir plugins dynamically. If we cannot resolve them all, we
        // continue searching static plugins that has correct metadata uri. Note that since we
        // only know the uri for a static plugin, and not the filename, we cannot know which
        // static plugin belongs to which listed plugin inside qmldir. And for this reason,
        // mixing dynamic and static plugins inside a single module is not recommended.

        int dynamicPluginsFound = 0;
        int staticPluginsLoaded = 0;

        for (const QQmlDirParser::Plugin &plugin : qmldirPlugins) {
            const QString resolvedFilePath = resolvePlugin(plugin.path, plugin.name);

            if (!canUseUris && resolvedFilePath.isEmpty())
                continue;

            importVersion = importDynamicPlugin(
                        resolvedFilePath,
                        canUseUris ? uri : QFileInfo(resolvedFilePath).absoluteFilePath(),
                        plugin.optional);
            if (importVersion.isValid())
                ++dynamicPluginsFound;
            else if (!resolvedFilePath.isEmpty())
                return QTypeRevision();
        }

        if (dynamicPluginsFound < qmldirPluginCount) {
            // Check if the missing plugins can be resolved statically. We do this by looking at
            // the URIs embedded in a plugins meta data. Since those URIs can be anything from
            // fully versioned to unversioned, we need to compare with differnt version strings.
            // If a module has several plugins, they must all have the same version. Start by
            // populating pluginPairs with relevant plugins to cut the list short early on:
            const QStringList versionUris = versionUriList(uri, importVersion);
            QVector<StaticPluginData> pluginPairs;
            if (!populatePluginDataVector(pluginPairs, versionUris))
                return QTypeRevision();

            for (const QString &versionUri : versionUris) {
                int staticPluginsFound = 0;
                for (const StaticPluginData &pair : std::as_const(pluginPairs)) {
                    for (const QJsonValueConstRef metaTagUri : pair.uriList) {
                        if (versionUri == metaTagUri.toString()) {
                            staticPluginsFound++;
                            QObject *instance = pair.plugin.instance();
                            importVersion = importStaticPlugin(
                                        instance,
                                        canUseUris ? uri : QString::asprintf("%p", instance));
                            if (!importVersion.isValid()) {
                                continue;
                            }
                            staticPluginsLoaded++;
                            qCDebug(lcQmlImport)
                                    << "importExtension" << "loaded static plugin " << versionUri;
                            break;
                        }
                    }
                }
                if (staticPluginsLoaded > 0)
                    break;
                if (staticPluginsFound > 0) {
                    // found matched plugins, but failed to load, early exit to preserve the error
                    return QTypeRevision();
                }
            }
        }

        if ((dynamicPluginsFound + staticPluginsLoaded) < qmldirPluginCount) {
            if (errors) {
                QQmlError error;
                if (qmldirPluginCount > 1 && staticPluginsLoaded > 0) {
                    error.setDescription(QQmlImportDatabase::tr(
                                                 "could not resolve all plugins for module \"%1\"")
                                                 .arg(uri));
                } else {
                    error.setDescription(
                            QQmlImportDatabase::tr("module \"%1\" plugin \"%2\" not found")
                                    .arg(uri, qmldirPlugins[dynamicPluginsFound].name));
                }
                error.setUrl(QUrl::fromLocalFile(qmldir->qmldirLocation()));
                errors->prepend(error);
            }
            return QTypeRevision();
        }

        database->modulesForWhichPluginsHaveBeenLoaded.insert(moduleId);
    }
    return QQmlImports::validVersion(importVersion);
}

QT_END_NAMESPACE
