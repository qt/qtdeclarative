// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemgenerator_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qquickitem_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

QQuickItemGenerator::QQuickItemGenerator(const QString &fileName,
                                         QQuickVectorImageGenerator::GeneratorFlags flags)
    : QQuickGenerator(fileName, flags)
{
}

QQuickItemGenerator::~QQuickItemGenerator() = default;

QQuickItem *QQuickItemGenerator::takeRootItem()
{
    QQuickItem *item = m_rootItem;
    m_rootItem = nullptr;
    return item;
}

bool QQuickItemGenerator::generateRootNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (info.stage == StructureNodeStage::Start) {
        auto *root = new QQuickItem;
        if (info.size.width() > 0)
            root->setImplicitWidth(info.size.width());
        if (info.size.height() > 0)
            root->setImplicitHeight(info.size.height());
        m_rootItem = root;
    }
    return true;
}

QString QQuickItemGenerator::generateNodeBase(const NodeInfo &info, const QString &idSuffix)
{
    Q_UNUSED(idSuffix)
    return info.id;
}

bool QQuickItemGenerator::generateStructureNode(const StructureNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateStructureNode: not yet implemented";
    Q_UNUSED(info)
    return true;
}

void QQuickItemGenerator::generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    qCDebug(lcQuickVectorImage) << "generatePath: not yet implemented";
    Q_UNUSED(info)
    Q_UNUSED(overrideBoundingRect)
}

void QQuickItemGenerator::outputShapePath(const PathNodeInfo &info, const QPainterPath *path,
                                          const QQuadPath *quadPath,
                                          QQuickVectorImageGenerator::PathSelector pathSelector,
                                          const QRectF &boundingRect)
{
    qCDebug(lcQuickVectorImage) << "outputShapePath: not yet implemented";
    Q_UNUSED(info)
    Q_UNUSED(path)
    Q_UNUSED(quadPath)
    Q_UNUSED(pathSelector)
    Q_UNUSED(boundingRect)
}

void QQuickItemGenerator::generateImageNode(const ImageNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateImageNode: not yet implemented";
    Q_UNUSED(info)
}

void QQuickItemGenerator::generateTextNode(const TextNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateTextNode: not yet implemented";
    Q_UNUSED(info)
}

void QQuickItemGenerator::generateNode(const NodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateNode: not yet implemented";
    Q_UNUSED(info)
}

void QQuickItemGenerator::generateUseNode(const UseNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateUseNode: not yet implemented";
    Q_UNUSED(info)
}

bool QQuickItemGenerator::generateDefsNode(const StructureNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateDefsNode: not yet implemented";
    Q_UNUSED(info)
    return true;
}

void QQuickItemGenerator::generateDefsInstantiationNode(const StructureNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateDefsInstantiationNode: not yet implemented";
    Q_UNUSED(info)
}

bool QQuickItemGenerator::generateMaskNode(const MaskNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateMaskNode: not yet implemented";
    Q_UNUSED(info)
    return true;
}

void QQuickItemGenerator::generateFilterNode(const FilterNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateFilterNode: not yet implemented";
    Q_UNUSED(info)
}

bool QQuickItemGenerator::generateMarkerNode(const MarkerNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generateMarkerNode: not yet implemented";
    Q_UNUSED(info)
    return true;
}

bool QQuickItemGenerator::generatePatternNode(const PatternNodeInfo &info)
{
    qCDebug(lcQuickVectorImage) << "generatePatternNode: not yet implemented";
    Q_UNUSED(info)
    return true;
}

QT_END_NAMESPACE
