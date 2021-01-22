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

#ifndef QSGMATERIAL_H
#define QSGMATERIAL_H

#include <QtQuick/qtquickglobal.h>
#include <QtQuick/qsgmaterialshader.h>
#include <QtQuick/qsgmaterialrhishader.h>
#include <QtQuick/qsgmaterialtype.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGMaterial
{
public:
    enum Flag {
        Blending            = 0x0001,
        RequiresDeterminant = 0x0002, // Allow precalculated translation and 2D rotation
        RequiresFullMatrixExceptTranslate = 0x0004 | RequiresDeterminant, // Allow precalculated translation
        RequiresFullMatrix  = 0x0008 | RequiresFullMatrixExceptTranslate,

        CustomCompileStep   = 0x0010,

        SupportsRhiShader = 0x0020,

        RhiShaderWanted = 0x1000 // // ### Qt 6: remove
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QSGMaterial();
    virtual ~QSGMaterial();

    virtual QSGMaterialType *type() const = 0;
    virtual QSGMaterialShader *createShader() const = 0;
    virtual int compare(const QSGMaterial *other) const;

    QSGMaterial::Flags flags() const { return m_flags; }
    void setFlag(Flags flags, bool on = true);

private:
    Flags m_flags;
    void *m_reserved;
    Q_DISABLE_COPY(QSGMaterial)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGMaterial::Flags)

QT_END_NAMESPACE

#endif
