/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgdistancefieldglyphcache_p.h"

#include <qmath.h>
#include <private/qsgpathsimplifier_p.h>
#include <private/qdeclarativeglobal_p.h>
#include <qopenglshaderprogram.h>
#include <QtGui/private/qopenglengineshadersource_p.h>
#include <private/qsgcontext_p.h>
#include <private/qrawfont_p.h>
#include <qopenglfunctions.h>
#include <qglyphrun.h>
#include <qrawfont.h>
#include <qdir.h>
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

#define QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE 54
#define QT_DISTANCEFIELD_DEFAULT_TILESIZE 64
#define QT_DISTANCEFIELD_DEFAULT_SCALE 16
#define QT_DISTANCEFIELD_DEFAULT_RADIUS 80
#define QT_DISTANCEFIELD_HIGHGLYPHCOUNT 2000

#define QT_DISTANCEFIELD_BASEFONTSIZE \
    (m_textureData->doubleGlyphResolution ? QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE * 2 : \
                                           QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE)
#define QT_DISTANCEFIELD_TILESIZE \
    (m_textureData->doubleGlyphResolution ? QT_DISTANCEFIELD_DEFAULT_TILESIZE * 2 : \
                                           QT_DISTANCEFIELD_DEFAULT_TILESIZE)
#define QT_DISTANCEFIELD_SCALE \
    (m_textureData->doubleGlyphResolution ? QT_DISTANCEFIELD_DEFAULT_SCALE / 2 : \
                                           QT_DISTANCEFIELD_DEFAULT_SCALE)
#define QT_DISTANCEFIELD_RADIUS \
    (m_textureData->doubleGlyphResolution ? QT_DISTANCEFIELD_DEFAULT_RADIUS / 2 : \
                                           QT_DISTANCEFIELD_DEFAULT_RADIUS)

static inline int qt_next_power_of_two(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}

static float defaultThresholdFunc(float glyphScale)
{
    static float base = qgetenv("QT_DF_BASE").isEmpty() ? 0.5f : qgetenv("QT_DF_BASE").toFloat();
    static float baseDev = qgetenv("QT_DF_BASEDEVIATION").isEmpty() ? 0.065f : qgetenv("QT_DF_BASEDEVIATION").toFloat();
    static float devScaleMin = qgetenv("QT_DF_SCALEFORMAXDEV").isEmpty() ? 0.15f : qgetenv("QT_DF_SCALEFORMAXDEV").toFloat();
    static float devScaleMax = qgetenv("QT_DF_SCALEFORNODEV").isEmpty() ? 0.3f : qgetenv("QT_DF_SCALEFORNODEV").toFloat();
    return base - ((qBound(devScaleMin, glyphScale, devScaleMax) - devScaleMin) / (devScaleMax - devScaleMin) * -baseDev + baseDev);
}

static float defaultAntialiasingSpreadFunc(float glyphScale)
{
    static float range = qgetenv("QT_DF_RANGE").isEmpty() ? 0.06f : qgetenv("QT_DF_RANGE").toFloat();
    return range / glyphScale;
}

namespace
{
    enum FillHDir
    {
        LeftToRight,
        RightToLeft
    };

    enum FillVDir
    {
        TopDown,
        BottomUp
    };

    enum FillClip
    {
        NoClip,
        Clip
    };
}

template <FillClip clip, FillHDir dir>
inline void fillLine(qint32 *, int, int, int, qint32, qint32)
{
}

template <>
inline void fillLine<Clip, LeftToRight>(qint32 *line, int width, int lx, int rx, qint32 d, qint32 dd)
{
    int fromX = qMax(0, lx >> 8);
    int toX = qMin(width, rx >> 8);
    int x = toX - fromX;
    if (x <= 0)
        return;
    qint32 val = d + (((fromX << 8) + 0xff - lx) * dd >> 8);
    line += fromX;
    do {
        *line = abs(val) < abs(*line) ? val : *line;
        val += dd;
        ++line;
    } while (--x);
}

template <>
inline void fillLine<Clip, RightToLeft>(qint32 *line, int width, int lx, int rx, qint32 d, qint32 dd)
{
    int fromX = qMax(0, lx >> 8);
    int toX = qMin(width, rx >> 8);
    int x = toX - fromX;
    if (x <= 0)
        return;
    qint32 val = d + (((toX << 8) + 0xff - rx) * dd >> 8);
    line += toX;
    do {
        val -= dd;
        --line;
        *line = abs(val) < abs(*line) ? val : *line;
    } while (--x);
}

template <>
inline void fillLine<NoClip, LeftToRight>(qint32 *line, int, int lx, int rx, qint32 d, qint32 dd)
{
    int fromX = lx >> 8;
    int toX = rx >> 8;
    int x = toX - fromX;
    if (x <= 0)
        return;
    qint32 val = d + ((~lx & 0xff) * dd >> 8);
    line += fromX;
    do {
        *line = abs(val) < abs(*line) ? val : *line;
        val += dd;
        ++line;
    } while (--x);
}

template <>
inline void fillLine<NoClip, RightToLeft>(qint32 *line, int, int lx, int rx, qint32 d, qint32 dd)
{
    int fromX = lx >> 8;
    int toX = rx >> 8;
    int x = toX - fromX;
    if (x <= 0)
        return;
    qint32 val = d + ((~rx & 0xff) * dd >> 8);
    line += toX;
    do {
        val -= dd;
        --line;
        *line = abs(val) < abs(*line) ? val : *line;
    } while (--x);
}

template <FillClip clip, FillVDir vDir, FillHDir hDir>
inline void fillLines(qint32 *bits, int width, int height, int upperY, int lowerY,
                      int &lx, int ldx, int &rx, int rdx, qint32 &d, qint32 ddy, qint32 ddx)
{
    Q_UNUSED(height);
    Q_ASSERT(upperY < lowerY);
    int y = lowerY - upperY;
    if (vDir == TopDown) {
        qint32 *line = bits + upperY * width;
        do {
            fillLine<clip, hDir>(line, width, lx, rx, d, ddx);
            lx += ldx;
            d += ddy;
            rx += rdx;
            line += width;
        } while (--y);
    } else {
        qint32 *line = bits + lowerY * width;
        do {
            lx -= ldx;
            d -= ddy;
            rx -= rdx;
            line -= width;
            fillLine<clip, hDir>(line, width, lx, rx, d, ddx);
        } while (--y);
    }
}

