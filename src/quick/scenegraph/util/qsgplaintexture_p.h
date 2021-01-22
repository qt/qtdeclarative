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

#ifndef QSGPLAINTEXTURE_P_H
#define QSGPLAINTEXTURE_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

class QSGPlainTexturePrivate;

class Q_QUICK_PRIVATE_EXPORT QSGPlainTexture : public QSGTexture
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGPlainTexture)
public:
    QSGPlainTexture();
    ~QSGPlainTexture() override;

    void setOwnsTexture(bool owns) { m_owns_texture = owns; }
    bool ownsTexture() const { return m_owns_texture; }

    void setTextureId(int id);
    int textureId() const override;
    void setTextureSize(const QSize &size) { m_texture_size = size; }
    QSize textureSize() const override { return m_texture_size; }

    void setHasAlphaChannel(bool alpha) { m_has_alpha = alpha; }
    bool hasAlphaChannel() const override { return m_has_alpha; }

    bool hasMipmaps() const override { return mipmapFiltering() != QSGTexture::None; }

    void setImage(const QImage &image);
    const QImage &image() { return m_image; }

    void bind() override;

    void setTexture(QRhiTexture *texture);
    void setTextureFromNativeObject(QRhi *rhi, QQuickWindow::NativeObjectType type,
                                    const void *nativeObjectPtr, int nativeLayout,
                                    const QSize &size, bool mipmap);

    static QSGPlainTexture *fromImage(const QImage &image) {
        QSGPlainTexture *t = new QSGPlainTexture();
        t->setImage(image);
        return t;
    }

protected:
    QSGPlainTexture(QSGPlainTexturePrivate &dd);

    QImage m_image;

    uint m_texture_id;
    QSize m_texture_size;
    QRectF m_texture_rect;
    QRhiTexture *m_texture;

    uint m_has_alpha : 1;
    uint m_dirty_texture : 1;
    uint m_dirty_bind_options : 1; // legacy (GL-only)
    uint m_owns_texture : 1;
    uint m_mipmaps_generated : 1;
    uint m_retain_image : 1;
    uint m_mipmap_warned : 1; // RHI only
};

class QSGPlainTexturePrivate : public QSGTexturePrivate
{
    Q_DECLARE_PUBLIC(QSGPlainTexture)
public:
    int comparisonKey() const override;
    QRhiTexture *rhiTexture() const override;
    void updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) override;

    QSGTexture::Filtering m_last_mipmap_filter = QSGTexture::None;
};

QT_END_NAMESPACE

#endif // QSGPLAINTEXTURE_P_H
