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
#include <QtCore/qstandardpaths.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QString sanitizeString(const QString &input)
{
    QString s = input;
    s.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
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
    m_oldIndentLevels.push(0);

    if (outFileName.isEmpty()) {
        setRetainFilePaths(true);
        setAssetFileDirectory(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        setAssetFilePrefix(QStringLiteral("_qt_vectorimage_"));
        setUrlPrefix(QStringLiteral("file:"));
    }
}

QQuickQmlGenerator::~QQuickQmlGenerator()
{
}

bool QQuickQmlGenerator::save()
{
    if (Q_UNLIKELY(errorState()))
        return false;

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

QString QQuickQmlGenerator::generateNodeBase(const NodeInfo &info, const QString &idSuffix)
{
    static qint64 maxNodes = qEnvironmentVariableIntegerValue("QT_QUICKVECTORIMAGE_MAX_NODES").value_or(10000);
    if (Q_UNLIKELY(!checkSanityLimit(++m_nodeCounter, maxNodes, "nodes"_L1)))
        return {};

    if (!info.nodeId.isEmpty())
        stream() << "objectName: \"" << info.nodeId << "\"";

    if (!info.id.isEmpty())
        stream() << "id: " << info.id << idSuffix;

    if (!info.bounds.isNull() || !info.boundsReferenceId.isEmpty()) {
        stream() << "property var originalBounds: ";
        if (!info.bounds.isNull()) {
            stream(SameLine) << "Qt.rect(" << info.bounds.x() << ", " << info.bounds.y() << ", "
                             << info.bounds.width() << ", " << info.bounds.height() << ")";
        } else {
            stream(SameLine) << info.boundsReferenceId << ".originalBounds";
        }
        stream() << "width: originalBounds.width";
        stream() << "height: originalBounds.height";
    }

    stream() << "transformOrigin: Item.TopLeft";

    if (info.filterId.isEmpty() && info.maskId.isEmpty()) {
        if (!info.isDefaultOpacity)
            stream() << "opacity: " << info.opacity.defaultValue().toReal();
        generateItemAnimations(info.id, info);
    }

    return info.id;
}

void QQuickQmlGenerator::generateNodeEnd(const NodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return;
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
                QVariantList defaultProps = info.motionPath.defaultValue().value<QVariantList>();
                const bool adaptAngle = defaultProps.value(1).toBool();
                const qreal baseRotation = defaultProps.value(2).toReal();
                QString interpolatorId = idString + QStringLiteral("_motion_interpolator");
                if (adaptAngle || !qFuzzyIsNull(baseRotation)) {
                    stream() << "Rotation {";
                    m_indentLevel++;

                    if (adaptAngle) {
                        stream() << "angle: " << interpolatorId << ".angle";
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

                stream() << "x: " << interpolatorId << ".x";
                stream() << "y: " << interpolatorId << ".y";

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

bool QQuickQmlGenerator::generateDefsNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    if (info.stage == StructureNodeStage::Start) {
        m_oldIndentLevels.push(m_indentLevel);

        stream() << "Component {";
        m_indentLevel++;

        stream() << "id: " << info.id << "_container";

        stream() << "Item {";
        m_indentLevel++;

        generateTimelineFields(info);
        if (!info.transformReferenceChildId.isEmpty()) {
            stream() << "property alias transformMatrix: "
                     << info.transformReferenceChildId << ".transformMatrix";
        }

        generateNodeBase(info, QStringLiteral("_defs"));
    } else {
        generateNodeEnd(info);

        m_indentLevel--;
        stream() << "}"; // Component

        stream() << m_defsSuffix;
        m_defsSuffix.clear();

        m_indentLevel = m_oldIndentLevels.pop();
    }

    return true;
}

void QQuickQmlGenerator::generateImageNode(const ImageNodeInfo &info)
{
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
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

void QQuickQmlGenerator::generateMarkers(const PathNodeInfo &info)
{
    const QPainterPath path = info.path.defaultValue().value<QPainterPath>();
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element element = path.elementAt(i);
        QString markerId;
        qreal angle = 0;

        // Copied from Qt SVG
        auto getMeanAngle = [](QPointF p0, QPointF p1, QPointF p2) -> qreal {
            QPointF t1 = p1 - p0;
            QPointF t2 = p2 - p1;
            qreal hyp1 =  hypot(t1.x(), t1.y());
            if (hyp1 > 0)
                t1 /= hyp1;
            else
                return 0.;
            qreal hyp2 =  hypot(t2.x(), t2.y());
            if (hyp2 > 0)
                t2 /= hyp2;
            else
                return 0.;
            QPointF tangent = t1 + t2;
            return -atan2(tangent.y(), tangent.x()) / M_PI * 180.;
        };

        if (i == 0) {
            markerId = info.markerStartId;
            angle = path.angleAtPercent(0.0);
        } else if (i == path.elementCount() - 1) {
            markerId = info.markerEndId;
            angle = path.angleAtPercent(1.0);
        } else if (path.elementAt(i + 1).type != QPainterPath::CurveToDataElement) {
            markerId = info.markerMidId;

            const QPainterPath::Element prevElement = path.elementAt(i - 1);
            const QPainterPath::Element nextElement = path.elementAt(i + 1);

            QPointF p1(prevElement.x, prevElement.y);
            QPointF p2(element.x, element.y);
            QPointF p3(nextElement.x, nextElement.y);

            angle = getMeanAngle(p1, p2, p3);
        }

        if (!markerId.isEmpty()) {
            stream() << "Loader {";
            m_indentLevel++;

            //stream() << "clip: true";
            stream() << "sourceComponent: " << markerId << "_container";
            stream() << "property real strokeWidth: " << info.strokeStyle.width.defaultValue().toReal();
            stream() << "transform: [";
            m_indentLevel++;
            if (i == 0) {
                stream() << "Scale { "
                         << "xScale: " << markerId << "_markerParameters.startReversed ? -1 : 1; "
                         << "yScale: " << markerId << "_markerParameters.startReversed ? -1 : 1 },";
            }
            stream() << "Rotation { angle: " << markerId << "_markerParameters.autoAngle(" << -angle << ") },";
            stream() << "Translate { x: " << element.x << "; y: " << element.y << "}";

            m_indentLevel--;
            stream() << "]";

            m_indentLevel--;
            stream() << "}";
        }
    }
}

void QQuickQmlGenerator::generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
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

        if (!info.markerStartId.isEmpty()
            || !info.markerMidId.isEmpty()
            || !info.markerEndId.isEmpty()) {
            generateMarkers(info);
        }

        generateNodeEnd(info);
        m_inShapeItemLevel--;
    }
}

void QQuickQmlGenerator::generateGradient(const QGradient *grad,
                                          const QString &propertyName,
                                          const QRectF &coordinateConversion)
{
    const QSizeF &scale = coordinateConversion.size();
    const QPointF &translation = coordinateConversion.topLeft();

    if (grad->type() == QGradient::LinearGradient) {
        auto *linGrad = static_cast<const QLinearGradient *>(grad);
        stream() << propertyName << ": LinearGradient {";
        m_indentLevel++;

        QRectF gradRect(linGrad->start(), linGrad->finalStop());

        stream() << "x1: " << (gradRect.left() * scale.width()) + translation.x();
        stream() << "y1: " << (gradRect.top() * scale.height()) + translation.y();
        stream() << "x2: " << (gradRect.right() * scale.width()) + translation.x();
        stream() << "y2: " << (gradRect.bottom() * scale.height()) + translation.y();
        for (auto &stop : linGrad->stops())
            stream() << "GradientStop { position: " << QString::number(stop.first, 'g', 7)
                     << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
    } else if (grad->type() == QGradient::RadialGradient) {
        auto *radGrad = static_cast<const QRadialGradient*>(grad);
        stream() << propertyName << ": RadialGradient {";
        m_indentLevel++;

        stream() << "centerX: " << (radGrad->center().x() * scale.width()) + translation.x();
        stream() << "centerY: " << (radGrad->center().y() * scale.height()) + translation.y();
        stream() << "centerRadius: " << (radGrad->radius() * scale.width()); // ### ?
        stream() << "focalX:" << (radGrad->focalPoint().x() * scale.width()) + translation.x();
        stream() << "focalY:" << (radGrad->focalPoint().y() * scale.height()) + translation.y();
        for (auto &stop : radGrad->stops())
            stream() << "GradientStop { position: " << QString::number(stop.first, 'g', 7)
                     << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
    }

    stream() << "spread: ShapeGradient.";
    switch (grad->spread()) {
    case QGradient::PadSpread:
        stream(SameLine) << "PadSpread";
        break;
    case QGradient::ReflectSpread:
        stream(SameLine) << "ReflectSpread";
        break;
    case QGradient::RepeatSpread:
        stream(SameLine) << "RepeatSpread";
        break;
    }

    m_indentLevel--;
    stream() << "}";
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

void QQuickQmlGenerator::generateEasing(const QQuickAnimatedProperty::PropertyAnimation &animation,
                                        int time, int streamFlags)
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
            if (streamFlags & SameLine)
                stream(streamFlags) << "; ";
            stream(streamFlags) << "easing: " << m_topLevelIdString << "." << id;
        }
    }
}

