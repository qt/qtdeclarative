// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgcurveglyphatlas_p.h"
#include "qsgcurvefillnode_p.h"
#include "qsgcurvestrokenode_p.h"
#include "qsgcurveprocessor_p.h"
#include "util/qquadpath_p.h"

#include <QtGui/qrawfont.h>
#include <QtGui/qpainterpath.h>

QT_BEGIN_NAMESPACE

QSGCurveGlyphAtlas::QSGCurveGlyphAtlas(const QRawFont &font)
    : m_font(font)
{
    // The font size used for the curve atlas currently affects the outlines, since we don't
    // really support cosmetic outlines. Therefore we need to pick one which gives large enough
    // triangles relative to glyph size that we can reuse the same triangles for any font size.
    // 64 is used as a "base font size" by the distance field renderer and other places in Qt
    // because this also has the benefit it's big enough that hinting will be disabled.
    static int curveGlyphAtlasFontSize = qEnvironmentVariableIntValue("QSGCURVEGLYPHATLAS_FONT_SIZE");
    m_font.setPixelSize(curveGlyphAtlasFontSize > 0 ? qreal(curveGlyphAtlasFontSize) : 64.0);
}

QSGCurveGlyphAtlas::~QSGCurveGlyphAtlas()
{
}

void QSGCurveGlyphAtlas::populate(const QList<glyph_t> &glyphs)
{
    for (glyph_t glyphIndex : glyphs) {
        if (!m_glyphs.contains(glyphIndex)) {
            QPainterPath path = m_font.pathForGlyph(glyphIndex);
            QQuadPath quadPath = QQuadPath::fromPainterPath(path);
            quadPath.setFillRule(Qt::WindingFill);

            Glyph glyph;

            QSGCurveProcessor::processStroke(quadPath, 2, 2, Qt::MiterJoin, Qt::FlatCap,
                                             [&glyph](const std::array<QVector2D, 3> &s,
                                                      const std::array<QVector2D, 3> &p,
                                                      const std::array<QVector2D, 3> &n,
                                                      bool isLine) {
                glyph.strokeVertices.append(s.at(0));
                glyph.strokeVertices.append(s.at(1));
                glyph.strokeVertices.append(s.at(2));

                glyph.strokeUvs.append(p.at(0));
                glyph.strokeUvs.append(p.at(1));
                glyph.strokeUvs.append(p.at(2));

                glyph.strokeNormals.append(n.at(0));
                glyph.strokeNormals.append(n.at(1));
                glyph.strokeNormals.append(n.at(2));

                glyph.strokeElementIsLine.append(isLine);
            });

            quadPath = quadPath.subPathsClosed();
            quadPath.addCurvatureData(); // ### Since the inside of glyphs is defined by order of
                                         // vertices, this step could be simplified
            QSGCurveProcessor::solveOverlaps(quadPath);

            QSGCurveProcessor::processFill(quadPath,
                                           Qt::WindingFill,
                                           [&glyph](const std::array<QVector2D, 3> &v,
                                                    const std::array<QVector2D, 3> &n,
                                                    QSGCurveProcessor::uvForPointCallback uvForPoint)
                                           {
                                               glyph.vertices.append(v.at(0));
                                               glyph.vertices.append(v.at(1));
                                               glyph.vertices.append(v.at(2));

                                               QVector3D uv1 = uvForPoint(v.at(0));
                                               glyph.uvs.append(uv1);
                                               glyph.uvs.append(uvForPoint(v.at(1)));
                                               glyph.uvs.append(uvForPoint(v.at(2)));

                                               glyph.normals.append(n.at(0));
                                               glyph.normals.append(n.at(1));
                                               glyph.normals.append(n.at(2));

                                               glyph.duvdx.append(QVector2D(uvForPoint(v.at(0) + QVector2D(1, 0))) - QVector2D(uv1));
                                               glyph.duvdy.append(QVector2D(uvForPoint(v.at(0) + QVector2D(0, 1))) - QVector2D(uv1));
                                           });

            m_glyphs.insert(glyphIndex, glyph);
        }
    }
}

void QSGCurveGlyphAtlas::addStroke(QSGCurveStrokeNode *node,
                              glyph_t glyphIndex,
                              const QPointF &position) const
{
    const Glyph &glyph = m_glyphs[glyphIndex];

    const QVector2D v(position);
    for (qsizetype i = glyph.strokeElementIsLine.size() - 1; i >= 0; --i) {
        QVector2D v1 = glyph.strokeVertices.at(i * 3 + 0) + v;
        QVector2D v2 = glyph.strokeVertices.at(i * 3 + 1) + v;
        QVector2D v3 = glyph.strokeVertices.at(i * 3 + 2) + v;
        if (glyph.strokeElementIsLine.at(i)) {
            node->appendTriangle({ v1, v2, v3 },
                                 std::array<QVector2D, 2>({ glyph.strokeUvs.at(i * 3 + 0) + v, glyph.strokeUvs.at(i * 3 + 2) + v }),
                                 { glyph.strokeNormals.at(i * 3 + 0), glyph.strokeNormals.at(i * 3 + 1), glyph.strokeNormals.at(i * 3 + 2) });
        } else {
            node->appendTriangle({ v1, v2, v3 },
                                 { glyph.strokeUvs.at(i * 3 + 0) + v, glyph.strokeUvs.at(i * 3 + 1) + v, glyph.strokeUvs.at(i * 3 + 2) + v },
                                 { glyph.strokeNormals.at(i * 3 + 0), glyph.strokeNormals.at(i * 3 + 1), glyph.strokeNormals.at(i * 3 + 2) });

        }
    }
}

void QSGCurveGlyphAtlas::addGlyph(QSGCurveFillNode *node,
                             glyph_t glyphIndex,
                             const QPointF &position,
                             qreal pixelSize) const
{
    const Glyph &glyph = m_glyphs[glyphIndex];

    const float scaleFactor = pixelSize / m_font.pixelSize();
    const QVector2D v(position);
    for (qsizetype i = 0; i < glyph.vertices.size() / 3; ++i) {
        node->appendTriangle(scaleFactor * glyph.vertices.at(i * 3 + 0) + v,
                             scaleFactor * glyph.vertices.at(i * 3 + 1) + v,
                             scaleFactor * glyph.vertices.at(i * 3 + 2) + v,
                             glyph.uvs.at(i * 3 + 0),
                             glyph.uvs.at(i * 3 + 1),
                             glyph.uvs.at(i * 3 + 2),
                             glyph.normals.at(i * 3 + 0),
                             glyph.normals.at(i * 3 + 1),
                             glyph.normals.at(i * 3 + 2),
                             glyph.duvdx.at(i) / scaleFactor,
                             glyph.duvdy.at(i) / scaleFactor);
    }
}

QT_END_NAMESPACE
