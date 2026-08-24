// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemgenerator_p.h"
#include "qquicknodeinfo_p.h"
#include <QtQuickVectorImageHelpers/private/qquickitemspy_p.h>
#include <QtQuickVectorImageHelpers/private/qquicktransformgroup_p.h>
#include <QtQuickVectorImageHelpers/private/qquickpathinterpolated_p.h>
#include "qquicktransformsource_p.h"
#include "qquickanimationrootitem_p.h"

#include <private/qquickitem_p.h>
#include <private/qquicktranslate_p.h>
#include <private/qquickshape_p.h>
#include <private/qquickpath_p.h>
#include <private/qquickimage_p.h>
#include <private/qquicktext_p.h>
#include <private/qquickrectangle_p.h>
#include <private/qquickshadereffect_p.h>
#include <private/qquickshadereffectsource_p.h>
#include <private/qquickmultieffect_p.h>
#include <private/qquickanimation_p.h>
#include <private/qquickanimation_p_p.h>
#include <private/qquickpathinterpolator_p.h>

#include "utils_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtGui/qfontmetrics.h>
#include <QtGui/private/qbezier_p.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class OpacityAppliedColor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor baseColor READ baseColor WRITE setBaseColor)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
    OpacityAppliedColor(std::function<void(const QColor &)> setter, const QColor &baseColor,
                        qreal opacity, QObject *parent)
        : QObject(parent), m_setter(std::move(setter)), m_baseColor(baseColor), m_opacity(opacity)
    {
        apply();
    }

    QColor baseColor() const { return m_baseColor; }
    void setBaseColor(const QColor &baseColor)
    {
        if (m_baseColor == baseColor)
            return;
        m_baseColor = baseColor;
        apply();
    }

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity)
    {
        if (m_opacity == opacity)
            return;
        m_opacity = opacity;
        apply();
    }

private:
    void apply()
    {
        QColor color = m_baseColor;
        color.setAlphaF(m_opacity);
        m_setter(color);
    }

    std::function<void(const QColor &)> m_setter;
    QColor m_baseColor;
    qreal m_opacity;
};

class TransformLinker : public QObject
{
    Q_OBJECT
public:
    TransformLinker(QQuickTransformSource *source, QQuickMatrix4x4 *target, QObject *parent)
        : QObject(parent), m_source(source), m_target(target)
    {
        connect(source, &QQuickTransformSource::transformMatrixChanged, this,
                &TransformLinker::updateMatrix);
        updateMatrix();
    }

public Q_SLOTS:
    void updateMatrix() { m_target->setMatrix(m_source->transformMatrix()); }

private:
    QQuickTransformSource *m_source;
    QQuickMatrix4x4 *m_target;
};

class QQuickCallbackAnimationJob : public QAbstractAnimationJob
{
public:
    explicit QQuickCallbackAnimationJob(std::function<void()> function)
        : m_function(std::move(function))
    {
    }

protected:
    void updateState(State newState, State oldState) override
    {
        Q_UNUSED(oldState);
        if (newState == Running && m_function)
            m_function();
    }

private:
    std::function<void()> m_function;
};

class QQuickFunctionAction : public QQuickAbstractAnimation
{
public:
    explicit QQuickFunctionAction(std::function<void()> function, QObject *parent = nullptr)
        : QQuickAbstractAnimation(parent), m_function(std::move(function))
    {
    }

protected:
    QAbstractAnimationJob *transition(QQuickStateActions &, QQmlProperties &, TransitionDirection,
                                      QObject * = nullptr) override
    {
        return initInstance(new QQuickCallbackAnimationJob(m_function));
    }

private:
    std::function<void()> m_function;
};

QQuickItemGenerator::QQuickItemGenerator(const QString &fileName,
                                         QQuickVectorImageGenerator::GeneratorFlags flags,
                                         QQmlContext *context)
    : QQuickGenerator(fileName, flags), m_context(context)
{
}

QQuickItemGenerator::~QQuickItemGenerator() = default;

void QQuickItemGenerator::setAnimationProvider(std::unique_ptr<QQuickGeneratorAnimationProvider> provider)
{
    m_animationProvider = std::move(provider);
}

QQuickItem *QQuickItemGenerator::takeRootItem()
{
    QQuickItem *item = m_rootItem;
    m_rootItem = nullptr;
    return item;
}

QList<std::function<void()>> *QQuickItemGenerator::activeRecord() const
{
    if (m_currentDefsRecord)
        return m_currentDefsRecord;
    return m_currentMarkerRecord;
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
        if (!info.isDefaultOpacity && !info.opacity.isAnimated())
            item->setOpacity(info.opacity.defaultValue().toReal());
    }

    if (!info.isDefaultTransform && !info.transform.isAnimated()) {
        QTransform transform = info.transform.defaultValue().value<QTransform>();
        auto *matrix = new QQuickMatrix4x4(item);
        matrix->setMatrix(QMatrix4x4(transform));
        auto transformProp = item->transform();
        transformProp.append(&transformProp, matrix);
    }

    if (info.maskId.isEmpty())
        generateItemAnimations(item, info);

    return info.id;
}

bool QQuickItemGenerator::generateRootNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (info.stage == StructureNodeStage::Start) {
        auto *root = new QQuickAnimationRootItem;
        if (info.size.width() > 0)
            root->setImplicitWidth(info.size.width());
        if (info.size.height() > 0)
            root->setImplicitHeight(info.size.height());
        m_rootItem = root;
        m_containerSize = info.viewBox.isEmpty() ? QSizeF(info.size) : info.viewBox.size();

        if (!isNodeVisible(info))
            return false;

        if (!info.viewBox.isEmpty() && info.size.width() > 0 && info.size.height() > 0) {
            auto transformProp = root->transform();
            if (!qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y())) {
                auto *translate = new QQuickTranslate(root);
                translate->setX(-info.viewBox.x());
                translate->setY(-info.viewBox.y());
                transformProp.append(&transformProp, translate);
            }
            auto *scale = new QQuickScale(root);
            scale->setXScale(info.size.width() / info.viewBox.width());
            scale->setYScale(info.size.height() / info.viewBox.height());
            transformProp.append(&transformProp, scale);
        }

        m_topLevelScaleSpy = new QQuickItemSpy(root);
        m_topLevelScaleSpy->setWidth(1);
        m_topLevelScaleSpy->setHeight(1);
        m_topLevelScaleSpy->setVisible(false);

        pushItem(root);

        bool scopePushed = false;
        if (m_animationProvider && info.timelineInfo) {
            if (auto *master = m_animationProvider->enterTimelineScope(root, *info.timelineInfo))
                root->addMasterAnimation(master);
            scopePushed = true;
        }
        m_scopePushed.push(scopePushed);

        generateNodeBase(info);
    } else {
        for (const PendingLinkedTransform &pending : m_pendingLinkedTransforms) {
            Q_ASSERT(pending.item);
            auto it = m_transformSourceItems.constFind(pending.transformReferenceId);
            if (it == m_transformSourceItems.cend()) {
                qCWarning(lcQuickVectorImage)
                        << "generateRootNode: transformReferenceId does not refer to a "
                           "transform source item:"
                        << pending.transformReferenceId;
                continue;
            }
            new TransformLinker(it.value(), pending.linkedMatrix, pending.item);
        }
        m_pendingLinkedTransforms.clear();

        if (m_scopePushed.pop() && m_animationProvider)
            m_animationProvider->exitTimelineScope();
        popItem();
    }

    return true;
}

bool QQuickItemGenerator::generateStructureNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (auto *rec = activeRecord()) {
        rec->append([this, info]() { generateStructureNode(info); });
        return true;
    }

    if (!isNodeVisible(info))
        return false;

    if (info.stage == StructureNodeStage::Start) {
        if (!info.clipBox.isEmpty()) {
            auto *clipItem = new QQuickItem;
            clipItem->setWidth(info.clipBox.width());
            clipItem->setHeight(info.clipBox.height());
            clipItem->setClip(true);
            pushItem(clipItem);
        }

        QQuickItem *item = nullptr;
        if (!info.forceSeparatePaths && info.isPathContainer) {
            item = createShapeContainer();
        } else {
            if (m_animationProvider && !info.customItemType.isEmpty())
                item = m_animationProvider->createCustomItem(info.customItemType);
            if (!item)
                item = new QQuickItem;

            if (!info.viewBox.isEmpty()) {
                auto transformProp = item->transform();
                if (!qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y())) {
                    auto *translate = new QQuickTranslate(item);
                    translate->setX(-info.viewBox.x());
                    translate->setY(-info.viewBox.y());
                    transformProp.append(&transformProp, translate);
                }
                auto *scale = new QQuickScale(item);
                scale->setXScale(info.size.width() / info.viewBox.width());
                scale->setYScale(info.size.height() / info.viewBox.height());
                transformProp.append(&transformProp, scale);
            }
        }

        bool scopePushed = false;
        if (m_animationProvider && info.timelineInfo) {
            if (auto *master = m_animationProvider->enterTimelineScope(item, *info.timelineInfo)) {
                if (auto *root = qobject_cast<QQuickAnimationRootItem *>(m_rootItem))
                    root->addMasterAnimation(master);
            }
            scopePushed = true;
        }
        m_scopePushed.push(scopePushed);

        if (!info.id.isEmpty()) {
            if (auto *source = qobject_cast<QQuickTransformSource *>(item))
                m_transformSourceItems.insert(info.id, source);
        }

        pushItem(item);
        generateNodeBase(info);
    } else {
        if (m_scopePushed.pop() && m_animationProvider)
            m_animationProvider->exitTimelineScope();

        QQuickItem *item = popItem();
        QQuickItem *effectItem = item;
        if (!info.filterId.isEmpty())
            effectItem = generateFilter(item, info);
        if (!info.maskId.isEmpty())
            generateMask(effectItem ? effectItem : item, info);

        if (!info.clipBox.isEmpty())
            popItem();
    }

    return true;
}

