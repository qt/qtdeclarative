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

#ifndef RHIITEM_P_H
#define RHIITEM_P_H

#include "rhiitem.h"
#include <QSGSimpleTextureNode>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qsgplaintexture_p.h>

class RhiItemNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

public:
    RhiItemNode(RhiItem *item);
    ~RhiItemNode();

    QSGTexture *texture() const override;

    void sync();
    bool isValid() const { return m_rhi && m_texture && m_sgWrapperTexture; }
    void scheduleUpdate();
    bool hasRenderer() const { return m_renderer; }
    void setRenderer(RhiItemRenderer *r) { m_renderer = r; }
    void setMirrorVertically(bool b);

private slots:
    void render();

private:
    void createNativeTexture();
    void releaseNativeTexture();

    RhiItem *m_item;
    QQuickWindow *m_window;
    QSize m_pixelSize;
    qreal m_dpr = 0.0f;
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_texture = nullptr;
    QSGPlainTexture *m_sgWrapperTexture = nullptr;
    bool m_renderPending = true;
    RhiItemRenderer *m_renderer = nullptr;
};

class RhiItemPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(RhiItem)
public:
    static RhiItemPrivate *get(RhiItem *item) { return item->d_func(); }
    mutable RhiItemNode *node = nullptr;
    int explicitTextureWidth = 0;
    int explicitTextureHeight = 0;
    bool blend = true;
    bool mirrorVertically = false;
    QSize effectiveTextureSize;
};

#endif
