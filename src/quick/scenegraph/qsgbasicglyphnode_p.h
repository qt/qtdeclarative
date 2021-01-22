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

#ifndef QSGBASICGLYPHNODE_P_H
#define QSGBASICGLYPHNODE_P_H

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

class QSGMaterial;

class Q_QUICK_PRIVATE_EXPORT QSGBasicGlyphNode: public QSGGlyphNode
{
public:
    QSGBasicGlyphNode();
    virtual ~QSGBasicGlyphNode();

    QPointF baseLine() const override { return m_baseLine; }
    void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) override;
    void setColor(const QColor &color) override;

    void setPreferredAntialiasingMode(AntialiasingMode) override { }
    void setStyle(QQuickText::TextStyle) override;
    void setStyleColor(const QColor &) override;

    virtual void setMaterialColor(const QColor &color) = 0;
    void update() override = 0;

protected:
    QGlyphRun m_glyphs;
    QPointF m_position;
    QColor m_color;
    QQuickText::TextStyle m_style;
    QColor m_styleColor;

    QPointF m_baseLine;
    QSGMaterial *m_material;

    QSGGeometry m_geometry;
};

QT_END_NAMESPACE

#endif
