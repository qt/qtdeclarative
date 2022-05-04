/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QSHAREDIMAGEPROVIDER_H
#define QSHAREDIMAGEPROVIDER_H

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

#include "qtlabssharedimageglobal_p.h"

#include <QQuickImageProvider>
#include <private/qquickpixmapcache_p.h>
#include <QScopedPointer>

#include "qsharedimageloader_p.h"

QT_BEGIN_NAMESPACE

class SharedImageProvider;

class QuickSharedImageLoader : public QSharedImageLoader
{
    Q_OBJECT
    friend class SharedImageProvider;

public:
    enum ImageParameter {
        OriginalSize = 0,
        RequestedSize,
        ProviderOptions,
        NumImageParameters
    };

    QuickSharedImageLoader(QObject *parent = nullptr);
protected:
    QImage loadFile(const QString &path, ImageParameters *params) override;
    QString key(const QString &path, ImageParameters *params) override;
};

class Q_LABSSHAREDIMAGE_PRIVATE_EXPORT SharedImageProvider : public QQuickImageProviderWithOptions
{
public:
    SharedImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize, const QQuickImageProviderOptions &options) override;

protected:
    QScopedPointer<QuickSharedImageLoader> loader;
};

QT_END_NAMESPACE

#endif // QSHAREDIMAGEPROVIDER_H
