/******************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick 2d Renderer module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

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
