/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt SceneGraph Raster Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/


#include "pluginmain.h"
#include "context.h"
#include "renderloop.h"
#include "threadedrenderloop.h"

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

ContextPlugin::ContextPlugin(QObject *parent)
    : QSGContextPlugin(parent)
{
}

QStringList ContextPlugin::keys() const
{
    return QStringList() << QLatin1String("softwarecontext");
}

QSGContext *ContextPlugin::create(const QString &) const
{
    if (!instance)
        instance = new SoftwareContext::Context();
    return instance;
}

QSGRenderLoop *ContextPlugin::createWindowManager()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedPixmaps) ||
        qgetenv("QSG_RENDER_LOOP") == QByteArrayLiteral("basic"))
        return new RenderLoop();

    return new ThreadedRenderLoop();
}

SoftwareContext::Context *ContextPlugin::instance = 0;



