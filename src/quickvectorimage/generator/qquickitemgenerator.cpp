// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemgenerator_p.h"
#include "utils_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qsgcurveprocessor_p.h>
#include <private/qquickshape_p.h>
#include <private/qquadpath_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickimagebase_p_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQuickVectorImage)

QQuickItemGenerator::QQuickItemGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags, QQuickItem *parentItem)
    :QQuickGenerator(fileName, flags)
{
    Q_ASSERT(parentItem);
    m_items.push(parentItem);
    m_parentItem = parentItem;
}

QQuickItemGenerator::~QQuickItemGenerator()
{
}

void QQuickItemGenerator::generateNodeBase(const NodeInfo &info)
{
    if (!info.isDefaultTransform) {
        auto sx = info.transform.m11();
        auto sy = info.transform.m22();
        auto x = info.transform.m31();
        auto y = info.transform.m32();

        auto xformProp = currentItem()->transform();
        if (info.transform.type() == QTransform::TxTranslate) {
            auto *translate = new QQuickTranslate;
            translate->setX(x);
            translate->setY(y);
            xformProp.append(&xformProp, translate);
        } else if (info.transform.type() == QTransform::TxScale && !x && !y) {
            auto scale = new QQuickScale;
            scale->setParent(currentItem());
            scale->setXScale(sx);
            scale->setYScale(sy);
            xformProp.append(&xformProp, scale);
        } else {
            const QMatrix4x4 m(info.transform);
            auto xform = new QQuickMatrix4x4;
            xform->setMatrix(m);
            xformProp.append(&xformProp, xform);
        }
    }
    if (!info.isDefaultOpacity) {
        currentItem()->setOpacity(info.opacity);
    }
}

bool QQuickItemGenerator::generateDefsNode(const NodeInfo &info)
{
    Q_UNUSED(info)

    return false;
}

void QQuickItemGenerator::generateImageNode(const ImageNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    auto *imageItem = new QQuickImage;
    auto *imagePriv = static_cast<QQuickImageBasePrivate*>(QQuickItemPrivate::get(imageItem));
    imagePriv->currentPix->setImage(info.image);

    imageItem->setX(info.rect.x());
    imageItem->setY(info.rect.y());
    imageItem->setWidth(info.rect.width());
    imageItem->setHeight(info.rect.height());

    addCurrentItem(imageItem, info);
    generateNodeBase(info);

    m_items.pop();
}

void QQuickItemGenerator::generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    if (!isNodeVisible(info))
        return;

    if (m_inShapeItem) {
        if (!info.isDefaultTransform)
            qCWarning(lcQuickVectorImage) << "Skipped transform for node" << info.nodeId << "type" << info.typeName << "(this is not supposed to happen)";
        optimizePaths(info, overrideBoundingRect);
    } else {
        auto *shapeItem = new QQuickShape;
        if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
            shapeItem->setPreferredRendererType(QQuickShape::CurveRenderer);
        shapeItem->setContainsMode(QQuickShape::ContainsMode::FillContains); // TODO: configurable?
        addCurrentItem(shapeItem, info);
        m_parentShapeItem = shapeItem;
        m_inShapeItem = true;

        generateNodeBase(info);

        optimizePaths(info, overrideBoundingRect);
        //qCDebug(lcQuickVectorGraphics) << *node->qpath();
        m_items.pop();
        m_inShapeItem = false;
        m_parentShapeItem = nullptr;
    }
}

