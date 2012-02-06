/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgdistancefieldutil_p.h"

#include <private/qsgadaptationlayer_p.h>
#include <QtGui/private/qopenglengineshadersource_p.h>
#include <QtQuick/private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

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

QSGDistanceFieldGlyphCacheManager::QSGDistanceFieldGlyphCacheManager(QSGContext *c)
    : sgCtx(c)
    , m_threshold_func(defaultThresholdFunc)
    , m_antialiasingSpread_func(defaultAntialiasingSpreadFunc)
{
#ifndef QT_OPENGL_ES
    m_defaultAntialiasingMode = QSGGlyphNode::HighQualitySubPixelAntialiasing;
#else
    m_defaultAntialiasingMode = QSGGlyphNode::GrayAntialiasing;
#endif
}

QSGDistanceFieldGlyphCacheManager::~QSGDistanceFieldGlyphCacheManager()
{
    qDeleteAll(m_caches.values());
}

QSGDistanceFieldGlyphCache *QSGDistanceFieldGlyphCacheManager::cache(const QRawFont &font)
{
    QRawFontPrivate *fontD = QRawFontPrivate::get(font);
    QHash<QFontEngine *, QSGDistanceFieldGlyphCache *>::iterator cache = m_caches.find(fontD->fontEngine);
    if (cache == m_caches.end())
        cache = m_caches.insert(fontD->fontEngine, sgCtx->createDistanceFieldGlyphCache(font));
    return cache.value();
}

QT_END_NAMESPACE
