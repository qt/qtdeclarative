/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSGTEXTUREREADER_H
#define QSGTEXTUREREADER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QString>
#include <QFileInfo>

QT_BEGIN_NAMESPACE

class QIODevice;
class QQuickTextureFactory;
class QTextureFileReader;

class QSGTextureReader
{
public:
    QSGTextureReader(QIODevice *device, const QString &fileName = QString());
    ~QSGTextureReader();

    QQuickTextureFactory *read();
    bool isTexture();

    // TBD access function to params
    // TBD ask for identified fmt

    static QList<QByteArray> supportedFileFormats();

private:
    QTextureFileReader *m_reader = nullptr;
};

QT_END_NAMESPACE

#endif // QSGTEXTUREREADER_H
