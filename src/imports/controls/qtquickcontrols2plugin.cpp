/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/private/qfileselector_p.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/private/qqmldirparser_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p_p.h>

QT_BEGIN_NAMESPACE

class QtQuickControls2Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2Plugin(QObject *parent = nullptr);
    ~QtQuickControls2Plugin();

    void initializeEngine(QQmlEngine *engine, const char *uri) override;
    void registerTypes(const char *uri) override;
    void unregisterTypes() override;

private:
    void init();

    QList<QQuickStylePlugin *> loadStylePlugins();
    QQuickTheme *createTheme(const QString &name);
};

QtQuickControls2Plugin::QtQuickControls2Plugin(QObject *parent) : QQmlExtensionPlugin(parent)
{
}

QtQuickControls2Plugin::~QtQuickControls2Plugin()
{
    // Intentionally empty: we use register/unregisterTypes() to do
    // initialization and cleanup, as plugins are not unloaded on macOS.
}

void QtQuickControls2Plugin::initializeEngine(QQmlEngine *engine, const char */*uri*/)
{
    engine->addUrlInterceptor(&QQuickStylePrivate::urlInterceptor);
    init();
}

void QtQuickControls2Plugin::registerTypes(const char */*uri*/)
{
    QQuickStylePrivate::init(baseUrl());

    const QString style = QQuickStyle::name();
    if (!style.isEmpty())
        QFileSelectorPrivate::addStatics(QStringList() << style.toLower());
}

void QtQuickControls2Plugin::unregisterTypes()
{
    QQuickStylePrivate::reset();
}

void QtQuickControls2Plugin::init()
{
    const QString style = QQuickStyle::name();
    QQuickTheme *theme = createTheme(style.isEmpty() ? QLatin1String("Default") : style);

    // load the style's plugins to get access to its resources and initialize the theme
    QList<QQuickStylePlugin *> stylePlugins = loadStylePlugins();
    for (QQuickStylePlugin *stylePlugin : stylePlugins)
        stylePlugin->initializeTheme(theme);
    qDeleteAll(stylePlugins);
}

QList<QQuickStylePlugin *> QtQuickControls2Plugin::loadStylePlugins()
{
    QList<QQuickStylePlugin *> stylePlugins;

    QFileInfo fileInfo = QQmlFile::urlToLocalFileOrQrc(resolvedUrl(QStringLiteral("qmldir")));
    if (fileInfo.exists() && fileInfo.path() != QQmlFile::urlToLocalFileOrQrc(baseUrl())) {
        QFile file(fileInfo.filePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QQmlDirParser parser;
            parser.parse(QString::fromUtf8(file.readAll()));
            if (!parser.hasError()) {
#ifdef QT_STATIC
                const auto plugins = QPluginLoader::staticInstances();
                for (QObject *instance : plugins) {
                    QQuickStylePlugin *stylePlugin = qobject_cast<QQuickStylePlugin *>(instance);
                    if (!stylePlugin || !parser.classNames().contains(QLatin1String(instance->metaObject()->className())))
                        continue;
                    stylePlugins += stylePlugin;
                }
#elif QT_CONFIG(library)
                QPluginLoader loader;
                const auto plugins = parser.plugins();
                for (const QQmlDirParser::Plugin &plugin : plugins) {
                    QDir dir = fileInfo.dir();
                    if (!plugin.path.isEmpty() && !dir.cd(plugin.path))
                        continue;
                    QString filePath = dir.filePath(plugin.name);
#if defined(Q_OS_MACOS) && defined(QT_DEBUG)
                    // Avoid mismatching plugins on macOS so that we don't end up loading both debug and
                    // release versions of the same Qt libraries (due to the plugin's dependencies).
                    filePath += QStringLiteral("_debug");
#endif // Q_OS_MACOS && QT_DEBUG
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
                    // Debug versions of plugins have a "d" prefix on Windows.
                    filePath += QLatin1Char('d');
#endif // Q_OS_WIN && QT_DEBUG
                    loader.setFileName(filePath);
                    QQuickStylePlugin *stylePlugin = qobject_cast<QQuickStylePlugin *>(loader.instance());
                    if (stylePlugin)
                        stylePlugins += stylePlugin;
                }
#endif
            }
        }
    }
    return stylePlugins;
}

QQuickTheme *QtQuickControls2Plugin::createTheme(const QString &name)
{
    QQuickTheme *theme = new QQuickTheme;
#if QT_CONFIG(settings)
    QQuickThemePrivate *p = QQuickThemePrivate::get(theme);
    QSharedPointer<QSettings> settings = QQuickStylePrivate::settings(name);
    if (settings) {
        p->defaultFont.reset(QQuickStylePrivate::readFont(settings));
        // Set the default font as the System scope, because that's what
        // QQuickControlPrivate::parentFont() uses as its fallback if no
        // parent item has a font explicitly set. QQuickControlPrivate::parentFont()
        // is used as the starting point for font inheritance/resolution.
        // The same goes for palettes below.
        theme->setFont(QQuickTheme::System, *p->defaultFont);

        p->defaultPalette.reset(QQuickStylePrivate::readPalette(settings));
        theme->setPalette(QQuickTheme::System, *p->defaultPalette);
    }
#endif
    QQuickThemePrivate::instance.reset(theme);
    return theme;
}

QT_END_NAMESPACE

#include "qtquickcontrols2plugin.moc"