void QQuickItemGenerator::outputShapePath(const PathNodeInfo &info, const QPainterPath *painterPath, const QQuadPath *quadPath, QQuickVectorImageGenerator::PathSelector pathSelector, const QRectF &boundingRect)
{
    Q_UNUSED(pathSelector)
    Q_ASSERT(painterPath || quadPath);

    const bool noPen = info.strokeStyle.color == QColorConstants::Transparent;
    if (pathSelector == QQuickVectorImageGenerator::StrokePath && noPen)
        return;

    const bool noFill = info.grad.type() == QGradient::NoGradient && info.fillColor == QColorConstants::Transparent;

    if (pathSelector == QQuickVectorImageGenerator::FillPath && noFill)
        return;

    QQuickShapePath::FillRule fillRule = QQuickShapePath::FillRule(painterPath ? painterPath->fillRule() : quadPath->fillRule());

    QQuickShapePath *shapePath = new QQuickShapePath;
    Q_ASSERT(shapePath);

    if (!info.nodeId.isEmpty())
        shapePath->setObjectName(QStringLiteral("svg_path:") + info.nodeId);

    if (noPen || !(pathSelector & QQuickVectorImageGenerator::StrokePath)) {
        shapePath->setStrokeColor(Qt::transparent);
    } else {
        shapePath->setStrokeColor(info.strokeStyle.color);
        shapePath->setStrokeWidth(info.strokeStyle.width);
        shapePath->setCapStyle(QQuickShapePath::CapStyle(info.strokeStyle.lineCapStyle));
        shapePath->setJoinStyle(QQuickShapePath::JoinStyle(info.strokeStyle.lineJoinStyle));
        shapePath->setMiterLimit(info.strokeStyle.miterLimit);
        if (info.strokeStyle.dashArray.length() != 0) {
            shapePath->setStrokeStyle(QQuickShapePath::DashLine);
            shapePath->setDashPattern(info.strokeStyle.dashArray.toVector());
            shapePath->setDashOffset(info.strokeStyle.dashOffset);
        }
    }

    QTransform fillTransform = info.fillTransform;
    if (!(pathSelector & QQuickVectorImageGenerator::FillPath)) {
        shapePath->setFillColor(Qt::transparent);
    } else if (info.grad.type() != QGradient::NoGradient) {
        generateGradient(&info.grad, shapePath);
        if (info.grad.coordinateMode() == QGradient::ObjectMode) {
            QTransform objectToUserSpace;
            objectToUserSpace.translate(boundingRect.x(), boundingRect.y());
            objectToUserSpace.scale(boundingRect.width(), boundingRect.height());
            fillTransform *= objectToUserSpace;
        }
    } else {
        shapePath->setFillColor(info.fillColor);
    }

    shapePath->setFillRule(fillRule);
    if (!fillTransform.isIdentity())
        shapePath->setFillTransform(fillTransform);

    QString svgPathString = painterPath ? QQuickVectorImageGenerator::Utils::toSvgString(*painterPath) : QQuickVectorImageGenerator::Utils::toSvgString(*quadPath);

    auto *pathSvg = new QQuickPathSvg;
    pathSvg->setPath(svgPathString);
    pathSvg->setParent(shapePath);

    auto pathElementProp = shapePath->pathElements();
    pathElementProp.append(&pathElementProp, pathSvg);

    shapePath->setParent(currentItem());
    auto shapeDataProp = m_parentShapeItem->data();
    shapeDataProp.append(&shapeDataProp, shapePath);
}

void QQuickItemGenerator::generateGradient(const QGradient *grad, QQuickShapePath *shapePath)
{
    if (!shapePath)
        return;

    auto setStops = [=](QQuickShapeGradient *quickGrad, const QGradientStops &stops) {
        auto stopsProp = quickGrad->stops();
        for (auto &stop : stops) {
            auto *stopObj = new QQuickGradientStop(quickGrad);
            stopObj->setPosition(stop.first);
            stopObj->setColor(stop.second);
            stopsProp.append(&stopsProp, stopObj);
        }
    };

    if (grad->type() == QGradient::LinearGradient) {
        auto *linGrad = static_cast<const QLinearGradient *>(grad);

        auto *quickGrad = new QQuickShapeLinearGradient(shapePath);
        quickGrad->setX1(linGrad->start().x());
        quickGrad->setY1(linGrad->start().y());
        quickGrad->setX2(linGrad->finalStop().x());
        quickGrad->setY2(linGrad->finalStop().y());
        setStops(quickGrad, linGrad->stops());

        shapePath->setFillGradient(quickGrad);
    } else if (grad->type() == QGradient::RadialGradient) {
        auto *radGrad = static_cast<const QRadialGradient*>(grad);
        auto *quickGrad = new QQuickShapeRadialGradient(shapePath);
        quickGrad->setCenterX(radGrad->center().x());
        quickGrad->setCenterY(radGrad->center().y());
        quickGrad->setCenterRadius(radGrad->radius());
        quickGrad->setFocalX(radGrad->focalPoint().x());
        quickGrad->setFocalY(radGrad->focalPoint().y());
        setStops(quickGrad, radGrad->stops());

        shapePath->setFillGradient(quickGrad);
    }
}

void QQuickItemGenerator::generateNode(const NodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    qCWarning(lcQuickVectorImage) << "SVG NODE NOT IMPLEMENTED: "
                                  << info.nodeId
                                  << " type: " << info.typeName;
}