void QQuickItemGenerator::generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    if (Q_UNLIKELY(errorState()))
        return;

    if (auto *rec = activeRecord()) {
        rec->append(
                [this, info, overrideBoundingRect]() { generatePath(info, overrideBoundingRect); });
        return;
    }

    if (!isNodeVisible(info))
        return;

    if (qobject_cast<QQuickShape *>(currentItem()) && info.markerStartId.isEmpty()
        && info.markerMidId.isEmpty() && info.markerEndId.isEmpty() && info.filterId.isEmpty()) {
        optimizePaths(info, overrideBoundingRect);
    } else {
        auto *shape = createShapeContainer();
        pushItem(shape);
        generateNodeBase(info);
        optimizePaths(info, overrideBoundingRect);
        QQuickItem *item = popItem();
        if (!info.markerStartId.isEmpty() || !info.markerMidId.isEmpty()
            || !info.markerEndId.isEmpty()) {
            generateMarkers(info);
        }
        QQuickItem *effectItem = item;
        if (!info.filterId.isEmpty())
            effectItem = generateFilter(item, info);
        if (!info.maskId.isEmpty())
            generateMask(effectItem ? effectItem : item, info);
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
    const bool noFill = info.grad.type() == QGradient::NoGradient && info.patternId.isEmpty()
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
        if (info.strokeStyle.cosmetic)
            shapePath->setCosmeticStroke(true);
        if (!info.strokeStyle.dashArray.isEmpty()) {
            shapePath->setStrokeStyle(QQuickShapePath::DashLine);
            shapePath->setDashPattern(info.strokeStyle.dashArray);
            shapePath->setDashOffset(info.strokeStyle.dashOffset.defaultValue().toReal());
        }
    }

    QTransform fillTransform = info.fillTransform;
    if (!(pathSelector & QQuickVectorImageGenerator::FillPath)) {
        shapePath->setFillColor(QColorConstants::Transparent);
    } else if (!info.patternId.isEmpty()) {
        generatePattern(shapePath, info, boundingRect, fillTransform);
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

    if (info.trim.enabled) {
        shapePath->trim()->setStart(info.trim.start.defaultValue().toReal());
        shapePath->trim()->setEnd(info.trim.end.defaultValue().toReal());
        shapePath->trim()->setOffset(info.trim.offset.defaultValue().toReal());
    }

    QQuickPathInterpolated *pathInterpolatedObj = nullptr;
    QQuickAnimatedProperty::PropertyAnimation pathIndexAnim;

    const bool pathAnimated = info.path.isAnimated() && info.path.animationCount() > 0
            && !(info.path.animation(0).startOffset == 0 && info.path.animation(0).isConstant());

    if (pathAnimated) {
        const auto &pathAnim = info.path.animation(0);
        QStringList svgPaths;
        QMap<int, QVariant> indexFrames;
        QString lastSvg;
        int pathIdx = -1;
        for (auto it = pathAnim.frames.constBegin(); it != pathAnim.frames.constEnd(); ++it) {
            const QString svg =
                    QQuickVectorImageGenerator::Utils::toSvgString(it->value<QPainterPath>());
            if (svg != lastSvg) {
                svgPaths.append(svg);
                ++pathIdx;
                lastSvg = svg;
            }
            indexFrames.insert(it.key(), QVariant::fromValue(qreal(pathIdx)));
        }

        if (svgPaths.size() > 1) {
            auto *interpolated = new QQuickPathInterpolated(shapePath);
            interpolated->setSvgPaths(svgPaths);
            auto pathElems = shapePath->pathElements();
            pathElems.append(&pathElems, interpolated);
            pathIndexAnim = pathAnim;
            pathIndexAnim.frames = indexFrames;
            pathInterpolatedObj = interpolated;
        }
    }

    if (!pathInterpolatedObj) {
        const QString svgString = path ? QQuickVectorImageGenerator::Utils::toSvgString(*path)
                                       : QQuickVectorImageGenerator::Utils::toSvgString(*quadPath);
        auto *pathSvg = new QQuickPathSvg(shapePath);
        pathSvg->setPath(svgString);
        auto pathElems = shapePath->pathElements();
        pathElems.append(&pathElems, pathSvg);
    }

    auto shapeData = shape->data();
    shapeData.append(&shapeData, shapePath);

    const bool hasPathAnim = pathInterpolatedObj != nullptr;
    const bool hasFillColorAnim = info.fillColor.isAnimated();
    const bool hasFillOpacityAnim = info.fillOpacity.isAnimated();
    const bool hasStrokeColorAnim = info.strokeStyle.color.isAnimated();
    const bool hasStrokeOpacityAnim = info.strokeStyle.opacity.isAnimated();
    const bool hasStrokeWidthAnim = info.strokeStyle.width.isAnimated();
    const bool hasDashOffsetAnim = info.strokeStyle.dashOffset.isAnimated();
    const bool hasTrimStartAnim = info.trim.start.isAnimated();
    const bool hasTrimEndAnim = info.trim.end.isAnimated();
    const bool hasTrimOffsetAnim = info.trim.offset.isAnimated();

    if (!hasPathAnim && !hasFillColorAnim && !hasFillOpacityAnim && !hasStrokeColorAnim
        && !hasStrokeOpacityAnim && !hasStrokeWidthAnim && !hasDashOffsetAnim && !hasTrimStartAnim
        && !hasTrimEndAnim && !hasTrimOffsetAnim) {
        return;
    }

    auto identity = [](const QVariant &v) { return v; };

    if (hasPathAnim) {
        bindPropertyAnimation(pathInterpolatedObj, QStringLiteral("factor"), pathIndexAnim,
                              identity);
    }

    if (hasTrimStartAnim || hasTrimEndAnim || hasTrimOffsetAnim) {
        auto identity = [](const QVariant &v) { return v; };
        if (hasTrimStartAnim) {
            bindAnimatedProperty(shapePath->trim(), QStringLiteral("start"), info.trim.start,
                                 identity);
        }
        if (hasTrimEndAnim)
            bindAnimatedProperty(shapePath->trim(), QStringLiteral("end"), info.trim.end, identity);
        if (hasTrimOffsetAnim) {
            bindAnimatedProperty(shapePath->trim(), QStringLiteral("offset"), info.trim.offset,
                                 identity);
        }
    }

    if (hasFillColorAnim || hasFillOpacityAnim) {
        bindColorWithOpacity(shapePath, QStringLiteral("fillColor"), info.fillColor,
                             info.fillOpacity,
                             [shapePath](const QColor &color) { shapePath->setFillColor(color); });
    }

    if (hasStrokeColorAnim || hasStrokeOpacityAnim) {
        bindColorWithOpacity(shapePath, QStringLiteral("strokeColor"), info.strokeStyle.color,
                             info.strokeStyle.opacity, [shapePath](const QColor &color) {
                                 shapePath->setStrokeColor(color);
                             });
    }

    if (hasStrokeWidthAnim) {
        bindAnimatedProperty(shapePath, QStringLiteral("strokeWidth"), info.strokeStyle.width,
                             identity);
    }

    if (hasDashOffsetAnim) {
        bindAnimatedProperty(shapePath, QStringLiteral("dashOffset"), info.strokeStyle.dashOffset,
                             identity);
    }
}

