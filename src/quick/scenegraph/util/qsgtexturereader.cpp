// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgtexturereader_p.h"
#include <private/qtexturefilereader_p.h>

#include <private/qsgcompressedtexture_p.h>

QT_BEGIN_NAMESPACE

QSGTextureReader::QSGTextureReader(QIODevice *device, const QString &fileName)
{
    m_reader = new QTextureFileReader(device, fileName);
}

QSGTextureReader::~QSGTextureReader()
{
    delete m_reader;
}

QQuickTextureFactory *QSGTextureReader::read()
{
    if (!m_reader)
        return nullptr;

    QTextureFileData texData = m_reader->read();
    if (!texData.isValid())
        return nullptr;

    return new QSGCompressedTextureFactory(texData);
}

bool QSGTextureReader::isTexture()
{
    return m_reader ? m_reader->canRead() : false;
}

QList<QByteArray> QSGTextureReader::supportedFileFormats()
{
    return QTextureFileReader::supportedFileFormats();
}

QT_END_NAMESPACE
