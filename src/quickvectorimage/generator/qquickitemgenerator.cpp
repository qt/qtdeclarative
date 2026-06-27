// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemgenerator_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qquickitem_p.h>
#include <private/qquicktranslate_p.h>
#include <private/qquickshape_p.h>
#include <private/qquickpath_p.h>
#include <private/qquickimage_p.h>
#include <private/qquicktext_p.h>
#include <private/qquickrectangle_p.h>

#include "utils_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtGui/qfontmetrics.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlparserstatus.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQuickItemGenerator::QQuickItemGenerator(const QString &fileName,
                                         QQuickVectorImageGenerator::GeneratorFlags flags,
                                         QQmlContext *context)
    : QQuickGenerator(fileName, flags), m_context(context)
{
}

QQuickItemGenerator::~QQuickItemGenerator() = default;

QQuickItem *QQuickItemGenerator::takeRootItem()
{
    QQuickItem *item = m_rootItem;
    m_rootItem = nullptr;
    return item;
}

QQuickShape *QQuickItemGenerator::createShapeContainer()
{
    auto *shape = new QQuickShape;
    if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
        shape->setPreferredRendererType(QQuickShape::CurveRenderer);
    if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::AsyncShapes))
        shape->setAsynchronous(true);
    return shape;
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
        QQuickItem *item;
        if (!info.forceSeparatePaths && info.isPathContainer) {
            item = createShapeContainer();
        } else {
            item = new QQuickItem;

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
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
        return;

    if (qobject_cast<QQuickShape *>(currentItem())) {
        optimizePaths(info, overrideBoundingRect);
    } else {
        auto *shape = createShapeContainer();
        pushItem(shape);
        generateNodeBase(info);
        optimizePaths(info, overrideBoundingRect);
        popItem();
    }
}

static QQuickShapeGradient *createShapeGradient(const QGradient &grad, const QRectF &coordSys,
                                                QObject *parent)
{
    const qreal sx = coordSys.width();
    const qreal sy = coordSys.height();
    const qreal tx = coordSys.x();
    const qreal ty = coordSys.y();

    QQuickShapeGradient *result = nullptr;

    if (grad.type() == QGradient::LinearGradient) {
        const auto *linGrad = static_cast<const QLinearGradient *>(&grad);
        auto *g = new QQuickShapeLinearGradient(parent);
        g->setX1(linGrad->start().x() * sx + tx);
        g->setY1(linGrad->start().y() * sy + ty);
        g->setX2(linGrad->finalStop().x() * sx + tx);
        g->setY2(linGrad->finalStop().y() * sy + ty);
        result = g;
    } else if (grad.type() == QGradient::RadialGradient) {
        const auto *radGrad = static_cast<const QRadialGradient *>(&grad);
        auto *g = new QQuickShapeRadialGradient(parent);
        g->setCenterX(radGrad->center().x() * sx + tx);
        g->setCenterY(radGrad->center().y() * sy + ty);
        g->setCenterRadius(radGrad->radius() * sx);
        g->setFocalX(radGrad->focalPoint().x() * sx + tx);
        g->setFocalY(radGrad->focalPoint().y() * sy + ty);
        result = g;
    } else {
        return nullptr;
    }

    for (const auto &stop : grad.stops()) {
        auto *s = new QQuickGradientStop(result);
        s->setPosition(stop.first);
        s->setColor(stop.second);
        auto stopsProp = result->stops();
        stopsProp.append(&stopsProp, s);
    }
    result->setSpread(QQuickShapeGradient::SpreadMode(grad.spread()));
    return result;
}

