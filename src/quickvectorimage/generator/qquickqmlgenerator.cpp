// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickqmlgenerator_p.h"
#include "qquicknodeinfo_p.h"
#include "utils_p.h"

#include <private/qsgcurveprocessor_p.h>
#include <private/qquickshape_p.h>
#include <private/qquadpath_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickimagebase_p_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktranslate_p.h>
#include <private/qquickimage_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qdir.h>

QT_BEGIN_NAMESPACE

static QString sanitizeString(const QString &input)
{
    QString s = input;
    s.replace(QLatin1Char('"'), QLatin1String("\\\""));
    return s;
}


QQuickAnimatedProperty::PropertyAnimation QQuickAnimatedProperty::PropertyAnimation::simplified() const
{
    QQuickAnimatedProperty::PropertyAnimation res = *this;
    int consecutiveEquals = 0;
    int prevTimePoint = -1;
    QVariant prevValue;
    for (const auto &[timePoint, value] : frames.asKeyValueRange()) {
        if (value != prevValue) {
            consecutiveEquals = 1;
            prevValue = value;
        } else if (consecutiveEquals < 2) {
            consecutiveEquals++;
        } else {
            // Third consecutive equal value found, remove the redundant middle one
            res.frames.remove(prevTimePoint);
            res.easingPerFrame.remove(prevTimePoint);
        }
        prevTimePoint = timePoint;
    }

    return res;
}

QQuickQmlGenerator::QQuickQmlGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags, const QString &outFileName)
    : QQuickGenerator(fileName, flags)
    , outputFileName(outFileName)
{
    m_result.open(QIODevice::ReadWrite);
}

QQuickQmlGenerator::~QQuickQmlGenerator()
{
}

bool QQuickQmlGenerator::save()
{
    bool res = true;
    if (!outputFileName.isEmpty()) {
        QFileInfo fileInfo(outputFileName);
        QDir dir(fileInfo.absolutePath());
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            qCWarning(lcQuickVectorImage) << "Failed to create path" << dir.absolutePath();
            res = false;
        } else {
            QFile outFile(outputFileName);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(m_result.data());
                outFile.close();
            } else {
                qCWarning(lcQuickVectorImage) << "Failed to write to file" << outFile.fileName();
                res = false;
            }
        }
    }

    if (lcQuickVectorImage().isDebugEnabled())
        qCDebug(lcQuickVectorImage).noquote() << m_result.data().left(300);

    return res;
}

void QQuickQmlGenerator::setShapeTypeName(const QString &name)
{
    m_shapeTypeName = name.toLatin1();
}

QString QQuickQmlGenerator::shapeTypeName() const
{
    return QString::fromLatin1(m_shapeTypeName);
}

void QQuickQmlGenerator::setCommentString(const QString commentString)
{
    m_commentString = commentString;
}

QString QQuickQmlGenerator::commentString() const
{
    return m_commentString;
}

QString QQuickQmlGenerator::generateNodeBase(const NodeInfo &info)
{
    if (!info.nodeId.isEmpty())
        stream() << "objectName: \"" << info.nodeId << "\"";

    QString idString = info.id;
    if (!idString.isEmpty()) {
        // If input contains multiple items with the same id, then this is invalid. However we
        // shouldn't generate invalid QML from it. So we add a suffix if this is detected.
        if (m_generatedIds.contains(idString))
            idString += QStringLiteral("_%1").arg(m_generatedIds.size());

        m_generatedIds.insert(idString);

        stream() << "id: " << idString;
    }

    if (!info.bounds.isNull()) {
        stream() << "property var originalBounds: Qt.rect("
                 << info.bounds.x() << ", "
                 << info.bounds.y() << ", "
                 << info.bounds.width() << ", "
                 << info.bounds.height() << ")";
        stream() << "implicitWidth: originalBounds.width";
        stream() << "implicitHeight: originalBounds.height";
    }

    stream() << "transformOrigin: Item.TopLeft";

    if (info.filterId.isEmpty() && info.maskId.isEmpty()) {
        if (!info.isDefaultOpacity)
            stream() << "opacity: " << info.opacity.defaultValue().toReal();
        generateItemAnimations(idString, info);
    }

    return idString;
}

void QQuickQmlGenerator::generateNodeEnd(const NodeInfo &info)
{
    m_indentLevel--;
    stream() << "}";
    generateShaderUse(info);
}

void QQuickQmlGenerator::generateItemAnimations(const QString &idString, const NodeInfo &info)
{
    const bool hasTransform = info.transform.isAnimated()
                              || !info.maskId.isEmpty()
                              || !info.filterId.isEmpty()
                              || !info.isDefaultTransform
                              || !info.transformReferenceId.isEmpty()
                              || info.motionPath.isAnimated();

    if (hasTransform) {
        stream() << "transform: TransformGroup {";
        m_indentLevel++;

        bool hasNonConstantTransform = false;
        int earliestOverrideGroup = -1;

        if (!idString.isEmpty()) {
            stream() << "id: " << idString << "_transform_base_group";

            if (!info.maskId.isEmpty() || !info.filterId.isEmpty())
                stream() << "Translate { x: " << idString << ".sourceX; y: " << idString << ".sourceY }";

            if (info.transform.isAnimated()) {
                for (int groupIndex = 0; groupIndex < info.transform.animationGroupCount(); ++groupIndex) {
                    stream() << "TransformGroup {";
                    m_indentLevel++;

                    if (!idString.isEmpty())
                        stream() << "id: " << idString << "_transform_group_" << groupIndex;

                    int animationStart = info.transform.animationGroup(groupIndex);
                    int nextAnimationStart = groupIndex + 1 < info.transform.animationGroupCount()
                                                 ? info.transform.animationGroup(groupIndex + 1)
                                                 : info.transform.animationCount();

                    const QQuickAnimatedProperty::PropertyAnimation &firstAnimation = info.transform.animation(animationStart);
                    const bool replace = firstAnimation.flags & QQuickAnimatedProperty::PropertyAnimation::ReplacePreviousAnimations;
                    if (replace && earliestOverrideGroup < 0)
                        earliestOverrideGroup = groupIndex;

                    for (int i = nextAnimationStart - 1; i >= animationStart; --i) {
                        const QQuickAnimatedProperty::PropertyAnimation &animation = info.transform.animation(i);
                        if (animation.frames.isEmpty())
                            continue;

                        const QVariantList &parameters = animation.frames.first().value<QVariantList>();
                        switch (animation.subtype) {
                        case QTransform::TxTranslate:
                            if (animation.isConstant()) {
                                const QPointF translation = parameters.value(0).value<QPointF>();
                                if (!translation.isNull())
                                    stream() << "Translate { x: " << translation.x() << "; y: " << translation.y() << " }";
                            } else {
                                hasNonConstantTransform = true;
                                stream() << "Translate { id: " << idString << "_transform_" << groupIndex << "_" << i << " }";
                            }
                            break;
                        case QTransform::TxScale:
                            if (animation.isConstant()) {
                                const QPointF scale = parameters.value(0).value<QPointF>();
                                if (scale != QPointF(1, 1))
                                    stream() << "Scale { xScale: " << scale.x() << "; yScale: " << scale.y() << " }";
                            } else {
                                hasNonConstantTransform = true;
                                stream() << "Scale { id: " << idString << "_transform_" << groupIndex << "_" << i << "}";
                            }
                            break;
                        case QTransform::TxRotate:
                            if (animation.isConstant()) {
                                const QPointF center = parameters.value(0).value<QPointF>();
                                const qreal angle = parameters.value(1).toReal();
                                if (!qFuzzyIsNull(angle))
                                    stream() << "Rotation { angle: " << angle << "; origin.x: " << center.x() << "; origin.y: " << center.y() << " }"; //### center relative to what?
                            } else {
                                hasNonConstantTransform = true;
                                stream() << "Rotation { id: " << idString << "_transform_" << groupIndex << "_" << i << " }";
                            }
                            break;
                        case QTransform::TxShear:
                            if (animation.isConstant()) {
                                const QPointF skew = parameters.value(0).value<QPointF>();
                                if (!skew.isNull())
                                    stream() << "Shear { xAngle: " << skew.x() << "; yAngle: " << skew.y() << " }";
                            } else {
                                hasNonConstantTransform = true;
                                stream() << "Shear { id: " << idString << "_transform_" << groupIndex << "_" << i << " }";
                            }
                            break;
                        default:
                            Q_UNREACHABLE();
                        }
                    }

                    m_indentLevel--;
                    stream() << "}";
                }
            }

            if (info.motionPath.isAnimated()) {
                QVariantPair defaultProps = info.motionPath.defaultValue().value<QVariantPair>();
                const bool adaptAngle = defaultProps.first.toBool();
                const qreal baseRotation = defaultProps.second.toReal();
                if (adaptAngle || !qFuzzyIsNull(baseRotation)) {
                    stream() << "Rotation {";
                    m_indentLevel++;

                    if (adaptAngle) {
                        stream() << "angle: " << idString
                                 << "_motion_animation.currentInterpolator.angle";
                        if (!qFuzzyIsNull(baseRotation))
                            stream(SameLine) << " + " << baseRotation;
                    } else {
                        stream() << "angle: " << baseRotation;
                    }

                    m_indentLevel--;
                    stream() << "}";
                }

                stream() << "Translate {";
                m_indentLevel++;

                stream() << "x: " << idString << "_motion_animation.currentInterpolator.x";
                stream() << "y: " << idString << "_motion_animation.currentInterpolator.y";

                m_indentLevel--;
                stream() << "}";
            }
        }

        if (!info.isDefaultTransform) {
            QTransform xf = info.transform.defaultValue().value<QTransform>();
            if (xf.type() <= QTransform::TxTranslate) {
                stream() << "Translate { x: " << xf.dx() << "; y: " << xf.dy() << "}";
            } else {
                stream() << "Matrix4x4 { matrix: ";
                generateTransform(xf);
                stream(SameLine) << "}";
            }
        }

        if (!info.transformReferenceId.isEmpty())
            stream() << "Matrix4x4 { matrix: " << info.transformReferenceId << ".transformMatrix }";

        m_indentLevel--;
        stream() << "}";

        if (hasNonConstantTransform) {
            generateAnimateTransform(idString, info);
        } else if (info.transform.isAnimated() && earliestOverrideGroup >= 0) {
            // We have animations, but they are all constant? Then we still need to respect the
            // override flag of the animations
            stream() << "Component.onCompleted: {";
            m_indentLevel++;

            stream() << idString << "_transform_base_group.activateOverride("
                     << idString << "_transform_group_" << earliestOverrideGroup << ")";

            m_indentLevel--;
            stream() << "}";
        }
    }

    generateAnimateMotionPath(idString, info.motionPath);

    generatePropertyAnimation(info.opacity, idString, QStringLiteral("opacity"));
    generatePropertyAnimation(info.visibility, idString, QStringLiteral("visible"));
}