static int processAnimationTime(int time)
{
    static qreal multiplier = qreal(qEnvironmentVariable("QT_QUICKVECTORIMAGE_TIME_DILATION", QStringLiteral("1.0"))
                                        .toDouble());
    return std::round(multiplier * time);
}

void QQuickQmlGenerator::generatePropertyAnimation(const QQuickAnimatedProperty &property,
                                                   const QString &targetName,
                                                   const QString &propertyName,
                                                   AnimationType animationType)
{
    if (!property.isAnimated())
        return;

    if (usingTimelineAnimation())
        return generatePropertyTimeline(property, targetName, propertyName, animationType);

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

        const int startOffset = processAnimationTime(animation.startOffset);
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
            const int frameTime = processAnimationTime(time - previousTime);
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

void QQuickQmlGenerator::generateTimelinePropertySetter(
        const QString &targetName,
        const QString &propertyName,
        const QQuickAnimatedProperty::PropertyAnimation &animation,
        std::function<QVariant(const QVariant &)> const& extractValue,
        int valueIndex)
{
    if (animation.repeatCount != 1 || animation.startOffset
        || animation.flags != QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd) {
        qCWarning(lcQuickVectorImage) << "Animation feature not implemented in timeline mode, for"
                                      << targetName << propertyName;
    }

    stream() << "KeyframeGroup {";
    m_indentLevel++;
    stream() << "target: " << targetName;
    stream() << "property: \"" << propertyName << "\"";

    for (const auto &[frame, rawValue] : animation.frames.asKeyValueRange()) {
        QVariant value;
        if (rawValue.typeId() == QMetaType::QVariantList)
            value = extractValue(rawValue.toList().value(valueIndex));
        else
            value = extractValue(rawValue);

        stream() << "Keyframe { frame: " << frame << "; value: ";
        if (value.typeId() == QMetaType::QVector3D) {
            const QVector3D &v = value.value<QVector3D>();
            stream(SameLine) << "Qt.vector3d(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
        } else if (value.typeId() == QMetaType::QColor) {
            stream(SameLine) << "\"" << value.toString() << "\"";
        } else {
            stream(SameLine) << value.toReal();
        }
        generateEasing(animation, frame, SameLine);
        stream(SameLine) << " }";
    }

    m_indentLevel--;
    stream() << "}";
}

void QQuickQmlGenerator::generatePropertyTimeline(const QQuickAnimatedProperty &property,
                                                  const QString &targetName,
                                                  const QString &propertyName,
                                                  AnimationType animationType)
{
    if (animationType == QQuickQmlGenerator::AnimationType::ColorOpacity) {
        qCWarning(lcQuickVectorImage) << "ColorOpacity animation not available in timeline mode";
        return;
    }

    if (property.animationGroupCount() > 1 || property.animationCount() > 1) {
        qCWarning(lcQuickVectorImage) << "Property feature not implemented in timeline mode, for"
                                      << targetName << propertyName;
    }

    stream() << "Timeline {";
    m_indentLevel++;
    stream() << "currentFrame: " << property.timelineReferenceId() << ".frameCounter";
    stream() << "enabled: true";

    auto extractor = [](const QVariant &value) { return value; };
    generateTimelinePropertySetter(targetName, propertyName, property.animation(0), extractor);

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

    if (Q_UNLIKELY(errorState()))
        return;

    const bool invalidGradientBounds = info.strokeGrad.coordinateMode() == QGradient::ObjectMode
                                       && (qFuzzyIsNull(boundingRect.width()) ||
                                           qFuzzyIsNull(boundingRect.height()));

    const QColor strokeColor = info.strokeStyle.color.defaultValue().value<QColor>();
    const bool noPen = (strokeColor == QColorConstants::Transparent || !strokeColor.isValid())
                       && !info.strokeStyle.color.isAnimated()
                       && !info.strokeStyle.opacity.isAnimated()
                       && (info.strokeGrad.type() == QGradient::NoGradient
                           || invalidGradientBounds);
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
        if (info.strokeGrad.type() != QGradient::NoGradient && !invalidGradientBounds) {
            QRectF coordinateSys = info.strokeGrad.coordinateMode() == QGradient::ObjectMode
                                       ? boundingRect
                                       : QRectF(0.0, 0.0, 1.0, 1.0);
            generateGradient(&info.strokeGrad, QStringLiteral("strokeGradient"), coordinateSys);
        } else if (info.strokeStyle.opacity.isAnimated()) {
            stream() << "property color strokeBase: \"" << strokeColor.name(QColor::HexArgb) << "\"";
            stream() << "property real strokeOpacity: " << info.strokeStyle.opacity.defaultValue().toReal();
            stream() << "strokeColor: Qt.rgba(strokeBase.r, strokeBase.g, strokeBase.b, strokeOpacity)";
        } else {
            stream() << "strokeColor: \"" << strokeColor.name(QColor::HexArgb) << "\"";
        }
        stream() << "strokeWidth: " << info.strokeStyle.width.defaultValue().toReal();
        stream() << "capStyle: " << QQuickVectorImageGenerator::Utils::strokeCapStyleString(info.strokeStyle.lineCapStyle);
        stream() << "joinStyle: " << QQuickVectorImageGenerator::Utils::strokeJoinStyleString(info.strokeStyle.lineJoinStyle);
        stream() << "miterLimit: " << info.strokeStyle.miterLimit;
        if (info.strokeStyle.cosmetic)
            stream() << "cosmeticStroke: true";
        if (info.strokeStyle.dashArray.length() != 0) {
            stream() << "strokeStyle: " << "ShapePath.DashLine";
            stream() << "dashPattern: " << QQuickVectorImageGenerator::Utils::listString(info.strokeStyle.dashArray);
            stream() << "dashOffset: " << info.strokeStyle.dashOffset.defaultValue().toReal();
        }
    }

    QTransform fillTransform = info.fillTransform;
    if (!(pathSelector & QQuickVectorImageGenerator::FillPath)) {
        stream() << "fillColor: \"transparent\"";
    } else if (info.grad.type() != QGradient::NoGradient) {
        generateGradient(&info.grad, QStringLiteral("fillGradient"));

        // Scaling done via fillTransform to get correct order of operations
        if (info.grad.coordinateMode() == QGradient::ObjectMode) {
            QTransform objectToUserSpace;
            objectToUserSpace.translate(boundingRect.x(), boundingRect.y());
            objectToUserSpace.scale(boundingRect.width(), boundingRect.height());
            fillTransform *= objectToUserSpace;
        }
    } else {
        if (info.fillOpacity.isAnimated()) {
            stream() << "property color fillBase: \"" << fillColor.name(QColor::HexArgb) << "\"";
            stream() << "property real fillOpacity:" << info.fillOpacity.defaultValue().toReal();
            stream() << "fillColor: Qt.rgba(fillBase.r, fillBase.g, fillBase.b, fillOpacity)";
        } else {
            stream() << "fillColor: \"" << fillColor.name(QColor::HexArgb) << "\"";
        }
    }

    if (!info.patternId.isEmpty()) {
        stream() << "fillItem: ShaderEffectSource {";
        m_indentLevel++;

        stream() << "parent: " << info.id;
        stream() << "sourceItem: " << info.patternId;
        stream() << "hideSource: true";
        stream() << "visible: false";
        stream() << "width: " << info.patternId << ".width";
        stream() << "height: " << info.patternId << ".height";
        stream() << "wrapMode: ShaderEffectSource.Repeat";
        stream() << "textureSize: Qt.size(width * __qt_toplevel_scale_itemspy.requiredTextureSize.width, "
                 << "height * __qt_toplevel_scale_itemspy.requiredTextureSize.height)";;
        stream() << "sourceRect: " << info.patternId << ".sourceRect("
                 << info.id << ".width, "
                 << info.id << ".height)";

        m_indentLevel--;
        stream() << "}";

        // Fill transform has to include the inverse of the scene scale, since the texture size
        // is scaled by this amount
        stream() << "function calculateFillTransform(xScale, yScale) {";
        m_indentLevel++;

        stream() << "var m = ";
        generateTransform(fillTransform);

        stream() << "m.translate(" << info.patternId << ".sourceOffset("
                 << info.id << ".width, "
                 << info.id << ".height))";

        stream() << "m.scale(1.0 / xScale, 1.0 / yScale, 1.0)";
        stream() << "return m";

        m_indentLevel--;
        stream() << "}";

        stream() << "fillTransform: calculateFillTransform(__qt_toplevel_scale_itemspy.requiredTextureSize.width, "
                 << "__qt_toplevel_scale_itemspy.requiredTextureSize.height)";

    } else if (!fillTransform.isIdentity()) {
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
    pathFactor.setTimelineReferenceId(info.path.timelineReferenceId());
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

    if (info.strokeStyle.opacity.isAnimated()) {
        generatePropertyAnimation(info.strokeStyle.color, shapePathId, QStringLiteral("strokeBase"));
        generatePropertyAnimation(info.strokeStyle.opacity, shapePathId, QStringLiteral("strokeOpacity"));
    } else {
        generatePropertyAnimation(info.strokeStyle.color, shapePathId, QStringLiteral("strokeColor"));
    }
    if (info.strokeStyle.width.isAnimated())
        generatePropertyAnimation(info.strokeStyle.width, shapePathId, QStringLiteral("strokeWidth"));
    if (info.strokeStyle.dashOffset.isAnimated())
        generatePropertyAnimation(info.strokeStyle.dashOffset, shapePathId, QStringLiteral("dashOffset"));

    if (info.fillOpacity.isAnimated()) {
        generatePropertyAnimation(info.fillColor, shapePathId, QStringLiteral("fillBase"));
        generatePropertyAnimation(info.fillOpacity, shapePathId, QStringLiteral("fillOpacity"));
    } else {
        generatePropertyAnimation(info.fillColor, shapePathId, QStringLiteral("fillColor"));
    }
}

void QQuickQmlGenerator::generateNode(const NodeInfo &info)
{
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
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
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
        return;

    stream() << "Item {";
    m_indentLevel++;
    generateNodeBase(info);

    if (!info.isTextArea)
        stream() << "Item { id: textAlignItem_" << m_textNodeCounter << "; x: " << info.position.x() << "; y: " << info.position.y() << "}";

    stream() << "Text {";

    m_indentLevel++;

    const QString textItemId = QStringLiteral("_qt_textItem_%1").arg(m_textNodeCounter);
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
        stream() << "anchors.baseline: textAlignItem_" << m_textNodeCounter << ".top";
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
        stream() << "anchors." << hAlign << ": textAlignItem_" << m_textNodeCounter << ".left";
    }
    m_textNodeCounter++;

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
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
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

    QPainterPath path = property.defaultValue().value<QVariantList>().value(0).value<QPainterPath>();
    const QString mainAnimationId = targetName + QStringLiteral("_motion_interpolator");
    stream() << "PathInterpolator {";
    m_indentLevel++;
    stream() << "id: " << mainAnimationId;
    const QString svgPathString = QQuickVectorImageGenerator::Utils::toSvgString(path);
    stream() << "path: Path { PathSvg { path: \"" << svgPathString << "\" } }";
    m_indentLevel--;
    stream() << "}";

    generatePropertyAnimation(property, mainAnimationId, QStringLiteral("progress"));
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

    if (usingTimelineAnimation())
        return generateTransformTimeline(targetName, info);

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

        const int startOffset = processAnimationTime(firstAnimation.startOffset);
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
                const int frameTime = processAnimationTime(time - previousTime);
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

void QQuickQmlGenerator::generateTransformTimeline(const QString &targetName, const NodeInfo &info)
{
    stream() << "Timeline {";
    m_indentLevel++;
    stream() << "currentFrame: " << info.transform.timelineReferenceId() << ".frameCounter";
    stream() << "enabled: true";

    const int groupIndex = 0;
    for (int i = 0; i < info.transform.animationCount(); ++i) {
        const QQuickAnimatedProperty::PropertyAnimation &animation = info.transform.animation(i);
        if (animation.isConstant())
            continue;
        if (info.transform.animationGroupCount() > 1
            || animation.repeatCount != 1 || animation.startOffset
            || animation.flags != QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd) {
            qCWarning(lcQuickVectorImage) << "Feature not implemented in timeline xf animation mode, for"
                                          << targetName << "subtype" << animation.subtype;
        }

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

        auto pointFxExtractor = [](const QVariant &value) { return value.toPointF().x(); };
        auto pointFyExtractor = [](const QVariant &value) { return value.toPointF().y(); };
        auto realExtractor = [](const QVariant &value) { return value.toReal(); };
        auto pointFtoVector3dExtractor = [](const QVariant &v) { return QVector3D(v.toPointF()); };

        const QString propertyTargetName = targetName
                + QStringLiteral("_transform_")
                + QString::number(groupIndex)
                + QStringLiteral("_")
                + QString::number(i);

        switch (animation.subtype) {
        case QTransform::TxTranslate:
            generateTimelinePropertySetter(propertyTargetName, "x"_L1, animation, pointFxExtractor);
            generateTimelinePropertySetter(propertyTargetName, "y"_L1, animation, pointFyExtractor);
            break;
        case QTransform::TxScale:
            generateTimelinePropertySetter(propertyTargetName, "xScale"_L1, animation, pointFxExtractor);
            generateTimelinePropertySetter(propertyTargetName, "yScale"_L1, animation, pointFyExtractor);
            break;
        case QTransform::TxRotate:
            if (hasRotationCenter)
                generateTimelinePropertySetter(propertyTargetName, "origin"_L1, animation, pointFtoVector3dExtractor);
            generateTimelinePropertySetter(propertyTargetName, "angle"_L1, animation, realExtractor, 1);
            break;
        case QTransform::TxShear:
            generateTimelinePropertySetter(propertyTargetName, "xAngle"_L1, animation, pointFxExtractor);
            generateTimelinePropertySetter(propertyTargetName, "yAngle"_L1, animation, pointFyExtractor);
            break;
        }
    }

    m_indentLevel--;
    stream() << "}";
}

bool QQuickQmlGenerator::generateStructureNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState() || !isNodeVisible(info)))
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

        generateTimelineFields(info);

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
    if (Q_UNLIKELY(errorState()))
        return false;

    // Generate an invisible item subtree which can be used in ShaderEffectSource
    if (info.stage == StructureNodeStage::End) {
        // Generate code to add after defs block
        startDefsSuffixBlock();
        stream() << "Loader {";
        m_indentLevel++;

        stream() << "id: " << info.id; // This is in a different scope, so we can reuse the ID
        stream() << "visible: false";
        stream() << "sourceComponent: " << info.id << "_container";
        stream() << "width: item !== null ? item.originalBounds.width : 0";
        stream() << "height: item !== null ? item.originalBounds.height : 0";

        if (info.boundsReferenceId.isEmpty()) {
            stream() << "property real maskX: " << info.maskRect.left();
            stream() << "property real maskY: " << info.maskRect.top();
            stream() << "property real maskWidth: " << info.maskRect.width();
            stream() << "property real maskHeight: " << info.maskRect.height();
        }

        stream() << "function maskRect(otherX, otherY, otherWidth, otherHeight) {";
        m_indentLevel++;

        stream() << "return ";
        if (!info.boundsReferenceId.isEmpty()) {
            stream(SameLine) << info.boundsReferenceId << ".originalBounds";
        } else if (info.isMaskRectRelativeCoordinates) {
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

        m_indentLevel--;
        stream() << "}";

        endDefsSuffixBlock();
    }

    return true;
}

