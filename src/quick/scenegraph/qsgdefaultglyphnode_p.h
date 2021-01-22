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
#include <private/qsgbasicglyphnode_p.h>
#include <qlinkedlist.h>

QT_BEGIN_NAMESPACE

class QSGDefaultGlyphNode : public QSGBasicGlyphNode
{
public:
    QSGDefaultGlyphNode(QSGRenderContext *context);
    ~QSGDefaultGlyphNode();
    void setMaterialColor(const QColor &color) override;
    void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) override;
    void update() override;
    void preprocess() override;
    void setPreferredAntialiasingMode(AntialiasingMode) override;
    void updateGeometry();

private:
    enum DefaultGlyphNodeType {
        RootGlyphNode,
        SubGlyphNode
    };

    void setGlyphNodeType(DefaultGlyphNodeType type) { m_glyphNodeType = type; }

    QSGRenderContext *m_context;
    DefaultGlyphNodeType m_glyphNodeType;
    QVector<QSGNode *> m_nodesToDelete;

    struct GlyphInfo {
        QVector<quint32> indexes;
        QVector<QPointF> positions;
    };

    uint m_dirtyGeometry: 1;

    AntialiasingMode m_preferredAntialiasingMode;
};

QT_END_NAMESPACE

#endif
