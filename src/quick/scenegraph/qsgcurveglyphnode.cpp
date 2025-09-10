// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgcurveglyphnode_p.h"
#include "qsgcurveglyphatlas_p.h"
#include "qsgcurvefillnode_p.h"
#include "qsgcurvestrokenode_p.h"

#include <private/qsgcurveabstractnode_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgtexturematerial_p.h>

#include <private/qrawfont_p.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

QSGCurveGlyphNode::QSGCurveGlyphNode(QSGRenderContext *context)
    : m_context(context)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
    , m_dirtyGeometry(false)
{
    setFlag(UsePreprocess);
    setFlag(OwnsMaterial);

    // #### To avoid asserts: we should probably merge this with QSGCurveFillNode
    setGeometry(&m_geometry);
    setMaterial(new QSGTextureMaterial);
}

QSGCurveGlyphNode::~QSGCurveGlyphNode()
{
}

void QSGCurveGlyphNode::setPreferredAntialiasingMode(AntialiasingMode mode)
{
    Q_UNUSED(mode);
}

void QSGCurveGlyphNode::setColor(const QColor &color)
{
    m_color = color;
    if (m_glyphNode != nullptr)
        m_glyphNode->setColor(color);
}

void QSGCurveGlyphNode::setStyleColor(const QColor &styleColor)
{
    m_styleColor = styleColor;
    if (m_styleNode != nullptr)
        m_styleNode->setColor(styleColor);
}

void QSGCurveGlyphNode::setStyle(QQuickText::TextStyle style)
{
    if (m_style != style) {
        m_style = style;
        m_dirtyGeometry = true;
        update();
    }
}

void QSGCurveGlyphNode::setGlyphs(const QPointF &position, const QGlyphRun &glyphs)
{
    m_glyphs = glyphs;

    QRawFont font = glyphs.rawFont();
    m_fontSize = font.pixelSize();
    m_position = QPointF(position.x(), position.y() - font.ascent());


    m_dirtyGeometry = true;

#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QString::number(glyphs.glyphIndexes().count())
                                      + QStringLiteral(" curve glyphs: ")
                                      + m_glyphs.rawFont().familyName()
                                      + QStringLiteral(" ")
                                      + QString::number(m_glyphs.rawFont().pixelSize()));
#endif
}

void QSGCurveGlyphNode::update()
{
    markDirty(DirtyGeometry);
}

void QSGCurveGlyphNode::preprocess()
{
    if (m_dirtyGeometry)
        updateGeometry();
}

void QSGCurveGlyphNode::updateGeometry()
{
    delete m_glyphNode;
    m_glyphNode = nullptr;

    delete m_styleNode;
    m_styleNode = nullptr;

    QSGCurveGlyphAtlas *curveGlyphAtlas = m_context->curveGlyphAtlas(m_glyphs.rawFont());
    curveGlyphAtlas->populate(m_glyphs.glyphIndexes());

    m_glyphNode = new QSGCurveFillNode;
    m_glyphNode->setColor(m_color);

    QPointF offset;

    float fontScale = float(m_fontSize / curveGlyphAtlas->fontSize());
    QSGCurveFillNode *raisedSunkenStyleNode = nullptr;
    QSGCurveStrokeNode *outlineNode = nullptr;
    if (m_style == QQuickText::Raised || m_style == QQuickText::Sunken) {
        raisedSunkenStyleNode = new QSGCurveFillNode;
        raisedSunkenStyleNode ->setColor(m_styleColor);

        offset = m_style == QQuickText::Raised ? QPointF(0.0f, 1.0f) : QPointF(0.0f, -1.0f);
        m_styleNode = raisedSunkenStyleNode;
    } else if (m_style == QQuickText::Outline) {
        outlineNode = new QSGCurveStrokeNode;
        outlineNode->setColor(m_styleColor);
        outlineNode->setStrokeWidth(2 / fontScale);
        outlineNode->setLocalScale(fontScale);

        m_styleNode = outlineNode;
    }

    const QVector<quint32> indexes = m_glyphs.glyphIndexes();
    const QVector<QPointF> positions = m_glyphs.positions();
    for (qsizetype i = 0; i < indexes.size(); ++i) {
        if (i == 0)
            m_baseLine = positions.at(i);
        curveGlyphAtlas->addGlyph(m_glyphNode,
                                 indexes.at(i),
                                 m_position + positions.at(i),
                                 m_fontSize);
        if (raisedSunkenStyleNode != nullptr) {
            curveGlyphAtlas->addGlyph(raisedSunkenStyleNode,
                                      indexes.at(i),
                                      m_position + positions.at(i) + offset,
                                      m_fontSize);
        }
        if (outlineNode != nullptr) {
            // Since the stroke node will scale everything by fontScale internally (the
            // shader does not support pre-transforming the vertices), we have to also first
            // do the inverse scale on the glyph position to get the correct position.
            curveGlyphAtlas->addStroke(outlineNode,
                                       indexes.at(i),
                                       (m_position + positions.at(i)) / fontScale);
        }
    }

    if (m_styleNode != nullptr) {
        m_styleNode->cookGeometry();
        appendChildNode(m_styleNode);
    }

    m_glyphNode->cookGeometry();
    appendChildNode(m_glyphNode);

    m_dirtyGeometry = false;
}

QT_END_NAMESPACE
