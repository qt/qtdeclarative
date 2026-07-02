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

#include <QtCore/qhash.h>
#include <QtCore/qstack.h>

#include <functional>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickShape;
class QQmlContext;
class QQuickShaderEffect;
class QQuickShaderEffectSource;
class QQuickMatrix4x4;

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

    QQuickItem *m_rootItem = nullptr;
    QStack<QQuickItem *> m_itemStack;
    QQmlContext *m_context = nullptr;
    QSizeF m_containerSize;
    quint32 m_nodeCounter = 0;
    QHash<QString, QList<std::function<void()>>> m_defs;
    QList<std::function<void()>> *m_currentDefsRecord = nullptr;
    QHash<QString, MaskDef> m_maskDefs;

    QQuickShape *createShapeContainer();
    void pushItem(QQuickItem *item);
    QQuickItem *popItem();
    QQuickItem *currentItem() const;
    void generateMaskContainer(const MaskNodeInfo &info);
    void generateMask(QQuickItem *item, const NodeInfo &info);
};

QT_END_NAMESPACE

#endif // QQUICKITEMGENERATOR_P_H
