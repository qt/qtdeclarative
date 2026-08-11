// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickvectorimagesource_p.h"
#include "qquickgenerator_p.h"

#include <QtCore/qbuffer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

QQuickVectorImageSource::QQuickVectorImageSource(const QString &fileName, const QByteArray &data)
    : m_fileName(fileName)
    , m_data(data)
{
}

QQuickVectorImageSource::~QQuickVectorImageSource() = default;

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QQuickVectorImageSource &source)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QQuickVectorImageSource(";
    if (source.isFile())
        debug << "file " << source.fileName();
    else
        debug << source.data().size() << " bytes of data";
    debug << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
