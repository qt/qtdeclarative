/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSHAREDIMAGELOADER_H
#define QSHAREDIMAGELOADER_H

#include <QImage>
#include <QVariant>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSharedImage);

class QSharedImageLoaderPrivate;

class QSharedImageLoader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSharedImageLoader)

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
