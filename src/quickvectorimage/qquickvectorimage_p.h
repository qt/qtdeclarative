// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(RendererType preferredRendererType READ preferredRendererType WRITE setPreferredRendererType NOTIFY preferredRendererTypeChanged)
    QML_NAMED_ELEMENT(VectorImage)

public:
    enum FillMode {
        NoResize,
        PreserveAspectFit,
        PreserveAspectCrop,
        Stretch
    };
    Q_ENUM(FillMode)

    enum RendererType {
        GeometryRenderer,
        CurveRenderer
    };
    Q_ENUM(RendererType)

    QQuickVectorImage(QQuickItem *parent = nullptr);

    QUrl source() const;
    void setSource(const QUrl &source);

    FillMode fillMode() const;
    void setFillMode(FillMode newFillMode);

    RendererType preferredRendererType() const;
    void setPreferredRendererType(RendererType newPreferredRendererType);

signals:
    void sourceChanged();
    void fillModeChanged();

    void preferredRendererTypeChanged();

private slots:
    void updateSvgItemScale();

private:
    Q_DISABLE_COPY(QQuickVectorImage)
    Q_DECLARE_PRIVATE(QQuickVectorImage)
};

QT_END_NAMESPACE

#endif // QQUICKVECTORIMAGE_P_H

