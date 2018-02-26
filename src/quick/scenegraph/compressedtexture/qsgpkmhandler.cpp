/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgpkmhandler_p.h"
#include "qsgcompressedtexture_p.h"

#include <QFile>
#include <QDebug>
#include <qendian.h>
#include <qopenglfunctions.h>
#include <qqmlfile.h>
#include <QOpenGLTexture>

//#define ETC_DEBUG

QT_BEGIN_NAMESPACE

static const int headerSize = 16;

static unsigned int typeMap[5] = {
    QOpenGLTexture::RGB8_ETC1,                     // GL_ETC1_RGB8_OES,
    QOpenGLTexture::RGB8_ETC2,                     // GL_COMPRESSED_RGB8_ETC2,
    0,                                             // unused (obsolete)
    QOpenGLTexture::RGBA8_ETC2_EAC,                // GL_COMPRESSED_RGBA8_ETC2_EAC,
    QOpenGLTexture::RGB8_PunchThrough_Alpha1_ETC2  // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
};

bool QSGPkmHandler::canRead(const QByteArray &suffix, const QByteArray &block)
{
    Q_UNUSED(suffix)

    return block.startsWith("PKM ");
}

QQuickTextureFactory *QSGPkmHandler::read()
{
    if (!device())
        return nullptr;

    QSGCompressedTexture::DataPtr texData(QSGCompressedTexture::DataPtr::create());

    texData->data = device()->readAll();
    if (texData->data.size() < headerSize || !canRead(QByteArray(), texData->data)) {
        qCDebug(QSG_LOG_TEXTUREIO, "Invalid PKM file %s", logName().constData());
        return nullptr;
    }

    const char *rawData = texData->data.constData();

    // ignore version (rawData + 4 & 5)

    // texture type
    quint16 type = qFromBigEndian<quint16>(rawData + 6);
    if (type > sizeof(typeMap)/sizeof(typeMap[0])) {
        qCDebug(QSG_LOG_TEXTUREIO, "Unknown compression format in PKM file %s", logName().constData());
        return nullptr;
    }
    texData->format = typeMap[type];
    texData->hasAlpha = !QSGCompressedTexture::formatIsOpaque(texData->format);

    // texture size
    const int bpb = (texData->format == QOpenGLTexture::RGBA8_ETC2_EAC) ? 16 : 8;
    QSize paddedSize(qFromBigEndian<quint16>(rawData + 8), qFromBigEndian<quint16>(rawData + 10));
    texData->dataLength = (paddedSize.width() / 4) * (paddedSize.height() / 4) * bpb;
    QSize texSize(qFromBigEndian<quint16>(rawData + 12), qFromBigEndian<quint16>(rawData + 14));
    texData->size = texSize;

    texData->dataOffset = headerSize;

    if (!texData->isValid()) {
        qCDebug(QSG_LOG_TEXTUREIO, "Invalid values in header of PKM file %s", logName().constData());
        return nullptr;
    }

    texData->logName = logName();
#ifdef ETC_DEBUG
    qDebug() << "PKM file handler read" << texData.data();
#endif
    return new QSGCompressedTextureFactory(texData);
}

QT_END_NAMESPACE
