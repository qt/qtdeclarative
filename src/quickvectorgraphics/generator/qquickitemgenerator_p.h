// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
#include <QStack>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORGRAPHICSGENERATOR_EXPORT QQuickItemGenerator : public QQuickGenerator
{
public:
    QQuickItemGenerator(const QString fileName, QQuickVectorGraphics::GeneratorFlags flags, QQuickItem *parentItem);
    ~QQuickItemGenerator();

protected:
    void generateNodeBase(NodeInfo &info) override;
    bool generateDefsNode(NodeInfo &info) override;
    void generateImageNode(ImageNodeInfo &info) override;
    void generatePath(PathNodeInfo &info) override;
    void generateNode(NodeInfo &info) override;
    void generateTextNode(TextNodeInfo &info) override;
    void generateStructureNode(StructureNodeInfo &info) override;
    void generateRootNode(StructureNodeInfo &info) override;
    void outputShapePath(const PathNodeInfo &info, const QPainterPath *path, const QQuadPath *quadPath, QQuickVectorGraphics::PathSelector pathSelector, const QRectF &boundingRect) override;
    void generateGradient(const QGradient *grad, QQuickShapePath *shapePath, const QRectF &boundingRect) override;

private:
    QQuickItem *currentItem();
    void addCurrentItem(QQuickItem *item, const QString &name);

    bool m_inShapeItem = false;
    QQuickShape *m_parentShapeItem = nullptr;

    QStack<QQuickItem *> m_items;

    QQuickItem *m_loadedItem = nullptr;
    QQuickItem *m_parentItem = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKITEMGENERATOR_P_H
