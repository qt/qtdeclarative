// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpathinterpolated_p.h"
#include <QtGui/private/qguisvg_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcPath, "qt.quick.shapes.path")

/*!
    \qmltype PathInterpolated
    \nativetype QQuickPathInterpolated
    \inqmlmodule QtQuick.VectorImage.Helpers
    \brief Defines a path as an interpolated value between two of the paths in a list.
    \since 6.11

    This item provides a simple way to specify an interpolated path. This is useful for displaying
    animated paths, where one path is gradually morphed into the next.

    The interpolation end points are specified in the \l svgPaths list property, using the same
    syntax as the PathSvg item. Based on the \l factor property, the resulting path will be an
    interpolation between path \e n and \e n+1 in the list, where \e n is the integer part of the
    factor. The fractional part determines the interpolation weight between the two.

    \sa Path, PathSvg
*/

QQuickPathInterpolated::QQuickPathInterpolated(QObject *parent) : QQuickCurve{ parent }
{
    connect(this, &QQuickPathInterpolated::factorChanged, this, &QQuickPathElement::changed);
    connect(this, &QQuickPathInterpolated::svgPathsChanged, this, &QQuickPathElement::changed);
}

/*!
    \qmlproperty real QtQuick.VectorImage.Helpers::PathInterpolated::factor

    This property holds the interpolation factor. The effective value is clamped to \e{[0,
    svgPaths.size - 1]}.
*/

qreal QQuickPathInterpolated::factor() const
{
    return m_factor;
}

void QQuickPathInterpolated::setFactor(qreal newFactor)
{
    if (qFuzzyCompare(m_factor, newFactor) || !qIsFinite(newFactor))
        return;
    m_factor = newFactor;
    emit factorChanged();
}

/*!
    \qmlproperty stringlist QtQuick.VectorImage.Helpers::PathInterpolated::svgPaths

    This property holds a list of paths, specified as SVG text strings in the manner of \l PathSvg.

    The generation of an interpolated value between two of the paths in the list depends on them
    having the same number and types of path elements. The resulting path has the same elements,
    with coordinates linearly interpolated between the two source paths.
*/

QStringList QQuickPathInterpolated::svgPaths() const
{
    return m_svgPaths;
}

void QQuickPathInterpolated::setSvgPaths(const QStringList &newSvgPaths)
{
    if (m_svgPaths == newSvgPaths)
        return;
    m_svgPaths = newSvgPaths;
    m_dirty = true;
    emit svgPathsChanged();
}

void QQuickPathInterpolated::addToPath(QPainterPath &path, const QQuickPathData &)
{
    const qsizetype pathCount = m_svgPaths.size();
    if (m_dirty) {
        m_paths.clear();
        m_paths.resize(pathCount);
        for (qsizetype i = 0; i < pathCount; i++) {
            std::optional<QPainterPath> qpath = QGuiSvg::parsePath(m_svgPaths.at(i));
            if (qpath)
                m_paths[i] = qpath.value();
            else
                qCDebug(lcPath) << "Syntax error in svg path no." << i;
        }
        m_dirty = false;
    }
    Q_ASSERT(m_paths.size() == pathCount);

    if (m_paths.isEmpty())
        return;

    QPainterPath res;
    qreal factorIntValue = 0;
    const qreal f = std::modf(m_factor, &factorIntValue);
    const qsizetype pathIdx = qsizetype(qBound(qreal(0), factorIntValue, qreal(pathCount - 1)));

    if (m_paths.size() == 1 || (pathIdx == 0 && f <= 0)) {
        res = m_paths.first();
    } else {
        res = m_paths.at(pathIdx);
        if ((pathIdx < pathCount - 1) && f > 0) {
            const QPainterPath &p0 = m_paths.at(pathIdx);
            const QPainterPath &p1 = m_paths.at(pathIdx + 1);
            if (p0.elementCount() == p1.elementCount()) {
                for (qsizetype i = 0; i < p0.elementCount(); i++) {
                    QPainterPath::Element e0 = p0.elementAt(i);
                    QPainterPath::Element e1 = p1.elementAt(i);
                    if (e0.type != e1.type) {
                        qCDebug(lcPath) << "Differing elements in svg path no." << i;
                        break;
                    }
                    QPointF cp = QPointF(e0) + f * (QPointF(e1) - QPointF(e0));
                    res.setElementPositionAt(i, cp.x(), cp.y());
                }
            } else {
                qCDebug(lcPath) << "Differing element count in svg path no." << pathIdx + 1;
            }
        }
    }
    path.addPath(res);
}

QT_END_NAMESPACE
