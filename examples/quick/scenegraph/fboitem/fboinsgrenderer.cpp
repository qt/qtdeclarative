// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "fboinsgrenderer.h"
#include "logorenderer.h"

#include <QOpenGLFramebufferObject>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>

class LogoInFboRenderer : public QQuickFramebufferObject::Renderer
{
public:
    LogoInFboRenderer()
    {
        logo.initialize();
    }

    void render() override {
        logo.render();
        update();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    LogoRenderer logo;
};

QQuickFramebufferObject::Renderer *FboInSGRenderer::createRenderer() const
{
    return new LogoInFboRenderer();
}
