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

#ifndef QSHAREDIMAGELOADER_H
#define QSHAREDIMAGELOADER_H

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

#include <QImage>
#include <QVariant>
#include <QLoggingCategory>
#include <qqml.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSharedImage);

class QSharedImageLoaderPrivate;

class Q_LABSSHAREDIMAGE_PRIVATE_EXPORT QSharedImageLoader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSharedImageLoader)

    // We need to provide some type, in order to mention the 1.0 version.
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(1, 0)

public:
    typedef QVector<QVariant> ImageParameters;

    QSharedImageLoader(QObject *parent = nullptr);
    ~QSharedImageLoader();

    QImage load(const QString &path, ImageParameters *params = nullptr);

protected:
    virtual QImage loadFile(const QString &path, ImageParameters *params);
    virtual QString key(const QString &path, ImageParameters *params);

private:
    Q_DISABLE_COPY(QSharedImageLoader)
};

QT_END_NAMESPACE

#endif // QSHAREDIMAGELOADER_H