void QQuickQmlGenerator::generateFilterNode(const FilterNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return;

    stream() << "Item {";
    m_indentLevel++;

    generateNodeBase(info);

    stream() << "property real originalWidth: filterSourceItem.sourceItem.originalBounds.width";
    stream() << "property real originalHeight: filterSourceItem.sourceItem.originalBounds.height";
    stream() << "property rect filterRect: " << info.id << "_filterParameters"
             << ".adaptToFilterRect(0, 0, originalWidth, originalHeight)";

    for (qsizetype i = 0; i < info.steps.size();)
        i = generateFilterStep(info, i);

    // Generate code to be added after defs block
    startDefsSuffixBlock();
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

    if (info.csFilterRect == FilterNodeInfo::CoordinateSystem::Absolute) {
        stream() << "return Qt.rect(filterRect.x, filterRect.y, filterRect.width, filterRect.height)";
    } else {
        stream() <<  "return Qt.rect(sx + sw * filterRect.x, sy + sh * filterRect.y, sw * filterRect.width, sh * filterRect.height)";
    }

    m_indentLevel--;
    stream() << "}";

    m_indentLevel--;
    stream() << "}";
    endDefsSuffixBlock();

    generateNodeEnd(info);
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
        QList<std::pair<FilterNodeInfo::FilterInput, QString> > inputs;
        for (; stepIndex < info.steps.size(); ++stepIndex) {
            const FilterNodeInfo::FilterStep &nodeStep = info.steps.at(stepIndex);
            if (nodeStep.filterType != FilterNodeInfo::Type::MergeNode)
                break;

            inputs.emplace_back(nodeStep.input1, nodeStep.namedInput1);
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
                : std::pair(FilterNodeInfo::FilterInput::None, u"null"_s);

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
        stream() << "property real fpx1: parent.filterRect.x";
        stream() << "property real fpy1: parent.filterRect.y";
        stream() << "property real fpx2: parent.filterRect.x + parent.filterRect.width";
        stream() << "property real fpy2: parent.filterRect.y + parent.filterRect.height";
    }

    stream() << "sourceItem: " << primitiveId;
    stream() << "sourceRect: Qt.rect(fpx1 - parent.filterRect.x, fpy1 - parent.filterRect.y, width, height)";

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

void QQuickQmlGenerator::generateTimelineFields(const StructureNodeInfo &info)
{
    if (usingTimelineAnimation() && info.timelineInfo) {
        QString frameCounterRef = info.timelineInfo->frameCounterReference
                + QStringLiteral(".frameCounter");

        if (info.timelineInfo->generateVisibility) {
            stream() << "visible: " << frameCounterRef << " >= " << info.timelineInfo->startFrame
                     << " && " << frameCounterRef << " < " << info.timelineInfo->endFrame;
        }

        if (info.timelineInfo->generateFrameCounter) {
            stream() << "property real frameCounter: ";
            if (info.timelineInfo->frameCounterMapper.isAnimated()) {
                // Animated frame counter remapping; overrides offset / multiplier
                stream(SameLine) << "0";
                generatePropertyTimeline(info.timelineInfo->frameCounterMapper, info.id, "frameCounter"_L1);
            } else {
                const auto offset = info.timelineInfo->frameCounterOffset;
                const auto multiplier = info.timelineInfo->frameCounterMultiplier;
                const bool needsParens = (offset && multiplier);
                if (needsParens)
                    stream(SameLine) << "(";
                stream(SameLine) << frameCounterRef;
                if (offset)
                    stream(SameLine) << (offset > 0 ? " + " : " - ") << qAbs(offset);
                if (needsParens)
                    stream(SameLine) << ")";
                if (multiplier)
                    stream(SameLine) << " * " << multiplier;
            }
        }
    }
}

bool QQuickQmlGenerator::generatePatternNode(const PatternNodeInfo &info)
{
    if (info.stage == StructureNodeStage::Start) {
        return true;
    } else {
        startDefsSuffixBlock();
        stream() << "Loader {";
        m_indentLevel++;

        stream() << "id: " << info.id; // This is in a different scope, so we can reuse the ID
        stream() << "sourceComponent: " << info.id << "_container";
        stream() << "width: item !== null ? item.originalBounds.width : 0";
        stream() << "height: item !== null ? item.originalBounds.height : 0";
        stream() << "visible: false";
        stream() << "function sourceRect(targetWidth, targetHeight) {";
        m_indentLevel++;

        stream() << "return Qt.rect(0, 0, ";
        if (!info.isPatternRectRelativeCoordinates) {
            stream(SameLine) << info.patternRect.width() << ", "
                             << info.patternRect.height();
        } else {
            stream(SameLine) << info.patternRect.width() << " * targetWidth, "
                             << info.patternRect.height() << " * targetHeight";
        }
        stream(SameLine) << ")";
        m_indentLevel--;
        stream() << "}";

        stream() << "function sourceOffset(targetWidth, targetHeight) {";
        m_indentLevel++;

        stream() << "return Qt.vector3d(";
        if (!info.isPatternRectRelativeCoordinates) {
            stream(SameLine) << info.patternRect.x() << ", "
                             << info.patternRect.y() << ", ";
        } else {
            stream(SameLine) << info.patternRect.x() << " * targetWidth, "
                             << info.patternRect.y() << " * targetHeight, ";
        }
        stream(SameLine) << "0.0)";
        m_indentLevel--;
        stream() << "}";


        m_indentLevel--;
        stream() << "}";

        endDefsSuffixBlock();

        return true;
    }
}

void QQuickQmlGenerator::generateDefsInstantiationNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return;

    if (info.stage == StructureNodeStage::Start) {
        stream() << "Loader {";
        m_indentLevel++;

        stream() << "sourceComponent: " << info.defsId << "_container";
        generateNodeBase(info);
        generateTimelineFields(info);
    } else {
        m_indentLevel--;
        stream() << "}";
    }
}

