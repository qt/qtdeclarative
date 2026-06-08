// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSGDEFAULTGLYPHNODE_P_H
#define QSGDEFAULTGLYPHNODE_P_H

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

#include <private/qsgadaptationlayer_p.h>

QT_BEGIN_NAMESPACE

class QSGDefaultGlyphNode : public QSGGlyphNode
{
public:
    QSGDefaultGlyphNode(QSGRenderContext *context);
    ~QSGDefaultGlyphNode() override;
    void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) override;
    void setColor(const QColor &color) override;
    void setStyle(QQuickText::TextStyle) override;
    void setStyleColor(const QColor &) override;
    QPointF baseLine() const override { return m_baseLine; }
    void update() override;
    void preprocess() override;
    void setPreferredAntialiasingMode(AntialiasingMode) override;
    void updateGeometry();

    void recycle() override;

private:
    void cleanup();

    enum DefaultGlyphNodeType {
        RootGlyphNode,
        SubGlyphNode
    };

    void setGlyphNodeType(DefaultGlyphNodeType type) { m_glyphNodeType = type; }

    QSGRenderContext *m_context;
    DefaultGlyphNodeType m_glyphNodeType;
    QList<QSGNode *> m_nodesToDelete;

    struct GlyphInfo {
        QList<quint32> indexes;
        QList<QPointF> positions;
    };

    uint m_dirtyGeometry: 1;
    uint m_recycled : 1;

    AntialiasingMode m_preferredAntialiasingMode;
    QGlyphRun m_glyphs;
    QPointF m_position;
    QColor m_color;
    QQuickText::TextStyle m_style;
    QColor m_styleColor;
    QPointF m_baseLine;
    QSGGeometry m_geometry;
};

QT_END_NAMESPACE

#endif
