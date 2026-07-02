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
#include <private/qquickshadereffect_p.h>
#include <private/qquickshadereffectsource_p.h>

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
        m_containerSize = info.viewBox.isEmpty() ? QSizeF(info.size) : info.viewBox.size();

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
    if (Q_UNLIKELY(errorState()))
        return false;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateStructureNode(info); });
        return true;
    }

    if (!isNodeVisible(info))
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
        QQuickItem *item = popItem();
        if (!info.maskId.isEmpty())
            generateMask(item, info);
    }

    return true;
}

void QQuickItemGenerator::generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    if (Q_UNLIKELY(errorState()))
        return;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append(
                [this, info, overrideBoundingRect]() { generatePath(info, overrideBoundingRect); });
        return;
    }

    if (!isNodeVisible(info))
        return;

    if (qobject_cast<QQuickShape *>(currentItem())) {
        optimizePaths(info, overrideBoundingRect);
    } else {
        auto *shape = createShapeContainer();
        pushItem(shape);
        generateNodeBase(info);
        optimizePaths(info, overrideBoundingRect);
        QQuickItem *item = popItem();
        if (!info.maskId.isEmpty())
            generateMask(item, info);
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
    if (Q_UNLIKELY(errorState()))
        return;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateImageNode(info); });
        return;
    }

    if (!isNodeVisible(info))
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
    parserStatus->classBegin();
    image->setSource(QUrl::fromLocalFile(filePath));
    parserStatus->componentComplete();
    QQuickItem *item = popItem();
    if (!info.maskId.isEmpty())
        generateMask(item, info);
}

void QQuickItemGenerator::generateTextNode(const TextNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateTextNode(info); });
        return;
    }

    if (!isNodeVisible(info))
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

    QQuickItem *textItem = popItem();
    if (!info.maskId.isEmpty())
        generateMask(textItem, info);
}

void QQuickItemGenerator::generateNode(const NodeInfo &info)
{
    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateNode(info); });
        return;
    }
    qCDebug(lcQuickVectorImage) << "generateNode: not yet implemented";
    Q_UNUSED(info)
}

void QQuickItemGenerator::generateUseNode(const UseNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateUseNode(info); });
        return;
    }

    if (!isNodeVisible(info))
        return;

    if (info.stage == StructureNodeStage::Start) {
        auto *item = new QQuickItem;
        pushItem(item);
        generateNodeBase(info);
    } else {
        QQuickItem *item = popItem();
        if (!info.maskId.isEmpty())
            generateMask(item, info);
    }
}

bool QQuickItemGenerator::generateDefsNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (info.stage == StructureNodeStage::Start) {
        m_defs[info.id] = {};
        m_currentDefsRecord = &m_defs[info.id];
    } else {
        m_currentDefsRecord = nullptr;
        auto it = m_defs.find(info.id);
        if (it != m_defs.end()) {
            auto *container = new QQuickItem;
            m_itemStack.push(container);
            for (const auto &step : *it)
                step();
            m_itemStack.pop();
        }
    }
    return true;
}

void QQuickItemGenerator::generateDefsInstantiationNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateDefsInstantiationNode(info); });
        return;
    }

    if (info.stage != StructureNodeStage::Start)
        return;

    auto it = m_defs.find(info.defsId);
    if (it == m_defs.end()) {
        qCWarning(lcQuickVectorImage)
                << "generateDefsInstantiationNode: unknown defs id:" << info.defsId;
        return;
    }
    for (const auto &step : *it)
        step();
}

bool QQuickItemGenerator::generateMaskNode(const MaskNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateMaskNode(info); });
        return true;
    }

    if (info.stage == StructureNodeStage::Start) {
        if (info.isMaskContentRelativeCoordinates) {
            auto *transformerItem = new QQuickItem;
            pushItem(transformerItem);
        }
        return true;
    }

    generateMaskContainer(info);
    return true;
}

void QQuickItemGenerator::generateMaskContainer(const MaskNodeInfo &info)
{
    QQuickItem *transformer = nullptr;
    QQuickMatrix4x4 *transformerMatrix = nullptr;
    if (info.isMaskContentRelativeCoordinates) {
        transformer = popItem();
        transformerMatrix = new QQuickMatrix4x4(transformer);
        auto xformProp = transformer->transform();
        xformProp.append(&xformProp, transformerMatrix);
    }

    auto *container = currentItem();
    m_maskDefs[info.id] = { container,
                            info.maskRect,
                            info.isMaskRectRelativeCoordinates,
                            info.isMaskContentRelativeCoordinates,
                            transformer,
                            transformerMatrix };
}