template <FillClip clip>
void drawTriangle(qint32 *bits, int width, int height, const QPoint *center,
                  const QPoint *v1, const QPoint *v2, qint32 value)
{
    const int y1 = clip == Clip ? qBound(0, v1->y() >> 8, height) : v1->y() >> 8;
    const int y2 = clip == Clip ? qBound(0, v2->y() >> 8, height) : v2->y() >> 8;
    const int yC = clip == Clip ? qBound(0, center->y() >> 8, height) : center->y() >> 8;

    const int v1Frac = clip == Clip ? (y1 << 8) + 0xff - v1->y() : ~v2->y() & 0xff;
    const int v2Frac = clip == Clip ? (y2 << 8) + 0xff - v2->y() : ~v1->y() & 0xff;
    const int centerFrac = clip == Clip ? (yC << 8) + 0xff - center->y() : ~center->y() & 0xff;

    int dx1, x1, dx2, x2;
    qint32 dd1, d1, dd2, d2;
    if (v1->y() != center->y()) {
        dx1 = ((v1->x() - center->x()) << 8) / (v1->y() - center->y());
        x1 = center->x() + centerFrac * (v1->x() - center->x()) / (v1->y() - center->y());
    }
    if (v2->y() != center->y()) {
        dx2 = ((v2->x() - center->x()) << 8) / (v2->y() - center->y());
        x2 = center->x() + centerFrac * (v2->x() - center->x()) / (v2->y() - center->y());
    }

    const qint32 div = (v2->x() - center->x()) * (v1->y() - center->y())
                     - (v2->y() - center->y()) * (v1->x() - center->x());
    const qint32 dd = div ? qint32((qint64(value * (v1->y() - v2->y())) << 8) / div) : 0;

    if (y2 < yC) {
        if (y1 < yC) {
            // Center at the bottom.
            if (y2 < y1) {
                // y2 < y1 < yC
                // Long right edge.
                d1 = centerFrac * value / (v1->y() - center->y());
                dd1 = ((value << 8) / (v1->y() - center->y()));
                fillLines<clip, BottomUp, LeftToRight>(bits, width, height, y1, yC, x1, dx1,
                                                       x2, dx2, d1, dd1, dd);
                dx1 = ((v1->x() - v2->x()) << 8) / (v1->y() - v2->y());
                x1 = v1->x() + v1Frac * (v1->x() - v2->x()) / (v1->y() - v2->y());
                fillLines<clip, BottomUp, LeftToRight>(bits, width, height, y2, y1, x1, dx1,
                                                       x2, dx2, value, 0, dd);
            } else {
                // y1 <= y2 < yC
                // Long left edge.
                d2 = centerFrac * value / (v2->y() - center->y());
                dd2 = ((value << 8) / (v2->y() - center->y()));
                fillLines<clip, BottomUp, RightToLeft>(bits, width, height, y2, yC, x1, dx1,
                                                       x2, dx2, d2, dd2, dd);
                if (y1 != y2) {
                    dx2 = ((v1->x() - v2->x()) << 8) / (v1->y() - v2->y());
                    x2 = v2->x() + v2Frac * (v1->x() - v2->x()) / (v1->y() - v2->y());
                    fillLines<clip, BottomUp, RightToLeft>(bits, width, height, y1, y2, x1, dx1,
                                                           x2, dx2, value, 0, dd);
                }
            }
        } else {
            // y2 < yC <= y1
            // Center to the right.
            int dx = ((v1->x() - v2->x()) << 8) / (v1->y() - v2->y());
            int xUp, xDn;
            xUp = xDn = v2->x() + (clip == Clip ? (yC << 8) + 0xff - v2->y()
                                                : (center->y() | 0xff) - v2->y())
                        * (v1->x() - v2->x()) / (v1->y() - v2->y());
            fillLines<clip, BottomUp, LeftToRight>(bits, width, height, y2, yC, xUp, dx,
                                                   x2, dx2, value, 0, dd);
            if (yC != y1)
                fillLines<clip, TopDown, LeftToRight>(bits, width, height, yC, y1, xDn, dx,
                                                      x1, dx1, value, 0, dd);
        }
    } else {
        if (y1 < yC) {
            // y1 < yC <= y2
            // Center to the left.
            int dx = ((v1->x() - v2->x()) << 8) / (v1->y() - v2->y());
            int xUp, xDn;
            xUp = xDn = v1->x() + (clip == Clip ? (yC << 8) + 0xff - v1->y()
                                                : (center->y() | 0xff) - v1->y())
                        * (v1->x() - v2->x()) / (v1->y() - v2->y());
            fillLines<clip, BottomUp, RightToLeft>(bits, width, height, y1, yC, x1, dx1,
                                                   xUp, dx, value, 0, dd);
            if (yC != y2)
                fillLines<clip, TopDown, RightToLeft>(bits, width, height, yC, y2, x2, dx2,
                                                      xDn, dx, value, 0, dd);
        } else {
            // Center at the top.
            if (y2 < y1) {
                // yC <= y2 < y1
                // Long right edge.
                if (yC != y2) {
                    d2 = centerFrac * value / (v2->y() - center->y());
                    dd2 = ((value << 8) / (v2->y() - center->y()));
                    fillLines<clip, TopDown, LeftToRight>(bits, width, height, yC, y2, x2, dx2,
                                                          x1, dx1, d2, dd2, dd);
                }
                dx2 = ((v1->x() - v2->x()) << 8) / (v1->y() - v2->y());
                x2 = v2->x() + v2Frac * (v1->x() - v2->x()) / (v1->y() - v2->y());
                fillLines<clip, TopDown, LeftToRight>(bits, width, height, y2, y1, x2, dx2,
                                                      x1, dx1, value, 0, dd);
            } else {
                // Long left edge.
                // yC <= y1 <= y2
                if (yC != y1) {
                    d1 = centerFrac * value / (v1->y() - center->y());
                    dd1 = ((value << 8) / (v1->y() - center->y()));
                    fillLines<clip, TopDown, RightToLeft>(bits, width, height, yC, y1, x2, dx2,
                                                          x1, dx1, d1, dd1, dd);
                }
                if (y1 != y2) {
                    dx1 = ((v1->x() - v2->x()) << 8) / (v1->y() - v2->y());
                    x1 = v1->x() + v1Frac * (v1->x() - v2->x()) / (v1->y() - v2->y());
                    fillLines<clip, TopDown, RightToLeft>(bits, width, height, y1, y2, x2, dx2,
                                                          x1, dx1, value, 0, dd);
                }
            }
        }
    }
}

