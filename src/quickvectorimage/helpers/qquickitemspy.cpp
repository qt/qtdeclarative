// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemspy_p.h"

#include <QtQuick/private/qquickitem_p.h>
#include <QtGui/rhi/qrhi.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ItemSpy
    \inqmlmodule QtQuick.VectorImage.Helpers
    \brief Provides a way to track the world size of an item and bind to it.
    \since 6.11

    This type is used by VectorImage. It enables adjusting a post-processing effect's texture
    size to automatically match the on-screen size of an item subtree.
*/
QQuickItemSpy::QQuickItemSpy(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemObservesViewport);
}

QQuickItemSpy::~QQuickItemSpy()
{
}

/*!
    \qmlproperty size QtQuick.VectorImage.Helpers::ItemSpy::requiredTextureSize

    The texture size needed to render this item's width and height without scaling artifacts.
    This accounts for the current transform as well as high-dpi scaling.

    \note The returned size will never exceed the maximum supported texture size, and it will
    never be smaller than \c{1x1}.
*/
QSizeF QQuickItemSpy::requiredTextureSize() const
{
    QSizeF sz = mapRectToScene(QRectF(0, 0, width(), height())).size();

    const qreal dpr = window() != nullptr ? window()->devicePixelRatio() : 1.0;

    sz.setWidth(std::max(1.0, sz.width() * dpr));
    sz.setHeight(std::max(1.0, sz.height() * dpr));

    if (window() != nullptr && window()->rhi() != nullptr) {
        QRhi *rhi = window()->rhi();
        const int textureSizeMax = rhi->resourceLimit(QRhi::TextureSizeMax);

        if (sz.width() > textureSizeMax || sz.height() > textureSizeMax) {
            if (sz.width() > sz.height())
                sz = QSizeF(textureSizeMax, sz.height() * (textureSizeMax / sz.width()));
            else
                sz = QSizeF(sz.width() * (textureSizeMax / sz.height()), textureSizeMax);
        }
    }

    return sz;
}

void QQuickItemSpy::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == ItemTransformHasChanged || change == ItemDevicePixelRatioHasChanged)
        emit requiredTextureSizeChanged();

    QQuickItem::itemChange(change, value);
}


QT_END_NAMESPACE
