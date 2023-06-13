// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qsgcompressedtexture_p.h"
#include <QDebug>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QSG_LOG_TEXTUREIO, "qt.scenegraph.textureio");

QSGCompressedTexture::QSGCompressedTexture(const QTextureFileData &texData)
    : QSGTexture(*(new QSGTexturePrivate(this))),
      m_textureData(texData)
{
    m_size = m_textureData.size();
    m_hasAlpha = !formatIsOpaque(m_textureData.glInternalFormat());
}

QSGCompressedTexture::~QSGCompressedTexture()
{
    delete m_texture;
}

qint64 QSGCompressedTexture::comparisonKey() const
{
    if (m_texture)
        return qint64(m_texture);

    // two textures (and so materials) with not-yet-created texture underneath are never equal
    return qint64(this);
}

QSize QSGCompressedTexture::textureSize() const
{
    return m_size;
}

bool QSGCompressedTexture::hasAlphaChannel() const
{
    return m_hasAlpha;
}

bool QSGCompressedTexture::hasMipmaps() const
{
    return false;
}

namespace QInternalGLTextureFormat {
    // Copied from QOpenGLTexture.
    // TODO: make a general solution that can be shared with QtQuick3D (QTBUG-82431)
    enum {
        NoFormat               = 0,         // GL_NONE

        // Unsigned normalized formats
        R8_UNorm               = 0x8229,    // GL_R8
        RG8_UNorm              = 0x822B,    // GL_RG8
        RGB8_UNorm             = 0x8051,    // GL_RGB8
        RGBA8_UNorm            = 0x8058,    // GL_RGBA8

        R16_UNorm              = 0x822A,    // GL_R16
        RG16_UNorm             = 0x822C,    // GL_RG16
        RGB16_UNorm            = 0x8054,    // GL_RGB16
        RGBA16_UNorm           = 0x805B,    // GL_RGBA16

        // Signed normalized formats
        R8_SNorm               = 0x8F94,    // GL_R8_SNORM
        RG8_SNorm              = 0x8F95,    // GL_RG8_SNORM
        RGB8_SNorm             = 0x8F96,    // GL_RGB8_SNORM
        RGBA8_SNorm            = 0x8F97,    // GL_RGBA8_SNORM

        R16_SNorm              = 0x8F98,    // GL_R16_SNORM
        RG16_SNorm             = 0x8F99,    // GL_RG16_SNORM
        RGB16_SNorm            = 0x8F9A,    // GL_RGB16_SNORM
        RGBA16_SNorm           = 0x8F9B,    // GL_RGBA16_SNORM

        // Unsigned integer formats
        R8U                    = 0x8232,    // GL_R8UI
        RG8U                   = 0x8238,    // GL_RG8UI
        RGB8U                  = 0x8D7D,    // GL_RGB8UI
        RGBA8U                 = 0x8D7C,    // GL_RGBA8UI

        R16U                   = 0x8234,    // GL_R16UI
        RG16U                  = 0x823A,    // GL_RG16UI
        RGB16U                 = 0x8D77,    // GL_RGB16UI
        RGBA16U                = 0x8D76,    // GL_RGBA16UI

        R32U                   = 0x8236,    // GL_R32UI
        RG32U                  = 0x823C,    // GL_RG32UI
        RGB32U                 = 0x8D71,    // GL_RGB32UI
        RGBA32U                = 0x8D70,    // GL_RGBA32UI

        // Signed integer formats
        R8I                    = 0x8231,    // GL_R8I
        RG8I                   = 0x8237,    // GL_RG8I
        RGB8I                  = 0x8D8F,    // GL_RGB8I
        RGBA8I                 = 0x8D8E,    // GL_RGBA8I

        R16I                   = 0x8233,    // GL_R16I
        RG16I                  = 0x8239,    // GL_RG16I
        RGB16I                 = 0x8D89,    // GL_RGB16I
        RGBA16I                = 0x8D88,    // GL_RGBA16I

        R32I                   = 0x8235,    // GL_R32I
        RG32I                  = 0x823B,    // GL_RG32I
        RGB32I                 = 0x8D83,    // GL_RGB32I
        RGBA32I                = 0x8D82,    // GL_RGBA32I