template <FillClip clip>
void drawRectangle(qint32 *bits, int width, int height,
                   const QPoint *int1, const QPoint *center1, const QPoint *ext1,
                   const QPoint *int2, const QPoint *center2, const QPoint *ext2,
                   qint32 extValue)
{
    if (center1->y() > center2->y()) {
        qSwap(center1, center2);
        qSwap(int1, ext2);
        qSwap(ext1, int2);
        extValue = -extValue;
    }

    Q_ASSERT(ext1->x() - center1->x() == center1->x() - int1->x());
    Q_ASSERT(ext1->y() - center1->y() == center1->y() - int1->y());
    Q_ASSERT(ext2->x() - center2->x() == center2->x() - int2->x());
    Q_ASSERT(ext2->y() - center2->y() == center2->y() - int2->y());

    const int yc1 = clip == Clip ? qBound(0, center1->y() >> 8, height) : center1->y() >> 8;
    const int yc2 = clip == Clip ? qBound(0, center2->y() >> 8, height) : center2->y() >> 8;
    const int yi1 = clip == Clip ? qBound(0, int1->y() >> 8, height) : int1->y() >> 8;
    const int yi2 = clip == Clip ? qBound(0, int2->y() >> 8, height) : int2->y() >> 8;
    const int ye1 = clip == Clip ? qBound(0, ext1->y() >> 8, height) : ext1->y() >> 8;
    const int ye2 = clip == Clip ? qBound(0, ext2->y() >> 8, height) : ext2->y() >> 8;

    const int center1Frac = clip == Clip ? (yc1 << 8) + 0xff - center1->y() : ~center1->y() & 0xff;
    const int center2Frac = clip == Clip ? (yc2 << 8) + 0xff - center2->y() : ~center2->y() & 0xff;
    const int int1Frac = clip == Clip ? (yi1 << 8) + 0xff - int1->y() : ~int1->y() & 0xff;
    const int ext1Frac = clip == Clip ? (ye1 << 8) + 0xff - ext1->y() : ~ext1->y() & 0xff;

    int dxC, dxE; // cap slope, edge slope
    qint32 ddC;
    if (ext1->y() != int1->y()) {
        dxC = ((ext1->x() - int1->x()) << 8) / (ext1->y() - int1->y());
        ddC = (extValue << 9) / (ext1->y() - int1->y());
    }
    if (ext1->y() != ext2->y())
        dxE = ((ext1->x() - ext2->x()) << 8) / (ext1->y() - ext2->y());

    const qint32 div = (ext1->x() - int1->x()) * (ext2->y() - int1->y())
                     - (ext1->y() - int1->y()) * (ext2->x() - int1->x());
    const qint32 dd = div ? qint32((qint64(extValue * (ext2->y() - ext1->y())) << 9) / div) : 0;

    int xe1, xe2, xc1, xc2;
    qint32 d;

    qint32 intValue = -extValue;

    if (center2->x() < center1->x()) {
        // Leaning to the right. '/'
        if (int1->y() < ext2->y()) {
            // Mostly vertical.
            Q_ASSERT(ext1->y() != ext2->y());
            xe1 = ext1->x() + ext1Frac * (ext1->x() - ext2->x()) / (ext1->y() - ext2->y());
            xe2 = int1->x() + int1Frac * (ext1->x() - ext2->x()) / (ext1->y() - ext2->y());
            if (ye1 != yi1) {
                xc2 = center1->x() + center1Frac * (ext1->x() - int1->x()) / (ext1->y() - int1->y());
                xc2 += (ye1 - yc1) * dxC;
                fillLines<clip, TopDown, LeftToRight>(bits, width, height, ye1, yi1, xe1, dxE,
                                                      xc2, dxC, extValue, 0, dd);
            }
            if (yi1 != ye2)
                fillLines<clip, TopDown, LeftToRight>(bits, width, height, yi1, ye2, xe1, dxE,
                                                      xe2, dxE, extValue, 0, dd);
            if (ye2 != yi2) {
                xc1 = center2->x() + center2Frac * (ext1->x() - int1->x()) / (ext1->y() - int1->y());
                xc1 += (ye2 - yc2) * dxC;
                fillLines<clip, TopDown, RightToLeft>(bits, width, height, ye2, yi2, xc1, dxC,
                                                      xe2, dxE, intValue, 0, dd);
            }
        } else {
            // Mostly horizontal.
            Q_ASSERT(ext1->y() != int1->y());
            xc1 = center2->x() + center2Frac * (ext1->x() - int1->x()) / (ext1->y() - int1->y());
            xc2 = center1->x() + center1Frac * (ext1->x() - int1->x()) / (ext1->y() - int1->y());
            xc1 += (ye2 - yc2) * dxC;
            xc2 += (ye1 - yc1) * dxC;
            if (ye1 != ye2) {
                xe1 = ext1->x() + ext1Frac * (ext1->x() - ext2->x()) / (ext1->y() - ext2->y());
                fillLines<clip, TopDown, LeftToRight>(bits, width, height, ye1, ye2, xe1, dxE,
                                                      xc2, dxC, extValue, 0, dd);
            }
            if (ye2 != yi1) {
                d = (clip == Clip ? (ye2 << 8) + 0xff - center2->y()
                                  : (ext2->y() | 0xff) - center2->y())
                    * 2 * extValue / (ext1->y() - int1->y());
                fillLines<clip, TopDown, LeftToRight>(bits, width, height, ye2, yi1, xc1, dxC,
                                                      xc2, dxC, d, ddC, dd);
            }
            if (yi1 != yi2) {
                xe2 = int1->x() + int1Frac * (ext1->x() - ext2->x()) / (ext1->y() - ext2->y());
                fillLines<clip, TopDown, RightToLeft>(bits, width, height, yi1, yi2, xc1, dxC,
                                                      xe2, dxE, intValue, 0, dd);
            }
        }
    } else {
        // Leaning to the left. '\'
        if (ext1->y() < int2->y()) {
            // Mostly vertical.
            Q_ASSERT(ext1->y() != ext2->y());
            xe1 = ext1->x() + ext1Frac * (ext1->x() - ext2->x()) / (ext1->y() - ext2->y());
            xe2 = int1->x() + int1Frac * (ext1->x() - ext2->x()) / (ext1->y() - ext2->y());
            if (yi1 != ye1) {
                xc1 = center1->x() + center1Frac * (ext1->x() - int1->x()) / (ext1->y() - int1->y());
                xc1 += (yi1 - yc1) * dxC;
                fillLines<clip, TopDown, RightToLeft>(bits, width, height, yi1, ye1, xc1, dxC,
                                                      xe2, dxE, intValue, 0, dd);
            }
            if (ye1 != yi2)
                fillLines<clip, TopDown, RightToLeft>(bits, width, height, ye1, yi2, xe1, dxE,
                                                      xe2, dxE, intValue, 0, dd);
            if (yi2 != ye2) {
                xc2 = center2->x() + center2Frac * (ext1->x() - int1->x()) / (ext1->y() - int1->y());
                xc2 += (yi2 - yc2) * dxC;
                fillLines<clip, TopDown, LeftToRight>(bits, width, height, yi2, ye2, xe1, dxE,
                                                      xc2, dxC, extValue, 0, dd);
            }
        } else {
            // Mostly horizontal.
            Q_ASSERT(ext1->y() != int1->y());
            xc1 = center1->x() + center1Frac * (ext1->x() - int1->x()) / (ext1->y() - int1->y());
            xc2 = center2->x() + center2Frac * (ext1->x() - int1->x()) / (ext1->y() - int1->y());
            xc1 += (yi1 - yc1) * dxC;
            xc2 += (yi2 - yc2) * dxC;
            if (yi1 != yi2) {
                xe2 = int1->x() + int1Frac * (ext1->x() - ext2->x()) / (ext1->y() - ext2->y());
                fillLines<clip, TopDown, RightToLeft>(bits, width, height, yi1, yi2, xc1, dxC,
                                                      xe2, dxE, intValue, 0, dd);
            }
            if (yi2 != ye1) {
                d = (clip == Clip ? (yi2 << 8) + 0xff - center2->y()
                                  : (int2->y() | 0xff) - center2->y())
                    * 2 * extValue / (ext1->y() - int1->y());
                fillLines<clip, TopDown, RightToLeft>(bits, width, height, yi2, ye1, xc1, dxC,
                                                      xc2, dxC, d, ddC, dd);
            }
            if (ye1 != ye2) {
                xe1 = ext1->x() + ext1Frac * (ext1->x() - ext2->x()) / (ext1->y() - ext2->y());
                fillLines<clip, TopDown, LeftToRight>(bits, width, height, ye1, ye2, xe1, dxE,
                                                      xc2, dxC, extValue, 0, dd);
            }
        }
    }
}

