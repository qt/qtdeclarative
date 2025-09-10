// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrhisupport_p.h"

#include <Metal/MTLPixelFormat.h>

QT_BEGIN_NAMESPACE

namespace QSGRhiSupportMac {

QRhiTexture::Format toRhiTextureFormatFromMetal(uint format, QRhiTexture::Flags *flags)
{
    auto rhiFormat = QRhiTexture::UnknownFormat;
    bool sRGB = false;
    switch (format) {
    case MTLPixelFormatRGBA8Unorm_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatRGBA8Unorm:
    case MTLPixelFormatInvalid:
        rhiFormat = QRhiTexture::RGBA8;
        break;
    case MTLPixelFormatBGRA8Unorm_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatBGRA8Unorm:
        rhiFormat = QRhiTexture::BGRA8;
        break;
#ifndef Q_OS_MACOS
    case MTLPixelFormatR8Unorm_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
#endif
    case MTLPixelFormatR8Unorm:
        rhiFormat = QRhiTexture::R8;
        break;
#ifndef Q_OS_MACOS
    case MTLPixelFormatRG8Unorm_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
#endif
    case MTLPixelFormatRG8Unorm:
        rhiFormat = QRhiTexture::RG8;
        break;
    case MTLPixelFormatR16Unorm:
        rhiFormat = QRhiTexture::R16;
        break;
    case MTLPixelFormatRG16Unorm:
        rhiFormat = QRhiTexture::RG16;
        break;
    case MTLPixelFormatRGBA16Float:
        rhiFormat = QRhiTexture::RGBA16F;
        break;
    case MTLPixelFormatRGBA32Float:
        rhiFormat = QRhiTexture::RGBA32F;
        break;
    case MTLPixelFormatR16Float:
        rhiFormat = QRhiTexture::R16F;
        break;
    case MTLPixelFormatR32Float:
        rhiFormat = QRhiTexture::R32F;
        break;
    case MTLPixelFormatRGB10A2Unorm:
        rhiFormat = QRhiTexture::RGB10A2;
        break;
#ifdef Q_OS_MACOS
    case MTLPixelFormatDepth16Unorm:
        rhiFormat = QRhiTexture::D16;
        break;
    case MTLPixelFormatDepth24Unorm_Stencil8:
        rhiFormat = QRhiTexture::D24S8;
        break;
#endif
    case MTLPixelFormatDepth32Float_Stencil8:
        rhiFormat = QRhiTexture::D24S8;
        break;
    case MTLPixelFormatDepth32Float:
        rhiFormat = QRhiTexture::D32F;
        break;
#ifdef Q_OS_MACOS
    case MTLPixelFormatBC1_RGBA_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatBC1_RGBA:
        rhiFormat = QRhiTexture::BC1;
        break;
    case MTLPixelFormatBC2_RGBA_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatBC2_RGBA:
        rhiFormat = QRhiTexture::BC2;
        break;
    case MTLPixelFormatBC3_RGBA_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatBC3_RGBA:
        rhiFormat = QRhiTexture::BC3;
        break;
    case MTLPixelFormatBC4_RUnorm:
        rhiFormat = QRhiTexture::BC4;
        break;
    case MTLPixelFormatBC6H_RGBUfloat:
        rhiFormat = QRhiTexture::BC6H;
        break;
    case MTLPixelFormatBC7_RGBAUnorm_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatBC7_RGBAUnorm:
        rhiFormat = QRhiTexture::BC7;
        break;
#endif
#ifndef Q_OS_MACOS
    case MTLPixelFormatETC2_RGB8_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatETC2_RGB8:
        rhiFormat = QRhiTexture::ETC2_RGB8;
        break;
    case MTLPixelFormatETC2_RGB8A1_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatETC2_RGB8A1:
        rhiFormat = QRhiTexture::ETC2_RGB8A1;
        break;
    case MTLPixelFormatEAC_RGBA8_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatEAC_RGBA8:
        rhiFormat = QRhiTexture::ETC2_RGBA8;
        break;
    case MTLPixelFormatASTC_4x4_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_4x4_LDR:
        rhiFormat = QRhiTexture::ASTC_4x4;
        break;
    case MTLPixelFormatASTC_5x4_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_5x4_LDR:
        rhiFormat = QRhiTexture::ASTC_5x4;
        break;
    case MTLPixelFormatASTC_5x5_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_5x5_LDR:
        rhiFormat = QRhiTexture::ASTC_5x5;
        break;
    case MTLPixelFormatASTC_6x5_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_6x5_LDR:
        rhiFormat = QRhiTexture::ASTC_6x5;
        break;
    case MTLPixelFormatASTC_6x6_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_6x6_LDR:
        rhiFormat = QRhiTexture::ASTC_6x6;
        break;
    case MTLPixelFormatASTC_8x5_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_8x5_LDR:
        rhiFormat = QRhiTexture::ASTC_8x5;
        break;
    case MTLPixelFormatASTC_8x6_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_8x6_LDR:
        rhiFormat = QRhiTexture::ASTC_8x6;
        break;
    case MTLPixelFormatASTC_8x8_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_8x8_LDR:
        rhiFormat = QRhiTexture::ASTC_8x8;
        break;
    case MTLPixelFormatASTC_10x5_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_10x5_LDR:
        rhiFormat = QRhiTexture::ASTC_10x5;
        break;
    case MTLPixelFormatASTC_10x6_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_10x6_LDR:
        rhiFormat = QRhiTexture::ASTC_10x6;
        break;
    case MTLPixelFormatASTC_10x8_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_10x8_LDR:
        rhiFormat = QRhiTexture::ASTC_10x8;
        break;
    case MTLPixelFormatASTC_10x10_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_10x10_LDR:
        rhiFormat = QRhiTexture::ASTC_10x10;
        break;
    case MTLPixelFormatASTC_12x10_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_12x10_LDR:
        rhiFormat = QRhiTexture::ASTC_12x10;
        break;
    case MTLPixelFormatASTC_12x12_sRGB:
        sRGB = true;
        Q_FALLTHROUGH();
    case MTLPixelFormatASTC_12x12_LDR:
        rhiFormat = QRhiTexture::ASTC_12x12;
        break;
#endif
    default:
        qWarning("MTLPixelFormat %d is not supported", format);
        break;
    }
    if (sRGB)
        (*flags) |=(QRhiTexture::sRGB);
    return rhiFormat;
}

}

QT_END_NAMESPACE
