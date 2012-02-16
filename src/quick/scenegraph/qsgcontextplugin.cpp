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

#include "qsgcontextplugin_p.h"
#include <QtQuick/private/qsgcontext_p.h>
#include <QtGui/qguiapplication.h>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/qlibraryinfo.h>

QT_BEGIN_NAMESPACE

QSGContextPlugin::QSGContextPlugin(QObject *parent)
    : QObject(parent)
{
}

QSGContextPlugin::~QSGContextPlugin()
{
}

#if !defined (QT_NO_LIBRARY) && !defined(QT_NO_SETTINGS)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QSGContextFactoryInterface_iid, QLatin1String("/scenegraph")))
#endif

struct QSGAdaptionPluginData
{
    QSGAdaptionPluginData()
        : tried(false)
        , factory(0)
    {
    }

    ~QSGAdaptionPluginData()
    {
        delete factory;
    }

    bool tried;
    QSGContextFactoryInterface *factory;
    QString deviceName;
};

Q_GLOBAL_STATIC(QSGAdaptionPluginData, qsg_adaptation_data)

QSGAdaptionPluginData *contextFactory()
{
    QSGAdaptionPluginData *plugin = qsg_adaptation_data();
    if (!plugin->tried) {

        plugin->tried = true;
        const QStringList args = QGuiApplication::arguments();
        QString device;
        for (int index = 0; index < args.count(); ++index) {
            if (args.at(index).startsWith(QLatin1String("--device="))) {
                device = args.at(index).mid(9);
                break;
            }
        }
        if (device.isEmpty())
            device = QString::fromLocal8Bit(qgetenv("QMLSCENE_DEVICE"));

#if !defined (QT_NO_LIBRARY) && !defined(QT_NO_SETTINGS)
        if (!device.isEmpty()) {
            plugin->factory = qobject_cast<QSGContextFactoryInterface*>(loader()->instance(device));
            plugin->deviceName = device;
        }
#ifndef QT_NO_DEBUG
        if (!device.isEmpty()) {
            qWarning("Could not create scene graph context for device '%s'"
                     " - check that plugins are installed correctly in %s",
                     qPrintable(device),
                     qPrintable(QLibraryInfo::location(QLibraryInfo::PluginsPath)));
        }
#endif

#endif // QT_NO_LIBRARY || QT_NO_SETTINGS
    }
    return plugin;
}



/*!
    \fn QSGContext *QSGContext::createDefaultContext()

    Creates a default scene graph context for the current hardware.
    This may load a device-specific plugin.
*/
QSGContext *QSGContext::createDefaultContext()
{
    QSGAdaptionPluginData *plugin = contextFactory();
    if (plugin->factory)
        return plugin->factory->create(plugin->deviceName);
    return new QSGContext();
}



/*!
    \fn QQuickTextureFactory *createTextureFactoryFromImage(const QImage &image)

    Calls into the scene graph adaptation if available and creates a texture
    factory. The primary purpose of this function is to reimplement hardware
    specific asynchronous texture frameskip-less uploads that can happen on
    the image providers thread.
 */

QQuickTextureFactory *QSGContext::createTextureFactoryFromImage(const QImage &image)
{
    QSGAdaptionPluginData *plugin = contextFactory();
    if (plugin->factory)
        return plugin->factory->createTextureFactoryFromImage(image);
    return 0;
}



QT_END_NAMESPACE
