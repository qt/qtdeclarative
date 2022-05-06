/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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