        // Floating point formats
        R16F                   = 0x822D,    // GL_R16F
        RG16F                  = 0x822F,    // GL_RG16F
        RGB16F                 = 0x881B,    // GL_RGB16F
        RGBA16F                = 0x881A,    // GL_RGBA16F

        R32F                   = 0x822E,    // GL_R32F
        RG32F                  = 0x8230,    // GL_RG32F
        RGB32F                 = 0x8815,    // GL_RGB32F
        RGBA32F                = 0x8814,    // GL_RGBA32F

        // Packed formats
        RGB9E5                 = 0x8C3D,    // GL_RGB9_E5
        RG11B10F               = 0x8C3A,    // GL_R11F_G11F_B10F
        RG3B2                  = 0x2A10,    // GL_R3_G3_B2
        R5G6B5                 = 0x8D62,    // GL_RGB565
        RGB5A1                 = 0x8057,    // GL_RGB5_A1
        RGBA4                  = 0x8056,    // GL_RGBA4
        RGB10A2                = 0x906F,    // GL_RGB10_A2UI

        // Depth formats
        D16                    = 0x81A5,    // GL_DEPTH_COMPONENT16
        D24                    = 0x81A6,    // GL_DEPTH_COMPONENT24
        D24S8                  = 0x88F0,    // GL_DEPTH24_STENCIL8
        D32                    = 0x81A7,    // GL_DEPTH_COMPONENT32
        D32F                   = 0x8CAC,    // GL_DEPTH_COMPONENT32F
        D32FS8X24              = 0x8CAD,    // GL_DEPTH32F_STENCIL8
        S8                     = 0x8D48,    // GL_STENCIL_INDEX8