void QQuickQmlGenerator::generateShaderUse(const NodeInfo &info)
{
    const bool hasMask = !info.maskId.isEmpty();
    const bool hasFilters = !info.filterId.isEmpty();
    if (!hasMask && !hasFilters)
        return;

    const QString effectId = hasFilters
        ? info.filterId + QStringLiteral("_") + info.id + QStringLiteral("_effect")
        : QString{};

    QString animatedItemId;
    if (hasFilters) {
        stream() << "ShaderEffectSource {";
        m_indentLevel++;

        const QString seId = info.id + QStringLiteral("_se");
        stream() << "id: " << seId;

        stream() << "ItemSpy {";
        m_indentLevel++;
        stream() << "id: " << info.id << "_itemspy";
        stream() << "anchors.fill: parent";
        m_indentLevel--;
        stream() << "}";

        stream() << "hideSource: true";
        stream() << "wrapMode: " << info.filterId << "_filterParameters.wrapMode";
        stream() << "sourceItem: " << info.id;
        stream() << "sourceRect: " << info.filterId
                 << "_filterParameters.adaptToFilterRect("
                 << info.id << ".originalBounds.x, "
                 << info.id << ".originalBounds.y, "
                 << info.id << ".originalBounds.width, "
                 << info.id << ".originalBounds.height)";
        stream() << "textureSize: " << info.id << "_itemspy.requiredTextureSize";
        stream() << "width: sourceRect.width";
        stream() << "height: sourceRect.height";
        stream() << "visible: false";

        m_indentLevel--;
        stream() << "}";

        stream() << "Loader {";
        m_indentLevel++;

        animatedItemId = effectId;
        stream() << "id: " << effectId;

        stream() << "property var filterSourceItem: " << seId;
        stream() << "sourceComponent: " << info.filterId << "_container";
        stream() << "property real sourceX: " << info.id << ".originalBounds.x";
        stream() << "property real sourceY: " << info.id << ".originalBounds.y";
        stream() << "width: " << info.id << ".originalBounds.width";
        stream() << "height: " << info.id << ".originalBounds.height";

        if (hasMask) {
            m_indentLevel--;
            stream() << "}";
        }
    }

    if (hasMask) {
        // Shader effect source for the mask itself
        stream() << "ShaderEffectSource {";
        m_indentLevel++;

        const QString maskId = info.maskId + QStringLiteral("_") + info.id + QStringLiteral("_mask");
        stream() << "id: " << maskId;
        stream() << "sourceItem: " << info.maskId;
        stream() << "visible: false";
        stream() << "hideSource: true";

        stream() << "ItemSpy {";
        m_indentLevel++;
        stream() << "id: " << maskId << "_itemspy";
        stream() << "anchors.fill: parent";
        m_indentLevel--;
        stream() << "}";
        stream() << "textureSize: " << maskId << "_itemspy.requiredTextureSize";

        stream() << "sourceRect: " << info.maskId << ".maskRect("
                 << info.id << ".originalBounds.x,"
                 << info.id << ".originalBounds.y,"
                 << info.id << ".originalBounds.width,"
                 << info.id << ".originalBounds.height)";

        stream() << "width: sourceRect.width";
        stream() << "height: sourceRect.height";

        m_indentLevel--;
        stream() << "}";

        // Shader effect source of the masked item
        stream() << "ShaderEffectSource {";
        m_indentLevel++;

        const QString seId = info.id + QStringLiteral("_masked_se");
        stream() << "id: " << seId;

        stream() << "ItemSpy {";
        m_indentLevel++;
        stream() << "id: " << info.id << "_masked_se_itemspy";
        stream() << "anchors.fill: parent";
        m_indentLevel--;
        stream() << "}";

        stream() << "hideSource: true";
        if (hasFilters)
            stream() << "sourceItem: " << effectId;
        else
            stream() << "sourceItem: " << info.id;
        stream() << "textureSize: " << info.id << "_masked_se_itemspy.requiredTextureSize";
        if (!hasFilters) {
            stream() << "sourceRect: " << info.maskId << ".maskRect("
                     << info.id << ".originalBounds.x,"
                     << info.id << ".originalBounds.y,"
                     << info.id << ".originalBounds.width,"
                     << info.id << ".originalBounds.height)";
        } else {
            stream() << "sourceRect: " << info.maskId << ".maskRect(0, 0,"
                     << info.id << ".originalBounds.width,"
                     << info.id << ".originalBounds.height)";
        }
        stream() << "width: sourceRect.width";
        stream() << "height: sourceRect.height";
        stream() << "smooth: false";
        stream() << "visible: false";

        m_indentLevel--;
        stream() << "}";

        stream() << "ShaderEffect {";
        m_indentLevel++;

        const QString maskShaderId = maskId + QStringLiteral("_se");
        animatedItemId = maskShaderId;

        stream() << "id:" << maskShaderId;

        stream() << "property real sourceX: " << maskId << ".sourceRect.x";
        stream() << "property real sourceY: " << maskId << ".sourceRect.y";
        stream() << "width: " << maskId << ".sourceRect.width";
        stream() << "height: " << maskId << ".sourceRect.height";

        stream() << "fragmentShader: \"qrc:/qt-project.org/quickvectorimage/helpers/shaders_ng/genericmask.frag.qsb\"";
        stream() << "property var source: " << seId;
        stream() << "property var maskSource: " << maskId;
        stream() << "property bool isAlpha: " << (info.isMaskAlpha ? "true" : "false");
        stream() << "property bool isInverted: " << (info.isMaskInverted ? "true" : "false");
    }

    if (!info.isDefaultOpacity)
        stream() << "opacity: " << info.opacity.defaultValue().toReal();

    generateItemAnimations(animatedItemId, info);

    m_indentLevel--;
    stream() << "}";
}

bool QQuickQmlGenerator::generateDefsNode(const NodeInfo &info)
{
    Q_UNUSED(info)

    return false;
}

void QQuickQmlGenerator::generateImageNode(const ImageNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    const QFileInfo outputFileInfo(outputFileName);
    const QDir outputDir(outputFileInfo.absolutePath());

    QString filePath;

    if (!m_retainFilePaths || info.externalFileReference.isEmpty()) {
        filePath = m_assetFileDirectory;
        if (filePath.isEmpty())
            filePath = outputDir.absolutePath();

        if (!filePath.isEmpty() && !filePath.endsWith(u'/'))
            filePath += u'/';

        QDir fileDir(filePath);
        if (!fileDir.exists()) {
            if (!fileDir.mkpath(QStringLiteral(".")))
                qCWarning(lcQuickVectorImage) << "Failed to create image resource directory:" << filePath;
        }

        filePath += QStringLiteral("%1%2.png").arg(m_assetFilePrefix.isEmpty()
                                                   ? QStringLiteral("svg_asset_")
                                                   : m_assetFilePrefix)
                                              .arg(info.image.cacheKey());

        if (!info.image.save(filePath))
            qCWarning(lcQuickVectorImage) << "Unabled to save image resource" << filePath;
        qCDebug(lcQuickVectorImage) << "Saving copy of IMAGE" << filePath;
    } else {
        filePath = info.externalFileReference;
    }

    const QFileInfo assetFileInfo(filePath);

    stream() << "Image {";

    m_indentLevel++;
    generateNodeBase(info);
    stream() << "x: " << info.rect.x();
    stream() << "y: " << info.rect.y();
    stream() << "width: " << info.rect.width();
    stream() << "height: " << info.rect.height();
    stream() << "source: \"" << m_urlPrefix << outputDir.relativeFilePath(assetFileInfo.absoluteFilePath()) <<"\"";
    generateNodeEnd(info);
}