bool QQuickQmlGenerator::generateMarkerNode(const MarkerNodeInfo &info)
{
    if (info.stage == StructureNodeStage::Start) {
        startDefsSuffixBlock();
        stream() << "QtObject {";
        m_indentLevel++;

        stream() << "id: " << info.id << "_markerParameters";

        stream() << "property bool startReversed: ";
        if (info.orientation == MarkerNodeInfo::Orientation::AutoStartReverse)
            stream(SameLine) << "true";
        else
            stream(SameLine) << "false";

        stream() << "function autoAngle(adaptedAngle) {";
        m_indentLevel++;
        if (info.orientation == MarkerNodeInfo::Orientation::Value)
            stream() << "return " << info.angle;
        else
            stream() << "return adaptedAngle";
        m_indentLevel--;
        stream() << "}";

        m_indentLevel--;
        stream() << "}";
        endDefsSuffixBlock();

        if (!info.clipBox.isEmpty()) {
            stream() << "Item {";
            m_indentLevel++;

            stream() << "x: " << info.clipBox.x();
            if (info.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth)
                stream(SameLine) << " * strokeWidth";
            stream() << "y: " << info.clipBox.y();
            if (info.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth)
                stream(SameLine) << " * strokeWidth";
            stream() << "width: " << info.clipBox.width();
            if (info.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth)
                stream(SameLine) << " * strokeWidth";
            stream() << "height: " << info.clipBox.height();
            if (info.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth)
                stream(SameLine) << " * strokeWidth";
            stream() << "clip: true";
        }

        stream() << "Item {";
        m_indentLevel++;

        if (!info.clipBox.isEmpty()) {
            stream() << "x: " << -info.clipBox.x();
            if (info.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth)
                stream(SameLine) << " * strokeWidth";
            stream() << "y: " << -info.clipBox.y();
            if (info.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth)
                stream(SameLine) << " * strokeWidth";
        }

        stream() << "id: " << info.id;

        stream() << "property real markerWidth: " << info.markerSize.width();
        if (info.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth)
            stream(SameLine) << " * strokeWidth";

        stream() << "property real markerHeight: " << info.markerSize.height();
        if (info.markerUnits == MarkerNodeInfo::MarkerUnits::StrokeWidth)
            stream(SameLine) << " * strokeWidth";

        stream() << "function calculateMarkerScale(w, h) {";
        m_indentLevel++;

        stream() << "var scaleX = 1.0";
        stream() << "var scaleY = 1.0";
        stream() << "var offsetX = 0.0";
        stream() << "var offsetY = 0.0";
        if (info.viewBox.width() > 0)
            stream() << "if (w > 0) scaleX = w / " << info.viewBox.width();
        if (info.viewBox.height() > 0)
            stream() << "if (h > 0) scaleY = h / " << info.viewBox.height();

        if (info.preserveAspectRatio & MarkerNodeInfo::xyMask) {
            stream() << "if (scaleX != scaleY) {";
            m_indentLevel++;

            if (info.preserveAspectRatio & MarkerNodeInfo::meet)
                stream() << "scaleX = scaleY = Math.min(scaleX, scaleY)";
            else
                stream() << "scaleX = scaleY = Math.max(scaleX, scaleY)";

            QString overflowX = QStringLiteral("scaleX * %1 - w").arg(info.viewBox.width());
            QString overflowY = QStringLiteral("scaleY * %1 - h").arg(info.viewBox.height());

            const quint8 xRatio = info.preserveAspectRatio & MarkerNodeInfo::xMask;
            if (xRatio == MarkerNodeInfo::xMid)
                stream() << "offsetX -= " << overflowX << " / 2";
            else if (xRatio == MarkerNodeInfo::xMax)
                stream() << "offsetX -= " << overflowX;

            const quint8 yRatio = info.preserveAspectRatio & MarkerNodeInfo::yMask;
            if (yRatio == MarkerNodeInfo::yMid)
                stream() << "offsetY -= " << overflowY << " / 2";
            else if (yRatio == MarkerNodeInfo::yMax)
                stream() << "offsetY -= " << overflowY;

            m_indentLevel--;
            stream() << "}";
        }

        stream() << "return Qt.vector4d("
                 << "offsetX - " << info.anchorPoint.x() << " * scaleX, "
                 << "offsetY - " << info.anchorPoint.y() << " * scaleY, "
                 << "scaleX, "
                 << "scaleY)";

        m_indentLevel--;
        stream() << "}";

        stream() << "property vector4d markerScale: calculateMarkerScale(markerWidth, markerHeight)";

        stream() << "transform: [";
        m_indentLevel++;

        stream() << "Scale { xScale: " << info.id << ".markerScale.z; yScale: " << info.id << ".markerScale.w },";
        stream() << "Translate { x: " << info.id << ".markerScale.x; y: " << info.id << ".markerScale.y }";

        m_indentLevel--;
        stream() << "]";

    } else {
        generateNodeEnd(info);

        if (!info.clipBox.isEmpty()) {
            m_indentLevel--;
            stream() << "}";
        }
    }

    return true;
}