        // Compressed formats
        RGB_DXT1               = 0x83F0,    // GL_COMPRESSED_RGB_S3TC_DXT1_EXT
        RGBA_DXT1              = 0x83F1,    // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
        RGBA_DXT3              = 0x83F2,    // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        RGBA_DXT5              = 0x83F3,    // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
        R_ATI1N_UNorm          = 0x8DBB,    // GL_COMPRESSED_RED_RGTC1
        R_ATI1N_SNorm          = 0x8DBC,    // GL_COMPRESSED_SIGNED_RED_RGTC1
        RG_ATI2N_UNorm         = 0x8DBD,    // GL_COMPRESSED_RG_RGTC2
        RG_ATI2N_SNorm         = 0x8DBE,    // GL_COMPRESSED_SIGNED_RG_RGTC2
        RGB_BP_UNSIGNED_FLOAT  = 0x8E8F,    // GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB
        RGB_BP_SIGNED_FLOAT    = 0x8E8E,    // GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB
        RGB_BP_UNorm           = 0x8E8C,    // GL_COMPRESSED_RGBA_BPTC_UNORM_ARB
        R11_EAC_UNorm          = 0x9270,    // GL_COMPRESSED_R11_EAC
        R11_EAC_SNorm          = 0x9271,    // GL_COMPRESSED_SIGNED_R11_EAC
        RG11_EAC_UNorm         = 0x9272,    // GL_COMPRESSED_RG11_EAC
        RG11_EAC_SNorm         = 0x9273,    // GL_COMPRESSED_SIGNED_RG11_EAC
        RGB8_ETC2              = 0x9274,    // GL_COMPRESSED_RGB8_ETC2
        SRGB8_ETC2             = 0x9275,    // GL_COMPRESSED_SRGB8_ETC2
        RGB8_PunchThrough_Alpha1_ETC2 = 0x9276, // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
        SRGB8_PunchThrough_Alpha1_ETC2 = 0x9277, // GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
        RGBA8_ETC2_EAC         = 0x9278,    // GL_COMPRESSED_RGBA8_ETC2_EAC
        SRGB8_Alpha8_ETC2_EAC  = 0x9279,    // GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
        RGB8_ETC1              = 0x8D64,    // GL_ETC1_RGB8_OES
        RGBA_ASTC_4x4          = 0x93B0,    // GL_COMPRESSED_RGBA_ASTC_4x4_KHR
        RGBA_ASTC_5x4          = 0x93B1,    // GL_COMPRESSED_RGBA_ASTC_5x4_KHR
        RGBA_ASTC_5x5          = 0x93B2,    // GL_COMPRESSED_RGBA_ASTC_5x5_KHR
        RGBA_ASTC_6x5          = 0x93B3,    // GL_COMPRESSED_RGBA_ASTC_6x5_KHR
        RGBA_ASTC_6x6          = 0x93B4,    // GL_COMPRESSED_RGBA_ASTC_6x6_KHR
        RGBA_ASTC_8x5          = 0x93B5,    // GL_COMPRESSED_RGBA_ASTC_8x5_KHR
        RGBA_ASTC_8x6          = 0x93B6,    // GL_COMPRESSED_RGBA_ASTC_8x6_KHR
        RGBA_ASTC_8x8          = 0x93B7,    // GL_COMPRESSED_RGBA_ASTC_8x8_KHR
        RGBA_ASTC_10x5         = 0x93B8,    // GL_COMPRESSED_RGBA_ASTC_10x5_KHR
        RGBA_ASTC_10x6         = 0x93B9,    // GL_COMPRESSED_RGBA_ASTC_10x6_KHR
        RGBA_ASTC_10x8         = 0x93BA,    // GL_COMPRESSED_RGBA_ASTC_10x8_KHR
        RGBA_ASTC_10x10        = 0x93BB,    // GL_COMPRESSED_RGBA_ASTC_10x10_KHR
        RGBA_ASTC_12x10        = 0x93BC,    // GL_COMPRESSED_RGBA_ASTC_12x10_KHR
        RGBA_ASTC_12x12        = 0x93BD,    // GL_COMPRESSED_RGBA_ASTC_12x12_KHR
        SRGB8_Alpha8_ASTC_4x4  = 0x93D0,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
        SRGB8_Alpha8_ASTC_5x4  = 0x93D1,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR
        SRGB8_Alpha8_ASTC_5x5  = 0x93D2,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR
        SRGB8_Alpha8_ASTC_6x5  = 0x93D3,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR
        SRGB8_Alpha8_ASTC_6x6  = 0x93D4,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR
        SRGB8_Alpha8_ASTC_8x5  = 0x93D5,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR
        SRGB8_Alpha8_ASTC_8x6  = 0x93D6,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR
        SRGB8_Alpha8_ASTC_8x8  = 0x93D7,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR
        SRGB8_Alpha8_ASTC_10x5 = 0x93D8,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR
        SRGB8_Alpha8_ASTC_10x6 = 0x93D9,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR
        SRGB8_Alpha8_ASTC_10x8 = 0x93DA,    // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR
        SRGB8_Alpha8_ASTC_10x10 = 0x93DB,   // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR
        SRGB8_Alpha8_ASTC_12x10 = 0x93DC,   // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR
        SRGB8_Alpha8_ASTC_12x12 = 0x93DD,   // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR

        // sRGB formats
        SRGB8                  = 0x8C41,    // GL_SRGB8
        SRGB8_Alpha8           = 0x8C43,    // GL_SRGB8_ALPHA8
        SRGB_DXT1              = 0x8C4C,    // GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
        SRGB_Alpha_DXT1        = 0x8C4D,    // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
        SRGB_Alpha_DXT3        = 0x8C4E,    // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
        SRGB_Alpha_DXT5        = 0x8C4F,    // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
        SRGB_BP_UNorm          = 0x8E8D,    // GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB

        // ES 2 formats
        DepthFormat            = 0x1902,    // GL_DEPTH_COMPONENT
        AlphaFormat            = 0x1906,    // GL_ALPHA
        RGBFormat              = 0x1907,    // GL_RGB
        RGBAFormat             = 0x1908,    // GL_RGBA
        LuminanceFormat        = 0x1909,    // GL_LUMINANCE
        LuminanceAlphaFormat   = 0x190A

    };
}

