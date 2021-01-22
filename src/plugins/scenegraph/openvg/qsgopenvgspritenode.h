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
