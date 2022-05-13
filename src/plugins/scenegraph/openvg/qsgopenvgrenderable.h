// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGRENDERABLE_H
#define QSGOPENVGRENDERABLE_H

#include <QtGlobal>

#include <VG/openvg.h>

#include "qopenvgmatrix.h"

QT_BEGIN_NAMESPACE

class QSGOpenVGRenderable
{
public:
    QSGOpenVGRenderable();
    virtual ~QSGOpenVGRenderable();

    virtual void render() = 0;

    virtual void setOpacity(float opacity);
    float opacity() const;
    VGPaint opacityPaint() const;

    virtual void setTransform(const QOpenVGMatrix &transform);
    const QOpenVGMatrix &transform() const;

private:
    float m_opacity;
    VGPaint m_opacityPaint;
    QOpenVGMatrix m_transform;

};

QT_END_NAMESPACE

#endif // QSGOPENVGRENDERABLE_H
