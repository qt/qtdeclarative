// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICKGENERATOR_P_H
#define QQUICKGENERATOR_P_H

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

#include <private/qquickvectorgraphicsglobal_p.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QSvgVisitorImpl;
class QPainterPath;
class QGradient;
class QQuickShapePath;
class QQuadPath;
class QQuickItem;
class QQuickShape;
class QRectF;

struct NodeInfo;
struct ImageNodeInfo;
struct PathNodeInfo;
struct TextNodeInfo;
struct StructureNodeInfo;

class Q_QUICKVECTORGRAPHICSGENERATOR_EXPORT QQuickGenerator
{
public:
    QQuickGenerator(const QString fileName, QQuickVectorGraphics::GeneratorFlags flags);
    virtual ~QQuickGenerator();

    void setGeneratorFlags(QQuickVectorGraphics::GeneratorFlags flags);
    QQuickVectorGraphics::GeneratorFlags generatorFlags();

    void generate();

protected:
    virtual void generateNodeBase(NodeInfo &info) = 0;
    virtual bool generateDefsNode(NodeInfo &info) = 0;
    virtual void generateImageNode(ImageNodeInfo &info) = 0;
    virtual void generatePath(PathNodeInfo &info) = 0;
    virtual void generateNode(NodeInfo &info) = 0;
    virtual void generateTextNode(TextNodeInfo &info) = 0;
    virtual void generateStructureNode(StructureNodeInfo &info) = 0;
    virtual void generateRootNode(StructureNodeInfo &info) = 0;
    virtual void generateGradient(const QGradient *grad, QQuickShapePath *shapePath, const QRectF &boundingRect) = 0;
    virtual void outputShapePath(const PathNodeInfo &info, const QPainterPath *path, const QQuadPath *quadPath, QQuickVectorGraphics::PathSelector pathSelector, const QRectF &boundingRect) = 0;
    void optimizePaths(const PathNodeInfo &info);

protected:
    QQuickVectorGraphics::GeneratorFlags m_flags;

private:
    QString m_fileName;
    QSvgVisitorImpl *m_loader;
    friend class QSvgVisitorImpl;
};

QT_END_NAMESPACE

#endif // QQUICKGENERATOR_P_H
