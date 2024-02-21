// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
};

struct ImageNodeInfo : NodeInfo
{
    QImage image;
    QRectF rect;
};

struct PathNodeInfo : NodeInfo
{
    QPainterPath painterPath;
    Qt::FillRule fillRule = Qt::FillRule::WindingFill;
    Qt::PenCapStyle capStyle = Qt::SquareCap;
    QString strokeColor;
    qreal strokeWidth;
    QString fillColor;
    const QGradient *grad;
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
    QString color;
    QString strokeColor;
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
