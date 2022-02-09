/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef RHIITEM_H
#define RHIITEM_H

#include <QQuickItem>
#include <QtGui/private/qrhi_p.h>

class RhiItem;
class RhiItemPrivate;

class RhiItemRenderer
{
public:
    virtual ~RhiItemRenderer();
    virtual void initialize(QRhi *rhi, QRhiTexture *outputTexture);
    virtual void synchronize(RhiItem *item);
    virtual void render(QRhiCommandBuffer *cb);

    void update();

private:
    void *data;
    friend class RhiItem;
};

class RhiItem : public QQuickItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(RhiItem)

    Q_PROPERTY(int explicitTextureWidth READ explicitTextureWidth WRITE setExplicitTextureWidth NOTIFY explicitTextureWidthChanged)
    Q_PROPERTY(int explicitTextureHeight READ explicitTextureHeight WRITE setExplicitTextureHeight NOTIFY explicitTextureHeightChanged)
    Q_PROPERTY(QSize effectiveTextureSize READ effectiveTextureSize NOTIFY effectiveTextureSizeChanged)
    Q_PROPERTY(bool alphaBlending READ alphaBlending WRITE setAlphaBlending NOTIFY alphaBlendingChanged)
    Q_PROPERTY(bool mirrorVertically READ mirrorVertically WRITE setMirrorVertically NOTIFY mirrorVerticallyChanged)

public:
    RhiItem(QQuickItem *parent = nullptr);

    virtual RhiItemRenderer *createRenderer() = 0;

    int explicitTextureWidth() const;
    void setExplicitTextureWidth(int w);
    int explicitTextureHeight() const;
    void setExplicitTextureHeight(int h);

    QSize effectiveTextureSize() const;

    bool alphaBlending() const;
    void setAlphaBlending(bool enable);

    bool mirrorVertically() const;
    void setMirrorVertically(bool enable);

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;
    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

Q_SIGNALS:
    void explicitTextureWidthChanged();
    void explicitTextureHeightChanged();
    void effectiveTextureSizeChanged();
    void alphaBlendingChanged();
    void mirrorVerticallyChanged();

private Q_SLOTS:
    void invalidateSceneGraph();
};

#endif
