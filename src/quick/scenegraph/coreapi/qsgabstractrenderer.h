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

#ifndef QSGABSTRACTRENDERER_H
#define QSGABSTRACTRENDERER_H

#include <QtQuick/qsgnode.h>

#ifndef GLuint
#define GLuint uint
#endif

QT_BEGIN_NAMESPACE

class QSGAbstractRendererPrivate;

class Q_QUICK_EXPORT QSGAbstractRenderer : public QObject
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

    // just have a warning about becoming private, ifdefing the whole class is not feasible
#if !defined(QT_BUILD_QUICK_LIB)
#if QT_DEPRECATED_SINCE(5, 15)
    QT_DEPRECATED_X("QSGAbstractRenderer is no longer going to be public in Qt 6.0. QSGEngine-based workflows are expected to migrate to QQuickRenderControl instead.")
#endif
#endif
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
    void setProjectionMatrix(const QMatrix4x4 &matrix);
    void setProjectionMatrixWithNativeNDC(const QMatrix4x4 &matrix);
    QMatrix4x4 projectionMatrix() const;
    QMatrix4x4 projectionMatrixWithNativeNDC() const;

    void setClearColor(const QColor &color);
    QColor clearColor() const;

    void setClearMode(ClearMode mode);
    ClearMode clearMode() const;

    virtual void renderScene(GLuint fboId = 0) = 0;

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