static void drawPolygons(qint32 *bits, int width, int height, const QPoint *vertices,
                         const quint32 *indices, int indexCount, qint32 value)
{
    Q_ASSERT(indexCount != 0);
    Q_ASSERT(height <= 128);
    QVarLengthArray<quint8, 16> scans[128];
    int first = 0;
    for (int i = 1; i < indexCount; ++i) {
        quint32 idx1 = indices[i - 1];
        quint32 idx2 = indices[i];
        Q_ASSERT(idx1 != quint32(-1));
        if (idx2 == quint32(-1)) {
            idx2 = indices[first];
            Q_ASSERT(idx2 != quint32(-1));
            first = ++i;
        }
        const QPoint *v1 = &vertices[idx1];
        const QPoint *v2 = &vertices[idx2];
        if (v2->y() < v1->y())
            qSwap(v1, v2);
        int fromY = qMax(0, v1->y() >> 8);
        int toY = qMin(height, v2->y() >> 8);
        if (fromY >= toY)
            continue;
        int dx = ((v2->x() - v1->x()) << 8) / (v2->y() - v1->y());
        int x = v1->x() + ((fromY << 8) + 0xff - v1->y()) * (v2->x() - v1->x()) / (v2->y() - v1->y());
        for (int y = fromY; y < toY; ++y) {
            quint32 c = quint32(x >> 8);
            if (c < quint32(width))
                scans[y].append(quint8(c));
            x += dx;
        }
    }
    for (int i = 0; i < height; ++i) {
        quint8 *scanline = scans[i].data();
        int size = scans[i].size();
        for (int j = 1; j < size; ++j) {
            int k = j;
            quint8 value = scanline[k];
            for (; k != 0 && value < scanline[k - 1]; --k)
                scanline[k] = scanline[k - 1];
            scanline[k] = value;
        }
        qint32 *line = bits + i * width;
        int j = 0;
        for (; j + 1 < size; j += 2) {
            for (quint8 x = scanline[j]; x < scanline[j + 1]; ++x)
                line[x] = value;
        }
        if (j < size) {
            for (int x = scanline[j]; x < width; ++x)
                line[x] = value;
        }
    }
}

