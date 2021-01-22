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

#ifndef QSGOPENVGINTERNALIMAGENODE_H
#define QSGOPENVGINTERNALIMAGENODE_H

#include <private/qsgadaptationlayer_p.h>
#include "qsgopenvgrenderable.h"

#include <VG/openvg.h>

QT_BEGIN_NAMESPACE

class QSGOpenVGInternalImageNode : public QSGInternalImageNode, public QSGOpenVGRenderable
{
public:
    QSGOpenVGInternalImageNode();
    ~QSGOpenVGInternalImageNode();

    void render() override;

    void setTargetRect(const QRectF &rect) override;
    void setInnerTargetRect(const QRectF &rect) override;
    void setInnerSourceRect(const QRectF &rect) override;
    void setSubSourceRect(const QRectF &rect) override;
    void setTexture(QSGTexture *texture) override;
    void setMirror(bool mirror) override;
    void setMipmapFiltering(QSGTexture::Filtering filtering) override;
    void setFiltering(QSGTexture::Filtering filtering) override;
    void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode) override;
    void setVerticalWrapMode(QSGTexture::WrapMode wrapMode) override;
    void update() override;

    void preprocess() override;

private:

    QRectF m_targetRect;
    QRectF m_innerTargetRect;
    QRectF m_innerSourceRect = QRectF(0, 0, 1, 1);
    QRectF m_subSourceRect = QRectF(0, 0, 1, 1);

    bool m_mirror = false;
    bool m_smooth = true;
    bool m_tileHorizontal = false;
    bool m_tileVertical = false;

    QSGTexture *m_texture = nullptr;

    VGImage m_subSourceRectImage = 0;
    bool m_subSourceRectImageDirty = true;
};

QT_END_NAMESPACE

#endif // QSGOPENVGINTERNALIMAGENODE_H
