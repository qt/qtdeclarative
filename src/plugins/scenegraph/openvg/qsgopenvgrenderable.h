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