void QQuickItemGenerator::generateImageNode(const ImageNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return;

    if (auto *rec = activeRecord()) {
        rec->append([this, info]() { generateImageNode(info); });
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
    {
        QQuickItem *effectItem = item;
        if (!info.filterId.isEmpty())
            effectItem = generateFilter(item, info);
        if (!info.maskId.isEmpty())
            generateMask(effectItem ? effectItem : item, info);
    }
}

void QQuickItemGenerator::generateTextNode(const TextNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return;

    if (auto *rec = activeRecord()) {
        rec->append([this, info]() { generateTextNode(info); });
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
        if (info.size.width() > 0) {
            text->setWidth(info.size.width());
            text->setWrapMode(QQuickText::Wrap);
        }
        if (info.size.height() > 0)
            text->setHeight(info.size.height());
        text->setClip(true);
    } else {
        const qreal anchorX = info.position.x();
        const qreal anchorY = info.position.y();
        text->setX(anchorX);
        auto updateY = [text, anchorY]() { text->setY(anchorY - text->baselineOffset()); };
        QObject::connect(text, &QQuickItem::baselineOffsetChanged, text, updateY);
        updateY();
        if (info.alignment == Qt::AlignHCenter || info.alignment == Qt::AlignRight) {
            const Qt::Alignment alignment = info.alignment;
            auto updateX = [text, anchorX, alignment](qreal contentWidth) {
                text->setX(alignment == Qt::AlignHCenter ? anchorX - qRound(contentWidth / 2)
                                                         : anchorX - contentWidth);
            };
            QObject::connect(text, &QQuickText::contentWidthChanged, text, updateX);
            updateX(text->contentWidth());
        }
    }

    const QColor strokeColor = info.strokeColor.defaultValue().value<QColor>();
    if (strokeColor != QColorConstants::Transparent || info.strokeColor.isAnimated()) {
        text->setStyleColor(strokeColor);
        text->setStyle(QQuickText::Outline);
    }

    const bool hasFillColorAnim = info.fillColor.isAnimated();
    const bool hasFillOpacityAnim = info.fillOpacity.isAnimated();
    const bool hasStrokeColorAnim = info.strokeColor.isAnimated();
    const bool hasStrokeOpacityAnim = info.strokeOpacity.isAnimated();

    if (hasFillColorAnim || hasFillOpacityAnim) {
        bindColorWithOpacity(text, QStringLiteral("color"), info.fillColor, info.fillOpacity,
                             [text](const QColor &color) { text->setColor(color); });
    }

    if (hasStrokeColorAnim || hasStrokeOpacityAnim) {
        bindColorWithOpacity(text, QStringLiteral("styleColor"), info.strokeColor,
                             info.strokeOpacity,
                             [text](const QColor &color) { text->setStyleColor(color); });
    }

    QQuickItem *textItem = popItem();
    {
        QQuickItem *effectItem = textItem;
        if (!info.filterId.isEmpty())
            effectItem = generateFilter(textItem, info);
        if (!info.maskId.isEmpty())
            generateMask(effectItem ? effectItem : textItem, info);
    }
}

void QQuickItemGenerator::generateNode(const NodeInfo &info)
{
    if (auto *rec = activeRecord()) {
        rec->append([this, info]() { generateNode(info); });
        return;
    }
    qCDebug(lcQuickVectorImage) << "generateNode: not yet implemented";
    Q_UNUSED(info)
}

void QQuickItemGenerator::generateUseNode(const UseNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return;

    if (auto *rec = activeRecord()) {
        rec->append([this, info]() { generateUseNode(info); });
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
        QQuickItem *effectItem = item;
        if (!info.filterId.isEmpty())
            effectItem = generateFilter(item, info);
        if (!info.maskId.isEmpty())
            generateMask(effectItem ? effectItem : item, info);
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

    if (auto *rec = activeRecord()) {
        rec->append([this, info]() { generateDefsInstantiationNode(info); });
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

    if (auto *rec = activeRecord()) {
        rec->append([this, info]() { generateMaskNode(info); });
        return true;
    }

    if (info.stage == StructureNodeStage::Start) {
        if (!info.isDefaultTransform) {
            auto *xfItem = new QQuickItem;
            xfItem->setTransformOrigin(QQuickItem::TopLeft);
            auto *matrix = new QQuickMatrix4x4(xfItem);
            matrix->setMatrix(QMatrix4x4(info.transform.defaultValue().value<QTransform>()));
            auto transformProp = xfItem->transform();
            transformProp.append(&transformProp, matrix);
            pushItem(xfItem);
        }
        if (info.isMaskContentRelativeCoordinates) {
            auto *transformerItem = new QQuickItem;
            pushItem(transformerItem);
        }
        return true;
    }

    generateMaskContainer(info);
    return true;
}

static QQuickShaderEffectSource *makeSES(QQuickItem *item, const QRectF &rect, QQuickItem *parent)
{
    auto *ses = new QQuickShaderEffectSource;
    ses->setSourceItem(item);
    ses->setWidth(rect.width());
    ses->setHeight(rect.height());
    ses->setVisible(false);
    ses->setParent(parent);
    ses->setParentItem(parent);
    return ses;
}

static QQuickShaderEffect *makeFilterEffect(QQuickItem *inputItem, const QUrl &shader,
                                            QQmlContext *context, QQuickItem *parent)
{
    auto *effect = new QQuickShaderEffect;
    if (context)
        QQmlEngine::setContextForObject(effect, context);
    auto *parserStatus = qobject_cast<QQmlParserStatus *>(effect);
    parserStatus->classBegin();
    effect->bindableWidth().setBinding([inputItem] { return inputItem->width(); });
    effect->bindableHeight().setBinding([inputItem] { return inputItem->height(); });
    effect->setVisible(false);
    effect->setParent(parent);
    effect->setParentItem(parent);
    effect->setFragmentShader(shader);
    return effect;
}

static QQuickShaderEffectSource *makeEffectSES(QQuickShaderEffect *effect, const QRectF &stepRect,
                                               const QRectF &filterRect, QQuickItem *parent)
{
    auto *wrapper = new QQuickItem;
    wrapper->setWidth(stepRect.width());
    wrapper->setHeight(stepRect.height());
    wrapper->setClip(true);
    wrapper->setParent(parent);
    wrapper->setParentItem(parent);
    effect->setVisible(true);
    effect->setParentItem(wrapper);
    effect->setX(filterRect.x() - stepRect.x());
    effect->setY(filterRect.y() - stepRect.y());
    auto *ses = makeSES(wrapper, stepRect, parent);
    ses->setHideSource(true);
    return ses;
}

void QQuickItemGenerator::bindTextureSize(QQuickShaderEffectSource *ses)
{
    auto *spy = new QQuickItemSpy(ses);
    spy->setVisible(false);

    auto updateTextureSize = [ses, spy]() {
        spy->setWidth(ses->width());
        spy->setHeight(ses->height());
        const QSizeF textureSize = spy->requiredTextureSize();
        ses->setTextureSize(QSize(qRound(textureSize.width()), qRound(textureSize.height())));
    };
    QObject::connect(spy, &QQuickItemSpy::requiredTextureSizeChanged, ses, updateTextureSize);
    QObject::connect(ses, &QQuickItem::widthChanged, ses, updateTextureSize);
    QObject::connect(ses, &QQuickItem::heightChanged, ses, updateTextureSize);
    updateTextureSize();
}

void QQuickItemGenerator::bindPatternTextureSize(QQuickShaderEffectSource *ses)
{
    if (!m_topLevelScaleSpy)
        return;

    QQuickItemSpy *scaleSpy = m_topLevelScaleSpy;
    auto updateTextureSize = [ses, scaleSpy]() {
        const QSizeF unitScale = scaleSpy->requiredTextureSize();
        const qreal width = ses->width() * unitScale.width();
        const qreal height = ses->height() * unitScale.height();
        ses->setTextureSize(QSize(qRound(width), qRound(height)));
    };
    QObject::connect(scaleSpy, &QQuickItemSpy::requiredTextureSizeChanged, ses, updateTextureSize);
    QObject::connect(ses, &QQuickItem::widthChanged, ses, updateTextureSize);
    QObject::connect(ses, &QQuickItem::heightChanged, ses, updateTextureSize);
    updateTextureSize();
}

void QQuickItemGenerator::generateMaskContainer(const MaskNodeInfo &info)
{
    QQuickItem *transformer = nullptr;
    QQuickMatrix4x4 *transformerMatrix = nullptr;
    if (info.isMaskContentRelativeCoordinates) {
        transformer = popItem();
        transformerMatrix = new QQuickMatrix4x4(transformer);
        auto transformProp = transformer->transform();
        transformProp.append(&transformProp, transformerMatrix);
    }

    if (!info.isDefaultTransform)
        popItem();

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

    auto *maskSES = makeSES(maskDef.container, svgMaskRect, m_rootItem);
    maskSES->setHideSource(true);
    maskSES->setSourceRect(svgMaskRect);
    bindTextureSize(maskSES);

    auto *itemSES = makeSES(item, svgMaskRect, m_rootItem);
    itemSES->setHideSource(true);
    itemSES->setSmooth(false);
    if (qobject_cast<QQuickShaderEffectSource *>(item))
        itemSES->setSourceRect(QRectF(0, 0, item->width(), item->height()));
    else
        itemSES->setSourceRect(svgMaskRect);
    bindTextureSize(itemSES);

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
        auto transformProp = shaderEffect->transform();
        transformProp.append(&transformProp, matrix);
        shaderEffect->setX(0);
        shaderEffect->setY(0);
    } else if (info.transform.isAnimated()) {
        auto *translate = new QQuickTranslate(shaderEffect);
        translate->setX(svgMaskRect.x());
        translate->setY(svgMaskRect.y());
        auto transformProp = shaderEffect->transform();
        transformProp.append(&transformProp, translate);
        shaderEffect->setX(0);
        shaderEffect->setY(0);
    } else {
        shaderEffect->setX(svgMaskRect.x());
        shaderEffect->setY(svgMaskRect.y());
    }

    generateItemAnimations(shaderEffect, info);
}

void QQuickItemGenerator::generateFilterNode(const FilterNodeInfo &info)
{
    if (auto *rec = activeRecord()) {
        rec->append([this, info]() { generateFilterNode(info); });
        return;
    }
    m_filterDefs[info.id] = info;
}

static QRectF resolveRect(const QRectF &rect, FilterNodeInfo::CoordinateSystem cs,
                          const QRectF &itemBounds)
{
    if (cs == FilterNodeInfo::CoordinateSystem::Relative)
        return QRectF(itemBounds.x() + rect.x() * itemBounds.width(),
                      itemBounds.y() + rect.y() * itemBounds.height(),
                      rect.width() * itemBounds.width(), rect.height() * itemBounds.height());
    return rect;
}

QQuickItem *QQuickItemGenerator::generateFilter(QQuickItem *item, const NodeInfo &info)
{
    auto it = m_filterDefs.find(info.filterId);
    if (it == m_filterDefs.end()) {
        qCWarning(lcQuickVectorImage) << "applyFilter: unknown filter id:" << info.filterId;
        return nullptr;
    }
    const FilterNodeInfo &filterInfo = *it;

    if (filterInfo.steps.isEmpty())
        return nullptr;

    QQuickItem *parentItem = item->parentItem();
    const QRectF itemBounds = info.bounds.isNull()
            ? QRectF(item->x(), item->y(), item->width(), item->height())
            : info.bounds;

    QRectF filterRect = resolveRect(filterInfo.filterRect, filterInfo.csFilterRect, itemBounds);
    if (filterRect.isEmpty())
        filterRect = itemBounds;

    auto *sourceGraphic = makeSES(item, filterRect, m_rootItem);
    sourceGraphic->setHideSource(true);
    sourceGraphic->setSourceRect(filterRect);
    if (filterInfo.wrapMode == QSGTexture::Repeat)
        sourceGraphic->setWrapMode(QQuickShaderEffectSource::Repeat);

    if (filterInfo.csFilterRect == FilterNodeInfo::CoordinateSystem::Relative) {
        const qreal wFactor = filterInfo.filterRect.width();
        const qreal hFactor = filterInfo.filterRect.height();
        sourceGraphic->bindableWidth().setBinding(
                [item, wFactor] { return item->width() * wFactor; });
        sourceGraphic->bindableHeight().setBinding(
                [item, hFactor] { return item->height() * hFactor; });

        const QRectF fractions = filterInfo.filterRect;
        auto updateSourceRect = [sourceGraphic, item, fractions]() {
            sourceGraphic->setSourceRect(QRectF(item->x() + fractions.x() * item->width(),
                                                item->y() + fractions.y() * item->height(),
                                                fractions.width() * item->width(),
                                                fractions.height() * item->height()));
        };
        QObject::connect(item, &QQuickItem::widthChanged, sourceGraphic, updateSourceRect);
        QObject::connect(item, &QQuickItem::heightChanged, sourceGraphic, updateSourceRect);
    }

    bindTextureSize(sourceGraphic);

    QHash<QString, QQuickShaderEffectSource *> namedOutputs;
    const QString sourceAlphaName = filterInfo.id + u"_source_alpha"_s;
    QQuickShaderEffectSource *sourceAlpha = nullptr;
    QQuickShaderEffectSource *lastOutput = sourceGraphic;

    const auto resolveInput = [&](FilterNodeInfo::FilterInput inputType,
                                  const QString &name) -> QQuickShaderEffectSource * {
        switch (inputType) {
        case FilterNodeInfo::FilterInput::SourceColor:
            return sourceGraphic;
        case FilterNodeInfo::FilterInput::SourceAlpha:
            return sourceAlpha;
        case FilterNodeInfo::FilterInput::Name:
            return namedOutputs.value(name, sourceGraphic);
        default:
            return lastOutput;
        }
    };

    QRectF lastStepRect = filterRect;

    for (int i = 0; i < filterInfo.steps.size(); ++i) {
        const FilterNodeInfo::FilterStep &step = filterInfo.steps.at(i);
        const QRectF stepRect =
                (step.filterPrimitiveRect.isNull()
                 || step.csFilterParameter == FilterNodeInfo::CoordinateSystem::MatchFilterRect)
                ? filterRect
                : resolveRect(step.filterPrimitiveRect, step.csFilterParameter, itemBounds);
        QQuickShaderEffectSource *output = nullptr;
        if (step.filterType == FilterNodeInfo::Type::Merge) {
            QList<QQuickShaderEffectSource *> mergeInputs;
            while (i + 1 < filterInfo.steps.size()
                   && filterInfo.steps.at(i + 1).filterType == FilterNodeInfo::Type::MergeNode) {
                ++i;
                const FilterNodeInfo::FilterStep &mergeNode = filterInfo.steps.at(i);
                mergeInputs.append(resolveInput(mergeNode.input1, mergeNode.namedInput1));
            }
            output = generateFilterMerge(mergeInputs, stepRect, filterRect);
        } else {
            auto *input1 = resolveInput(step.input1, step.namedInput1);
            auto *input2 = resolveInput(step.input2, step.namedInput2);
            output = generateFilterStep(step, input1, input2, stepRect, filterRect);
        }

        if (output) {
            bindTextureSize(output);
            lastOutput = output;
            lastStepRect = stepRect;
            if (!step.outputName.isEmpty()) {
                namedOutputs[step.outputName] = output;
                if (step.outputName == sourceAlphaName)
                    sourceAlpha = output;
            }
        }
    }

    if (lastOutput == sourceGraphic)
        return nullptr;

    lastOutput->setParent(parentItem);
    lastOutput->setParentItem(parentItem);
    lastOutput->setVisible(true);

    if (!info.isDefaultOpacity)
        lastOutput->setOpacity(info.opacity.defaultValue().toReal());

    if (!info.isDefaultTransform) {
        const QTransform elementXf = info.transform.defaultValue().value<QTransform>();
        QMatrix4x4 mat(elementXf);
        mat.translate(lastStepRect.x(), lastStepRect.y());
        auto *matrix = new QQuickMatrix4x4(lastOutput);
        matrix->setMatrix(mat);
        auto transformProp = lastOutput->transform();
        transformProp.append(&transformProp, matrix);
    } else {
        lastOutput->setX(lastStepRect.x());
        lastOutput->setY(lastStepRect.y());
    }

    return lastOutput;
}

QQuickShaderEffectSource *QQuickItemGenerator::generateFilterStep(
        const FilterNodeInfo::FilterStep &step, QQuickShaderEffectSource *input1,
        QQuickShaderEffectSource *input2, const QRectF &stepRect, const QRectF &filterRect)
{
    switch (step.filterType) {
    case FilterNodeInfo::Type::Flood:
        return generateFilterFlood(step, input1, stepRect, filterRect);
    case FilterNodeInfo::Type::Offset:
        return generateFilterOffset(step, input1, stepRect);
    case FilterNodeInfo::Type::ColorMatrix:
        return generateFilterColorMatrix(step, input1, stepRect, filterRect);
    case FilterNodeInfo::Type::BlendNormal:
    case FilterNodeInfo::Type::BlendMultiply:
    case FilterNodeInfo::Type::BlendScreen:
    case FilterNodeInfo::Type::BlendDarken:
    case FilterNodeInfo::Type::BlendLighten:
        return generateFilterBlend(step, input1, input2, stepRect, filterRect);
    case FilterNodeInfo::Type::CompositeOver:
    case FilterNodeInfo::Type::CompositeIn:
    case FilterNodeInfo::Type::CompositeOut:
    case FilterNodeInfo::Type::CompositeAtop:
    case FilterNodeInfo::Type::CompositeXor:
    case FilterNodeInfo::Type::CompositeLighter:
    case FilterNodeInfo::Type::CompositeArithmetic:
        return generateFilterComposite(step, input1, input2, stepRect, filterRect);
    case FilterNodeInfo::Type::GaussianBlur:
        return generateFilterGaussianBlur(step, input1, stepRect, filterRect);
    default:
        qCDebug(lcQuickVectorImage) << "generateFilterStep: filter type not yet implemented";
        return nullptr;
    }
}

QQuickShaderEffectSource *
QQuickItemGenerator::generateFilterMerge(const QList<QQuickShaderEffectSource *> &inputs,
                                         const QRectF &stepRect, const QRectF &filterRect)
{
    const int maxNodeCount = 8;
    if (inputs.isEmpty()) {
        qCWarning(lcQuickVectorImage) << "generateFilterMerge: requires at least one input";
        return nullptr;
    }
    if (inputs.size() > maxNodeCount)
        qCWarning(lcQuickVectorImage)
                << "generateFilterMerge: maximum of" << maxNodeCount << "nodes exceeded";

    static const QUrl shader(
            u"qrc:/qt-project.org/quickvectorimage/helpers/shaders_ng/femerge.frag.qsb"_s);
    auto *effect = makeFilterEffect(inputs.first(), shader, m_context, m_rootItem);

    const int count = qMin(maxNodeCount, inputs.size());
    effect->setProperty("sourceCount", count);
    for (int i = 0; i < maxNodeCount; ++i) {
        QQuickItem *src = i < inputs.size() ? inputs.at(i) : nullptr;
        effect->setProperty(QStringLiteral("source%1").arg(i + 1).toLatin1(),
                            QVariant::fromValue(src));
    }

    qobject_cast<QQmlParserStatus *>(effect)->componentComplete();
    return makeEffectSES(effect, stepRect, filterRect, m_rootItem);
}

QQuickShaderEffectSource *
QQuickItemGenerator::generateFilterFlood(const FilterNodeInfo::FilterStep &step,
                                         QQuickShaderEffectSource *input, const QRectF &stepRect,
                                         const QRectF &filterRect)
{
    auto *rect = new QQuickRectangle;
    rect->setColor(step.filterParameter.value<QColor>());
    rect->setVisible(false);
    rect->setParent(m_rootItem);
    rect->setParentItem(m_rootItem);
    if (input) {
        rect->bindableWidth().setBinding([input] { return input->width(); });
        rect->bindableHeight().setBinding([input] { return input->height(); });
    } else {
        rect->setWidth(filterRect.width());
        rect->setHeight(filterRect.height());
    }
    auto *ses = makeSES(rect, stepRect, m_rootItem);
    ses->setSourceRect(QRectF(stepRect.x() - filterRect.x(), stepRect.y() - filterRect.y(),
                              stepRect.width(), stepRect.height()));
    return ses;
}

QQuickShaderEffectSource *
QQuickItemGenerator::generateFilterOffset(const FilterNodeInfo::FilterStep &step,
                                          QQuickShaderEffectSource *input, const QRectF &stepRect)
{
    const QVector2D offset = step.filterParameter.value<QVector2D>();
    const qreal offsetX = step.csFilterParameter == FilterNodeInfo::CoordinateSystem::Relative
            ? offset.x() * stepRect.width()
            : offset.x();
    const qreal offsetY = step.csFilterParameter == FilterNodeInfo::CoordinateSystem::Relative
            ? offset.y() * stepRect.height()
            : offset.y();

    const QRectF offsetRect(0, 0, input->width() + offsetX, input->height() + offsetY);
    auto *offsetSES = makeSES(input, offsetRect, m_rootItem);
    offsetSES->setSourceRect(QRectF(-offsetX, -offsetY, offsetRect.width(), offsetRect.height()));
    bindTextureSize(offsetSES);

    auto *ses = makeSES(offsetSES, stepRect, m_rootItem);
    ses->setSourceRect(QRectF(0, 0, stepRect.width(), stepRect.height()));
    return ses;
}

QQuickShaderEffectSource *
QQuickItemGenerator::generateFilterColorMatrix(const FilterNodeInfo::FilterStep &step,
                                               QQuickShaderEffectSource *input,
                                               const QRectF &stepRect, const QRectF &filterRect)
{
    static const QUrl shader(
            u"qrc:/qt-project.org/quickvectorimage/helpers/shaders_ng/fecolormatrix.frag.qsb"_s);
    auto *effect = makeFilterEffect(input, shader, m_context, m_rootItem);
    effect->setProperty("source", QVariant::fromValue(input));

    const auto matrix = step.filterParameter.value<QGenericMatrix<5, 5, qreal>>();
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 5; ++col) {
            effect->setProperty(QStringLiteral("m_%1_%2").arg(row).arg(col).toLatin1(),
                                matrix(col, row));
        }
    }

    qobject_cast<QQmlParserStatus *>(effect)->componentComplete();
    return makeEffectSES(effect, stepRect, filterRect, m_rootItem);
}