void QQuickQmlGenerator::generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    if (!isNodeVisible(info))
        return;

    if (m_inShapeItemLevel > 0) {
        if (!info.isDefaultTransform)
            qWarning() << "Skipped transform for node" << info.nodeId << "type" << info.typeName << "(this is not supposed to happen)";
        optimizePaths(info, overrideBoundingRect);
    } else {
        m_inShapeItemLevel++;
        stream() << shapeName() << " {";

        m_indentLevel++;
        generateNodeBase(info);

        if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
            stream() << "preferredRendererType: Shape.CurveRenderer";
        if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::AsyncShapes))
            stream() << "asynchronous: true";
        optimizePaths(info, overrideBoundingRect);
        //qCDebug(lcQuickVectorGraphics) << *node->qpath();
        generateNodeEnd(info);
        m_inShapeItemLevel--;
    }
}

void QQuickQmlGenerator::generateGradient(const QGradient *grad)
{
    if (grad->type() == QGradient::LinearGradient) {
        auto *linGrad = static_cast<const QLinearGradient *>(grad);
        stream() << "fillGradient: LinearGradient {";
        m_indentLevel++;

        QRectF gradRect(linGrad->start(), linGrad->finalStop());

        stream() << "x1: " << gradRect.left();
        stream() << "y1: " << gradRect.top();
        stream() << "x2: " << gradRect.right();
        stream() << "y2: " << gradRect.bottom();
        for (auto &stop : linGrad->stops())
            stream() << "GradientStop { position: " << QString::number(stop.first, 'g', 7)
                     << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
        m_indentLevel--;
        stream() << "}";
    } else if (grad->type() == QGradient::RadialGradient) {
        auto *radGrad = static_cast<const QRadialGradient*>(grad);
        stream() << "fillGradient: RadialGradient {";
        m_indentLevel++;

        stream() << "centerX: " << radGrad->center().x();
        stream() << "centerY: " << radGrad->center().y();
        stream() << "centerRadius: " << radGrad->radius();
        stream() << "focalX:" << radGrad->focalPoint().x();
        stream() << "focalY:" << radGrad->focalPoint().y();
        for (auto &stop : radGrad->stops())
            stream() << "GradientStop { position: " << QString::number(stop.first, 'g', 7)
                     << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
        m_indentLevel--;
        stream() << "}";
    }
}

void QQuickQmlGenerator::generateAnimationBindings()
{
    QString prefix;
    if (Q_UNLIKELY(!isRuntimeGenerator()))
        prefix = QStringLiteral(".animations");

    stream() << "loops: " << m_topLevelIdString << prefix << ".loops";
    stream() << "paused: " << m_topLevelIdString << prefix << ".paused";
    stream() << "running: true";

    // We need to reset the animation when the loop count changes
    stream() << "onLoopsChanged: { if (running) { restart() } }";
}

void QQuickQmlGenerator::generateEasing(const QQuickAnimatedProperty::PropertyAnimation &animation, int time)
{
    if (animation.easingPerFrame.contains(time)) {
        QBezier bezier = animation.easingPerFrame.value(time);
        QPointF c1 = bezier.pt2();
        QPointF c2 = bezier.pt3();

        bool isLinear = (c1 == c1.transposed() && c2 == c2.transposed());
        if (!isLinear) {
            int nextIdx = m_easings.size();
            QString &id = m_easings[{c1.x(), c1.y(), c2.x(), c2.y()}];
            if (id.isNull())
                id = QString(QLatin1String("easing_%1")).arg(nextIdx, 2, 10, QLatin1Char('0'));
            stream() << "easing: " << m_topLevelIdString << "." << id;
        }
    }
}

void QQuickQmlGenerator::generatePropertyAnimation(const QQuickAnimatedProperty &property,
                                                   const QString &targetName,
                                                   const QString &propertyName,
                                                   AnimationType animationType)
{
    if (!property.isAnimated())
        return;

    QString mainAnimationId = targetName
                              + QStringLiteral("_")
                              + propertyName
                              + QStringLiteral("_animation");
    mainAnimationId.replace(QLatin1Char('.'), QLatin1Char('_'));

    QString prefix;
    if (Q_UNLIKELY(!isRuntimeGenerator()))
        prefix = QStringLiteral(".animations");

    stream() << "Connections { target: " << m_topLevelIdString << prefix << "; function onRestart() {" << mainAnimationId << ".restart() } }";

    stream() << "ParallelAnimation {";
    m_indentLevel++;

    stream() << "id: " << mainAnimationId;

    generateAnimationBindings();

    for (int i = 0; i < property.animationCount(); ++i) {
        const QQuickAnimatedProperty::PropertyAnimation &animation = property.animation(i);

        stream() << "SequentialAnimation {";
        m_indentLevel++;

        const int startOffset = animation.startOffset;
        if (startOffset > 0)
            stream() << "PauseAnimation { duration: " << startOffset << " }";

        stream() << "SequentialAnimation {";
        m_indentLevel++;

        const int repeatCount = animation.repeatCount;
        if (repeatCount < 0)
            stream() << "loops: Animation.Infinite";
        else
            stream() << "loops: " << repeatCount;

        int previousTime = 0;
        QVariant previousValue;
        for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it) {
            const int time = it.key();
            const int frameTime = time - previousTime;
            const QVariant &value = it.value();

            if (previousValue.isValid() && previousValue == value) {
                if (frameTime > 0)
                    stream() << "PauseAnimation { duration: " << frameTime << " }";
            } else if (animationType == AnimationType::Auto && value.typeId() == QMetaType::Bool) {
                // We special case bools, with PauseAnimation and then a setter at the end
                if (frameTime > 0)
                    stream() << "PauseAnimation { duration: " << frameTime << " }";
                stream() << "ScriptAction {";
                m_indentLevel++;

                stream() << "script:" << targetName << "." << propertyName << " = " << value.toString();

                m_indentLevel--;
                stream() << "}";
            } else {
                generateAnimatedPropertySetter(targetName,
                                               propertyName,
                                               value,
                                               animation,
                                               frameTime,
                                               time,
                                               animationType);
            }

            previousTime = time;
            previousValue = value;
        }

        if (!(animation.flags & QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd)) {
            stream() << "ScriptAction {";
            m_indentLevel++;
            stream() << "script: ";

            switch (animationType) {
            case AnimationType::Auto:
                stream(SameLine) << targetName << "." << propertyName << " = ";
                break;
            case AnimationType::ColorOpacity:
                stream(SameLine) << targetName << "." << propertyName << ".a = ";
                break;
            };

            QVariant value = property.defaultValue();
            if (value.typeId() == QMetaType::QColor)
                stream(SameLine) << "\"" << value.toString() << "\"";
            else
                stream(SameLine) << value.toReal();

            m_indentLevel--;
            stream() << "}";
        }

        m_indentLevel--;
        stream() << "}";

        m_indentLevel--;
        stream() << "}";
    }

    m_indentLevel--;
    stream() << "}";
}

void QQuickQmlGenerator::generateTransform(const QTransform &xf)
{
    if (xf.isAffine()) {
        stream(SameLine) << "PlanarTransform.fromAffineMatrix("
                         << xf.m11() << ", " << xf.m12() << ", "
                         << xf.m21() << ", " << xf.m22() << ", "
                         << xf.dx() << ", " << xf.dy() << ")";
    } else {
        QMatrix4x4 m(xf);
        stream(SameLine) << "Qt.matrix4x4(";
        m_indentLevel += 3;
        const auto *data = m.data();
        for (int i = 0; i < 4; i++) {
            stream() << data[i] << ", " << data[i+4] << ", " << data[i+8] << ", " << data[i+12];
            if (i < 3)
                stream(SameLine) << ", ";
        }
        stream(SameLine) << ")";
        m_indentLevel -= 3;
    }
}

