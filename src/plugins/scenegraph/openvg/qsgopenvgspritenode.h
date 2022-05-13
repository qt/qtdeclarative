// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGSPRITENODE_H
#define QSGOPENVGSPRITENODE_H

#include <private/qsgadaptationlayer_p.h>
#include "qsgopenvgrenderable.h"

QT_REQUIRE_CONFIG(quick_sprite);

QT_BEGIN_NAMESPACE
class QSGOpenVGTexture;
class QSGOpenVGSpriteNode : public QSGSpriteNode, public QSGOpenVGRenderable
{
public:
    QSGOpenVGSpriteNode();
    ~QSGOpenVGSpriteNode();

    void setTexture(QSGTexture *texture) override;
    void setTime(float time) override;
    void setSourceA(const QPoint &source) override;
    void setSourceB(const QPoint &source) override;
    void setSpriteSize(const QSize &size) override;
    void setSheetSize(const QSize &size) override;
    void setSize(const QSizeF &size) override;
    void setFiltering(QSGTexture::Filtering filtering) override;
    void update() override;

    void render() override;

private:
    QSGOpenVGTexture *m_texture = nullptr;
    float m_time;
    QPoint m_sourceA;
    QPoint m_sourceB;
    QSize m_spriteSize;
    QSize m_sheetSize;
    QSizeF m_size;
};

QT_END_NAMESPACE

#endif // QSGOPENVGSPRITENODE_H