QQuickShaderEffectSource *QQuickItemGenerator::generateFilterBlend(
        const FilterNodeInfo::FilterStep &step, QQuickShaderEffectSource *input1,
        QQuickShaderEffectSource *input2, const QRectF &stepRect, const QRectF &filterRect)
{
    QString shaderName;
    switch (step.filterType) {
    case FilterNodeInfo::Type::BlendNormal:
        shaderName = u"feblendnormal"_s;
        break;
    case FilterNodeInfo::Type::BlendMultiply:
        shaderName = u"feblendmultiply"_s;
        break;
    case FilterNodeInfo::Type::BlendScreen:
        shaderName = u"feblendscreen"_s;
        break;
    case FilterNodeInfo::Type::BlendDarken:
        shaderName = u"feblenddarken"_s;
        break;
    case FilterNodeInfo::Type::BlendLighten:
        shaderName = u"feblendlighten"_s;
        break;
    default:
        Q_UNREACHABLE();
    }

    const QUrl shaderUrl(u"qrc:/qt-project.org/quickvectorimage/helpers/shaders_ng/"_s + shaderName
                         + u".frag.qsb"_s);
    auto *effect = makeFilterEffect(input1, shaderUrl, m_context, m_rootItem);
    effect->setProperty("source", QVariant::fromValue(input1));
    effect->setProperty("source2", QVariant::fromValue(input2));
    qobject_cast<QQmlParserStatus *>(effect)->componentComplete();
    return makeEffectSES(effect, stepRect, filterRect, m_rootItem);
}