void QQuickQmlGenerator::outputShapePath(const PathNodeInfo &info, const QPainterPath *painterPath, const QQuadPath *quadPath, QQuickVectorImageGenerator::PathSelector pathSelector, const QRectF &boundingRect)
{
    Q_UNUSED(pathSelector)
    Q_ASSERT(painterPath || quadPath);

    const QColor strokeColor = info.strokeStyle.color.defaultValue().value<QColor>();
    const bool noPen = strokeColor == QColorConstants::Transparent
                       && !info.strokeStyle.color.isAnimated()
                       && !info.strokeStyle.opacity.isAnimated();
    if (pathSelector == QQuickVectorImageGenerator::StrokePath && noPen)
        return;

    const QColor fillColor = info.fillColor.defaultValue().value<QColor>();
    const bool noFill = info.grad.type() == QGradient::NoGradient
                        && fillColor == QColorConstants::Transparent
                        && !info.fillColor.isAnimated()
                        && !info.fillOpacity.isAnimated();
    if (pathSelector == QQuickVectorImageGenerator::FillPath && noFill)
        return;

    if (noPen && noFill)
        return;

    auto fillRule = QQuickShapePath::FillRule(painterPath ? painterPath->fillRule() : quadPath->fillRule());
    stream() << "ShapePath {";
    m_indentLevel++;

    QString shapePathId = info.id;
    if (pathSelector & QQuickVectorImageGenerator::FillPath)
        shapePathId += QStringLiteral("_fill");
    if (pathSelector & QQuickVectorImageGenerator::StrokePath)
        shapePathId += QStringLiteral("_stroke");

    stream() << "id: " << shapePathId;

    if (!info.nodeId.isEmpty()) {
        switch (pathSelector) {
        case QQuickVectorImageGenerator::FillPath:
            stream() << "objectName: \"svg_fill_path:" << info.nodeId << "\"";
            break;
        case QQuickVectorImageGenerator::StrokePath:
            stream() << "objectName: \"svg_stroke_path:" << info.nodeId << "\"";
            break;
        case QQuickVectorImageGenerator::FillAndStroke:
            stream() << "objectName: \"svg_path:" << info.nodeId << "\"";
            break;
        }
    }

    if (noPen || !(pathSelector & QQuickVectorImageGenerator::StrokePath)) {
        stream() << "strokeColor: \"transparent\"";
    } else {
        stream() << "strokeColor: \"" << strokeColor.name(QColor::HexArgb) << "\"";
        stream() << "strokeWidth: " << info.strokeStyle.width;
        stream() << "capStyle: " << QQuickVectorImageGenerator::Utils::strokeCapStyleString(info.strokeStyle.lineCapStyle);
        stream() << "joinStyle: " << QQuickVectorImageGenerator::Utils::strokeJoinStyleString(info.strokeStyle.lineJoinStyle);
        stream() << "miterLimit: " << info.strokeStyle.miterLimit;
        if (info.strokeStyle.dashArray.length() != 0) {
            stream() << "strokeStyle: " << "ShapePath.DashLine";
            stream() << "dashPattern: " << QQuickVectorImageGenerator::Utils::listString(info.strokeStyle.dashArray);
            stream() << "dashOffset: " << info.strokeStyle.dashOffset;
        }
    }

    QTransform fillTransform = info.fillTransform;
    if (!(pathSelector & QQuickVectorImageGenerator::FillPath)) {
        stream() << "fillColor: \"transparent\"";
    } else if (info.grad.type() != QGradient::NoGradient) {
        generateGradient(&info.grad);
        if (info.grad.coordinateMode() == QGradient::ObjectMode) {
            QTransform objectToUserSpace;
            objectToUserSpace.translate(boundingRect.x(), boundingRect.y());
            objectToUserSpace.scale(boundingRect.width(), boundingRect.height());
            fillTransform *= objectToUserSpace;
        }
    } else {
        stream() << "fillColor: \"" << fillColor.name(QColor::HexArgb) << "\"";
    }

    if (!fillTransform.isIdentity()) {
        const QTransform &xf = fillTransform;
        stream() << "fillTransform: ";
        if (info.fillTransform.type() == QTransform::TxTranslate)
            stream(SameLine) << "PlanarTransform.fromTranslate(" << xf.dx() << ", " << xf.dy() << ")";
        else if (info.fillTransform.type() == QTransform::TxScale && !xf.dx() && !xf.dy())
            stream(SameLine) << "PlanarTransform.fromScale(" << xf.m11() << ", " << xf.m22() << ")";
        else
            generateTransform(xf);
    }

    if (info.trim.enabled) {
        stream() << "trim.start: " << info.trim.start.defaultValue().toReal();
        stream() << "trim.end: " << info.trim.end.defaultValue().toReal();
        stream() << "trim.offset: " << info.trim.offset.defaultValue().toReal();

    }

    if (fillRule == QQuickShapePath::WindingFill)
        stream() << "fillRule: ShapePath.WindingFill";
    else
        stream() << "fillRule: ShapePath.OddEvenFill";

    QString hintStr;
    if (quadPath)
        hintStr = QQuickVectorImageGenerator::Utils::pathHintString(*quadPath);
    if (!hintStr.isEmpty())
        stream() << hintStr;

    QQuickAnimatedProperty pathFactor(QVariant::fromValue(0));
    QString pathId = shapePathId + "_ip"_L1;
    if (!info.path.isAnimated() || (info.path.animation(0).startOffset == 0 && info.path.animation(0).isConstant())) {
        QString svgPathString = painterPath ? QQuickVectorImageGenerator::Utils::toSvgString(*painterPath) : QQuickVectorImageGenerator::Utils::toSvgString(*quadPath);
        stream() <<   "PathSvg { path: \"" << svgPathString << "\" }";
    } else {
        stream() << "PathInterpolated {";
        m_indentLevel++;
        stream() << "id: " << pathId;
        stream() << "svgPaths: [";
        m_indentLevel++;
        QQuickAnimatedProperty::PropertyAnimation pathFactorAnim = info.path.animation(0);
        auto &frames = pathFactorAnim.frames;
        int pathIdx = -1;
        QString lastSvg;
        for (auto it = frames.begin(); it != frames.end(); ++it) {
            QString svg = QQuickVectorImageGenerator::Utils::toSvgString(it->value<QPainterPath>());
            if (svg != lastSvg) {
                if (pathIdx >= 0)
                    stream(SameLine) << ",";
                stream() << "\"" << svg << "\"";
                ++pathIdx;
                lastSvg = svg;
            }
            *it = QVariant::fromValue(pathIdx);
        }
        pathFactor.addAnimation(pathFactorAnim);
        m_indentLevel--;
        stream() << "]";
        m_indentLevel--;
        stream() << "}";
    }

    m_indentLevel--;
    stream() << "}";

    if (pathFactor.isAnimated())
        generatePropertyAnimation(pathFactor, pathId, "factor"_L1);

    if (info.trim.enabled) {
        generatePropertyAnimation(info.trim.start, shapePathId + QStringLiteral(".trim"), QStringLiteral("start"));
        generatePropertyAnimation(info.trim.end, shapePathId + QStringLiteral(".trim"), QStringLiteral("end"));
        generatePropertyAnimation(info.trim.offset, shapePathId + QStringLiteral(".trim"), QStringLiteral("offset"));
    }

    generatePropertyAnimation(info.strokeStyle.color, shapePathId, QStringLiteral("strokeColor"));
    generatePropertyAnimation(info.strokeStyle.opacity, shapePathId, QStringLiteral("strokeColor"), AnimationType::ColorOpacity);
    generatePropertyAnimation(info.fillColor, shapePathId, QStringLiteral("fillColor"));
    generatePropertyAnimation(info.fillOpacity, shapePathId, QStringLiteral("fillColor"), AnimationType::ColorOpacity);
}

void QQuickQmlGenerator::generateNode(const NodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    stream() << "// Missing Implementation for SVG Node: " << info.typeName;
    stream() << "// Adding an empty Item and skipping";
    stream() << "Item {";
    m_indentLevel++;
    generateNodeBase(info);
    generateNodeEnd(info);
}

