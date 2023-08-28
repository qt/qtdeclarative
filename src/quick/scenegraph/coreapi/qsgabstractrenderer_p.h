// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGABSTRACTRENDERER_P_H
#define QSGABSTRACTRENDERER_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/qsgnode.h>
#include <QtCore/qobject.h>

#ifndef GLuint
#define GLuint uint
#endif

QT_BEGIN_NAMESPACE

class QSGAbstractRendererPrivate;

class Q_QUICK_PRIVATE_EXPORT QSGAbstractRenderer : public QObject
{
    Q_OBJECT
public:
    enum ClearModeBit
    {
        ClearColorBuffer    = 0x0001,
        ClearDepthBuffer    = 0x0002,
        ClearStencilBuffer  = 0x0004
    };
    Q_DECLARE_FLAGS(ClearMode, ClearModeBit)
    Q_FLAG(ClearMode)

    enum MatrixTransformFlag
    {
        MatrixTransformFlipY = 0x01
    };
    Q_DECLARE_FLAGS(MatrixTransformFlags, MatrixTransformFlag)
    Q_FLAG(MatrixTransformFlags)

    ~QSGAbstractRenderer() override;

    void setRootNode(QSGRootNode *node);
    QSGRootNode *rootNode() const;
    void setDeviceRect(const QRect &rect);
    inline void setDeviceRect(const QSize &size) { setDeviceRect(QRect(QPoint(), size)); }
    QRect deviceRect() const;

    void setViewportRect(const QRect &rect);
    inline void setViewportRect(const QSize &size) { setViewportRect(QRect(QPoint(), size)); }
    QRect viewportRect() const;

    void setProjectionMatrixToRect(const QRectF &rect);
    void setProjectionMatrixToRect(const QRectF &rect, MatrixTransformFlags flags);
    void setProjectionMatrixToRect(const QRectF &rect, MatrixTransformFlags flags,
                                   bool nativeNDCFlipY);
    void setProjectionMatrix(const QMatrix4x4 &matrix);
    void setProjectionMatrixWithNativeNDC(const QMatrix4x4 &matrix);
    QMatrix4x4 projectionMatrix() const;
    QMatrix4x4 projectionMatrixWithNativeNDC() const;

    void setClearColor(const QColor &color);
    QColor clearColor() const;

    void setClearMode(ClearMode mode);
    ClearMode clearMode() const;

    virtual void renderScene() = 0;

    virtual void prepareSceneInline();
    virtual void renderSceneInline();

Q_SIGNALS:
    void sceneGraphChanged();

protected:
    explicit QSGAbstractRenderer(QObject *parent = nullptr);
    virtual void nodeChanged(QSGNode *node, QSGNode::DirtyState state) = 0;

private:
    Q_DECLARE_PRIVATE(QSGAbstractRenderer)
    friend class QSGRootNode;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGAbstractRenderer::ClearMode)

QT_END_NAMESPACE

#endif