QQuickShaderEffectSource *QQuickItemGenerator::generateFilterComposite(
        const FilterNodeInfo::FilterStep &step, QQuickShaderEffectSource *input1,
        QQuickShaderEffectSource *input2, const QRectF &stepRect, const QRectF &filterRect)
{
    QString shaderName;
    switch (step.filterType) {
    case FilterNodeInfo::Type::CompositeOver:
        shaderName = u"fecompositeover"_s;
        break;
    case FilterNodeInfo::Type::CompositeIn:
        shaderName = u"fecompositein"_s;
        break;
    case FilterNodeInfo::Type::CompositeOut:
        shaderName = u"fecompositeout"_s;
        break;
    case FilterNodeInfo::Type::CompositeAtop:
        shaderName = u"fecompositeatop"_s;
        break;
    case FilterNodeInfo::Type::CompositeXor:
        shaderName = u"fecompositexor"_s;
        break;
    case FilterNodeInfo::Type::CompositeLighter:
        shaderName = u"fecompositelighter"_s;
        break;
    case FilterNodeInfo::Type::CompositeArithmetic:
        shaderName = u"fecompositearithmetic"_s;
        break;
    default:
        Q_UNREACHABLE();
    }

    const QUrl shaderUrl(u"qrc:/qt-project.org/quickvectorimage/helpers/shaders_ng/"_s + shaderName
                         + u".frag.qsb"_s);
    auto *effect = makeFilterEffect(input1, shaderUrl, m_context, m_rootItem);
    effect->setProperty("source", QVariant::fromValue(input1));
    effect->setProperty("source2", QVariant::fromValue(input2));
    if (step.filterType == FilterNodeInfo::Type::CompositeArithmetic)
        effect->setProperty("k", QVariant::fromValue(step.filterParameter.value<QVector4D>()));
    qobject_cast<QQmlParserStatus *>(effect)->componentComplete();
    return makeEffectSES(effect, stepRect, filterRect, m_rootItem);
}

QQuickShaderEffectSource *
QQuickItemGenerator::generateFilterGaussianBlur(const FilterNodeInfo::FilterStep &step,
                                                QQuickShaderEffectSource *input,
                                                const QRectF &stepRect, const QRectF &filterRect)
{
    constexpr qreal maxDeviation = 12.0;
    const qreal deviation = step.filterParameter.toReal();
    const qreal blurValue = step.csFilterParameter == FilterNodeInfo::CoordinateSystem::Relative
            ? std::min(1.0, deviation * filterRect.width() / maxDeviation)
            : std::min(1.0, deviation / maxDeviation);

    auto *effect = new QQuickMultiEffect;
    if (m_context)
        QQmlEngine::setContextForObject(effect, m_context);
    auto *parserStatus = qobject_cast<QQmlParserStatus *>(effect);
    parserStatus->classBegin();
    effect->bindableWidth().setBinding([input] { return input->width(); });
    effect->bindableHeight().setBinding([input] { return input->height(); });
    effect->setVisible(false);
    effect->setParent(m_rootItem);
    effect->setParentItem(m_rootItem);
    effect->setSource(input);
    effect->setBlurEnabled(true);
    effect->setBlur(blurValue);
    effect->setBlurMax(64);
    parserStatus->componentComplete();

    auto *wrapper = new QQuickItem;
    wrapper->setWidth(stepRect.width());
    wrapper->setHeight(stepRect.height());
    wrapper->setClip(true);
    wrapper->setParent(m_rootItem);
    wrapper->setParentItem(m_rootItem);
    effect->setVisible(true);
    effect->setParentItem(wrapper);
    effect->setX(filterRect.x() - stepRect.x());
    effect->setY(filterRect.y() - stepRect.y());
    auto *ses = makeSES(wrapper, stepRect, m_rootItem);
    ses->setHideSource(true);
    return ses;
}

bool QQuickItemGenerator::generateMarkerNode(const MarkerNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (m_currentDefsRecord) {
        m_currentDefsRecord->append([this, info]() { generateMarkerNode(info); });
        return true;
    }

    if (info.stage == StructureNodeStage::Start) {
        m_markerDefs[info.id].info = info;
        m_currentMarkerRecord = &m_markerDefs[info.id].recording;
        return true;
    }

    m_markerDefs[info.id].info = info;
    m_currentMarkerRecord = nullptr;
    return true;
}

bool QQuickItemGenerator::generatePatternNode(const PatternNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (auto *rec = activeRecord()) {
        rec->append([this, info]() { generatePatternNode(info); });
        return true;
    }

    if (info.stage == StructureNodeStage::Start) {
        auto *containerItem = new QQuickItem;
        pushItem(containerItem);
        return true;
    }

    generatePatternContainer(info);
    return true;
}

void QQuickItemGenerator::generatePatternContainer(const PatternNodeInfo &info)
{
    auto *container = popItem();
    container->setParent(m_rootItem);
    container->setParentItem(m_rootItem);
    container->setVisible(false);
    if (!info.isPatternRectRelativeCoordinates) {
        container->setWidth(info.patternRect.width());
        container->setHeight(info.patternRect.height());
    }
    m_patternDefs[info.id] = { container, info.patternRect, info.isPatternRectRelativeCoordinates };
}

void QQuickItemGenerator::generatePattern(QQuickShapePath *shapePath, const PathNodeInfo &info,
                                          const QRectF &boundingRect, QTransform &fillTransform)
{
    auto it = m_patternDefs.find(info.patternId);
    if (it == m_patternDefs.end()) {
        qCWarning(lcQuickVectorImage) << "generatePattern: unknown pattern id:" << info.patternId;
        return;
    }
    PatternDef &patternDef = *it;

    qreal tileW, tileH, offsetX, offsetY;
    if (patternDef.isPatternRectRelativeCoordinates) {
        tileW = patternDef.patternRect.width() * boundingRect.width();
        tileH = patternDef.patternRect.height() * boundingRect.height();
        offsetX = patternDef.patternRect.x() * boundingRect.width();
        offsetY = patternDef.patternRect.y() * boundingRect.height();
        patternDef.container->setWidth(tileW);
        patternDef.container->setHeight(tileH);
    } else {
        tileW = patternDef.patternRect.width();
        tileH = patternDef.patternRect.height();
        offsetX = patternDef.patternRect.x();
        offsetY = patternDef.patternRect.y();
    }

    auto *ses = makeSES(patternDef.container, QRectF(0, 0, tileW, tileH), m_rootItem);
    ses->setHideSource(true);
    ses->setWrapMode(QQuickShaderEffectSource::Repeat);
    ses->setSourceRect(QRectF(0, 0, tileW, tileH));
    shapePath->setFillItem(ses);
    bindPatternTextureSize(ses);

    // Fill transform has to include the inverse of the scene scale, since the texture size
    // is scaled by this amount
    if (m_topLevelScaleSpy) {
        const QTransform baseTransform = fillTransform;
        auto *scaleSpy = m_topLevelScaleSpy;
        auto updateFillTransform = [shapePath, offsetX, offsetY, baseTransform, scaleSpy]() {
            const QSizeF unitScale = scaleSpy->requiredTextureSize();
            QTransform xf = baseTransform;
            xf.translate(offsetX, offsetY);
            xf.scale(1.0 / unitScale.width(), 1.0 / unitScale.height());
            shapePath->setFillTransform(QMatrix4x4(xf));
        };
        QObject::connect(scaleSpy, &QQuickItemSpy::requiredTextureSizeChanged, shapePath,
                         updateFillTransform);
        updateFillTransform();
    }

    fillTransform.translate(offsetX, offsetY);
}

