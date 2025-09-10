// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgopenvgtexture.h"
#include "qsgopenvghelpers.h"

QT_BEGIN_NAMESPACE

QSGOpenVGTexture::QSGOpenVGTexture(const QImage &image, uint flags)
{
    Q_UNUSED(flags);

    VGImageFormat format = QSGOpenVGHelpers::qImageFormatToVGImageFormat(image.format());
    m_image = vgCreateImage(format, image.width(), image.height(), VG_IMAGE_QUALITY_BETTER);

    // Do Texture Upload
    vgImageSubData(m_image, image.constBits(), image.bytesPerLine(), format, 0, 0, image.width(), image.height());
}

QSGOpenVGTexture::~QSGOpenVGTexture()
{
    vgDestroyImage(m_image);
}

qint64 QSGOpenVGTexture::comparisonKey() const
{
    return qint64(static_cast<int>(m_image));
}

QSize QSGOpenVGTexture::textureSize() const
{
    VGint imageWidth = vgGetParameteri(m_image, VG_IMAGE_WIDTH);
    VGint imageHeight = vgGetParameteri(m_image, VG_IMAGE_HEIGHT);
    return QSize(imageWidth, imageHeight);
}

bool QSGOpenVGTexture::hasAlphaChannel() const
{
    VGImageFormat format = static_cast<VGImageFormat>(vgGetParameteri(m_image, VG_IMAGE_FORMAT));

    switch (format) {
    case VG_sRGBA_8888:
    case VG_sRGBA_8888_PRE:
    case VG_sRGBA_5551:
    case VG_sRGBA_4444:
    case VG_lRGBA_8888:
    case VG_lRGBA_8888_PRE:
    case VG_A_8:
    case VG_A_1:
    case VG_A_4:
    case VG_sARGB_8888:
    case VG_sARGB_8888_PRE:
    case VG_sARGB_1555:
    case VG_sARGB_4444:
    case VG_lARGB_8888:
    case VG_lARGB_8888_PRE:
    case VG_sBGRA_8888:
    case VG_sBGRA_8888_PRE:
    case VG_sBGRA_5551:
    case VG_sBGRA_4444:
    case VG_lBGRA_8888:
    case VG_lBGRA_8888_PRE:
    case VG_sABGR_8888:
    case VG_sABGR_8888_PRE:
    case VG_sABGR_1555:
    case VG_sABGR_4444:
    case VG_lABGR_8888:
    case VG_lABGR_8888_PRE:
        return true;
        break;
    default:
        break;
    }
    return false;
}

bool QSGOpenVGTexture::hasMipmaps() const
{
    return false;
}

QT_END_NAMESPACE
