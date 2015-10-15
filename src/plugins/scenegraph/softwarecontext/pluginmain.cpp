/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of Qt Quick 2d Renderer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
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



