// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QSGTEXTUREPROVIDER_H
#define QSGTEXTUREPROVIDER_H

#include <QtQuick/qsgtexture.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGTextureProvider : public QObject
{
    Q_OBJECT
public:
    virtual QSGTexture *texture() const = 0;

Q_SIGNALS:
    void textureChanged();
};

QT_END_NAMESPACE

#endif