QSGCompressedTexture::FormatInfo QSGCompressedTexture::formatInfo(quint32 glTextureFormat)
{
    switch (glTextureFormat) {
    case QInternalGLTextureFormat::RGB_DXT1:
        return { QRhiTexture::BC1, false };
    case QInternalGLTextureFormat::SRGB_DXT1:
        return { QRhiTexture::BC1, true };

    case QInternalGLTextureFormat::RGBA_DXT3:
        return { QRhiTexture::BC2, false };
    case QInternalGLTextureFormat::SRGB_Alpha_DXT3:
        return { QRhiTexture::BC2, true };

    case QInternalGLTextureFormat::RGBA_DXT5:
        return { QRhiTexture::BC3, false };
    case QInternalGLTextureFormat::SRGB_Alpha_DXT5:
        return { QRhiTexture::BC3, true };

    case QInternalGLTextureFormat::RGB8_ETC2:
        return { QRhiTexture::ETC2_RGB8, false };
    case QInternalGLTextureFormat::SRGB8_ETC2:
        return { QRhiTexture::ETC2_RGB8, true };

    case QInternalGLTextureFormat::RGB8_PunchThrough_Alpha1_ETC2:
        return { QRhiTexture::ETC2_RGB8A1, false };
    case QInternalGLTextureFormat::SRGB8_PunchThrough_Alpha1_ETC2:
        return { QRhiTexture::ETC2_RGB8A1, true };

    case QInternalGLTextureFormat::RGBA8_ETC2_EAC:
        return { QRhiTexture::ETC2_RGBA8, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ETC2_EAC:
        return { QRhiTexture::ETC2_RGBA8, true };

    case QInternalGLTextureFormat::RGBA_ASTC_4x4:
        return { QRhiTexture::ASTC_4x4, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_4x4:
        return { QRhiTexture::ASTC_4x4, true };

    case QInternalGLTextureFormat::RGBA_ASTC_5x4:
        return { QRhiTexture::ASTC_5x4, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_5x4:
        return { QRhiTexture::ASTC_5x4, true };

    case QInternalGLTextureFormat::RGBA_ASTC_5x5:
        return { QRhiTexture::ASTC_5x5, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_5x5:
        return { QRhiTexture::ASTC_5x5, true };

    case QInternalGLTextureFormat::RGBA_ASTC_6x5:
        return { QRhiTexture::ASTC_6x5, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_6x5:
        return { QRhiTexture::ASTC_6x5, true };

    case QInternalGLTextureFormat::RGBA_ASTC_6x6:
        return { QRhiTexture::ASTC_6x6, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_6x6:
        return { QRhiTexture::ASTC_6x6, true };

    case QInternalGLTextureFormat::RGBA_ASTC_8x5:
        return { QRhiTexture::ASTC_8x5, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_8x5:
        return { QRhiTexture::ASTC_8x5, true };

    case QInternalGLTextureFormat::RGBA_ASTC_8x6:
        return { QRhiTexture::ASTC_8x6, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_8x6:
        return { QRhiTexture::ASTC_8x6, true };

    case QInternalGLTextureFormat::RGBA_ASTC_8x8:
        return { QRhiTexture::ASTC_8x8, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_8x8:
        return { QRhiTexture::ASTC_8x8, true };

    case QInternalGLTextureFormat::RGBA_ASTC_10x5:
        return { QRhiTexture::ASTC_10x5, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_10x5:
        return { QRhiTexture::ASTC_10x5, true };

    case QInternalGLTextureFormat::RGBA_ASTC_10x6:
        return { QRhiTexture::ASTC_10x6, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_10x6:
        return { QRhiTexture::ASTC_10x6, true };

    case QInternalGLTextureFormat::RGBA_ASTC_10x8:
        return { QRhiTexture::ASTC_10x8, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_10x8:
        return { QRhiTexture::ASTC_10x8, true };

    case QInternalGLTextureFormat::RGBA_ASTC_10x10:
        return { QRhiTexture::ASTC_10x10, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_10x10:
        return { QRhiTexture::ASTC_10x10, true };

    case QInternalGLTextureFormat::RGBA_ASTC_12x10:
        return { QRhiTexture::ASTC_12x10, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_12x10:
        return { QRhiTexture::ASTC_12x10, true };

    case QInternalGLTextureFormat::RGBA_ASTC_12x12:
        return { QRhiTexture::ASTC_12x12, false };
    case QInternalGLTextureFormat::SRGB8_Alpha8_ASTC_12x12:
        return { QRhiTexture::ASTC_12x12, true };

    default:
        return { QRhiTexture::UnknownFormat, false };
    }
}

QRhiTexture *QSGCompressedTexture::rhiTexture() const
{
    return m_texture;
}

void QSGCompressedTexture::commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    if (m_uploaded)
        return;

    m_uploaded = true; // even if fails, no point in trying again

    if (!m_textureData.isValid()) {
        qCDebug(QSG_LOG_TEXTUREIO, "Invalid texture data for %s", m_textureData.logName().constData());
        return;
    }

    FormatInfo fmt = formatInfo(m_textureData.glInternalFormat());
    if (fmt.rhiFormat == QRhiTexture::UnknownFormat) {
        qWarning("Unknown compressed format 0x%x", m_textureData.glInternalFormat());
        return;
    }

    if (!m_texture) {
        QRhiTexture::Flags texFlags;
        if (fmt.isSRGB)
            texFlags |= QRhiTexture::sRGB;

        if (!rhi->isTextureFormatSupported(fmt.rhiFormat, texFlags)) {
            qCDebug(QSG_LOG_TEXTUREIO, "Compressed texture format possibly unsupported: 0x%x",
                    m_textureData.glInternalFormat());
            // For the Metal backend, don't even try to create an unsupported texture
            // since trying to do so is invalid.
            if (rhi->backend() == QRhi::Metal) {
                qWarning("Unsupported compressed texture format 0x%x", m_textureData.glInternalFormat());
                return;
            }
        }

        m_texture = rhi->newTexture(fmt.rhiFormat, m_size, 1, texFlags);
        if (!m_texture->create()) {
            qWarning("Failed to create QRhiTexture for compressed data with format 0x%x",
                     m_textureData.glInternalFormat());
            delete m_texture;
            m_texture = nullptr;
            return;
        }
    }

    // only upload mip level 0 since we never do mipmapping for compressed textures (for now?)
    resourceUpdates->uploadTexture(
            m_texture,
            QRhiTextureUploadEntry(0, 0,
                                   QRhiTextureSubresourceUploadDescription(
                                           m_textureData.getDataView().toByteArray())));

    m_textureData = QTextureFileData(); // Release this memory, not needed anymore
}

QTextureFileData QSGCompressedTexture::textureData() const
{
    return m_textureData;
}

bool QSGCompressedTexture::formatIsOpaque(quint32 glTextureFormat)
{
    switch (glTextureFormat) {
    case QInternalGLTextureFormat::RGB_DXT1:
    case QInternalGLTextureFormat::R_ATI1N_UNorm:
    case QInternalGLTextureFormat::R_ATI1N_SNorm:
    case QInternalGLTextureFormat::RG_ATI2N_UNorm:
    case QInternalGLTextureFormat::RG_ATI2N_SNorm:
    case QInternalGLTextureFormat::RGB_BP_UNSIGNED_FLOAT:
    case QInternalGLTextureFormat::RGB_BP_SIGNED_FLOAT:
    case QInternalGLTextureFormat::R11_EAC_UNorm:
    case QInternalGLTextureFormat::R11_EAC_SNorm:
    case QInternalGLTextureFormat::RG11_EAC_UNorm:
    case QInternalGLTextureFormat::RG11_EAC_SNorm:
    case QInternalGLTextureFormat::RGB8_ETC2:
    case QInternalGLTextureFormat::SRGB8_ETC2:
    case QInternalGLTextureFormat::RGB8_ETC1:
    case QInternalGLTextureFormat::SRGB_DXT1:
        return true;
        break;
    default:
        return false;
    }
}

QSGCompressedTextureFactory::QSGCompressedTextureFactory(const QTextureFileData &texData)
    : m_textureData(texData)
{
}

QSGTexture *QSGCompressedTextureFactory::createTexture(QQuickWindow *window) const
{
    if (!m_textureData.isValid())
        return nullptr;

    // attempt to atlas the texture
    QSGRenderContext *context = QQuickWindowPrivate::get(window)->context;
    QSGTexture *t = context->compressedTextureForFactory(this);
    if (t)
        return t;

    return new QSGCompressedTexture(m_textureData);
}

int QSGCompressedTextureFactory::textureByteCount() const
{
    return m_textureData.getDataView().size();
}

QSize QSGCompressedTextureFactory::textureSize() const
{
    return m_textureData.size();
}

QT_END_NAMESPACE

#include "moc_qsgcompressedtexture_p.cpp"
