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

#include "qsgtexturereader_p.h"

#include <private/qtquickglobal_p.h>

#include <private/qsgtexturefilehandler_p.h>

#if QT_CONFIG(opengl)
#include <private/qsgpkmhandler_p.h>
#include <private/qsgktxhandler_p.h>
#endif

#include <QFileInfo>

QT_BEGIN_NAMESPACE

QSGTextureReader::QSGTextureReader(QIODevice *device, const QString &fileName)
    : m_device(device), m_fileInfo(fileName)
{
}

QSGTextureReader::~QSGTextureReader()
{
    delete m_handler;
}

QQuickTextureFactory *QSGTextureReader::read()
{
#if QT_CONFIG(opengl)
    if (!isTexture())
        return nullptr;
    return m_handler->read();
#else
    return nullptr;
#endif
}

bool QSGTextureReader::isTexture()
{
#if QT_CONFIG(opengl)
    if (!checked) {
        checked = true;
        if (!init())
            return false;

        QByteArray headerBlock = m_device->peek(64);
        QByteArray suffix = m_fileInfo.suffix().toLower().toLatin1();
        QByteArray logName = m_fileInfo.fileName().toUtf8();

        // Currently the handlers are hardcoded; later maybe a list of plugins
        if (QSGPkmHandler::canRead(suffix, headerBlock)) {
            m_handler = new QSGPkmHandler(m_device, logName);
        } else if (QSGKtxHandler::canRead(suffix, headerBlock)) {
            m_handler = new QSGKtxHandler(m_device, logName);
        }
        // else if OtherHandler::canRead() ...etc.
    }
    return (m_handler != nullptr);
#else
    return false;
#endif
}

QList<QByteArray> QSGTextureReader::supportedFileFormats()
{
    // Hardcoded for now
    return {QByteArrayLiteral("pkm"), QByteArrayLiteral("ktx")};
}

bool QSGTextureReader::init()
{
    if (!m_device)
        return false;
    return m_device->isReadable();
}

QT_END_NAMESPACE
