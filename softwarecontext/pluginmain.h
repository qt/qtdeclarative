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

#ifndef PLUGINMAIN_H
#define PLUGINMAIN_H

#include <private/qsgcontext_p.h>
#include <private/qsgcontextplugin_p.h>

#include <qplugin.h>

#include "context.h"

class ContextPlugin : public QSGContextPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSGContextFactoryInterface" FILE "softwarecontext.json")

public:
    ContextPlugin(QObject *parent = 0);

    QStringList keys() const;
    QSGContext *create(const QString &key) const;
    QSGRenderLoop *createWindowManager();

    static SoftwareContext::Context *instance;
};

#endif // PLUGINMAIN_H
