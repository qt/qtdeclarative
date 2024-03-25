// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKNODEINFO_P_H
#define QQUICKNODEINFO_P_H

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
#include <QPainter>
#include <QPainterPath>
#include <QMatrix4x4>
#include <QQuickItem>
#include <private/qquicktext_p.h>
#include <private/qquicktranslate_p.h>
#include <private/qquickimage_p.h>

QT_BEGIN_NAMESPACE

struct NodeInfo
{
    QString nodeId;
    QString typeName;
    QTransform transform;
    qreal opacity;
    bool isDefaultTransform;
    bool isDefaultOpacity;
    bool isVisible;
    bool isDisplayed; // TODO: Map to display enum in QtSvg
};

struct ImageNodeInfo : NodeInfo
{
    QImage image;
    QRectF rect;
};

struct StrokeStyle
{
    Qt::PenCapStyle lineCapStyle = Qt::SquareCap;
    Qt::PenJoinStyle lineJoinStyle = Qt::MiterJoin;
    qreal miterLimit = 4;
    qreal dashOffset = 0;
    QList<qreal> dashArray;
    QColor color = QColorConstants::Transparent;
    qreal width = 1.0;
};

struct PathNodeInfo : NodeInfo
{
    QPainterPath painterPath;
    Qt::FillRule fillRule = Qt::FillRule::WindingFill;
    QColor fillColor;
    StrokeStyle strokeStyle;
    QGradient grad;
};

struct TextNodeInfo : NodeInfo
{
    bool isTextArea;
    bool needsRichText;
    QPointF position;
    QSizeF size;
    QString text;
    QFont font;
    Qt::Alignment alignment;
    QColor fillColor;
    QColor strokeColor;
};

enum class StructureNodeStage
{
    Start,
    End
};

struct UseNodeInfo : NodeInfo
{
    QPointF startPos;
    StructureNodeStage stage;
};

struct StructureNodeInfo : NodeInfo
{
    StructureNodeStage stage;
    bool forceSeparatePaths;
    QRectF viewBox;
    QSize size;
    bool isPathContainer;
};


QT_END_NAMESPACE

#endif //QQUICKNODEINFO_P_H
