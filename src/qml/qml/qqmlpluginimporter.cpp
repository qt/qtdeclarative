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

struct QmlPlugin
{
    using Loader = std::unique_ptr<QPluginLoader>;
    QPluginLoader *loader() const
    {
        if (auto loader = std::get_if<Loader>(&data))
            return loader->get();
        return nullptr;
    }

    bool hasInstanceOrLoader() const
    {
        if (auto instance = std::get_if<QQmlExtensionPlugin *>(&data))
            return *instance;
        return bool(std::get<Loader>(data));
    }

    QString unloadOrErrorMessage() const
    {
        if (auto instance = std::get_if<QQmlExtensionPlugin *>(&data)) {
            if (*instance)
                (*instance)->unregisterTypes();
            return {};
        }
        const Loader &loader = std::get<Loader>(data);
        if (!loader)
            return {};

#if QT_CONFIG(library)
        if (auto extensionPlugin = qobject_cast<QQmlExtensionPlugin *>(loader->instance()))
            extensionPlugin->unregisterTypes();
        if (!loader->unload())
            return loader->errorString();
#endif
        return {};
    }
    std::variant<Loader, QQmlExtensionPlugin *> data;
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

struct StaticPluginMapping
{
    QStaticPlugin plugin;
    QString metadataURI;
};

struct VersionedURI
{
    QString id;
    QTypeRevision version;
};

/*
 * This function verifies if staticPlugin is valid QML plugin by
 * verifying the IID member of Metadata and
 * checks whether the plugin instance has the correct parent plugin extension class.
 * Checks the presence of URI member of Metadata and returns QJsonArray,
 * which in majority cases contains only 1 element.
 */
static QJsonArray tryExtractQmlPluginURIs(const QStaticPlugin &plugin)
{
    const auto isQmlPlugin = [](const QStaticPlugin &plugin, auto &&pluginMetadata) -> bool {
        const QString iid = pluginMetadata.value(QLatin1String("IID")).toString();
        const bool isQmlExtensionIID = iid == QLatin1String(QQmlEngineExtensionInterface_iid)
                || iid == QLatin1String(QQmlExtensionInterface_iid)
                || iid == QLatin1String(QQmlExtensionInterface_iid_old);
        if (Q_UNLIKELY(iid == QLatin1String(QQmlExtensionInterface_iid_old))) {
            qWarning() << QQmlImports::tr(
                    "Found plugin with old IID, this will be unsupported in upcoming Qt "
                    "releases:")
                       << pluginMetadata;
        }
        if (!isQmlExtensionIID) {
            return false;
        }

        // plugin.instance must not be called earlier to avoid instantiating
        // non-QML plugins, potentially from an unexpected thread(typeloader vs
        // main thread)
        const auto *pluginInstance = plugin.instance();
        return qobject_cast<const QQmlEngineExtensionPlugin *>(pluginInstance)
                || qobject_cast<const QQmlExtensionPlugin *>(pluginInstance);
    };

    const auto &pluginMetadata = plugin.metaData();
    if (!isQmlPlugin(plugin, pluginMetadata)) {
        return {};
    }

    const QJsonArray metadataUriList = pluginMetadata.value(QStringLiteral("uri")).toArray();
    if (metadataUriList.isEmpty()) {
        qWarning() << QQmlImports::tr("qml static plugin with name \"%2\" has no metadata URI")
                              .arg(plugin.instance()->metaObject()->className())
                   << pluginMetadata;
        return {};
    }
    return metadataUriList;
}

static QList<StaticPluginMapping> staticQmlPlugins()
{
    QList<StaticPluginMapping> qmlPlugins;
    const auto staticPlugins = QPluginLoader::staticPlugins();
    qmlPlugins.reserve(staticPlugins.size());

    for (const auto &plugin : staticPlugins) {
        // Filter out static plugins which are not Qml plugins
        for (const QJsonValueConstRef &pluginURI : tryExtractQmlPluginURIs(plugin)) {
            qmlPlugins.append({ plugin, pluginURI.toString() });
        }
    }
    return qmlPlugins;
}

/*
    Returns the list of possible versioned URI combinations. For example, if \a uri is
    (id = QtQml.Models, (vmaj = 2, vmin = 0)), this method returns the following:
    [QtQml.Models.2.0, QtQml.2.0.Models, QtQml.Models.2, QtQml.2.Models, QtQml.Models]
 */
static QStringList versionUriList(const VersionedURI &uri)
{
    QStringList result;
    for (int mode = QQmlImports::FullyVersioned; mode <= QQmlImports::Unversioned; ++mode) {
        int index = uri.id.size();
        do {
            QString versionUri = uri.id;
            versionUri.insert(
                    index,
                    QQmlImports::versionString(uri.version, QQmlImports::ImportVersion(mode)));
            result += versionUri;

            index = uri.id.lastIndexOf(u'.', index - 1);
        } while (index > 0 && mode != QQmlImports::Unversioned);
    }
    return result;
}

/*
    Get all static plugins that are QML plugins and has a meta data URI that matches with one of
    \a versionUris, which is a list of all possible versioned URI combinations - see
   versionUriList() above.
 */
static QList<StaticPluginMapping> staticQmlPluginsMatchingURI(const VersionedURI &uri)
{
    static const auto qmlPlugins = staticQmlPlugins();

    // Since plugin metadata URIs can be anything from
    // fully versioned to unversioned, we need to compare with differnt version strings.
    // If a module has several plugins, they must all have the same version. Start by
    // populating pluginPairs with relevant plugins to cut the list short early on:
    const QStringList versionedURIs = versionUriList(uri);
    QList<StaticPluginMapping> matches;
    std::copy_if(qmlPlugins.begin(), qmlPlugins.end(), std::back_inserter(matches),
                 [&](const auto &pluginMapping) {
                     return versionedURIs.contains(pluginMapping.metadataURI);
                 });
    return matches;
}

static bool unloadPlugin(const std::pair<const QString, QmlPlugin> &plugin)
{
    const QString errorMessage = plugin.second.unloadOrErrorMessage();
    if (errorMessage.isEmpty())
        return true;

    qWarning("Unloading %s failed: %s", qPrintable(plugin.first), qPrintable(errorMessage));
    return false;
}

/*!
    \internal
*/
static QTypeRevision lockModule(const QString &uri, const QString &typeNamespace,
                                             QTypeRevision version, QList<QQmlError> *errors)
{
    if (!version.hasMajorVersion()) {
        version = QQmlMetaType::latestModuleVersion(uri);
        if (!version.isValid())
            errors->prepend(QQmlImports::moduleNotFoundError(uri, version));
    }
    if (version.hasMajorVersion() && !typeNamespace.isEmpty()
        && !QQmlMetaType::protectModule(uri, version, true)) {
        // Not being able to protect the module means there are not types registered for it,
        // means the plugin we loaded didn't provide any, means we didn't find the module.
        // We output the generic error message as depending on the load order of imports we may
        // hit this path or another one that only figures "plugin is already loaded but module
        // unavailable" and doesn't try to protect it anymore.
        errors->prepend(QQmlImports::moduleNotFoundError(uri, version));
        return QTypeRevision();
    }

    return version;
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
        if (it->second.hasInstanceOrLoader())
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

    typeLoader->setPluginInitialized(pluginId);
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
            plugins->insert(std::make_pair(
                    pluginId, QmlPlugin{ qobject_cast<QQmlExtensionPlugin *>(instance) }));
            if (QQmlMetaType::registerPluginTypes(
                        instance, QFileInfo(qmldirPath).absoluteFilePath(), uri,
                        qmldir->typeNamespace(), importVersion, errors)
                    == QQmlMetaType::RegistrationResult::Failure) {
                return QTypeRevision();
            }

            importVersion = lockModule(uri, qmldir->typeNamespace(), importVersion, errors);
            if (!importVersion.isValid())
                return QTypeRevision();
        }