void QQuickItemGenerator::generateTextNode(const TextNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    QQuickItem *alignItem = nullptr;
    QQuickText *textItem = nullptr;

    QQuickItem *containerItem = new QQuickItem(currentItem());
    addCurrentItem(containerItem, info);

    generateNodeBase(info);

    if (!info.isTextArea) {
        alignItem = new QQuickItem(currentItem());
        alignItem->setX(info.position.x());
        alignItem->setY(info.position.y());
    }

    textItem = new QQuickText(containerItem);
    addCurrentItem(textItem, info);

    if (info.isTextArea) {
        textItem->setX(info.position.x());
        textItem->setY(info.position.y());
        if (info.size.width() > 0)
            textItem->setWidth(info.size.width());
        if (info.size.height() > 0)
            textItem->setHeight(info.size.height());
        textItem->setWrapMode(QQuickText::Wrap);
        textItem->setClip(true);
    } else {
        auto *anchors = QQuickItemPrivate::get(textItem)->anchors();
        auto *alignPrivate = QQuickItemPrivate::get(alignItem);
        anchors->setBaseline(alignPrivate->top());

        switch (info.alignment) {
        case Qt::AlignHCenter:
            anchors->setHorizontalCenter(alignPrivate->left());
            break;
        case Qt::AlignRight:
            anchors->setRight(alignPrivate->left());
            break;
        default:
            qCDebug(lcQuickVectorImage) << "Unexpected text alignment" << info.alignment;
            Q_FALLTHROUGH();
        case Qt::AlignLeft:
            anchors->setLeft(alignPrivate->left());
            break;
        }
    }

    textItem->setColor(info.fillColor);
    textItem->setTextFormat(info.needsRichText ? QQuickText::RichText : QQuickText::StyledText);
    textItem->setText(info.text);
    textItem->setFont(info.font);

    if (info.strokeColor != QColorConstants::Transparent) {
        textItem->setStyleColor(info.strokeColor);
        textItem->setStyle(QQuickText::Outline);
    }

    m_items.pop(); m_items.pop();
}

void QQuickItemGenerator::generateUseNode(const UseNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    if (info.stage == StructureNodeStage::Start) {
        QQuickItem *item = new QQuickItem();
        item->setPosition(info.startPos);
        addCurrentItem(item, info);
        generateNodeBase(info);
    } else {
        m_items.pop();
    }

}

void QQuickItemGenerator::generatePathContainer(const StructureNodeInfo &info)
{
    m_inShapeItem = true;
    auto *shapeItem = new QQuickShape;
    if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
        shapeItem->setPreferredRendererType(QQuickShape::CurveRenderer);
    m_parentShapeItem = shapeItem;
    addCurrentItem(shapeItem, info);
}

bool QQuickItemGenerator::generateStructureNode(const StructureNodeInfo &info)
{
    if (!isNodeVisible(info))
        return false;

    if (info.stage == StructureNodeStage::Start) {
        if (!info.forceSeparatePaths && info.isPathContainer) {
            generatePathContainer(info);
        } else {
            QQuickItem *item = !info.viewBox.isEmpty() ? new QQuickVectorImageGenerator::Utils::ViewBoxItem(info.viewBox) : new QQuickItem;
            addCurrentItem(item, info);
        }

        generateNodeBase(info);
    } else {
        m_inShapeItem = false;
        m_parentShapeItem = nullptr;
        m_items.pop();
    }

    return true;
}

bool QQuickItemGenerator::generateRootNode(const StructureNodeInfo &info)
{
    if (!isNodeVisible(info)) {
        QQuickItem *item = new QQuickItem();
        item->setParentItem(m_parentItem);

        if (info.size.width() > 0)
            m_parentItem->setImplicitWidth(info.size.width());

        if (info.size.height() > 0)
            m_parentItem->setImplicitHeight(info.size.height());

        item->setWidth(m_parentItem->implicitWidth());
        item->setHeight(m_parentItem->implicitHeight());

        return false;
    }

    if (info.stage == StructureNodeStage::Start) {
        QQuickItem *item = !info.viewBox.isEmpty() ? new QQuickVectorImageGenerator::Utils::ViewBoxItem(info.viewBox) : new QQuickItem;
        addCurrentItem(item, info);
        if (info.size.width() > 0)
            m_parentItem->setImplicitWidth(info.size.width());

        if (info.size.height() > 0)
            m_parentItem->setImplicitHeight(info.size.height());

        item->setWidth(m_parentItem->implicitWidth());
        item->setHeight(m_parentItem->implicitHeight());
        generateNodeBase(info);

        if (!info.forceSeparatePaths && info.isPathContainer)
            generatePathContainer(info);
    } else {
        if (m_inShapeItem) {
            m_inShapeItem = false;
            m_parentShapeItem = nullptr;
            m_items.pop();
        }

        m_items.pop();
    }

    return true;
}

QQuickItem *QQuickItemGenerator::currentItem()
{
    return m_items.top();
}

void QQuickItemGenerator::addCurrentItem(QQuickItem *item, const NodeInfo &info)
{
    item->setParentItem(currentItem());
    m_items.push(item);
    QStringView name = !info.nodeId.isEmpty() ? info.nodeId : info.typeName;
    item->setObjectName(name);
}

QT_END_NAMESPACE