void QQuickQmlGenerator::generateTextNode(const TextNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    static int counter = 0;
    stream() << "Item {";
    m_indentLevel++;
    generateNodeBase(info);

    if (!info.isTextArea)
        stream() << "Item { id: textAlignItem_" << counter << "; x: " << info.position.x() << "; y: " << info.position.y() << "}";

    stream() << "Text {";

    m_indentLevel++;

    const QString textItemId = QStringLiteral("_qt_textItem_%1").arg(counter);
    stream() << "id: " << textItemId;

    generatePropertyAnimation(info.fillColor, textItemId, QStringLiteral("color"));
    generatePropertyAnimation(info.fillOpacity, textItemId, QStringLiteral("color"), AnimationType::ColorOpacity);
    generatePropertyAnimation(info.strokeColor, textItemId, QStringLiteral("styleColor"));
    generatePropertyAnimation(info.strokeOpacity, textItemId, QStringLiteral("styleColor"), AnimationType::ColorOpacity);

    if (info.isTextArea) {
        stream() << "x: " << info.position.x();
        stream() << "y: " << info.position.y();
        if (info.size.width() > 0)
            stream() << "width: " << info.size.width();
        if (info.size.height() > 0)
            stream() << "height: " << info.size.height();
        stream() << "wrapMode: Text.Wrap"; // ### WordWrap? verify with SVG standard
        stream() << "clip: true"; //### Not exactly correct: should clip on the text level, not the pixel level
    } else {
        QString hAlign = QStringLiteral("left");
        stream() << "anchors.baseline: textAlignItem_" << counter << ".top";
        switch (info.alignment) {
        case Qt::AlignHCenter:
            hAlign = QStringLiteral("horizontalCenter");
            break;
        case Qt::AlignRight:
            hAlign = QStringLiteral("right");
            break;
        default:
            qCDebug(lcQuickVectorImage) << "Unexpected text alignment" << info.alignment;
            Q_FALLTHROUGH();
        case Qt::AlignLeft:
            break;
        }
        stream() << "anchors." << hAlign << ": textAlignItem_" << counter << ".left";
    }
    counter++;

    stream() << "color: \"" << info.fillColor.defaultValue().value<QColor>().name(QColor::HexArgb) << "\"";
    stream() << "textFormat:" << (info.needsRichText ? "Text.RichText" : "Text.StyledText");

    stream() << "text: \"" << sanitizeString(info.text) << "\"";
    stream() << "font.family: \"" << sanitizeString(info.font.family()) << "\"";
    if (info.font.pixelSize() > 0)
        stream() << "font.pixelSize:" << info.font.pixelSize();
    else if (info.font.pointSize() > 0)
        stream() << "font.pixelSize:" << info.font.pointSizeF();
    if (info.font.underline())
        stream() << "font.underline: true";
    if (info.font.weight() != QFont::Normal)
        stream() << "font.weight: " << int(info.font.weight());
    if (info.font.italic())
        stream() << "font.italic: true";
    switch (info.font.hintingPreference()) {
    case QFont::PreferFullHinting:
        stream() << "font.hintingPreference: Font.PreferFullHinting";
        break;
    case QFont::PreferVerticalHinting:
        stream() << "font.hintingPreference: Font.PreferVerticalHinting";
        break;
    case QFont::PreferNoHinting:
        stream() << "font.hintingPreference: Font.PreferNoHinting";
        break;
    case QFont::PreferDefaultHinting:
        stream() << "font.hintingPreference: Font.PreferDefaultHinting";
        break;
    };

    const QColor strokeColor = info.strokeColor.defaultValue().value<QColor>();
    if (strokeColor != QColorConstants::Transparent || info.strokeColor.isAnimated()) {
        stream() << "styleColor: \"" << strokeColor.name(QColor::HexArgb) << "\"";
        stream() << "style: Text.Outline";
    }

    m_indentLevel--;
    stream() << "}";

    generateNodeEnd(info);
}

void QQuickQmlGenerator::generateUseNode(const UseNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    if (info.stage == StructureNodeStage::Start) {
        stream() << "Item {";
        m_indentLevel++;
        generateNodeBase(info);
    } else {
        generateNodeEnd(info);
    }
}

void QQuickQmlGenerator::generatePathContainer(const StructureNodeInfo &info)
{
    Q_UNUSED(info);
    stream() << shapeName() <<" {";
    m_indentLevel++;
    if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
        stream() << "preferredRendererType: Shape.CurveRenderer";
    if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::AsyncShapes))
        stream() << "asynchronous: true";
    m_indentLevel--;

    m_inShapeItemLevel++;
}

void QQuickQmlGenerator::generateAnimateMotionPath(const QString &targetName,
                                                   const QQuickAnimatedProperty &property)
{
    if (!property.isAnimated())
        return;

    Q_ASSERT(property.animationCount() == 1);
    const auto &animation = property.animation(0);

    qsizetype count = 0;
    for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it, ++count) {
        QPainterPath path = it.value().value<QPainterPath>();

        // If the animation starts with an empty path (a pause animation), then we get the first
        // valid path and use this, just to get the correct position and angle at the beginning.
        // This path is never actually interpolated over, because we use a PauseAnimation later
        // instead of a PropertyAnimation, but it is used as the default start position
        qreal angle = 0.0;
        QPointF position;
        if (path.isEmpty() && it == animation.frames.constBegin()) {
            for (auto jt = std::next(it); jt != animation.frames.constEnd(); ++jt) {
                const QPainterPath &nextPath = jt.value().value<QPainterPath>();
                if (!nextPath.isEmpty()) {
                    position = nextPath.pointAtPercent(0.0);
                    angle = -nextPath.angleAtPercent(0.0);
                    break;
                }
            }

            stream() << "QtObject {";
            m_indentLevel++;

            stream() << "id: " << targetName << "_pathInterpolator_" << count;
            stream() << "property real x: " << position.x();
            stream() << "property real y: " << position.y();
            stream() << "property real angle: " << angle;

            m_indentLevel--;
            stream() << "}";

        } else if (!path.isEmpty()) {
            stream() << "PathInterpolator {";
            m_indentLevel++;

            stream() << "id: " << targetName << "_pathInterpolator_" << count;
            const QString svgPathString = QQuickVectorImageGenerator::Utils::toSvgString(path);

            stream() << "path: Path { PathSvg { path: \"" << svgPathString << "\" } }";

            m_indentLevel--;
            stream() << "}";
        }
    }

    const QString mainAnimationId = targetName + QStringLiteral("_motion_animation");

    QString prefix;
    if (Q_UNLIKELY(!isRuntimeGenerator()))
        prefix = QStringLiteral(".animations");
    stream() << "Connections { target: " << m_topLevelIdString << prefix << "; function onRestart() {" << mainAnimationId << ".restart() } }";

    stream() << "SequentialAnimation {";
    m_indentLevel++;

    stream() << "id: " << mainAnimationId;
    stream() << "property var currentInterpolator: " << targetName << "_pathInterpolator_0";

    generateAnimationBindings();

    Q_ASSERT(property.animationCount() == 1);

    int previousTime = 0;
    count = 0;
    for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it, ++count) {
        const int time = it.key();
        const int frameTime = time - previousTime;
        const QPainterPath path = it.value().value<QPainterPath>();
        if (frameTime > 0) {
            if (path.isEmpty()) {
                stream() << "PauseAnimation { duration: " << frameTime << " }";
            } else {
                stream() << "ScriptAction {";
                m_indentLevel++;

                stream() << "script: {";
                m_indentLevel++;

                stream() << mainAnimationId << ".currentInterpolator = "
                         << targetName << "_pathInterpolator_" << count;

                m_indentLevel--;
                stream() << "}";

                m_indentLevel--;
                stream() << "}";

                stream() << "PropertyAnimation {";
                m_indentLevel++;

                stream() << "id: " << targetName << "_motionAnimation_" << count;

                stream() << "duration: " << frameTime;
                stream() << "target: " << targetName << "_pathInterpolator_" << count;
                stream() << "property: \"progress\"";
                stream() << "from: 0; to: 1";

                generateEasing(animation, time);

                m_indentLevel--;
                stream() << "}";
            }
        }

        previousTime = time;
    }

    m_indentLevel--;
    stream() << "}";

}

void QQuickQmlGenerator::generateAnimatedPropertySetter(const QString &targetName,
                                                        const QString &propertyName,
                                                        const QVariant &value,
                                                        const QQuickAnimatedProperty::PropertyAnimation &animation,
                                                        int frameTime,
                                                        int time,
                                                        AnimationType animationType)
{
    if (frameTime > 0) {
        switch (animationType) {
        case AnimationType::Auto:
            if (value.typeId() == QMetaType::QColor)
                stream() << "ColorAnimation {";
            else
                stream() << "PropertyAnimation {";
            break;
        case AnimationType::ColorOpacity:
            stream() << "ColorOpacityAnimation {";
            break;
        };
        m_indentLevel++;

        stream() << "duration: " << frameTime;
        stream() << "target: " << targetName;
        stream() << "property: \"" << propertyName << "\"";
        stream() << "to: ";
        if (value.typeId() == QMetaType::QVector3D) {
            const QVector3D &v = value.value<QVector3D>();
            stream(SameLine) << "Qt.vector3d(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
        } else if (value.typeId() == QMetaType::QColor) {
            stream(SameLine) << "\"" << value.toString() << "\"";
        } else {
            stream(SameLine) << value.toReal();
        }
        generateEasing(animation, time);
        m_indentLevel--;
        stream() << "}";
    } else {
        stream() << "ScriptAction {";
        m_indentLevel++;
        stream() << "script:" << targetName << "." << propertyName;
        if (animationType == AnimationType::ColorOpacity)
            stream(SameLine) << ".a";

        stream(SameLine) << " = ";
        if (value.typeId() == QMetaType::QVector3D) {
            const QVector3D &v = value.value<QVector3D>();
            stream(SameLine) << "Qt.vector3d(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
        } else if (value.typeId() == QMetaType::QColor) {
            stream(SameLine) << "\"" << value.toString() << "\"";
        } else {
            stream(SameLine) << value.toReal();
        }
        m_indentLevel--;
        stream() << "}";
    }
}