static QImage makeDistanceField(int imgSize, const QPainterPath &path, int dfScale, int offs)
{
    QImage image(imgSize, imgSize, QImage::Format_Indexed8);

    if (path.isEmpty()) {
        image.fill(0);
        return image;
    }

    QTransform transform;
    transform.translate(offs, offs);
    transform.scale(qreal(1) / dfScale, qreal(1) / dfScale);

    QDataBuffer<quint32> pathIndices(0);
    QDataBuffer<QPoint> pathVertices(0);
    qSimplifyPath(path, pathVertices, pathIndices, transform);

    const qint32 interiorColor = -0x7f80; // 8:8 signed format, -127.5
    const qint32 exteriorColor = 0x7f80; // 8:8 signed format, 127.5

    QScopedArrayPointer<qint32> bits(new qint32[imgSize * imgSize]);
    for (int i = 0; i < imgSize * imgSize; ++i)
        bits[i] = exteriorColor;

    const qreal angleStep = qreal(15 * 3.141592653589793238 / 180);
    const QPoint rotation(qRound(cos(angleStep) * 0x4000),
                          qRound(sin(angleStep) * 0x4000)); // 2:14 signed

    const quint32 *indices = pathIndices.data();
    QVarLengthArray<QPoint> normals;
    QVarLengthArray<QPoint> vertices;
    QVarLengthArray<bool> isConvex;
    QVarLengthArray<bool> needsClipping;

    drawPolygons(bits.data(), imgSize, imgSize, pathVertices.data(), indices, pathIndices.size(),
                 interiorColor);

    int index = 0;

    while (index < pathIndices.size()) {
        normals.clear();
        vertices.clear();
        needsClipping.clear();

        // Find end of polygon.
        int end = index;
        while (indices[end] != quint32(-1))
            ++end;

        // Calculate vertex normals.
        for (int next = index, prev = end - 1; next < end; prev = next++) {
            quint32 fromVertexIndex = indices[prev];
            quint32 toVertexIndex = indices[next];

            const QPoint &from = pathVertices.at(fromVertexIndex);
            const QPoint &to = pathVertices.at(toVertexIndex);

            QPoint n(to.y() - from.y(), from.x() - to.x());
            if (n.x() == 0 && n.y() == 0)
                continue;
            int scale = qRound((offs << 16) / sqrt(qreal(n.x() * n.x() + n.y() * n.y()))); // 8:16
            n.rx() = n.x() * scale >> 8;
            n.ry() = n.y() * scale >> 8;
            normals.append(n);
            QPoint v(to.x() + 0x7f, to.y() + 0x7f);
            vertices.append(v);
            needsClipping.append((to.x() < offs << 8) || (to.x() >= (imgSize - offs) << 8)
                                 || (to.y() < offs << 8) || (to.y() >= (imgSize - offs) << 8));
        }

        isConvex.resize(normals.count());
        for (int next = 0, prev = normals.count() - 1; next < normals.count(); prev = next++) {
            isConvex[prev] = normals.at(prev).x() * normals.at(next).y()
                           - normals.at(prev).y() * normals.at(next).x() < 0;
        }

        // Draw quads.
        for (int next = 0, prev = normals.count() - 1; next < normals.count(); prev = next++) {
            QPoint n = normals.at(next);
            QPoint intPrev = vertices.at(prev);
            QPoint extPrev = vertices.at(prev);
            QPoint intNext = vertices.at(next);
            QPoint extNext = vertices.at(next);

            extPrev.rx() -= n.x();
            extPrev.ry() -= n.y();
            intPrev.rx() += n.x();
            intPrev.ry() += n.y();
            extNext.rx() -= n.x();
            extNext.ry() -= n.y();
            intNext.rx() += n.x();
            intNext.ry() += n.y();

            if (needsClipping[prev] || needsClipping[next]) {
                drawRectangle<Clip>(bits.data(), imgSize, imgSize,
                                    &intPrev, &vertices.at(prev), &extPrev,
                                    &intNext, &vertices.at(next), &extNext,
                                    exteriorColor);
            } else {
                drawRectangle<NoClip>(bits.data(), imgSize, imgSize,
                                      &intPrev, &vertices.at(prev), &extPrev,
                                      &intNext, &vertices.at(next), &extNext,
                                      exteriorColor);
            }

            if (isConvex.at(prev)) {
                QPoint p = extPrev;
                if (needsClipping[prev]) {
                    for (;;) {
                        QPoint rn((n.x() * rotation.x() - n.y() * rotation.y()) >> 14,
                                  (n.y() * rotation.x() + n.x() * rotation.y()) >> 14);
                        n = rn;
                        if (n.x() * normals.at(prev).y() - n.y() * normals.at(prev).x() <= 0) {
                            p.rx() = vertices.at(prev).x() - normals.at(prev).x();
                            p.ry() = vertices.at(prev).y() - normals.at(prev).y();
                            drawTriangle<Clip>(bits.data(), imgSize, imgSize, &vertices.at(prev),
                                               &extPrev, &p, exteriorColor);
                            break;
                        }

                        p.rx() = vertices.at(prev).x() - n.x();
                        p.ry() = vertices.at(prev).y() - n.y();
                        drawTriangle<Clip>(bits.data(), imgSize, imgSize, &vertices.at(prev),
                                           &extPrev, &p, exteriorColor);
                        extPrev = p;
                    }
                } else {
                    for (;;) {
                        QPoint rn((n.x() * rotation.x() - n.y() * rotation.y()) >> 14,
                                  (n.y() * rotation.x() + n.x() * rotation.y()) >> 14);
                        n = rn;
                        if (n.x() * normals.at(prev).y() - n.y() * normals.at(prev).x() <= 0) {
                            p.rx() = vertices.at(prev).x() - normals.at(prev).x();
                            p.ry() = vertices.at(prev).y() - normals.at(prev).y();
                            drawTriangle<NoClip>(bits.data(), imgSize, imgSize, &vertices.at(prev),
                                                 &extPrev, &p, exteriorColor);
                            break;
                        }

                        p.rx() = vertices.at(prev).x() - n.x();
                        p.ry() = vertices.at(prev).y() - n.y();
                        drawTriangle<NoClip>(bits.data(), imgSize, imgSize, &vertices.at(prev),
                                             &extPrev, &p, exteriorColor);
                        extPrev = p;
                    }
                }
            } else {
                QPoint p = intPrev;
                if (needsClipping[prev]) {
                    for (;;) {
                        QPoint rn((n.x() * rotation.x() + n.y() * rotation.y()) >> 14,
                                  (n.y() * rotation.x() - n.x() * rotation.y()) >> 14);
                        n = rn;
                        if (n.x() * normals.at(prev).y() - n.y() * normals.at(prev).x() >= 0) {
                            p.rx() = vertices.at(prev).x() + normals.at(prev).x();
                            p.ry() = vertices.at(prev).y() + normals.at(prev).y();
                            drawTriangle<Clip>(bits.data(), imgSize, imgSize, &vertices.at(prev),
                                               &p, &intPrev, interiorColor);
                            break;
                        }

                        p.rx() = vertices.at(prev).x() + n.x();
                        p.ry() = vertices.at(prev).y() + n.y();
                        drawTriangle<Clip>(bits.data(), imgSize, imgSize, &vertices.at(prev),
                                           &p, &intPrev, interiorColor);
                        intPrev = p;
                    }
                } else {
                    for (;;) {
                        QPoint rn((n.x() * rotation.x() + n.y() * rotation.y()) >> 14,
                                  (n.y() * rotation.x() - n.x() * rotation.y()) >> 14);
                        n = rn;
                        if (n.x() * normals.at(prev).y() - n.y() * normals.at(prev).x() >= 0) {
                            p.rx() = vertices.at(prev).x() + normals.at(prev).x();
                            p.ry() = vertices.at(prev).y() + normals.at(prev).y();
                            drawTriangle<NoClip>(bits.data(), imgSize, imgSize, &vertices.at(prev),
                                                 &p, &intPrev, interiorColor);
                            break;
                        }

                        p.rx() = vertices.at(prev).x() + n.x();
                        p.ry() = vertices.at(prev).y() + n.y();
                        drawTriangle<NoClip>(bits.data(), imgSize, imgSize, &vertices.at(prev),
                                             &p, &intPrev, interiorColor);
                        intPrev = p;
                    }
                }
            }
        }

        index = end + 1;
    }

    const qint32 *inLine = bits.data();
    uchar *outLine = image.bits();
    int padding = image.bytesPerLine() - image.width();
    for (int y = 0; y < imgSize; ++y) {
        for (int x = 0; x < imgSize; ++x, ++inLine, ++outLine)
            *outLine = uchar((0x7f80 - *inLine) >> 8);
        outLine += padding;
    }

    return image;
}