void QQuickItemGenerator::generateMask(QQuickItem *item, const NodeInfo &info)
{
    auto it = m_maskDefs.find(info.maskId);
    if (it == m_maskDefs.end()) {
        qCWarning(lcQuickVectorImage) << "generateMask: unknown mask id:" << info.maskId;
        return;
    }
    MaskDef &maskDef = *it;
    QQuickItem *parentItem = item->parentItem();

    const qreal w = item->width();
    const qreal h = item->height();

    const QRectF svgBounds =
            info.bounds.isNull() ? QRectF(item->x(), item->y(), w, h) : info.bounds;
    QRectF svgMaskRect;
    if (maskDef.isMaskRectRelativeCoordinates) {
        svgMaskRect = QRectF(maskDef.maskRect.x() * svgBounds.width() + svgBounds.x(),
                             maskDef.maskRect.y() * svgBounds.height() + svgBounds.y(),
                             maskDef.maskRect.width() * svgBounds.width(),
                             maskDef.maskRect.height() * svgBounds.height());
    } else {
        svgMaskRect = maskDef.maskRect;
    }

    if (maskDef.isMaskContentRelativeCoordinates && maskDef.transformerMatrix) {
        QMatrix4x4 mat;
        mat.translate(svgBounds.x(), svgBounds.y());
        mat.scale(svgBounds.width(), svgBounds.height(), 1.0f);
        maskDef.transformerMatrix->setMatrix(mat);
    }

    maskDef.container->setParent(m_rootItem);
    maskDef.container->setParentItem(m_rootItem);
    const qreal containerW = m_containerSize.width() > 0 ? m_containerSize.width() : w;
    const qreal containerH = m_containerSize.height() > 0 ? m_containerSize.height() : h;
    maskDef.container->setWidth(containerW);
    maskDef.container->setHeight(containerH);

    static const QUrl maskShaderUrl(
            u"qrc:/qt-project.org/quickvectorimage/helpers/shaders_ng/genericmask.frag.qsb"_s);

    auto *maskSES = new QQuickShaderEffectSource;
    maskSES->setSourceItem(maskDef.container);
    maskSES->setHideSource(true);
    maskSES->setVisible(false);
    maskSES->setParent(m_rootItem);
    maskSES->setParentItem(m_rootItem);
    maskSES->setSourceRect(svgMaskRect);
    maskSES->setWidth(svgMaskRect.width());
    maskSES->setHeight(svgMaskRect.height());

    auto *itemSES = new QQuickShaderEffectSource;
    itemSES->setSourceItem(item);
    itemSES->setHideSource(true);
    itemSES->setVisible(false);
    itemSES->setParent(m_rootItem);
    itemSES->setParentItem(m_rootItem);
    itemSES->setSourceRect(svgMaskRect);
    itemSES->setWidth(svgMaskRect.width());
    itemSES->setHeight(svgMaskRect.height());

    auto *shaderEffect = new QQuickShaderEffect;
    if (m_context)
        QQmlEngine::setContextForObject(shaderEffect, m_context);
    auto *parserStatus = qobject_cast<QQmlParserStatus *>(shaderEffect);
    parserStatus->classBegin();
    shaderEffect->setFragmentShader(maskShaderUrl);
    shaderEffect->setProperty("source", QVariant::fromValue<QQuickItem *>(itemSES));
    shaderEffect->setProperty("maskSource", QVariant::fromValue<QQuickItem *>(maskSES));
    shaderEffect->setProperty("isAlpha", info.isMaskAlpha);
    shaderEffect->setProperty("isInverted", info.isMaskInverted);
    parserStatus->componentComplete();

    if (!info.isDefaultOpacity)
        shaderEffect->setOpacity(info.opacity.defaultValue().toReal());

    shaderEffect->setTransformOrigin(QQuickItem::TopLeft);
    shaderEffect->setParent(parentItem);
    shaderEffect->setParentItem(parentItem);
    shaderEffect->setWidth(svgMaskRect.width());
    shaderEffect->setHeight(svgMaskRect.height());

    if (!info.isDefaultTransform) {
        const QTransform elementXf = info.transform.defaultValue().value<QTransform>();
        QMatrix4x4 mat(elementXf);
        mat.translate(svgMaskRect.x(), svgMaskRect.y());
        auto *matrix = new QQuickMatrix4x4(shaderEffect);
        matrix->setMatrix(mat);
        auto xformProp = shaderEffect->transform();
        xformProp.append(&xformProp, matrix);
        shaderEffect->setX(0);
        shaderEffect->setY(0);
    } else {
        shaderEffect->setX(svgMaskRect.x());
        shaderEffect->setY(svgMaskRect.y());
    }
}

void QQuickItemGenerator::generateFilterNode(const FilterNodeInfo &info)
{
    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateFilterNode(info); });
        return;
    }
    qCDebug(lcQuickVectorImage) << "generateFilterNode: not yet implemented";
    Q_UNUSED(info)
}

bool QQuickItemGenerator::generateMarkerNode(const MarkerNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateMarkerNode(info); });
        return true;
    }
    qCDebug(lcQuickVectorImage) << "generateMarkerNode: not yet implemented";
    Q_UNUSED(info)
    return true;
}

bool QQuickItemGenerator::generatePatternNode(const PatternNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generatePatternNode(info); });
        return true;
    }
    qCDebug(lcQuickVectorImage) << "generatePatternNode: not yet implemented";
    Q_UNUSED(info)
    return true;
}

QT_END_NAMESPACE
