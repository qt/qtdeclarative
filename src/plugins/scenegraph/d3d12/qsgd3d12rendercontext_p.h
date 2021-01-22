/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSGD3D12RENDERCONTEXT_P_H
#define QSGD3D12RENDERCONTEXT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qsgcontext_p.h>
#include <qsgrendererinterface.h>

QT_BEGIN_NAMESPACE

class QSGD3D12Engine;

class QSGD3D12RenderContext : public QSGRenderContext, public QSGRendererInterface
{
public:
    QSGD3D12RenderContext(QSGContext *ctx);
    bool isValid() const override;
    void initialize(const InitParams *params) override;
    void invalidate() override;
    void renderNextFrame(QSGRenderer *renderer, uint fbo) override;
    QSGTexture *createTexture(const QImage &image, uint flags) const override;
    QSGRenderer *createRenderer() override;
    int maxTextureSize() const override;

    void setEngine(QSGD3D12Engine *engine);
    QSGD3D12Engine *engine() { return m_engine; }

    // QSGRendererInterface
    GraphicsApi graphicsApi() const override;
    void *getResource(QQuickWindow *window, Resource resource) const override;
    ShaderType shaderType() const override;
    ShaderCompilationTypes shaderCompilationType() const override;
    ShaderSourceTypes shaderSourceType() const override;

private:
    QSGD3D12Engine *m_engine = nullptr;
    bool m_initialized = false;
};

QT_END_NAMESPACE

#endif // QSGD3D12RENDERCONTEXT_P_H