static bool fontHasNarrowOutlines(const QRawFont &f)
{
    QRawFont font = f;
    font.setPixelSize(QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE);
    Q_ASSERT(font.isValid());

    QVector<quint32> glyphIndices = font.glyphIndexesForString(QLatin1String("O"));
    if (glyphIndices.size() < 1)
        return false;

    QImage im = font.alphaMapForGlyph(glyphIndices.at(0), QRawFont::PixelAntialiasing);
    if (im.isNull())
        return false;

    int minHThick = 999;
    int minVThick = 999;

    int thick = 0;
    bool in = false;
    int y = (im.height() + 1) / 2;
    for (int x = 0; x < im.width(); ++x) {
        int a = qAlpha(im.pixel(x, y));
        if (a > 127) {
            in = true;
            ++thick;
        } else if (in) {
            in = false;
            minHThick = qMin(minHThick, thick);
            thick = 0;
        }
    }

    thick = 0;
    in = false;
    int x = (im.width() + 1) / 2;
    for (int y = 0; y < im.height(); ++y) {
        int a = qAlpha(im.pixel(x, y));
        if (a > 127) {
            in = true;
            ++thick;
        } else if (in) {
            in = false;
            minVThick = qMin(minVThick, thick);
            thick = 0;
        }
    }

    return minHThick == 1 || minVThick == 1;
}

QSGDistanceFieldGlyphCacheManager::QSGDistanceFieldGlyphCacheManager(QOpenGLContext *c)
    : ctx(c)
    , m_threshold_func(defaultThresholdFunc)
    , m_antialiasingSpread_func(defaultAntialiasingSpreadFunc)
    , m_maxTextureSize(0)
{
#ifndef QT_OPENGL_ES
    m_defaultAntialiasingMode = QSGGlyphNode::HighQualitySubPixelAntialiasing;
#else
    m_defaultAntialiasingMode = QSGGlyphNode::GrayAntialiasing;
#endif

    m_vertexCoordinateArray[0] = -1.0f;
    m_vertexCoordinateArray[1] = -1.0f;
    m_vertexCoordinateArray[2] =  1.0f;
    m_vertexCoordinateArray[3] = -1.0f;
    m_vertexCoordinateArray[4] =  1.0f;
    m_vertexCoordinateArray[5] =  1.0f;
    m_vertexCoordinateArray[6] = -1.0f;
    m_vertexCoordinateArray[7] =  1.0f;

    m_textureCoordinateArray[0] = 0.0f;
    m_textureCoordinateArray[1] = 0.0f;
    m_textureCoordinateArray[2] = 1.0f;
    m_textureCoordinateArray[3] = 0.0f;
    m_textureCoordinateArray[4] = 1.0f;
    m_textureCoordinateArray[5] = 1.0f;
    m_textureCoordinateArray[6] = 0.0f;
    m_textureCoordinateArray[7] = 1.0f;

    m_blitProgram = new QOpenGLShaderProgram;
    {
        QString source;
        source.append(QLatin1String(qopenglslMainWithTexCoordsVertexShader));
        source.append(QLatin1String(qopenglslUntransformedPositionVertexShader));

        QOpenGLShader *vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, m_blitProgram);
        vertexShader->compileSourceCode(source);

        m_blitProgram->addShader(vertexShader);
    }
    {
        QString source;
        source.append(QLatin1String(qopenglslMainFragmentShader));
        source.append(QLatin1String(qopenglslImageSrcFragmentShader));

        QOpenGLShader *fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, m_blitProgram);
        fragmentShader->compileSourceCode(source);

        m_blitProgram->addShader(fragmentShader);
    }
    m_blitProgram->bindAttributeLocation("vertexCoordsArray", QT_VERTEX_COORDS_ATTR);
    m_blitProgram->bindAttributeLocation("textureCoordArray", QT_TEXTURE_COORDS_ATTR);
    m_blitProgram->link();
}

QSGDistanceFieldGlyphCacheManager::~QSGDistanceFieldGlyphCacheManager()
{
    delete m_blitProgram;
    qDeleteAll(m_caches.values());
}

QSGDistanceFieldGlyphCache *QSGDistanceFieldGlyphCacheManager::cache(const QRawFont &font)
{
    QRawFontPrivate *fontD = QRawFontPrivate::get(font);
    QHash<QFontEngine *, QSGDistanceFieldGlyphCache *>::iterator cache = m_caches.find(fontD->fontEngine);
    if (cache == m_caches.end())
        cache = m_caches.insert(fontD->fontEngine, new QSGDistanceFieldGlyphCache(this, ctx, font));
    return cache.value();
}

int QSGDistanceFieldGlyphCacheManager::maxTextureSize() const
{
    if (!m_maxTextureSize)
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);
    return m_maxTextureSize;
}


QHash<QString, QOpenGLMultiGroupSharedResource> QSGDistanceFieldGlyphCache::m_textures_data;

QSGDistanceFieldGlyphCache::DistanceFieldTextureData *QSGDistanceFieldGlyphCache::textureData()
{
    QString key = QString::fromLatin1("%1_%2_%3_%4")
            .arg(m_font.familyName())
            .arg(m_font.styleName())
            .arg(m_font.weight())
            .arg(m_font.style());
    return m_textures_data[key].value<QSGDistanceFieldGlyphCache::DistanceFieldTextureData>(QOpenGLContext::currentContext());
}

QSGDistanceFieldGlyphCache::QSGDistanceFieldGlyphCache(QSGDistanceFieldGlyphCacheManager *man, QOpenGLContext *c, const QRawFont &font)
    : m_manager(man)
    , ctx(c)
{
    Q_ASSERT(font.isValid());
    m_font = font;

    m_textureData = textureData();

    QRawFontPrivate *fontD = QRawFontPrivate::get(m_font);
    m_glyphCount = fontD->fontEngine->glyphCount();

    m_textureData->doubleGlyphResolution = fontHasNarrowOutlines(font) && m_glyphCount < QT_DISTANCEFIELD_HIGHGLYPHCOUNT;

    m_referenceFont = m_font;
    m_referenceFont.setPixelSize(QT_DISTANCEFIELD_BASEFONTSIZE);
    Q_ASSERT(m_referenceFont.isValid());
}