static qreal meanAngle(QPointF p0, QPointF p1, QPointF p2)
{
    QPointF t1 = p1 - p0;
    QPointF t2 = p2 - p1;
    qreal hyp1 = hypot(t1.x(), t1.y());
    if (hyp1 > 0)
        t1 /= hyp1;
    else
        return 0.0;
    qreal hyp2 = hypot(t2.x(), t2.y());
    if (hyp2 > 0)
        t2 /= hyp2;
    else
        return 0.0;
    QPointF tangent = t1 + t2;
    return -atan2(tangent.y(), tangent.x()) / M_PI * 180.0;
}

void QQuickItemGenerator::generateMarkers(const PathNodeInfo &info)
{
    const QPainterPath path = info.path.defaultValue().value<QPainterPath>();

    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element element = path.elementAt(i);
        QString markerId;
        qreal angle = 0;

        if (i == 0) {
            markerId = info.markerStartId;
            angle = path.angleAtPercent(0.0);
        } else if (i == path.elementCount() - 1) {
            markerId = info.markerEndId;
            angle = path.angleAtPercent(1.0);
        } else if (path.elementAt(i + 1).type != QPainterPath::CurveToDataElement) {
            markerId = info.markerMidId;
            QPointF p1(path.elementAt(i - 1).x, path.elementAt(i - 1).y);
            QPointF p2(element.x, element.y);
            QPointF p3(path.elementAt(i + 1).x, path.elementAt(i + 1).y);
            angle = meanAngle(p1, p2, p3);
        }

        if (markerId.isEmpty())
            continue;

        auto it = m_markerDefs.find(markerId);
        if (it == m_markerDefs.end()) {
            qCWarning(lcQuickVectorImage) << "generateMarkers: unknown marker id:" << markerId;
            continue;
        }
        const MarkerDef &markerDef = *it;
        const MarkerNodeInfo &minfo = markerDef.info;

        const qreal sw = info.strokeStyle.width.defaultValue().toReal();
        const qreal markerW = minfo.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth
                ? minfo.markerSize.width() * sw
                : minfo.markerSize.width();
        const qreal markerH = minfo.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth
                ? minfo.markerSize.height() * sw
                : minfo.markerSize.height();

        qreal scaleX = 1.0, scaleY = 1.0, offsetX = 0.0, offsetY = 0.0;
        if (minfo.viewBox.width() > 0)
            scaleX = markerW / minfo.viewBox.width();
        if (minfo.viewBox.height() > 0)
            scaleY = markerH / minfo.viewBox.height();

        if (minfo.preserveAspectRatio & MarkerNodeInfo::xyMask) {
            if (!qFuzzyCompare(scaleX, scaleY)) {
                if (minfo.preserveAspectRatio & MarkerNodeInfo::meet)
                    scaleX = scaleY = qMin(scaleX, scaleY);
                else
                    scaleX = scaleY = qMax(scaleX, scaleY);

                const qreal overflowX = scaleX * minfo.viewBox.width() - markerW;
                const qreal overflowY = scaleY * minfo.viewBox.height() - markerH;

                const quint8 xRatio = minfo.preserveAspectRatio & MarkerNodeInfo::xMask;
                if (xRatio == MarkerNodeInfo::xMid)
                    offsetX -= overflowX / 2;
                else if (xRatio == MarkerNodeInfo::xMax)
                    offsetX -= overflowX;

                const quint8 yRatio = minfo.preserveAspectRatio & MarkerNodeInfo::yMask;
                if (yRatio == MarkerNodeInfo::yMid)
                    offsetY -= overflowY / 2;
                else if (yRatio == MarkerNodeInfo::yMax)
                    offsetY -= overflowY;
            }
        }

        const qreal anchorOffsetX = offsetX - minfo.anchorPoint.x() * scaleX;
        const qreal anchorOffsetY = offsetY - minfo.anchorPoint.y() * scaleY;

        const qreal instanceAngle =
                minfo.orientation == MarkerNodeInfo::Orientation::Value ? minfo.angle : -angle;

        auto *outerItem = new QQuickItem;
        auto outerXform = outerItem->transform();
        if (i == 0 && minfo.orientation == MarkerNodeInfo::Orientation::AutoStartReverse) {
            auto *flip = new QQuickScale(outerItem);
            flip->setXScale(-1);
            flip->setYScale(-1);
            outerXform.append(&outerXform, flip);
        }
        auto *rot = new QQuickRotation(outerItem);
        rot->setAngle(instanceAngle);
        outerXform.append(&outerXform, rot);
        auto *outerTr = new QQuickTranslate(outerItem);
        outerTr->setX(element.x);
        outerTr->setY(element.y);
        outerXform.append(&outerXform, outerTr);
        pushItem(outerItem);

        QQuickItem *clipItem = nullptr;
        if (!minfo.clipBox.isEmpty()) {
            clipItem = new QQuickItem;
            const qreal unitScale =
                    minfo.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth ? sw : 1.0;
            clipItem->setX(minfo.clipBox.x() * unitScale);
            clipItem->setY(minfo.clipBox.y() * unitScale);
            clipItem->setWidth(minfo.clipBox.width() * unitScale);
            clipItem->setHeight(minfo.clipBox.height() * unitScale);
            clipItem->setClip(true);
            pushItem(clipItem);
        }

        auto *innerItem = new QQuickItem;
        auto innerXform = innerItem->transform();
        auto *innerScale = new QQuickScale(innerItem);
        innerScale->setXScale(scaleX);
        innerScale->setYScale(scaleY);
        innerXform.append(&innerXform, innerScale);
        auto *innerTr = new QQuickTranslate(innerItem);
        innerTr->setX(anchorOffsetX);
        innerTr->setY(anchorOffsetY);
        innerXform.append(&innerXform, innerTr);
        if (clipItem) {
            auto *offsetItem = new QQuickItem;
            const qreal unitScale =
                    minfo.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth ? sw : 1.0;
            offsetItem->setX(-minfo.clipBox.x() * unitScale);
            offsetItem->setY(-minfo.clipBox.y() * unitScale);
            pushItem(offsetItem);
        }
        pushItem(innerItem);

        for (const auto &step : markerDef.recording)
            step();

        popItem();
        if (clipItem) {
            popItem();
            popItem();
        }
        popItem();
    }
}

static QQuickAnimatedProperty::PropertyAnimation
transformAnimation(const QQuickAnimatedProperty::PropertyAnimation &anim,
                   const std::function<QVariant(const QVariant &)> &extractor, int valueIndex)
{
    QQuickAnimatedProperty::PropertyAnimation transformed;
    transformed.easingPerFrame = anim.easingPerFrame;
    transformed.subtype = anim.subtype;
    transformed.repeatCount = anim.repeatCount;
    transformed.startOffset = anim.startOffset;
    transformed.flags = anim.flags;
    for (auto it = anim.frames.constBegin(); it != anim.frames.constEnd(); ++it) {
        const QVariant &rawValue = it.value();
        transformed.frames.insert(it.key(),
                                  rawValue.typeId() == QMetaType::QVariantList
                                          ? extractor(rawValue.toList().value(valueIndex))
                                          : extractor(rawValue));
    }
    return transformed;
}

static QEasingCurve easingForAnimationFrame(const QQuickAnimatedProperty::PropertyAnimation &anim,
                                            int time,
                                            QMap<std::array<qreal, 4>, QEasingCurve> &cache)
{
    QEasingCurve easing;
    auto it = anim.easingPerFrame.constFind(time);
    if (it == anim.easingPerFrame.constEnd())
        return easing;

    const QBezier &bezier = it.value();
    const QPointF c1 = bezier.pt2();
    const QPointF c2 = bezier.pt3();
    if (c1 == c1.transposed() && c2 == c2.transposed())
        return easing; // linear

    const std::array<qreal, 4> key{ c1.x(), c1.y(), c2.x(), c2.y() };
    auto cacheIt = cache.constFind(key);
    if (cacheIt != cache.constEnd())
        return cacheIt.value();

    easing.setType(QEasingCurve::BezierSpline);
    easing.addCubicBezierSegment(c1, c2, QPointF(1, 1));
    cache.insert(key, easing);
    return easing;
}

static void completeParserStatus(QQuickAbstractAnimation *anim)
{
    if (auto *ps = qobject_cast<QQmlParserStatus *>(anim))
        ps->componentComplete();
}

