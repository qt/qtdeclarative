/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgdistancefieldutil_p.h"

#include <private/qsgadaptationlayer_p.h>
#include <QtGui/private/qopenglengineshadersource_p.h>
#include <QtQuick/private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

static float qt_sg_envFloat(const char *name, float defaultValue)
{
    if (Q_LIKELY(!qEnvironmentVariableIsSet(name)))
        return defaultValue;
    bool ok = false;
    const float value = qgetenv(name).toFloat(&ok);
    return ok ? value : defaultValue;
}

static float defaultThresholdFunc(float glyphScale)
{
    static const float base = qt_sg_envFloat("QT_DF_BASE", 0.5f);
    static const float baseDev = qt_sg_envFloat("QT_DF_BASEDEVIATION", 0.065f);
    static const float devScaleMin = qt_sg_envFloat("QT_DF_SCALEFORMAXDEV", 0.15f);
    static const float devScaleMax = qt_sg_envFloat("QT_DF_SCALEFORNODEV", 0.3f);
    return base - ((qBound(devScaleMin, glyphScale, devScaleMax) - devScaleMin) / (devScaleMax - devScaleMin) * -baseDev + baseDev);
}

static float defaultAntialiasingSpreadFunc(float glyphScale)
{
    static const float range = qt_sg_envFloat("QT_DF_RANGE", 0.06f);
    return range / glyphScale;
}

QSGDistanceFieldGlyphCacheManager::QSGDistanceFieldGlyphCacheManager()
    : m_threshold_func(defaultThresholdFunc)
    , m_antialiasingSpread_func(defaultAntialiasingSpreadFunc)
{
}

QSGDistanceFieldGlyphCacheManager::~QSGDistanceFieldGlyphCacheManager()
{
    qDeleteAll(m_caches);
}

QSGDistanceFieldGlyphCache *QSGDistanceFieldGlyphCacheManager::cache(const QRawFont &font)
{
    return m_caches.value(fontKey(font), 0);
}

void QSGDistanceFieldGlyphCacheManager::insertCache(const QRawFont &font, QSGDistanceFieldGlyphCache *cache)
{
    m_caches.insert(fontKey(font), cache);
}

QString QSGDistanceFieldGlyphCacheManager::fontKey(const QRawFont &font)
{
    QFontEngine *fe = QRawFontPrivate::get(font)->fontEngine;
    if (!fe->faceId().filename.isEmpty()) {
        QByteArray keyName = fe->faceId().filename;
        if (font.style() != QFont::StyleNormal)
            keyName += QByteArray(" I");
        if (font.weight() != QFont::Normal)
            keyName += ' ' + QByteArray::number(font.weight());
        keyName += QByteArray(" DF");
        return QString::fromUtf8(keyName);
    } else {
        return QString::fromLatin1("%1_%2_%3_%4")
                  .arg(font.familyName())
                  .arg(font.styleName())
                  .arg(font.weight())
                  .arg(font.style());
    }
}

QT_END_NAMESPACE