QSGDistanceFieldGlyphCache::~QSGDistanceFieldGlyphCache()
{
}

GLuint QSGDistanceFieldGlyphCache::texture()
{
    return m_textureData->texture;
}

QSize QSGDistanceFieldGlyphCache::textureSize() const
{
    return m_textureData->size;
}

QSGDistanceFieldGlyphCache::Metrics QSGDistanceFieldGlyphCache::glyphMetrics(glyph_t glyph)
{
    QHash<glyph_t, Metrics>::iterator metric = m_metrics.find(glyph);
    if (metric == m_metrics.end()) {
        QPainterPath path = m_font.pathForGlyph(glyph);
        QRectF br = path.boundingRect();

        Metrics m;
        m.width = br.width();
        m.height = br.height();
        m.baselineX = br.x();
        m.baselineY = -br.y();

        metric = m_metrics.insert(glyph, m);
    }

    return metric.value();
}

QSGDistanceFieldGlyphCache::TexCoord QSGDistanceFieldGlyphCache::glyphTexCoord(glyph_t glyph)
{
    return m_textureData->texCoords.value(glyph);
}

QImage QSGDistanceFieldGlyphCache::renderDistanceFieldGlyph(glyph_t glyph) const
{
    QRawFont renderFont = m_font;
    renderFont.setPixelSize(QT_DISTANCEFIELD_BASEFONTSIZE * QT_DISTANCEFIELD_SCALE);

    QPainterPath path = renderFont.pathForGlyph(glyph);
    path.translate(-path.boundingRect().topLeft());
    path.setFillRule(Qt::WindingFill);

    QImage im = makeDistanceField(QT_DISTANCEFIELD_TILESIZE,
                                  path,
                                  QT_DISTANCEFIELD_SCALE,
                                  QT_DISTANCEFIELD_RADIUS / QT_DISTANCEFIELD_SCALE);
    return im;
}

qreal QSGDistanceFieldGlyphCache::fontScale() const
{
    return qreal(m_font.pixelSize()) / QT_DISTANCEFIELD_BASEFONTSIZE;
}

int QSGDistanceFieldGlyphCache::distanceFieldRadius() const
{
    return QT_DISTANCEFIELD_DEFAULT_RADIUS / QT_DISTANCEFIELD_SCALE;
}

void QSGDistanceFieldGlyphCache::populate(int count, const glyph_t *glyphs)
{
    // Avoid useless and costly glyph re-generation
    if (cacheIsFull() && !m_textureData->unusedGlyphs.isEmpty()) {
        for (int i = 0; i < count; ++i) {
            glyph_t glyphIndex = glyphs[i];
            if (m_textureData->texCoords.contains(glyphIndex) && m_textureData->unusedGlyphs.contains(glyphIndex))
                m_textureData->unusedGlyphs.remove(glyphIndex);
        }
    }

    for (int i = 0; i < count; ++i) {
        glyph_t glyphIndex = glyphs[i];
        if ((int) glyphIndex >= glyphCount()) {
            qWarning("Warning: distance-field glyph is not available with index %d", glyphIndex);
            continue;
        }

        if (++m_textureData->glyphRefCount[glyphIndex] == 1)
            m_textureData->unusedGlyphs.remove(glyphIndex);

        if (m_textureData->texCoords.contains(glyphIndex)
                || (cacheIsFull() && m_textureData->unusedGlyphs.isEmpty()))
            continue;

        QPainterPath path = m_referenceFont.pathForGlyph(glyphIndex);
        if (path.isEmpty()) {
            m_textureData->texCoords.insert(glyphIndex, TexCoord());
            continue;
        }
        QRectF br = path.boundingRect();

        TexCoord c;
        c.xMargin = QT_DISTANCEFIELD_RADIUS / qreal(QT_DISTANCEFIELD_SCALE);
        c.yMargin = QT_DISTANCEFIELD_RADIUS / qreal(QT_DISTANCEFIELD_SCALE);
        c.x = m_textureData->currX;
        c.y = m_textureData->currY;
        c.width = br.width();
        c.height = br.height();

        if (!cacheIsFull()) {
            m_textureData->currX += QT_DISTANCEFIELD_TILESIZE;
            if (m_textureData->currX >= m_manager->maxTextureSize()) {
                m_textureData->currX = 0;
                m_textureData->currY += QT_DISTANCEFIELD_TILESIZE;
            }
        } else {
            // Recycle glyphs
            if (!m_textureData->unusedGlyphs.isEmpty()) {
                glyph_t unusedGlyph = *m_textureData->unusedGlyphs.constBegin();
                TexCoord unusedCoord = glyphTexCoord(unusedGlyph);
                c.x = unusedCoord.x;
                c.y = unusedCoord.y;
                m_textureData->unusedGlyphs.remove(unusedGlyph);
                m_textureData->texCoords.remove(unusedGlyph);
            }
        }

        if (c.y < m_manager->maxTextureSize()) {
            m_textureData->texCoords.insert(glyphIndex, c);
            m_textureData->pendingGlyphs.add(glyphIndex);
        }
    }
}

void QSGDistanceFieldGlyphCache::derefGlyphs(int count, const glyph_t *glyphs)
{
    for (int i = 0; i < count; ++i)
        if (--m_textureData->glyphRefCount[glyphs[i]] == 0 && !glyphTexCoord(glyphs[i]).isNull())
            m_textureData->unusedGlyphs.insert(glyphs[i]);
}

void QSGDistanceFieldGlyphCache::createTexture(int width, int height)
{
    if (ctx->d_func()->workaround_brokenFBOReadBack && m_textureData->image.isNull())
        m_textureData->image = QImage(width, height, QImage::Format_Indexed8);

    while (glGetError() != GL_NO_ERROR) { }

    glGenTextures(1, &m_textureData->texture);
    glBindTexture(GL_TEXTURE_2D, m_textureData->texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_textureData->size = QSize(width, height);

    GLuint error = glGetError();
    if (error != GL_NO_ERROR) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &m_textureData->texture);
        m_textureData->texture = 0;
    }

}

