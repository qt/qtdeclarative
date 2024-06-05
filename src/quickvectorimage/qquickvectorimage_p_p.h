// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKVECTORIMAGE_P_P_H
#define QQUICKVECTORIMAGE_P_P_H

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

#include <QQuickPaintedItem>
#include <QSvgRenderer>
#include <private/qquickitem_p.h>
#include "qquickvectorimage_p.h"

QT_BEGIN_NAMESPACE

class QQuickVectorImagePrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickVectorImage)
public:
    QQuickVectorImagePrivate() = default;
    virtual ~QQuickVectorImagePrivate() = default;

    void setSource(const QUrl &source);
    void loadSvg();

    enum Format {
        Unknown,
        Svg
    };
    QQuickVectorImagePrivate::Format formatFromFilePath(const QString &filePath);

    QUrl sourceFile;
    QQuickItem *svgItem = nullptr;
    QQuickVectorImage::FillMode fillMode = QQuickVectorImage::Stretch;
    QQuickVectorImage::RendererType preferredRendererType = QQuickVectorImage::GeometryRenderer;
};

QT_END_NAMESPACE

#endif // QQUICKVECTORIMAGE_P_P_H