void QQuickQmlGenerator::generateAnimateTransform(const QString &targetName, const NodeInfo &info)
{
    if (!info.transform.isAnimated())
        return;

    const QString mainAnimationId = targetName
                                    + QStringLiteral("_transform_animation");

    QString prefix;
    if (Q_UNLIKELY(!isRuntimeGenerator()))
        prefix = QStringLiteral(".animations");
    stream() << "Connections { target: " << m_topLevelIdString << prefix << "; function onRestart() {" << mainAnimationId << ".restart() } }";

    stream() << "ParallelAnimation {";
    m_indentLevel++;

    stream() << "id:" << mainAnimationId;

    generateAnimationBindings();
    for (int groupIndex = 0; groupIndex < info.transform.animationGroupCount(); ++groupIndex) {
        int animationStart = info.transform.animationGroup(groupIndex);
        int nextAnimationStart = groupIndex + 1 < info.transform.animationGroupCount()
                                     ? info.transform.animationGroup(groupIndex + 1)
                                     : info.transform.animationCount();

        // The first animation in the group holds the shared properties for the whole group
        const QQuickAnimatedProperty::PropertyAnimation &firstAnimation = info.transform.animation(animationStart);
        const bool freeze = firstAnimation.flags & QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd;
        const bool replace = firstAnimation.flags & QQuickAnimatedProperty::PropertyAnimation::ReplacePreviousAnimations;

        stream() << "SequentialAnimation {";
        m_indentLevel++;

        const int startOffset = firstAnimation.startOffset;
        if (startOffset > 0)
            stream() << "PauseAnimation { duration: " << startOffset << " }";

        const int repeatCount = firstAnimation.repeatCount;
        if (repeatCount < 0)
            stream() << "loops: Animation.Infinite";
        else
            stream() << "loops: " << repeatCount;

        if (replace) {
            stream() << "ScriptAction {";
            m_indentLevel++;

            stream() << "script: " << targetName << "_transform_base_group"
                     << ".activateOverride(" << targetName << "_transform_group_" << groupIndex << ")";

            m_indentLevel--;
            stream() << "}";
        }

        stream() << "ParallelAnimation {";
        m_indentLevel++;

        for (int i = animationStart; i < nextAnimationStart; ++i) {
            const QQuickAnimatedProperty::PropertyAnimation &animation = info.transform.animation(i);
            if (animation.isConstant())
                continue;
            bool hasRotationCenter = false;
            if (animation.subtype == QTransform::TxRotate) {
                for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it) {
                    const QPointF center = it->value<QVariantList>().value(0).value<QPointF>();
                    if (!center.isNull()) {
                        hasRotationCenter = true;
                        break;
                    }
                }
            }

            stream() << "SequentialAnimation {";
            m_indentLevel++;

            int previousTime = 0;
            QVariantList previousParameters;
            for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it) {
                const int time = it.key();
                const int frameTime = time - previousTime;
                const QVariantList &parameters = it.value().value<QVariantList>();
                if (parameters.isEmpty())
                    continue;

                if (parameters == previousParameters) {
                    if (frameTime > 0)
                        stream() << "PauseAnimation { duration: " << frameTime << " }";
                } else {
                    stream() << "ParallelAnimation {";
                    m_indentLevel++;

                    const QString propertyTargetName = targetName
                                                       + QStringLiteral("_transform_")
                                                       + QString::number(groupIndex)
                                                       + QStringLiteral("_")
                                                       + QString::number(i);

                    switch (animation.subtype) {
                    case QTransform::TxTranslate:
                    {
                        const QPointF translation = parameters.first().value<QPointF>();

                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("x"),
                                                       translation.x(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("y"),
                                                       translation.y(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        break;
                    }
                    case QTransform::TxScale:
                    {
                        const QPointF scale = parameters.first().value<QPointF>();
                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("xScale"),
                                                       scale.x(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("yScale"),
                                                       scale.y(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        break;
                    }
                    case QTransform::TxRotate:
                    {
                        Q_ASSERT(parameters.size() == 2);
                        const qreal angle = parameters.value(1).toReal();
                        if (hasRotationCenter) {
                            const QPointF center = parameters.value(0).value<QPointF>();
                            generateAnimatedPropertySetter(propertyTargetName,
                                                           QStringLiteral("origin"),
                                                           QVector3D(center.x(), center.y(), 0.0),
                                                           animation,
                                                           frameTime,
                                                           time);
                        }
                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("angle"),
                                                       angle,
                                                       animation,
                                                       frameTime,
                                                       time);
                        break;
                    }
                    case QTransform::TxShear:
                    {
                        const QPointF skew = parameters.first().value<QPointF>();

                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("xAngle"),
                                                       skew.x(),
                                                       animation,
                                                       frameTime,
                                                       time);

                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("yAngle"),
                                                       skew.y(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        break;
                    }
                    default:
                        Q_UNREACHABLE();
                    }

                    m_indentLevel--;
                    stream() << "}"; // Parallel key frame animation
                }

                previousTime = time;
                previousParameters = parameters;
            }

            m_indentLevel--;
            stream() << "}"; // Parallel key frame animation
        }

        m_indentLevel--;
        stream() << "}"; // Parallel key frame animation

        // If the animation ever finishes, then we add an action on the end that handles itsr
        // freeze state
        if (firstAnimation.repeatCount >= 0) {
            stream() << "ScriptAction {";
            m_indentLevel++;

            stream() << "script: {";
            m_indentLevel++;

            if (!freeze) {
                stream() << targetName << "_transform_base_group.deactivate("
                         << targetName << "_transform_group_" << groupIndex << ")";
            } else if (!replace) {
                stream() << targetName << "_transform_base_group.deactivateOverride("
                         << targetName << "_transform_group_" << groupIndex << ")";
            }

            m_indentLevel--;
            stream() << "}";

            m_indentLevel--;
            stream() << "}";
        }

        m_indentLevel--;
        stream() << "}";
    }

    m_indentLevel--;
    stream() << "}";
}

bool QQuickQmlGenerator::generateStructureNode(const StructureNodeInfo &info)
{
    if (!isNodeVisible(info))
        return false;

    const bool isPathContainer = !info.forceSeparatePaths && info.isPathContainer;
    if (info.stage == StructureNodeStage::Start) {
        if (!info.clipBox.isEmpty()) {
            stream() << "Item { // Clip";

            m_indentLevel++;
            stream() << "width: " << info.clipBox.width();
            stream() << "height: " << info.clipBox.height();
            stream() << "clip: true";
        }

        if (isPathContainer) {
            generatePathContainer(info);
        } else if (!info.customItemType.isEmpty()) {
            stream() << info.customItemType << " {";
        } else {
            stream() << "Item { // Structure node";
        }

        m_indentLevel++;
        if (!info.viewBox.isEmpty()) {
            stream() << "transform: [";
            m_indentLevel++;
            bool translate = !qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y());
            if (translate)
                stream() << "Translate { x: " << -info.viewBox.x() << "; y: " << -info.viewBox.y() << " },";
            stream() << "Scale { xScale: width / " << info.viewBox.width() << "; yScale: height / " << info.viewBox.height() << " }";
            m_indentLevel--;
            stream() << "]";
        }

        generateNodeBase(info);
    } else {
        generateNodeEnd(info);
        if (isPathContainer)
            m_inShapeItemLevel--;

        if (!info.clipBox.isEmpty()) {
            m_indentLevel--;
            stream() << "}";
        }
    }

    return true;
}

bool QQuickQmlGenerator::generateMaskNode(const MaskNodeInfo &info)
{
    // Generate an invisible item subtree which can be used in ShaderEffectSource
    if (info.stage == StructureNodeStage::Start) {
        stream() << "Item {";
        m_indentLevel++;

        generateNodeBase(info);

        stream() << "width: originalBounds.width";
        stream() << "height: originalBounds.height";

        stream() << "property real maskX: " << info.maskRect.left();
        stream() << "property real maskY: " << info.maskRect.top();
        stream() << "property real maskWidth: " << info.maskRect.width();
        stream() << "property real maskHeight: " << info.maskRect.height();

        stream() << "function maskRect(otherX, otherY, otherWidth, otherHeight) {";
        m_indentLevel++;

        stream() << "return ";
        if (info.isMaskRectRelativeCoordinates) {
            stream(SameLine)
                << "Qt.rect("
                << info.id << ".maskX * otherWidth + otherX,"
                << info.id << ".maskY * otherHeight + otherY,"
                << info.id << ".maskWidth * otherWidth,"
                << info.id << ".maskHeight * otherHeight)";
        } else {
            stream(SameLine)
                << "Qt.rect("
                << info.id << ".maskX, "
                << info.id << ".maskY, "
                << info.id << ".maskWidth, "
                << info.id << ".maskHeight)";
        }

        m_indentLevel--;
        stream() << "}";

    } else {
        generateNodeEnd(info);
    }

    return true;
}

