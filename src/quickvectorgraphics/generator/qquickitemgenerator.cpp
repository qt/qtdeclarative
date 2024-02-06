// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquickitemgenerator_p.h"
#include "utils_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qsgcurveprocessor_p.h>
#include <private/qquickshape_p.h>
#include <private/qquadpath_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickimagebase_p_p.h>

#include<QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQuickVectorGraphics)

QQuickItemGenerator::QQuickItemGenerator(const QString fileName, QQuickVectorGraphics::GeneratorFlags flags, QQuickItem *parentItem)
    :QQuickGenerator(fileName, flags)
{
    Q_ASSERT(parentItem);
    m_items.push(parentItem);
    m_parentItem = parentItem;
}

QQuickItemGenerator::~QQuickItemGenerator()
{

}

void QQuickItemGenerator::generateNodeBase(NodeInfo &info)
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

bool QQuickItemGenerator::generateDefsNode(NodeInfo &info)
{
    Q_UNUSED(info)

    return false;
}

void QQuickItemGenerator::generateImageNode(ImageNodeInfo &info)
{
    auto *imageItem = new QQuickImage;
    auto *imagePriv = static_cast<QQuickImageBasePrivate*>(QQuickItemPrivate::get(imageItem));
    imagePriv->pix.setImage(info.image);

    imageItem->setX(info.rect.x());
    imageItem->setY(info.rect.y());
    imageItem->setWidth(info.rect.width());
    imageItem->setHeight(info.rect.height());

    generateNodeBase(info);

    addCurrentItem(imageItem, !info.nodeId.isEmpty() ? info.nodeId : info.typeName);
    m_items.pop();
}

void QQuickItemGenerator::generatePath(PathNodeInfo &info)
{
    if (m_inShapeItem) {
        if (!info.isDefaultTransform)
            qCWarning(lcQuickVectorGraphics) << "Skipped transform for node" << info.nodeId << "type" << info.typeName << "(this is not supposed to happen)";
        optimizePaths(info);
    } else {
        auto *shapeItem = new QQuickShape;
        if (m_flags.testFlag(QQuickVectorGraphics::GeneratorFlag::CurveRenderer))
            shapeItem->setPreferredRendererType(QQuickShape::CurveRenderer);
        shapeItem->setContainsMode(QQuickShape::ContainsMode::FillContains); // TODO: configurable?
        addCurrentItem(shapeItem, !info.nodeId.isEmpty() ? info.nodeId : info.typeName);
        m_parentShapeItem = shapeItem;
        m_inShapeItem = true;

        // Check ??
        generateNodeBase(info);

        optimizePaths(info);
        //qCDebug(lcQuickVectorGraphics) << *node->qpath();
        m_items.pop();
        m_inShapeItem = false;
        m_parentShapeItem = nullptr;
    }
}

void QQuickItemGenerator::outputShapePath(const PathNodeInfo &info, const QPainterPath *painterPath, const QQuadPath *quadPath, QQuickVectorGraphics::PathSelector pathSelector, const QRectF &boundingRect)
{
    Q_UNUSED(pathSelector)
    Q_ASSERT(painterPath || quadPath);

    QString penName = info.strokeColor;
    const bool noPen = penName.isEmpty() || penName == u"transparent";
    if (pathSelector == QQuickVectorGraphics::StrokePath && noPen)
        return;

    const bool noFill = !info.grad && info.fillColor == u"transparent";
    if (pathSelector == QQuickVectorGraphics::FillPath && noFill)
        return;

    QQuickShapePath::FillRule fillRule = QQuickShapePath::FillRule(painterPath ? painterPath->fillRule() : quadPath->fillRule());

    QQuickShapePath *shapePath = new QQuickShapePath;
    Q_ASSERT(shapePath);

    if (!info.nodeId.isEmpty()) {

        shapePath->setObjectName(QStringLiteral("svg_path:") + info.nodeId);
    }

    if (noPen || !(pathSelector & QQuickVectorGraphics::StrokePath)) {
        shapePath->setStrokeColor(Qt::transparent);
    } else {
        shapePath->setStrokeColor(QColor::fromString(penName));
        shapePath->setStrokeWidth(info.strokeWidth);
    }

    shapePath->setCapStyle(QQuickShapePath::CapStyle(info.capStyle));

    if (!(pathSelector & QQuickVectorGraphics::FillPath))
        shapePath->setFillColor(Qt::transparent);
    else if (auto *grad = info.grad)
        generateGradient(grad, shapePath, boundingRect);
    else
        shapePath->setFillColor(QColor::fromString(info.fillColor));

    shapePath->setFillRule(fillRule);

    QString svgPathString = painterPath ? QQuickVectorGraphics::Utils::toSvgString(*painterPath) : QQuickVectorGraphics::Utils::toSvgString(*quadPath);

    auto *pathSvg = new QQuickPathSvg;
    pathSvg->setPath(svgPathString);
    pathSvg->setParent(shapePath);

    auto pathElementProp = shapePath->pathElements();
    pathElementProp.append(&pathElementProp, pathSvg);

    shapePath->setParent(currentItem());
    auto shapeDataProp = m_parentShapeItem->data();
    shapeDataProp.append(&shapeDataProp, shapePath);
}