bool QQuickQmlGenerator::generateRootNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

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
        if (usingTimelineAnimation())
            stream() << "import QtQuick.Timeline";

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

        stream() << "Item {";
        m_indentLevel++;
        stream() << "width: 1";
        stream() << "height: 1";

        stream() << "ItemSpy { id: __qt_toplevel_scale_itemspy; anchors.fill: parent }";

        m_indentLevel--;
        stream() << "}";

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

        if (usingTimelineAnimation() && info.timelineInfo) {
            stream() << "property real startFrame: " << info.timelineInfo->startFrame;
            stream() << "property real endFrame: " << info.timelineInfo->endFrame;
            stream() << "property real frameRate: " << info.timelineInfo->frameRate;
            stream() << "property real frameCounter: " << info.timelineInfo->startFrame;
            if (!info.timelineInfo->markers.isEmpty()) {
                QStringList markerEntries;
                markerEntries.reserve(info.timelineInfo->markers.size());
                for (const LottieMarkerInfo &marker : info.timelineInfo->markers) {
                    markerEntries << u'"' + sanitizeString(marker.name) + u"\": ["
                                    + QString::number(marker.frame) + u", "
                                    + QString::number(marker.duration) + u']';
                }
                stream() << "property var markers: ({" << markerEntries.join(", "_L1) << "})";
            }
            stream() << "NumberAnimation on frameCounter {";
            m_indentLevel++;
            stream() << "objectName: \"_qt_frameCounterAnimation\"";
            stream() << "from: " << m_topLevelIdString << ".startFrame";
            stream() << "to: " << m_topLevelIdString << ".endFrame";
            stream() << "duration: " << processAnimationTime(1000) << " * Math.abs(to - from) / "
                     << "Math.max(" << m_topLevelIdString << ".frameRate, 1)";
            generateAnimationBindings();
            m_indentLevel--;
            stream() << "}";
            stream() << "visible: frameCounter >= " << info.timelineInfo->startFrame
                     << " && frameCounter < " << info.timelineInfo->endFrame;
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

void QQuickQmlGenerator::startDefsSuffixBlock()
{
    int tmp = m_oldIndentLevels.top();
    m_oldIndentLevels.push(m_indentLevel);
    m_indentLevel = tmp;
    m_stream.setString(&m_defsSuffix);
}

void QQuickQmlGenerator::endDefsSuffixBlock()
{
    m_indentLevel = m_oldIndentLevels.pop();
    m_stream.setDevice(&m_result);
}

QStringView QQuickQmlGenerator::indent()
{
    int indentWidth = m_indentLevel * 4;
    if (indentWidth > m_indentString.size())
        m_indentString.fill(QLatin1Char(' '), indentWidth * 2);
    return QStringView(m_indentString).first(indentWidth);
}

QTextStream &QQuickQmlGenerator::stream(int flags)
{
    if (m_stream.device() == nullptr && m_stream.string() == nullptr)
        m_stream.setDevice(&m_result);
    else if (!(flags & StreamFlags::SameLine))
        m_stream << Qt::endl << indent();

    static qint64 maxBufferSize = qEnvironmentVariableIntegerValue("QT_QUICKVECTORIMAGE_MAX_BUFFER").value_or(64 << 20); // 64MB
    if (m_stream.device()) {
        if (Q_UNLIKELY(!checkSanityLimit(m_stream.device()->size(), maxBufferSize, "buffer size"_L1)))
            m_stream.device()->reset();
    } else {
        if (Q_UNLIKELY(!checkSanityLimit(m_stream.string()->size(), maxBufferSize, "buffer string size"_L1)))
            m_stream.string()->clear();
    }

    return m_stream;
}

const char *QQuickQmlGenerator::shapeName() const
{
    return m_shapeTypeName.isEmpty() ? "Shape" : m_shapeTypeName.constData();
}

QT_END_NAMESPACE
