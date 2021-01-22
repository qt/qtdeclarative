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

#ifndef QSGFLATCOLORMATERIAL_H
#define QSGFLATCOLORMATERIAL_H

#include <QtQuick/qsgmaterial.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGFlatColorMaterial : public QSGMaterial
{
public:
    QSGFlatColorMaterial();
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader() const override;

    void setColor(const QColor &color);
    const QColor &color() const { return m_color; }

    int compare(const QSGMaterial *other) const override;

private:
    QColor m_color;
};

QT_END_NAMESPACE

#endif // FLATCOLORMATERIAL_H
