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

#ifndef QSGSOFTWAREPIXMAPTEXTURE_H
#define QSGSOFTWAREPIXMAPTEXTURE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qsgtexture_p.h>
#include <QtGui/QPixmap>

QT_BEGIN_NAMESPACE

class QSGSoftwarePixmapTexturePrivate;

class QSGSoftwarePixmapTexture : public QSGTexture
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGSoftwarePixmapTexture)
public:
    QSGSoftwarePixmapTexture(const QImage &image, uint flags);
    QSGSoftwarePixmapTexture(const QPixmap &pixmap);

    int textureId() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;
    void bind() override;

    const QPixmap &pixmap() const { return m_pixmap; }

private:
    QPixmap m_pixmap;
};

class QSGSoftwarePixmapTexturePrivate : public QSGTexturePrivate
{
    Q_DECLARE_PUBLIC(QSGSoftwarePixmapTexture)
public:
    int comparisonKey() const override;
};

QT_END_NAMESPACE

#endif // QSGSOFTWAREPIXMAPTEXTURE_H