void QQuickItemGenerator::outputShapePath(const PathNodeInfo &info, const QPainterPath *path,
                                          const QQuadPath *quadPath,
                                          QQuickVectorImageGenerator::PathSelector pathSelector,
                                          const QRectF &boundingRect)
{
    Q_ASSERT(path || quadPath);

    if (Q_UNLIKELY(errorState()))
        return;

    auto *shape = qobject_cast<QQuickShape *>(currentItem());
    if (!shape)
        return;

    const bool invalidGradientBounds = info.strokeGrad.coordinateMode() == QGradient::ObjectMode
            && (qFuzzyIsNull(boundingRect.width()) || qFuzzyIsNull(boundingRect.height()));
    const QColor strokeColor = info.strokeStyle.color.defaultValue().value<QColor>();
    const bool noPen = (strokeColor == QColorConstants::Transparent || !strokeColor.isValid())
            && !info.strokeStyle.color.isAnimated() && !info.strokeStyle.opacity.isAnimated()
            && (info.strokeGrad.type() == QGradient::NoGradient || invalidGradientBounds);
    if (pathSelector == QQuickVectorImageGenerator::StrokePath && noPen)
        return;

    const QColor fillColor = info.fillColor.defaultValue().value<QColor>();
    const bool noFill = info.grad.type() == QGradient::NoGradient
            && fillColor == QColorConstants::Transparent && !info.fillColor.isAnimated()
            && !info.fillOpacity.isAnimated();
    if (pathSelector == QQuickVectorImageGenerator::FillPath && noFill)
        return;

    if (noPen && noFill)
        return;

    auto *shapePath = new QQuickShapePath;
    shapePath->setParent(shape);

    if (!info.nodeId.isEmpty()) {
        switch (pathSelector) {
        case QQuickVectorImageGenerator::FillPath:
            shapePath->setObjectName(u"svg_fill_path:"_s + info.nodeId);
            break;
        case QQuickVectorImageGenerator::StrokePath:
            shapePath->setObjectName(u"svg_stroke_path:"_s + info.nodeId);
            break;
        case QQuickVectorImageGenerator::FillAndStroke:
            shapePath->setObjectName(u"svg_path:"_s + info.nodeId);
            break;
        }
    }

    if (noPen || !(pathSelector & QQuickVectorImageGenerator::StrokePath)) {
        shapePath->setStrokeColor(QColorConstants::Transparent);
    } else {
        if (info.strokeGrad.type() != QGradient::NoGradient && !invalidGradientBounds) {
            QRectF coordinateSys = info.strokeGrad.coordinateMode() == QGradient::ObjectMode
                    ? boundingRect
                    : QRectF(0.0, 0.0, 1.0, 1.0);
            shapePath->setStrokeGradient(
                    createShapeGradient(info.strokeGrad, coordinateSys, shapePath));
        } else {
            shapePath->setStrokeColor(strokeColor);
        }
        shapePath->setStrokeWidth(info.strokeStyle.width.defaultValue().toReal());
        shapePath->setCapStyle(QQuickShapePath::CapStyle(info.strokeStyle.lineCapStyle));
        shapePath->setJoinStyle(QQuickShapePath::JoinStyle(info.strokeStyle.lineJoinStyle));
        shapePath->setMiterLimit(info.strokeStyle.miterLimit);
    }

    QTransform fillTransform = info.fillTransform;
    if (!(pathSelector & QQuickVectorImageGenerator::FillPath)) {
        shapePath->setFillColor(QColorConstants::Transparent);
    } else if (info.grad.type() != QGradient::NoGradient) {
        if (info.grad.coordinateMode() == QGradient::ObjectMode) {
            QTransform objectToUserSpace;
            objectToUserSpace.translate(boundingRect.x(), boundingRect.y());
            objectToUserSpace.scale(boundingRect.width(), boundingRect.height());
            fillTransform *= objectToUserSpace;
        }
        shapePath->setFillGradient(
                createShapeGradient(info.grad, QRectF(0.0, 0.0, 1.0, 1.0), shapePath));
    } else {
        shapePath->setFillColor(fillColor);
    }
    if (!fillTransform.isIdentity())
        shapePath->setFillTransform(QMatrix4x4(fillTransform));

    shapePath->setFillRule(
            QQuickShapePath::FillRule(path ? path->fillRule() : quadPath->fillRule()));

    if (quadPath)
        shapePath->setPathHints(QQuickShapePath::PathHints(int(quadPath->pathHints())));

    const QString svgString = path ? QQuickVectorImageGenerator::Utils::toSvgString(*path)
                                   : QQuickVectorImageGenerator::Utils::toSvgString(*quadPath);
    auto *pathSvg = new QQuickPathSvg(shapePath);
    pathSvg->setPath(svgString);
    auto pathElems = shapePath->pathElements();
    pathElems.append(&pathElems, pathSvg);

    auto shapeData = shape->data();
    shapeData.append(&shapeData, shapePath);
}

void QQuickItemGenerator::generateImageNode(const ImageNodeInfo &info)
{
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
        return;

    QString filePath = info.externalFileReference;
    if (filePath.isEmpty()) {
        filePath =
                QDir::tempPath() + QStringLiteral("/svg_asset_%1.png").arg(info.image.cacheKey());
        if (!info.image.save(filePath))
            qCWarning(lcQuickVectorImage) << "Unable to save image resource" << filePath;
    } else if (QDir::isRelativePath(filePath)) {
        filePath = QFileInfo(fileName()).dir().absoluteFilePath(filePath);
    }

    auto *image = new QQuickImage;
    if (m_context)
        QQmlEngine::setContextForObject(image, m_context);
    pushItem(image);
    generateNodeBase(info);
    image->setX(info.rect.x());
    image->setY(info.rect.y());
    image->setWidth(info.rect.width());
    image->setHeight(info.rect.height());
    auto *parserStatus = qobject_cast<QQmlParserStatus *>(image);
    if (parserStatus)
        parserStatus->classBegin();
    image->setSource(QUrl::fromLocalFile(filePath));
    if (parserStatus)
        parserStatus->componentComplete();
    popItem();
}

void QQuickItemGenerator::generateTextNode(const TextNodeInfo &info)
{
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
        return;

    auto *item = new QQuickItem;
    pushItem(item);
    generateNodeBase(info);

    auto *text = new QQuickText;
    text->setParent(item);
    text->setParentItem(item);

    text->setColor(info.fillColor.defaultValue().value<QColor>());
    text->setFont(info.font);
    text->setText(info.text);
    text->setTextFormat(info.needsRichText ? QQuickText::RichText : QQuickText::StyledText);

    if (info.isTextArea) {
        text->setX(info.position.x());
        text->setY(info.position.y());
        if (info.size.width() > 0)
            text->setWidth(info.size.width());
        if (info.size.height() > 0)
            text->setHeight(info.size.height());
        text->setWrapMode(QQuickText::Wrap);
        text->setClip(true);
    } else {
        QFontMetricsF fm(info.font);
        text->setX(info.position.x());
        text->setY(info.position.y() - fm.ascent());
        switch (info.alignment) {
        case Qt::AlignHCenter:
            text->setHAlign(QQuickText::AlignHCenter);
            break;
        case Qt::AlignRight:
            text->setHAlign(QQuickText::AlignRight);
            break;
        default:
            text->setHAlign(QQuickText::AlignLeft);
            break;
        }
    }

    const QColor strokeColor = info.strokeColor.defaultValue().value<QColor>();
    if (strokeColor != QColorConstants::Transparent || info.strokeColor.isAnimated()) {
        text->setStyleColor(strokeColor);
        text->setStyle(QQuickText::Outline);
    }

    popItem();
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
