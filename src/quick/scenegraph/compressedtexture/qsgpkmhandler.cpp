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

#include <QFile>
#include <QDebug>
#include <qendian.h>
#include <qopenglfunctions.h>
#include <qqmlfile.h>

//#define ETC_DEBUG

#ifndef GL_ETC1_RGB8_OES
    #define GL_ETC1_RGB8_OES 0x8d64
#endif

#ifndef GL_COMPRESSED_RGB8_ETC2
    #define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif

#ifndef GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
    #define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#endif

#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
    #define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif

QT_BEGIN_NAMESPACE

static const int headerSize = 16;

static unsigned int typeMap[5] = {
    GL_ETC1_RGB8_OES,
    GL_COMPRESSED_RGB8_ETC2,
    0, // unused
    GL_COMPRESSED_RGBA8_ETC2_EAC,
    GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
};

QEtcTexture::QEtcTexture()
    : m_texture_id(0), m_uploaded(false)
{
    initializeOpenGLFunctions();
}

QEtcTexture::~QEtcTexture()
{
    if (m_texture_id)
        glDeleteTextures(1, &m_texture_id);
}

int QEtcTexture::textureId() const
{
    if (m_texture_id == 0) {
        QEtcTexture *texture = const_cast<QEtcTexture*>(this);
        texture->glGenTextures(1, &texture->m_texture_id);
    }
    return m_texture_id;
}

bool QEtcTexture::hasAlphaChannel() const
{
    return m_type == GL_COMPRESSED_RGBA8_ETC2_EAC ||
           m_type == GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
}


void QEtcTexture::bind()
{
    if (m_uploaded && m_texture_id) {
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        return;
    }

    if (m_texture_id == 0)
        glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);

#ifdef ETC_DEBUG
    qDebug() << "glCompressedTexImage2D, width: " << m_size.width() << "height" << m_size.height() <<
                "paddedWidth: " << m_paddedSize.width() << "paddedHeight: " << m_paddedSize.height();
#endif

#ifndef QT_NO_DEBUG
    while (glGetError() != GL_NO_ERROR) { }
#endif

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    Q_ASSERT(ctx != 0);
    ctx->functions()->glCompressedTexImage2D(GL_TEXTURE_2D, 0, m_type,
                                             m_size.width(), m_size.height(), 0,
                                             (m_paddedSize.width() * m_paddedSize.height()) / 2,
                                             m_data.data() + headerSize);

#ifndef QT_NO_DEBUG
    // Gracefully fail in case of an error...
    GLuint error = glGetError();
    if (error != GL_NO_ERROR) {
        qDebug () << "glCompressedTexImage2D for compressed texture failed, error: " << error;
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &m_texture_id);
        m_texture_id = 0;
        return;
    }
#endif

    m_uploaded = true;
    updateBindOptions(true);
}

class QEtcTextureFactory : public QQuickTextureFactory
{
public:
    QByteArray m_data;
    QSize m_size;
    QSize m_paddedSize;
    unsigned int m_type;

    QSize textureSize() const { return m_size; }
    int textureByteCount() const { return m_data.size(); }

    QSGTexture *createTexture(QQuickWindow *) const {
        QEtcTexture *texture = new QEtcTexture;
        texture->m_data = m_data;
        texture->m_size = m_size;
        texture->m_paddedSize = m_paddedSize;
        texture->m_type = m_type;
        return texture;
    }
};

QQuickTextureFactory *QSGPkmHandler::read(QIODevice *device)
{
    QScopedPointer<QEtcTextureFactory> ret(new QEtcTextureFactory);
    ret->m_data = device->readAll();
    if (ret->m_data.isEmpty() || ret->m_data.size() < headerSize)
        return nullptr;

    const char *rawData = ret->m_data.constData();

    // magic number
    if (qstrncmp(rawData, "PKM ", 4) != 0)
        return nullptr;

    // currently ignore version (rawData + 4)

    // texture type
    quint16 type = qFromBigEndian<quint16>(rawData + 6);
    static int typeCount = sizeof(typeMap)/sizeof(typeMap[0]);
    if (type >= typeCount)
        return nullptr;
    ret->m_type = typeMap[type];

    // texture size
    ret->m_paddedSize.setWidth(qFromBigEndian<quint16>(rawData + 8));
    ret->m_paddedSize.setHeight(qFromBigEndian<quint16>(rawData + 10));
    if ((ret->m_paddedSize.width() * ret->m_paddedSize.height()) / 2 > ret->m_data.size() - headerSize)
        return nullptr;
    ret->m_size.setWidth(qFromBigEndian<quint16>(rawData + 12));
    ret->m_size.setHeight(qFromBigEndian<quint16>(rawData + 14));
    if (ret->m_size.isEmpty())
        return nullptr;

#ifdef ETC_DEBUG
    qDebug() << "requestTexture returning: " << ret->m_data.length() << "bytes; width: " << ret->m_size.width() << ", height: " << ret->m_size.height();
#endif

    return ret.take();
}

QT_END_NAMESPACE