void QQuickItemGenerator::generateGradient(const QGradient *grad, QQuickShapePath *shapePath, const QRectF &boundingRect)
{
    auto setStops = [](QQuickShapeGradient *quickGrad, const QGradientStops &stops) {
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

        QRectF gradRect(linGrad->start(), linGrad->finalStop());
        QRectF logRect = linGrad->coordinateMode() == QGradient::LogicalMode ? gradRect : QQuickVectorGraphics::Utils::mapToQtLogicalMode(gradRect, boundingRect);

        if (shapePath) {
            auto *quickGrad = new QQuickShapeLinearGradient(shapePath);

            quickGrad->setX1(logRect.left());
            quickGrad->setY1(logRect.top());
            quickGrad->setX2(logRect.right());
            quickGrad->setY2(logRect.bottom());
            setStops(quickGrad, linGrad->stops());

            shapePath->setFillGradient(quickGrad);
        }
    } else if (grad->type() == QGradient::RadialGradient) {
        auto *radGrad = static_cast<const QRadialGradient*>(grad);

        if (shapePath) {
            auto *quickGrad = new QQuickShapeRadialGradient(shapePath);
            quickGrad->setCenterX(radGrad->center().x());
            quickGrad->setCenterY(radGrad->center().y());
            quickGrad->setCenterRadius(radGrad->radius());
            quickGrad->setFocalX(radGrad->center().x());
            quickGrad->setFocalY(radGrad->center().y());
            setStops(quickGrad, radGrad->stops());

            shapePath->setFillGradient(quickGrad);
        }
    }
}

void QQuickItemGenerator::generateNode(NodeInfo &info)
{
    qCWarning(lcQuickVectorGraphics) << "//### SVG NODE NOT IMPLEMENTED: " << info.nodeId << " type: " << info.typeName;
}

void QQuickItemGenerator::generateTextNode(TextNodeInfo &info)
{
    QQuickItem *alignItem = nullptr;
    QQuickText *textItem = nullptr;

    if (!info.isTextArea) {
        alignItem = new QQuickItem(currentItem());
        alignItem->setX(info.position.x());
        alignItem->setY(info.position.y());
    }

    textItem = new QQuickText;
    addCurrentItem(textItem, !info.nodeId.isEmpty() ? info.nodeId : info.typeName);

    if (info.isTextArea) {
        textItem->setX(info.position.x());
        textItem->setY(info.position.y());
        textItem->setWidth(info.size.width());
        textItem->setHeight(info.size.height());
        textItem->setWrapMode(QQuickText::Wrap);
        textItem->setClip(true);
    } else {
        auto *anchors = QQuickItemPrivate::get(textItem)->anchors();
        auto *alignPrivate = QQuickItemPrivate::get(alignItem);
        anchors->setBaseline(alignPrivate->top());

        QString hAlign = QStringLiteral("left");
        switch (info.alignment) {
        case Qt::AlignHCenter:
            hAlign = QStringLiteral("horizontalCenter");
            anchors->setHorizontalCenter(alignPrivate->left());
            break;
        case Qt::AlignRight:
            hAlign = QStringLiteral("right");
            anchors->setRight(alignPrivate->left());
            break;
        default:
            qCDebug(lcQuickVectorGraphics) << "Unexpected text alignment" << info.alignment;
            Q_FALLTHROUGH();
        case Qt::AlignLeft:
            anchors->setLeft(alignPrivate->left());
            break;
        }
    }

    textItem->setColor(QColor::fromString(info.color));
    textItem->setTextFormat(QQuickText::StyledText);
    textItem->setText(info.text);
    textItem->setFont(info.font);

    m_items.pop();
}

void QQuickItemGenerator::generateStructureNode(StructureNodeInfo &info)
{
    if (info.stage == StructureNodeInfo::StructureNodeStage::Start) {
        if (!info.forceSeparatePaths && info.isPathContainer) {
            m_inShapeItem = true;
            auto *shapeItem = new QQuickShape;
            if (m_flags.testFlag(QQuickVectorGraphics::GeneratorFlag::CurveRenderer))
                shapeItem->setPreferredRendererType(QQuickShape::CurveRenderer);
            m_parentShapeItem = shapeItem;
            addCurrentItem(shapeItem, !info.nodeId.isEmpty() ? info.nodeId : info.typeName);
        } else {
            QQuickItem *item = !info.viewBox.isEmpty() ? new QQuickVectorGraphics::Utils::ViewBoxItem(info.viewBox) : new QQuickItem;
            addCurrentItem(item, !info.nodeId.isEmpty() ? info.nodeId : info.typeName);
        }

        generateNodeBase(info);
    } else {
        m_inShapeItem = false;
        m_parentShapeItem = nullptr;
        m_items.pop();
    }
}

void QQuickItemGenerator::generateRootNode(StructureNodeInfo &info)
{
    if (info.stage == StructureNodeInfo::StructureNodeStage::Start) {
        QQuickItem *item = !info.viewBox.isEmpty() ? new QQuickVectorGraphics::Utils::ViewBoxItem(info.viewBox) : new QQuickItem;
        addCurrentItem(item, !info.nodeId.isEmpty() ? info.nodeId : info.typeName);
        if (info.size.width() > 0)
            m_parentItem->setImplicitWidth(info.size.width());

        if (info.size.height() > 0)
            m_parentItem->setImplicitHeight(info.size.height());
        m_loadedItem = item;
        m_loadedItem->setWidth(m_parentItem->implicitWidth());
        m_loadedItem->setHeight(m_parentItem->implicitHeight());
        generateNodeBase(info);
    } else {
        m_inShapeItem = false;
        m_parentShapeItem = nullptr;
        m_items.pop();
    }
}

QQuickItem *QQuickItemGenerator::currentItem()
{
    return m_items.top();
}

void QQuickItemGenerator::addCurrentItem(QQuickItem *item, const QString &name)
{
    item->setParentItem(currentItem());
    m_items.push(item);
    item->setObjectName(name);
}

QT_END_NAMESPACE
