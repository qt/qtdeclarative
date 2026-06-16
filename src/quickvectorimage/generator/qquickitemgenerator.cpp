// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemgenerator_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qquickitem_p.h>
#include <private/qquicktranslate_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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

void QQuickItemGenerator::pushItem(QQuickItem *item)
{
    if (!m_itemStack.isEmpty()) {
        item->setParent(m_itemStack.top());
        item->setParentItem(m_itemStack.top());
    }
    m_itemStack.push(item);
}

QQuickItem *QQuickItemGenerator::popItem()
{
    return m_itemStack.isEmpty() ? nullptr : m_itemStack.pop();
}

QQuickItem *QQuickItemGenerator::currentItem() const
{
    return m_itemStack.isEmpty() ? nullptr : m_itemStack.top();
}

QString QQuickItemGenerator::generateNodeBase(const NodeInfo &info, const QString &idSuffix)
{
    Q_UNUSED(idSuffix)

    static qint64 maxNodes =
            qEnvironmentVariableIntegerValue("QT_QUICKVECTORIMAGE_MAX_NODES").value_or(10000);
    if (Q_UNLIKELY(!checkSanityLimit(++m_nodeCounter, maxNodes, "nodes"_L1)))
        return {};

    QQuickItem *item = currentItem();
    if (!item)
        return info.id;

    if (!info.nodeId.isEmpty())
        item->setObjectName(info.nodeId);

    item->setTransformOrigin(QQuickItem::TopLeft);

    if (!info.bounds.isNull()) {
        item->setWidth(info.bounds.width());
        item->setHeight(info.bounds.height());
    }

    if (info.filterId.isEmpty() && info.maskId.isEmpty()) {
        if (!info.isDefaultOpacity)
            item->setOpacity(info.opacity.defaultValue().toReal());
    }

    if (!info.isDefaultTransform) {
        QTransform xf = info.transform.defaultValue().value<QTransform>();
        auto *matrix = new QQuickMatrix4x4(item);
        matrix->setMatrix(QMatrix4x4(xf));
        auto xformProp = item->transform();
        xformProp.append(&xformProp, matrix);
    }

    return info.id;
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

        if (!isNodeVisible(info))
            return false;

        if (!info.viewBox.isEmpty() && info.size.width() > 0 && info.size.height() > 0) {
            auto xformProp = root->transform();
            if (!qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y())) {
                auto *translate = new QQuickTranslate(root);
                translate->setX(-info.viewBox.x());
                translate->setY(-info.viewBox.y());
                xformProp.append(&xformProp, translate);
            }
            auto *scale = new QQuickScale(root);
            scale->setXScale(info.size.width() / info.viewBox.width());
            scale->setYScale(info.size.height() / info.viewBox.height());
            xformProp.append(&xformProp, scale);
        }

        pushItem(root);
        generateNodeBase(info);
    } else {
        popItem();
    }

    return true;
}

bool QQuickItemGenerator::generateStructureNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
        return false;

    if (info.stage == StructureNodeStage::Start) {
        auto *item = new QQuickItem;

        if (!info.viewBox.isEmpty()) {
            auto xformProp = item->transform();
            if (!qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y())) {
                auto *translate = new QQuickTranslate(item);
                translate->setX(-info.viewBox.x());
                translate->setY(-info.viewBox.y());
                xformProp.append(&xformProp, translate);
            }
            auto *scale = new QQuickScale(item);
            scale->setXScale(info.size.width() / info.viewBox.width());
            scale->setYScale(info.size.height() / info.viewBox.height());
            xformProp.append(&xformProp, scale);
        }

        pushItem(item);
        generateNodeBase(info);
    } else {
        popItem();
    }

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
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
        return;

    if (info.stage == StructureNodeStage::Start) {
        auto *item = new QQuickItem;
        pushItem(item);
        generateNodeBase(info);
    } else {
        popItem();
    }
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