void QSGDistanceFieldGlyphCache::resizeTexture(int width, int height)
{
    int oldWidth = m_textureData->size.width();
    int oldHeight = m_textureData->size.height();
    if (width == oldWidth && height == oldHeight)
        return;

    GLuint oldTexture = m_textureData->texture;
    createTexture(width, height);

    if (!oldTexture)
        return;

    if (ctx->d_func()->workaround_brokenFBOReadBack) {
        m_textureData->image = m_textureData->image.copy(0, 0, width, height);
        QImage copy = m_textureData->image.copy(0, 0, oldWidth, oldHeight);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, oldWidth, oldHeight, GL_ALPHA, GL_UNSIGNED_BYTE, copy.constBits());
        glDeleteTextures(1, &oldTexture);
        return;
    }

    if (!m_textureData->fbo)
        ctx->functions()->glGenFramebuffers(1, &m_textureData->fbo);
    ctx->functions()->glBindFramebuffer(GL_FRAMEBUFFER, m_textureData->fbo);

    GLuint tmp_texture;
    glGenTextures(1, &tmp_texture);
    glBindTexture(GL_TEXTURE_2D, tmp_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, oldWidth, oldHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    ctx->functions()->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_2D, tmp_texture, 0);

    ctx->functions()->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oldTexture);

    // save current render states
    GLboolean stencilTestEnabled;
    GLboolean depthTestEnabled;
    GLboolean scissorTestEnabled;
    GLboolean blendEnabled;
    GLint viewport[4];
    glGetBooleanv(GL_STENCIL_TEST, &stencilTestEnabled);
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glGetBooleanv(GL_SCISSOR_TEST, &scissorTestEnabled);
    glGetBooleanv(GL_BLEND, &blendEnabled);
    glGetIntegerv(GL_VIEWPORT, &viewport[0]);

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, oldWidth, oldHeight);

    ctx->functions()->glVertexAttribPointer(QT_VERTEX_COORDS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, m_manager->blitVertexArray());
    ctx->functions()->glVertexAttribPointer(QT_TEXTURE_COORDS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, m_manager->blitTextureArray());

    m_manager->blitProgram()->bind();
    m_manager->blitProgram()->enableAttributeArray(int(QT_VERTEX_COORDS_ATTR));
    m_manager->blitProgram()->enableAttributeArray(int(QT_TEXTURE_COORDS_ATTR));
    m_manager->blitProgram()->disableAttributeArray(int(QT_OPACITY_ATTR));
    m_manager->blitProgram()->setUniformValue("imageTexture", GLuint(0));

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindTexture(GL_TEXTURE_2D, m_textureData->texture);

    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldWidth, oldHeight);

    ctx->functions()->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                GL_RENDERBUFFER, 0);
    glDeleteTextures(1, &tmp_texture);
    glDeleteTextures(1, &oldTexture);

    ctx->functions()->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // restore render states
    if (stencilTestEnabled)
        glEnable(GL_STENCIL_TEST);
    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    if (scissorTestEnabled)
        glEnable(GL_SCISSOR_TEST);
    if (blendEnabled)
        glEnable(GL_BLEND);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void QSGDistanceFieldGlyphCache::updateCache()
{
    if (m_textureData->pendingGlyphs.isEmpty())
        return;

    int requiredWidth = m_manager->maxTextureSize();
    int rows = 128 / (requiredWidth / QT_DISTANCEFIELD_TILESIZE); // Enough rows to fill the latin1 set by default..
    int requiredHeight = qMin(m_manager->maxTextureSize(), qMax(m_textureData->currY + QT_DISTANCEFIELD_TILESIZE, QT_DISTANCEFIELD_TILESIZE * rows));

    resizeTexture((requiredWidth), (requiredHeight));
    glBindTexture(GL_TEXTURE_2D, m_textureData->texture);

    // ### Remove before final release
    static bool cacheDistanceFields = QGuiApplication::arguments().contains(QLatin1String("--cache-distance-fields"));

// #define QSGDISTANCEFIELDS_TIME_CREATION
#ifdef QSGDISTANCEFIELDS_TIME_CREATION
    QTime time;
    time.start();
#endif

    QString tmpPath = QString::fromLatin1("%1/.qt/").arg(QDir::tempPath());
    QString keyBase = QString::fromLatin1("%1%2%3_%4_%5_%6.fontblob")
            .arg(tmpPath)
            .arg(m_font.familyName())
            .arg(m_font.styleName())
            .arg(m_font.weight())
            .arg(m_font.style());

    if (cacheDistanceFields && !QFile::exists(tmpPath))
        QDir(tmpPath).mkpath(tmpPath);

    for (int i = 0; i < m_textureData->pendingGlyphs.size(); ++i) {
        glyph_t glyphIndex = m_textureData->pendingGlyphs.at(i);
        TexCoord c = m_textureData->texCoords.value(glyphIndex);

        if (cacheDistanceFields) {
            QString key = keyBase.arg(glyphIndex);
            QFile file(key);
            if (file.open(QFile::ReadOnly)) {
                int fileSize = file.size();
                int dim = sqrt(float(fileSize));
                QByteArray blob = file.readAll();
                glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y, dim, dim, GL_ALPHA, GL_UNSIGNED_BYTE, blob.constData());
                continue;
            }
        }

        QImage glyph = renderDistanceFieldGlyph(glyphIndex);

        if (ctx->d_func()->workaround_brokenFBOReadBack) {
            uchar *inBits = glyph.scanLine(0);
            uchar *outBits = m_textureData->image.scanLine(int(c.y)) + int(c.x);
            for (int y = 0; y < glyph.height(); ++y) {
                qMemCopy(outBits, inBits, glyph.width());
                inBits += glyph.bytesPerLine();
                outBits += m_textureData->image.bytesPerLine();
            }
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y, glyph.width(), glyph.height(), GL_ALPHA, GL_UNSIGNED_BYTE, glyph.constBits());

        if (cacheDistanceFields) {
            QString key = keyBase.arg(glyphIndex);
            QFile file(key);
            file.open(QFile::WriteOnly);
            file.write((const char *) glyph.constBits(), glyph.width() * glyph.height());
        }
    }

#ifdef QSGDISTANCEFIELDS_TIME_CREATION
        static int totalTime;
    totalTime += time.elapsed();
    printf("time: %d\n", totalTime);
#endif

    m_textureData->pendingGlyphs.reset();
}

bool QSGDistanceFieldGlyphCache::useWorkaroundBrokenFBOReadback() const
{
    return ctx->d_func()->workaround_brokenFBOReadBack;
}

int QSGDistanceFieldGlyphCache::glyphCount() const
{
    return m_glyphCount;
}

QT_END_NAMESPACE
