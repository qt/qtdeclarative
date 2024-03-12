// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICKVECTORIMAGE_P_H
#define QQUICKVECTORIMAGE_P_H

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

#include <QQuickItem>
#include <QtQuickVectorImage/qtquickvectorimageexports.h>

QT_BEGIN_NAMESPACE

class QQuickVectorImagePrivate;

class Q_QUICKVECTORIMAGE_EXPORT QQuickVectorImage : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    QML_NAMED_ELEMENT(VectorImage)

public:
    QQuickVectorImage(QQuickItem *parent = nullptr);

    QUrl source() const;
    void setSource(const QUrl &source);

signals:
    void sourceChanged();

private:
    Q_DISABLE_COPY(QQuickVectorImage)
    Q_DECLARE_PRIVATE(QQuickVectorImage)
};

QT_END_NAMESPACE

#endif // QQUICKVECTORIMAGE_P_H