void QQuickQmlGenerator::generateFilterNode(const FilterNodeInfo &info)
{
    stream() << "QtObject {";
    m_indentLevel++;

    stream() << "id: " << info.id << "_filterParameters";

    stream() << "property int wrapMode: ";
    if (info.wrapMode == QSGTexture::Repeat)
        stream(SameLine) << "ShaderEffectSource.Repeat";
    else
        stream(SameLine) << "ShaderEffectSource.ClampToEdge";

    stream() << "property rect filterRect: Qt.rect("
             << info.filterRect.x() << ", "
             << info.filterRect.y() << ", "
             << info.filterRect.width() << ", "
             << info.filterRect.height() << ")";

    stream() << "function adaptToFilterRect(sx, sy, sw, sh) {";
    m_indentLevel++;

    // If the shader requires an offset to the source rect, we apply that here
    if (info.csFilterRect == FilterNodeInfo::CoordinateSystem::Absolute) {
        stream() << "return Qt.rect(filterRect.x, filterRect.y, filterRect.width, filterRect.height)";
    } else {
        stream() << "return Qt.rect("
                 << "sx + sw * filterRect.x, sy + sh * filterRect.y,"
                 << "sw * filterRect.width, sh * filterRect.height)";
    }

    m_indentLevel--;
    stream() << "}";

    m_indentLevel--;
    stream() << "}";

    stream() << "Component {";
    m_indentLevel++;

    stream() << "id: " << info.id << "_container";

    // Container for transform and effects
    stream() << "Item {";
    m_indentLevel++;

    stream() << "property real originalWidth: filterSourceItem.sourceItem.originalBounds.width";
    stream() << "property real originalHeight: filterSourceItem.sourceItem.originalBounds.height";
    stream() << "property rect filterRect: " << info.id << "_filterParameters"
             << ".adaptToFilterRect(0, 0, originalWidth, originalHeight)";

    generateNodeBase(info);

    for (qsizetype i = 0; i < info.steps.size();)
        i = generateFilterStep(info, i);

    generateNodeEnd(info);

    m_indentLevel--;
    stream() << "}"; // End of Component
}

qsizetype QQuickQmlGenerator::generateFilterStep(const FilterNodeInfo &info,
                                                 qsizetype stepIndex)
{
    const FilterNodeInfo::FilterStep &step = info.steps.at(stepIndex);
    const QString primitiveId = info.id + QStringLiteral("_primitive") + QString::number(stepIndex);

    stepIndex++;

    QString inputId = step.input1 != FilterNodeInfo::FilterInput::SourceColor
        ? step.namedInput1
        : QStringLiteral("filterSourceItem");

    bool isComposite = false;
    switch (step.filterType) {
    case FilterNodeInfo::Type::Merge:
    {
        const int maxNodeCount = 8;

        // Find all nodes for this merge
        QList<QPair<FilterNodeInfo::FilterInput, QString> > inputs;
        for (; stepIndex < info.steps.size(); ++stepIndex) {
            const FilterNodeInfo::FilterStep &nodeStep = info.steps.at(stepIndex);
            if (nodeStep.filterType != FilterNodeInfo::Type::MergeNode)
                break;

            inputs.append(qMakePair(nodeStep.input1, nodeStep.namedInput1));
        }

        if (inputs.size() > maxNodeCount) {
            qCWarning(lcQuickVectorImage) << "Maximum of" << maxNodeCount
                                          << "nodes exceeded in merge effect.";
        }

        if (inputs.isEmpty()) {
            qCWarning(lcQuickVectorImage) << "Merge effect requires at least one node.";
            break;
        }

        stream() << "ShaderEffect {";
        m_indentLevel++;

        stream() << "id: " << primitiveId;
        stream() << "visible: false";

        stream() << "fragmentShader: \"qrc:/qt-project.org/quickvectorimage/helpers/shaders_ng/femerge.frag.qsb\"";
        stream() << "width: source1.width";
        stream() << "height: source1.height";
        stream() << "property int sourceCount: " << std::min(qsizetype(8), inputs.size());

        for (int i = 0; i < maxNodeCount; ++i) {
            auto input = i < inputs.size()
                ? inputs.at(i)
                : qMakePair(FilterNodeInfo::FilterInput::None, QStringLiteral("null"));

            QString inputId = input.first != FilterNodeInfo::FilterInput::SourceColor
                                  ? input.second
                                  : QStringLiteral("filterSourceItem");

            stream() << "property var source" << (i + 1) << ": " << inputId;
        }

        m_indentLevel--;
        stream() << "}";

        break;
    }
    case FilterNodeInfo::Type::CompositeOver:
    case FilterNodeInfo::Type::CompositeOut:
    case FilterNodeInfo::Type::CompositeIn:
    case FilterNodeInfo::Type::CompositeXor:
    case FilterNodeInfo::Type::CompositeAtop:
    case FilterNodeInfo::Type::CompositeArithmetic:
    case FilterNodeInfo::Type::CompositeLighter:
        isComposite = true;
        Q_FALLTHROUGH();

    case FilterNodeInfo::Type::BlendNormal:
    case FilterNodeInfo::Type::BlendMultiply:
    case FilterNodeInfo::Type::BlendScreen:
    case FilterNodeInfo::Type::BlendDarken:
    case FilterNodeInfo::Type::BlendLighten:
    {
        stream() << "ShaderEffect {";
        m_indentLevel++;

        QString input2Id = step.input2 != FilterNodeInfo::FilterInput::SourceColor
              ? step.namedInput2
              : QStringLiteral("filterSourceItem");

        stream() << "id: " << primitiveId;
        stream() << "visible: false";

        QString shader;
        switch (step.filterType) {
        case FilterNodeInfo::Type::CompositeOver:
            shader = QStringLiteral("fecompositeover");
            break;
        case FilterNodeInfo::Type::CompositeOut:
            shader = QStringLiteral("fecompositeout");
            break;
        case FilterNodeInfo::Type::CompositeIn:
            shader = QStringLiteral("fecompositein");
            break;
        case FilterNodeInfo::Type::CompositeXor:
            shader = QStringLiteral("fecompositexor");
            break;
        case FilterNodeInfo::Type::CompositeAtop:
            shader = QStringLiteral("fecompositeatop");
            break;
        case FilterNodeInfo::Type::CompositeArithmetic:
            shader = QStringLiteral("fecompositearithmetic");
            break;
        case FilterNodeInfo::Type::CompositeLighter:
            shader = QStringLiteral("fecompositelighter");
            break;
        case FilterNodeInfo::Type::BlendNormal:
            shader = QStringLiteral("feblendnormal");
            break;
        case FilterNodeInfo::Type::BlendMultiply:
            shader = QStringLiteral("feblendmultiply");
            break;
        case FilterNodeInfo::Type::BlendScreen:
            shader = QStringLiteral("feblendscreen");
            break;
        case FilterNodeInfo::Type::BlendDarken:
            shader = QStringLiteral("feblenddarken");
            break;
        case FilterNodeInfo::Type::BlendLighten:
            shader = QStringLiteral("feblendlighten");
            break;
        default:
            Q_UNREACHABLE();
        }

        stream() << "fragmentShader: \"qrc:/qt-project.org/quickvectorimage/helpers/shaders_ng/"
                 << shader << ".frag.qsb\"";
        stream() << "property var source: " << inputId;
        stream() << "property var source2: " << input2Id;
        stream() << "width: source.width";
        stream() << "height: source.height";

        if (isComposite) {
            QVector4D k = step.filterParameter.value<QVector4D>();
            stream() << "property var k: Qt.vector4d("
                     << k.x() << ", "
                     << k.y() << ", "
                     << k.z() << ", "
                     << k.w() << ")";
        }

        m_indentLevel--;
        stream() << "}";

        break;

    }
    case FilterNodeInfo::Type::Flood:
    {
        stream() << "Rectangle {";
        m_indentLevel++;

        stream() << "id: " << primitiveId;
        stream() << "visible: false";

        stream() << "width: " << inputId << ".width";
        stream() << "height: " << inputId << ".height";

        QColor floodColor = step.filterParameter.value<QColor>();
        stream() << "color: \"" << floodColor.name(QColor::HexArgb) << "\"";

        m_indentLevel--;
        stream() << "}";

        break;
    }
    case FilterNodeInfo::Type::ColorMatrix:
    {
        stream() << "ShaderEffect {";
        m_indentLevel++;

        stream() << "id: " << primitiveId;
        stream() << "visible: false";

        stream() << "fragmentShader: \"qrc:/qt-project.org/quickvectorimage/helpers/shaders_ng/fecolormatrix.frag.qsb\"";
        stream() << "property var source: " << inputId;
        stream() << "width: source.width";
        stream() << "height: source.height";

        QGenericMatrix<5, 5, qreal> matrix = step.filterParameter.value<QGenericMatrix<5, 5, qreal> >();
        for (int row = 0; row < 4; ++row) { // Last row is ignored

            // Qt SVG stores rows as columns, so we flip the coordinates
            for (int col = 0; col < 5; ++col)
                stream() << "property real m_" << row << "_" << col << ": " << matrix(col, row);
        }

        m_indentLevel--;
        stream() << "}";

        break;
    }

    case FilterNodeInfo::Type::Offset:
    {
        stream() << "ShaderEffectSource {";
        m_indentLevel++;

        stream() << "id: " << primitiveId;
        stream() << "visible: false";
        stream() << "sourceItem: " << inputId;
        stream() << "width: sourceItem.width + offset.x";
        stream() << "height: sourceItem.height + offset.y";

        QVector2D offset = step.filterParameter.value<QVector2D>();
        stream() << "property vector2d offset: Qt.vector2d(";
        if (step.csFilterParameter == FilterNodeInfo::CoordinateSystem::Absolute)
            stream(SameLine) << offset.x() << " / width, " << offset.y() << " / height)";
        else
            stream(SameLine) << offset.x() << ", " << offset.y() << ")";

        stream() << "sourceRect: Qt.rect(-offset.x, -offset.y, width, height)";

        stream() << "ItemSpy {";
        m_indentLevel++;
        stream() << "id: " << primitiveId << "_offset_itemspy";
        stream() << "anchors.fill: parent";

        m_indentLevel--;
        stream() << "}";
        stream() << "textureSize: " << primitiveId << "_offset_itemspy.requiredTextureSize";


        m_indentLevel--;
        stream() << "}";

        break;
    }

    case FilterNodeInfo::Type::GaussianBlur:
    {
        // Approximate blur effect with fast blur
        stream() << "MultiEffect {";
        m_indentLevel++;

        stream() << "id: " << primitiveId;
        stream() << "visible: false";

        stream() << "source: " << inputId;
        stream() << "blurEnabled: true";
        stream() << "width: source.width";
        stream() << "height: source.height";

        const qreal maxDeviation(12.0); // Decided experimentally
        const qreal deviation = step.filterParameter.toReal();
        if (step.csFilterParameter == FilterNodeInfo::CoordinateSystem::Relative)
            stream() << "blur: Math.min(1.0, " << deviation << " * filterSourceItem.width / " << maxDeviation << ")";
        else
            stream() << "blur: " << std::min(qreal(1.0), deviation / maxDeviation);
        stream() << "blurMax: 64";

        m_indentLevel--;
        stream() << "}";

        break;
    }
    default:
        qCWarning(lcQuickVectorImage) << "Unhandled filter type: " << int(step.filterType);
        // Dummy item to avoid empty component
        stream() << "Item { id: " << primitiveId << " }";
        break;
    }

    // Sample correct part of primitive
    stream() << "ShaderEffectSource {";
    m_indentLevel++;

    stream() << "id: " << step.outputName;
    if (stepIndex < info.steps.size())
        stream() << "visible: false";

    qreal x1, x2, y1, y2;
    step.filterPrimitiveRect.getCoords(&x1, &y1, &x2, &y2);
    if (step.csFilterParameter == FilterNodeInfo::CoordinateSystem::Absolute) {
        stream() << "property real fpx1: " << x1;
        stream() << "property real fpy1: " << y1;
        stream() << "property real fpx2: " << x2;
        stream() << "property real fpy2: " << y2;
    } else if (step.csFilterParameter == FilterNodeInfo::CoordinateSystem::Relative) {
        // If they are relative, they are actually in the coordinate system
        // of the original bounds of the filtered item. This means we first have to convert
        // them to the filter's coordinate system first.
        stream() << "property real fpx1: " << x1 << " * filterSourceItem.sourceItem.originalBounds.width";
        stream() << "property real fpy1: " << y1 << " * filterSourceItem.sourceItem.originalBounds.height";
        stream() << "property real fpx2: " << x2 << " * filterSourceItem.sourceItem.originalBounds.width";
        stream() << "property real fpy2: " << y2 << " * filterSourceItem.sourceItem.originalBounds.height";
    } else { // Just match filter rect
        stream() << "property real fpx1: filterRect.x";
        stream() << "property real fpy1: filterRect.y";
        stream() << "property real fpx2: filterRect.x + filterRect.width";
        stream() << "property real fpy2: filterRect.y + filterRect.height";
    }

    stream() << "sourceItem: " << primitiveId;
    stream() << "sourceRect: Qt.rect(fpx1 - filterRect.x, fpy1 - filterRect.y, width, height)";

    stream() << "x: fpx1";
    stream() << "y: fpy1";
    stream() << "width: " << "fpx2 - fpx1";
    stream() << "height: " << "fpy2 - fpy1";

    stream() << "ItemSpy {";
    m_indentLevel++;
    stream() << "id: " << primitiveId << "_itemspy";
    stream() << "anchors.fill: parent";

    m_indentLevel--;
    stream() << "}";
    stream() << "textureSize: " << primitiveId << "_itemspy.requiredTextureSize";

    m_indentLevel--;
    stream() << "}";

    return stepIndex;
}