static QQuickPropertyAnimation *
createSegmentAnimation(QObject *target, const QString &property, const QVariant &value,
                       const QQuickAnimatedProperty::PropertyAnimation &anim, int frameTime,
                       int time, QObject *parent,
                       QMap<std::array<qreal, 4>, QEasingCurve> &easingCache)
{
    QQuickPropertyAnimation *segment = value.typeId() == QMetaType::QColor
            ? static_cast<QQuickPropertyAnimation *>(new QQuickColorAnimation(parent))
            : new QQuickPropertyAnimation(parent);
    segment->setTargetObject(target);
    segment->setProperty(property);
    segment->setDuration(frameTime);
    segment->setTo(value);
    segment->setEasing(easingForAnimationFrame(anim, time, easingCache));
    completeParserStatus(segment);
    return segment;
}

static QQuickPropertyAction *createImmediateSetter(QObject *target, const QString &property,
                                                   const QVariant &value, QObject *parent)
{
    auto *action = new QQuickPropertyAction(parent);
    action->setTargetObject(target);
    action->setProperty(property);
    action->setValue(value);
    completeParserStatus(action);
    return action;
}

static QQuickAbstractAnimation *
createAnimationForOneEntry(QObject *target, const QString &property,
                           const QQuickAnimatedProperty::PropertyAnimation &anim,
                           const QVariant &defaultValue, QObject *parent,
                           QMap<std::array<qreal, 4>, QEasingCurve> &easingCache)
{
    auto *outer = new QQuickSequentialAnimation(parent);
    auto outerAnims = outer->animations();

    const int startOffset =
            QQuickVectorImageGenerator::Utils::processAnimationTime(anim.startOffset);
    if (startOffset > 0) {
        auto *pause = new QQuickPauseAnimation(outer);
        pause->setDuration(startOffset);
        completeParserStatus(pause);
        outerAnims.append(&outerAnims, pause);
    }

    auto *inner = new QQuickSequentialAnimation(outer);
    inner->setLoops(anim.repeatCount < 0 ? QQuickAbstractAnimation::Infinite : anim.repeatCount);
    auto innerAnims = inner->animations();

    int previousTime = 0;
    QVariant previousValue;
    bool havePreviousValue = false;
    for (auto it = anim.frames.constBegin(); it != anim.frames.constEnd(); ++it) {
        const int time = it.key();
        const int frameTime =
                QQuickVectorImageGenerator::Utils::processAnimationTime(time - previousTime);
        const QVariant &value = it.value();

        if (havePreviousValue && previousValue == value) {
            if (frameTime > 0) {
                auto *pause = new QQuickPauseAnimation(inner);
                pause->setDuration(frameTime);
                completeParserStatus(pause);
                innerAnims.append(&innerAnims, pause);
            }
        } else if (value.typeId() == QMetaType::Bool) {
            if (frameTime > 0) {
                auto *pause = new QQuickPauseAnimation(inner);
                pause->setDuration(frameTime);
                completeParserStatus(pause);
                innerAnims.append(&innerAnims, pause);
            }
            innerAnims.append(&innerAnims, createImmediateSetter(target, property, value, inner));
        } else if (frameTime > 0) {
            innerAnims.append(&innerAnims,
                              createSegmentAnimation(target, property, value, anim, frameTime, time,
                                                     inner, easingCache));
        } else {
            innerAnims.append(&innerAnims, createImmediateSetter(target, property, value, inner));
        }

        previousTime = time;
        previousValue = value;
        havePreviousValue = true;
    }

    if (!(anim.flags & QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd)) {
        innerAnims.append(&innerAnims,
                          createImmediateSetter(target, property, defaultValue, inner));
    }

    completeParserStatus(inner);
    outerAnims.append(&outerAnims, inner);
    completeParserStatus(outer);
    return outer;
}

void QQuickItemGenerator::bindPropertyAnimation(
        QObject *target, const QString &property,
        const QQuickAnimatedProperty::PropertyAnimation &anim,
        const std::function<QVariant(const QVariant &)> &extractor, int valueIndex,
        const QVariant &resetValue)
{
    if (!target || anim.frames.isEmpty())
        return;

    const QQuickAnimatedProperty::PropertyAnimation transformed =
            transformAnimation(anim, extractor, valueIndex);
    if (transformed.frames.isEmpty())
        return;

    if (m_animationProvider) {
        m_animationProvider->bindProperty(target, property.toUtf8(), transformed);
        return;
    }

    const QVariant defaultValue =
            resetValue.isValid() ? resetValue : target->property(property.toUtf8().constData());
    auto *entry = createAnimationForOneEntry(target, property, transformed, defaultValue, target,
                                             m_easingCache);
    if (auto *root = qobject_cast<QQuickAnimationRootItem *>(m_rootItem))
        root->addMasterAnimation(entry);
    entry->setRunning(true);
}

void QQuickItemGenerator::bindAnimatedProperty(
        QObject *target, const QString &property, const QQuickAnimatedProperty &animatedProperty,
        const std::function<QVariant(const QVariant &)> &extractor, int valueIndex)
{
    if (!target || !animatedProperty.isAnimated())
        return;

    if (m_animationProvider || animatedProperty.animationCount() == 1) {
        for (int i = 0; i < animatedProperty.animationCount(); ++i)
            bindPropertyAnimation(target, property, animatedProperty.animation(i), extractor,
                                  valueIndex);
        return;
    }

    auto *master = new QQuickParallelAnimation(target);
    auto masterAnims = master->animations();
    const QVariant defaultValue = target->property(property.toUtf8().constData());
    for (int i = 0; i < animatedProperty.animationCount(); ++i) {
        const QQuickAnimatedProperty::PropertyAnimation transformed =
                transformAnimation(animatedProperty.animation(i), extractor, valueIndex);
        if (transformed.frames.isEmpty())
            continue;
        masterAnims.append(&masterAnims,
                           createAnimationForOneEntry(target, property, transformed, defaultValue,
                                                      master, m_easingCache));
    }
    completeParserStatus(master);
    if (auto *root = qobject_cast<QQuickAnimationRootItem *>(m_rootItem))
        root->addMasterAnimation(master);
    master->setRunning(true);
}

void QQuickItemGenerator::bindColorWithOpacity(QObject *target, const QString &colorProperty,
                                               const QQuickAnimatedProperty &color,
                                               const QQuickAnimatedProperty &opacity,
                                               std::function<void(const QColor &)> setter)
{
    auto identity = [](const QVariant &v) { return v; };

    if (!opacity.isAnimated()) {
        bindAnimatedProperty(target, colorProperty, color, identity);
        return;
    }

    auto *appliedColor =
            new OpacityAppliedColor(std::move(setter), color.defaultValue().value<QColor>(),
                                    opacity.defaultValue().toReal(), target);
    bindAnimatedProperty(appliedColor, QStringLiteral("baseColor"), color, identity);
    bindAnimatedProperty(appliedColor, QStringLiteral("opacity"), opacity, identity);
}

