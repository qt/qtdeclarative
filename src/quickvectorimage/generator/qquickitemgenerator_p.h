// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKITEMGENERATOR_P_H
#define QQUICKITEMGENERATOR_P_H

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

#include "qquickgenerator_p.h"
#include "qquicknodeinfo_p.h"
#include "qquickanimatedproperty_p.h"
#include "qquickgeneratoranimationprovider_p.h"

#include <QtCore/qeasingcurve.h>
#include <QtCore/qhash.h>
#include <QtCore/qmap.h>
#include <QtCore/qstack.h>

#include <array>
#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickShape;
class QQuickShapePath;
class QQmlContext;
class QQuickShaderEffect;
class QQuickShaderEffectSource;
class QQuickMatrix4x4;
class QQuickItemSpy;
class QQuickTransform;
class QQuickTransformSource;
class QQuickAbstractAnimation;

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickItemGenerator : public QQuickGenerator
{
public:
    QQuickItemGenerator(const QString &fileName, QQuickVectorImageGenerator::GeneratorFlags flags,
                        QQmlContext *context = nullptr);
    ~QQuickItemGenerator() override;

    QQuickItem *takeRootItem();

    QString generateNodeBase(const NodeInfo &info, const QString &idSuffix = QString{}) override;
    bool generateRootNode(const StructureNodeInfo &info) override;
    bool generateStructureNode(const StructureNodeInfo &info) override;
    void generateUseNode(const UseNodeInfo &info) override;
    bool generateDefsNode(const StructureNodeInfo &info) override;
    void generateDefsInstantiationNode(const StructureNodeInfo &info) override;
    void generateImageNode(const ImageNodeInfo &info) override;
    void generatePath(const PathNodeInfo &info,
                      const QRectF &overrideBoundingRect = QRectF{}) override;
    void generateNode(const NodeInfo &info) override;
    void generateTextNode(const TextNodeInfo &info) override;
    void generateFilterNode(const FilterNodeInfo &info) override;
    bool generateMaskNode(const MaskNodeInfo &info) override;
    bool generateMarkerNode(const MarkerNodeInfo &info) override;
    bool generatePatternNode(const PatternNodeInfo &info) override;
    void outputShapePath(const PathNodeInfo &info, const QPainterPath *path,
                         const QQuadPath *quadPath,
                         QQuickVectorImageGenerator::PathSelector pathSelector,
                         const QRectF &boundingRect) override;

    void setAnimationProvider(std::unique_ptr<QQuickGeneratorAnimationProvider> provider) override;

private:
    struct MaskDef
    {
        QQuickItem *container = nullptr;
        QRectF maskRect;
        bool isMaskRectRelativeCoordinates = false;
        bool isMaskContentRelativeCoordinates = false;
        QQuickItem *transformer = nullptr;
        QQuickMatrix4x4 *transformerMatrix = nullptr;
    };

    struct PatternDef
    {
        QQuickItem *container = nullptr;
        QRectF patternRect;
        bool isPatternRectRelativeCoordinates = false;
    };

    struct MarkerDef
    {
        QList<std::function<void()>> recording;
        MarkerNodeInfo info;
    };

    struct PendingLinkedTransform
    {
        QQuickItem *item;
        QString transformReferenceId;
        QQuickMatrix4x4 *linkedMatrix;
    };

    QQuickItem *m_rootItem = nullptr;
    QStack<QQuickItem *> m_itemStack;
    QQmlContext *m_context = nullptr;
    QSizeF m_containerSize;
    quint32 m_nodeCounter = 0;
    QHash<QString, QList<std::function<void()>>> m_defs;
    QList<std::function<void()>> *m_currentDefsRecord = nullptr;
    QList<std::function<void()>> *m_currentMarkerRecord = nullptr;
    QHash<QString, MaskDef> m_maskDefs;
    QHash<QString, PatternDef> m_patternDefs;
    QHash<QString, MarkerDef> m_markerDefs;
    QHash<QString, FilterNodeInfo> m_filterDefs;
    QQuickItemSpy *m_topLevelScaleSpy = nullptr;

    QHash<QString, QQuickTransformSource *> m_transformSourceItems;
    QList<PendingLinkedTransform> m_pendingLinkedTransforms;
    std::unique_ptr<QQuickGeneratorAnimationProvider> m_animationProvider;
    QStack<bool> m_scopePushed;
    QMap<std::array<qreal, 4>, QEasingCurve> m_easingCache;

    QList<std::function<void()>> *activeRecord() const;
    QQuickShape *createShapeContainer();
    void pushItem(QQuickItem *item);
    QQuickItem *popItem();
    QQuickItem *currentItem() const;

    void bindTextureSize(QQuickShaderEffectSource *ses);
    void bindPatternTextureSize(QQuickShaderEffectSource *ses);
    void bindPropertyAnimation(QObject *target, const QString &property,
                               const QQuickAnimatedProperty::PropertyAnimation &anim,
                               const std::function<QVariant(const QVariant &)> &extractor,
                               int valueIndex = 0, const QVariant &resetValue = QVariant());
    void bindAnimatedProperty(QObject *target, const QString &property,
                              const QQuickAnimatedProperty &animatedProperty,
                              const std::function<QVariant(const QVariant &)> &extractor,
                              int valueIndex = 0);
    void bindColorWithOpacity(QObject *target, const QString &colorProperty,
                              const QQuickAnimatedProperty &color,
                              const QQuickAnimatedProperty &opacity,
                              std::function<void(const QColor &)> setter);
    void generateItemAnimations(QQuickItem *item, const NodeInfo &info);
    QQuickTransform *createAnimatedTransformGroup(QQuickItem *item, const NodeInfo &info);
    void bindMotionPath(QQuickItem *item, const QQuickAnimatedProperty &motionPath);

    void generateMaskContainer(const MaskNodeInfo &info);
    void generateMask(QQuickItem *item, const NodeInfo &info, const QPointF &sourceOrigin);
    void generatePatternContainer(const PatternNodeInfo &info);
    void generatePattern(QQuickShapePath *shapePath, const PathNodeInfo &info,
                         const QRectF &boundingRect, QTransform &fillTransform);
    void generateMarkers(const PathNodeInfo &info);
    QQuickItem *generateFilter(QQuickItem *item, const NodeInfo &info, QPointF *outputOrigin);
    QQuickShaderEffectSource *generateFilterStep(const FilterNodeInfo::FilterStep &step,
                                                 QQuickShaderEffectSource *input1,
                                                 QQuickShaderEffectSource *input2,
                                                 const QRectF &stepRect, const QRectF &filterRect);
    QQuickShaderEffectSource *generateFilterMerge(const QList<QQuickShaderEffectSource *> &inputs,
                                                  const QRectF &stepRect, const QRectF &filterRect);
    QQuickShaderEffectSource *generateFilterFlood(const FilterNodeInfo::FilterStep &step,
                                                  QQuickShaderEffectSource *input,
                                                  const QRectF &stepRect, const QRectF &filterRect);
    QQuickShaderEffectSource *generateFilterOffset(const FilterNodeInfo::FilterStep &step,
                                                   QQuickShaderEffectSource *input,
                                                   const QRectF &stepRect);
    QQuickShaderEffectSource *generateFilterColorMatrix(const FilterNodeInfo::FilterStep &step,
                                                        QQuickShaderEffectSource *input,
                                                        const QRectF &stepRect,
                                                        const QRectF &filterRect);
    QQuickShaderEffectSource *generateFilterBlend(const FilterNodeInfo::FilterStep &step,
                                                  QQuickShaderEffectSource *input1,
                                                  QQuickShaderEffectSource *input2,
                                                  const QRectF &stepRect, const QRectF &filterRect);
    QQuickShaderEffectSource *generateFilterComposite(const FilterNodeInfo::FilterStep &step,
                                                      QQuickShaderEffectSource *input1,
                                                      QQuickShaderEffectSource *input2,
                                                      const QRectF &stepRect,
                                                      const QRectF &filterRect);
    QQuickShaderEffectSource *generateFilterGaussianBlur(const FilterNodeInfo::FilterStep &step,
                                                         QQuickShaderEffectSource *input,
                                                         const QRectF &stepRect,
                                                         const QRectF &filterRect);
};

QT_END_NAMESPACE

#endif // QQUICKITEMGENERATOR_P_H