bool QQuickQmlGenerator::generateRootNode(const StructureNodeInfo &info)
{
    const QStringList comments = m_commentString.split(u'\n');

    if (!isNodeVisible(info)) {
        m_indentLevel = 0;

        if (comments.isEmpty()) {
            stream() << "// Generated from SVG";
        } else {
            for (const auto &comment : comments)
                stream() << "// " << comment;
        }

        stream() << "import QtQuick";
        stream() << "import QtQuick.Shapes" << Qt::endl;
        stream() << "Item {";
        m_indentLevel++;

        double w = info.size.width();
        double h = info.size.height();
        if (w > 0)
            stream() << "implicitWidth: " << w;
        if (h > 0)
            stream() << "implicitHeight: " << h;

        m_indentLevel--;
        stream() << "}";

        return false;
    }

    if (info.stage == StructureNodeStage::Start) {
        m_indentLevel = 0;

        if (comments.isEmpty())
            stream() << "// Generated from SVG";
        else
            for (const auto &comment : comments)
                stream() << "// " << comment;

        stream() << "import QtQuick";
        stream() << "import QtQuick.VectorImage";
        stream() << "import QtQuick.VectorImage.Helpers";
        stream() << "import QtQuick.Shapes";
        stream() << "import QtQuick.Effects";

        for (const auto &import : std::as_const(m_extraImports))
            stream() << "import " << import;

        stream() << Qt::endl << "Item {";
        m_indentLevel++;

        double w = info.size.width();
        double h = info.size.height();
        if (w > 0)
            stream() << "implicitWidth: " << w;
        if (h > 0)
            stream() << "implicitHeight: " << h;

        if (Q_UNLIKELY(!isRuntimeGenerator())) {
            stream() << "component AnimationsInfo : QtObject";
            stream() << "{";
            m_indentLevel++;
        }

        stream() << "property bool paused: false";
        stream() << "property int loops: 1";
        stream() << "signal restart()";

        if (Q_UNLIKELY(!isRuntimeGenerator())) {
            m_indentLevel--;
            stream() << "}";
            stream() << "property AnimationsInfo animations : AnimationsInfo {}";
        }

        if (!info.viewBox.isEmpty()) {
            stream() << "transform: [";
            m_indentLevel++;
            bool translate = !qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y());
            if (translate)
                stream() << "Translate { x: " << -info.viewBox.x() << "; y: " << -info.viewBox.y() << " },";
            stream() << "Scale { xScale: width / " << info.viewBox.width() << "; yScale: height / " << info.viewBox.height() << " }";
            m_indentLevel--;
            stream() << "]";
        }

        if (!info.forceSeparatePaths && info.isPathContainer) {
            m_topLevelIdString = QStringLiteral("__qt_toplevel");
            stream() << "id: " << m_topLevelIdString;

            generatePathContainer(info);
            m_indentLevel++;

            generateNodeBase(info);
        } else {
            m_topLevelIdString = generateNodeBase(info);
            if (m_topLevelIdString.isEmpty())
                qCWarning(lcQuickVectorImage) << "No ID specified for top level item";
        }
    } else {
        if (m_inShapeItemLevel > 0) {
            m_inShapeItemLevel--;
            m_indentLevel--;
            stream() << "}";
        }

        for (const auto [coords, id] : m_easings.asKeyValueRange()) {
            stream() << "readonly property easingCurve " << id << ": ({ type: Easing.BezierSpline, bezierCurve: [ ";
            for (auto coord : coords)
                stream(SameLine) << coord << ", ";
            stream(SameLine) << "1, 1 ] })";
        }

        generateNodeEnd(info);
        stream().flush();
    }

    return true;
}

QStringView QQuickQmlGenerator::indent()
{
    static QString indentString;
    int indentWidth = m_indentLevel * 4;
    if (indentWidth > indentString.size())
        indentString.fill(QLatin1Char(' '), indentWidth * 2);
    return QStringView(indentString).first(indentWidth);
}

QTextStream &QQuickQmlGenerator::stream(int flags)
{
    if (m_stream.device() == nullptr)
        m_stream.setDevice(&m_result);
    else if (!(flags & StreamFlags::SameLine))
        m_stream << Qt::endl << indent();
    return m_stream;
}

const char *QQuickQmlGenerator::shapeName() const
{
    return m_shapeTypeName.isEmpty() ? "Shape" : m_shapeTypeName.constData();
}

QT_END_NAMESPACE