QQuickTransform *QQuickItemGenerator::createAnimatedTransformGroup(QQuickItem *item,
                                                                   const NodeInfo &info)
{
    if (!info.transform.isAnimated())
        return nullptr;

    auto *baseGroup = new QQuickTransformGroup(item);

    for (int groupIndex = 0; groupIndex < info.transform.animationGroupCount(); ++groupIndex) {
        int animStart = info.transform.animationGroup(groupIndex);
        int nextAnimStart = (groupIndex + 1 < info.transform.animationGroupCount())
                ? info.transform.animationGroup(groupIndex + 1)
                : info.transform.animationCount();

        auto *subGroup = new QQuickTransformGroup(baseGroup);
        auto baseSeq = baseGroup->transformSequence();
        baseSeq.append(&baseSeq, subGroup);

        const QQuickAnimatedProperty::PropertyAnimation &firstAnimation =
                info.transform.animation(animStart);
        const bool replace = firstAnimation.flags
                & QQuickAnimatedProperty::PropertyAnimation::ReplacePreviousAnimations;
        const bool freeze =
                firstAnimation.flags & QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd;
        bool hasNonConstant = false;

        for (int i = nextAnimStart - 1; i >= animStart; --i) {
            const QQuickAnimatedProperty::PropertyAnimation &anim = info.transform.animation(i);
            if (anim.frames.isEmpty())
                continue;

            const QVariantList &firstParams = anim.frames.first().value<QVariantList>();
            auto subSeq = subGroup->transformSequence();
            if (!anim.isConstant())
                hasNonConstant = true;

            switch (anim.subtype) {
            case QTransform::TxTranslate: {
                auto *translate = new QQuickTranslate(subGroup);
                if (anim.isConstant()) {
                    const QPointF point = firstParams.value(0).value<QPointF>();
                    translate->setX(point.x());
                    translate->setY(point.y());
                } else {
                    const QPointF defaultPoint = firstParams.value(0).value<QPointF>();
                    translate->setX(defaultPoint.x());
                    translate->setY(defaultPoint.y());
                    auto extractX = [](const QVariant &v) { return QVariant(v.toPointF().x()); };
                    auto extractY = [](const QVariant &v) { return QVariant(v.toPointF().y()); };
                    bindPropertyAnimation(translate, QStringLiteral("x"), anim, extractX, 0, 0.0);
                    bindPropertyAnimation(translate, QStringLiteral("y"), anim, extractY, 0, 0.0);
                }
                subSeq.append(&subSeq, translate);
                break;
            }
            case QTransform::TxScale: {
                auto *scale = new QQuickScale(subGroup);
                if (anim.isConstant()) {
                    const QPointF point = firstParams.value(0).value<QPointF>();
                    scale->setXScale(point.x());
                    scale->setYScale(point.y());
                } else {
                    const QPointF defaultPoint = firstParams.value(0).value<QPointF>();
                    scale->setXScale(defaultPoint.x());
                    scale->setYScale(defaultPoint.y());
                    auto extractX = [](const QVariant &v) { return QVariant(v.toPointF().x()); };
                    auto extractY = [](const QVariant &v) { return QVariant(v.toPointF().y()); };
                    bindPropertyAnimation(scale, QStringLiteral("xScale"), anim, extractX, 0, 1.0);
                    bindPropertyAnimation(scale, QStringLiteral("yScale"), anim, extractY, 0, 1.0);
                }
                subSeq.append(&subSeq, scale);
                break;
            }
            case QTransform::TxRotate: {
                auto *rotation = new QQuickRotation(subGroup);
                bool hasCenter = false;
                for (auto it = anim.frames.constBegin(); it != anim.frames.constEnd(); ++it) {
                    if (!it->value<QVariantList>().value(0).value<QPointF>().isNull()) {
                        hasCenter = true;
                        break;
                    }
                }
                if (anim.isConstant()) {
                    const QPointF center = firstParams.value(0).value<QPointF>();
                    const qreal angle = firstParams.value(1).toReal();
                    rotation->setAngle(angle);
                    rotation->setOrigin(QVector3D(center));
                } else {
                    const QPointF defaultCenter = firstParams.value(0).value<QPointF>();
                    const qreal defaultAngle = firstParams.value(1).toReal();
                    rotation->setAngle(defaultAngle);
                    rotation->setOrigin(QVector3D(defaultCenter));
                    if (hasCenter) {
                        auto extractOrigin = [](const QVariant &v) {
                            return QVariant::fromValue(QVector3D(v.toPointF()));
                        };
                        bindPropertyAnimation(rotation, QStringLiteral("origin"), anim,
                                              extractOrigin, 0,
                                              QVariant::fromValue(QVector3D(0, 0, 0)));
                    }
                    auto extractAngle = [](const QVariant &v) { return QVariant(v.toReal()); };
                    bindPropertyAnimation(rotation, QStringLiteral("angle"), anim, extractAngle, 1,
                                          0.0);
                }
                subSeq.append(&subSeq, rotation);
                break;
            }
            case QTransform::TxShear: {
                auto *shear = new QQuickShear(subGroup);
                if (anim.isConstant()) {
                    const QPointF point = firstParams.value(0).value<QPointF>();
                    shear->setXAngle(point.x());
                    shear->setYAngle(point.y());
                } else {
                    const QPointF defaultPoint = firstParams.value(0).value<QPointF>();
                    shear->setXAngle(defaultPoint.x());
                    shear->setYAngle(defaultPoint.y());
                    auto extractX = [](const QVariant &v) { return QVariant(v.toPointF().x()); };
                    auto extractY = [](const QVariant &v) { return QVariant(v.toPointF().y()); };
                    bindPropertyAnimation(shear, QStringLiteral("xAngle"), anim, extractX, 0, 0.0);
                    bindPropertyAnimation(shear, QStringLiteral("yAngle"), anim, extractY, 0, 0.0);
                }
                subSeq.append(&subSeq, shear);
                break;
            }
            default:
                break;
            }
        }

        if (replace && hasNonConstant) {
            const int startOffsetMs = QQuickVectorImageGenerator::Utils::processAnimationTime(
                    firstAnimation.startOffset);

            auto *activateSeq = new QQuickSequentialAnimation(item);
            auto activateAnims = activateSeq->animations();
            if (startOffsetMs > 0) {
                auto *pause = new QQuickPauseAnimation(activateSeq);
                pause->setDuration(startOffsetMs);
                completeParserStatus(pause);
                activateAnims.append(&activateAnims, pause);
            }
            auto *activateAction = new QQuickFunctionAction(
                    [baseGroup, subGroup]() { baseGroup->activateOverride(subGroup); },
                    activateSeq);
            completeParserStatus(activateAction);
            activateAnims.append(&activateAnims, activateAction);
            completeParserStatus(activateSeq);
            if (auto *root = qobject_cast<QQuickAnimationRootItem *>(m_rootItem))
                root->addMasterAnimation(activateSeq);
            activateSeq->setRunning(true);

            if (firstAnimation.repeatCount >= 0) {
                const int loopDurationMs = firstAnimation.frames.isEmpty()
                        ? 0
                        : QQuickVectorImageGenerator::Utils::processAnimationTime(
                                firstAnimation.frames.lastKey());
                const int totalDurationMs =
                        startOffsetMs + loopDurationMs * qMax(firstAnimation.repeatCount, 1);

                auto *endSeq = new QQuickSequentialAnimation(item);
                auto endAnims = endSeq->animations();
                if (totalDurationMs > 0) {
                    auto *pause = new QQuickPauseAnimation(endSeq);
                    pause->setDuration(totalDurationMs);
                    completeParserStatus(pause);
                    endAnims.append(&endAnims, pause);
                }
                auto *endAction = new QQuickFunctionAction(
                        [baseGroup, subGroup, freeze]() {
                            if (!freeze)
                                baseGroup->deactivate(subGroup);
                        },
                        endSeq);
                completeParserStatus(endAction);
                endAnims.append(&endAnims, endAction);
                completeParserStatus(endSeq);
                if (auto *root = qobject_cast<QQuickAnimationRootItem *>(m_rootItem))
                    root->addMasterAnimation(endSeq);
                endSeq->setRunning(true);
            }
        }
    }

    if (!info.isDefaultTransform) {
        const QTransform transform = info.transform.defaultValue().value<QTransform>();
        auto *staticMatrix = new QQuickMatrix4x4(baseGroup);
        staticMatrix->setMatrix(QMatrix4x4(transform));
        auto baseSeq = baseGroup->transformSequence();
        baseSeq.append(&baseSeq, staticMatrix);
    }

    return baseGroup;
}

void QQuickItemGenerator::bindMotionPath(QQuickItem *item, const QQuickAnimatedProperty &motionPath)
{
    if (!motionPath.isAnimated() || motionPath.animationCount() == 0)
        return;

    const QVariantList defaultProps = motionPath.defaultValue().value<QVariantList>();
    const QPainterPath path = defaultProps.value(0).value<QPainterPath>();
    const bool adaptAngle = defaultProps.value(1).toBool();
    const qreal baseRotation = defaultProps.value(2).toReal();

    const QQuickAnimatedProperty::PropertyAnimation &pathAnim = motionPath.animation(0);

    auto *pathObj = new QQuickPath(item);
    auto *pathSvg = new QQuickPathSvg(pathObj);
    auto pathElements = pathObj->pathElements();
    pathElements.append(&pathElements, pathSvg);
    pathSvg->setPath(QQuickVectorImageGenerator::Utils::toSvgString(path));

    auto *interpolator = new QQuickPathInterpolator(item);
    interpolator->setPath(pathObj);

    QQuickAnimatedProperty::PropertyAnimation progressAnim;
    progressAnim.frames = pathAnim.frames;
    progressAnim.easingPerFrame = pathAnim.easingPerFrame;
    progressAnim.flags = QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd;

    auto identity = [](const QVariant &v) { return v; };
    bindPropertyAnimation(interpolator, QStringLiteral("progress"), progressAnim, identity);

    auto *translate = new QQuickTranslate(item);
    QObject::connect(interpolator, &QQuickPathInterpolator::xChanged, translate,
                     [translate, interpolator]() { translate->setX(interpolator->x()); });
    QObject::connect(interpolator, &QQuickPathInterpolator::yChanged, translate,
                     [translate, interpolator]() { translate->setY(interpolator->y()); });
    const qreal initialProgress = qBound(0.0, interpolator->property("progress").toDouble(), 1.0);
    const QPointF initialPoint = path.pointAtPercent(initialProgress);
    translate->setX(initialPoint.x());
    translate->setY(initialPoint.y());

    if (adaptAngle || !qFuzzyIsNull(baseRotation)) {
        auto *rotation = new QQuickRotation(item);
        if (adaptAngle) {
            QObject::connect(interpolator, &QQuickPathInterpolator::angleChanged, rotation,
                             [rotation, interpolator, baseRotation]() {
                                 rotation->setAngle(interpolator->angle() + baseRotation);
                             });
            rotation->setAngle(interpolator->angle() + baseRotation);
        } else {
            rotation->setAngle(baseRotation);
        }
        auto transformProp = item->transform();
        transformProp.append(&transformProp, rotation);
    }
    auto transformProp = item->transform();
    transformProp.append(&transformProp, translate);
}

void QQuickItemGenerator::generateItemAnimations(QQuickItem *item, const NodeInfo &info)
{
    const bool hasTransformAnim = info.transform.isAnimated();
    const bool hasOpacityAnim = info.opacity.isAnimated();
    const bool hasMotionPathAnim = info.motionPath.isAnimated();
    const bool hasLinkedTransform = !info.transformReferenceId.isEmpty();

    if (hasOpacityAnim) {
        auto identity = [](const QVariant &v) { return v; };
        bindAnimatedProperty(item, QStringLiteral("opacity"), info.opacity, identity);
    }

    if (hasTransformAnim) {
        auto *baseGroup = createAnimatedTransformGroup(item, info);
        if (baseGroup) {
            auto transformProp = item->transform();
            transformProp.append(&transformProp, baseGroup);
        }
    }

    if (hasMotionPathAnim)
        bindMotionPath(item, info.motionPath);

    if (hasLinkedTransform) {
        auto *linkedMatrix = new QQuickMatrix4x4(item);
        auto transformProp = item->transform();
        transformProp.append(&transformProp, linkedMatrix);
        m_pendingLinkedTransforms.append({ item, info.transformReferenceId, linkedMatrix });
    }
}

QT_END_NAMESPACE

#include "qquickitemgenerator.moc"