        // Release the lock on plugins early as we're done with the global part. Releasing the lock
        // also allows other QML loader threads to acquire the lock while this thread is blocking
        // in the initializeEngine call to the gui thread (which in turn may be busy waiting for
        // other QML loader threads and thus not process the initializeEngine call).
    }

    if (!typeLoader->isPluginInitialized(pluginId))
        finalizePlugin(instance, pluginId);

    return QQmlImports::validVersion(importVersion);
}

QTypeRevision QQmlPluginImporter::importDynamicPlugin(
        const QString &filePath, const QString &pluginId, bool optional)
{
    QObject *instance = nullptr;
    QTypeRevision importVersion = version;

    const bool engineInitialized = typeLoader->isPluginInitialized(pluginId);
    {
        PluginMapPtr plugins(qmlPluginsById());
        const auto plugin = plugins->find(pluginId);
        bool typesRegistered = plugin != plugins->end();

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
                    importVersion = lockModule(uri, qmldir->typeNamespace(), importVersion, errors);
                    if (!importVersion.isValid())
                        return QTypeRevision();
                    // instance and loader intentionally left at nullptr
                    plugins->insert(std::make_pair(pluginId, QmlPlugin()));
                    // Not calling initializeEngine with null instance
                    typeLoader->setPluginInitialized(pluginId);
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

                QmlPlugin plugin;
                plugin.data = std::make_unique<QPluginLoader>(fileInfo.absoluteFilePath());
                if (!plugin.loader()->load()) {
                    if (errors) {
                        QQmlError error;
                        error.setDescription(plugin.loader()->errorString());
                        errors->prepend(error);
                    }
                    return QTypeRevision();
                }

                instance = plugin.loader()->instance();
                plugins->insert(std::make_pair(pluginId, std::move(plugin)));

                // Continue with shared code path for dynamic and static plugins:
                if (QQmlMetaType::registerPluginTypes(
                            instance, fileInfo.absolutePath(), uri, qmldir->typeNamespace(),
                            importVersion, errors)
                        == QQmlMetaType::RegistrationResult::Failure) {
                    return QTypeRevision();
                }

                importVersion = lockModule(uri, qmldir->typeNamespace(), importVersion, errors);
                if (!importVersion.isValid())
                    return QTypeRevision();
            } else {
                Q_ASSERT(plugin != plugins->end());
                if (const auto &loader = plugin->second.loader()) {
                    instance = loader->instance();
                } else if (!optional) {
                    // If the plugin is not optional, we absolutely need to have a loader.
                    // Not having a loader here can mean that the plugin was loaded statically
                    // before. Return an invalid result to have the caller try that option.
                    return QTypeRevision();
                }
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
  path of the qmldir file itself, and the plugin paths of the QQmlTypeLoader
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
  filePluginPath entries in the QQmlTypeLoader is checked in turn. If the
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

    QStringList searchPaths = typeLoader->pluginPathList();
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

    if (typeLoader->isModulePluginProcessingDone(moduleId)) {
        return QQmlImports::validVersion(importVersion);
    }

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
                resolvedFilePath, canUseUris ? uri : QFileInfo(resolvedFilePath).absoluteFilePath(),
                plugin.optional);
        if (importVersion.isValid())
            ++dynamicPluginsFound;
        else if (!resolvedFilePath.isEmpty())
            return QTypeRevision();
    }

    if (dynamicPluginsFound < qmldirPluginCount) {
        // Check if the missing plugins can be resolved statically. We do this by looking at
        // the URIs embedded in a plugins meta data.
        const auto pluginPairs = staticQmlPluginsMatchingURI({ uri, importVersion });

        for (const auto &pluginWithURI : std::as_const(pluginPairs)) {
            QObject *instance = pluginWithURI.plugin.instance();
            importVersion = importStaticPlugin(
                    instance, canUseUris ? uri : QString::asprintf("%p", instance));
            if (!importVersion.isValid()) {
                continue;
            }
            staticPluginsLoaded++;
            qCDebug(lcQmlImport) << "importExtension" << "loaded static plugin "
                                 << pluginWithURI.metadataURI;
        }
        if (!pluginPairs.empty() && staticPluginsLoaded == 0) {
            // found matched plugins, but failed to load, early exit to preserve the error
            return QTypeRevision();
        }
    }

    if ((dynamicPluginsFound + staticPluginsLoaded) < qmldirPluginCount) {
        if (errors) {
            QQmlError error;
            if (qmldirPluginCount > 1 && staticPluginsLoaded > 0) {
                error.setDescription(
                        QQmlImports::tr("could not resolve all plugins for module \"%1\"")
                                .arg(uri));
            } else {
                error.setDescription(QQmlImports::tr("module \"%1\" plugin \"%2\" not found")
                                             .arg(uri, qmldirPlugins[dynamicPluginsFound].name));
            }
            error.setUrl(QUrl::fromLocalFile(qmldir->qmldirLocation()));
            errors->prepend(error);
        }
        return QTypeRevision();
    }

    typeLoader->setModulePluginProcessingDone(moduleId);

    return QQmlImports::validVersion(importVersion);
}

QT_END_NAMESPACE
