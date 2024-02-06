// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquickgenerator_p.h"
#include "qsvgvisitorimpl_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qsgcurveprocessor_p.h>
#include <private/qquickshape_p.h>
#include <private/qquadpath_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickimagebase_p_p.h>

#include<QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQuickVectorGraphics, "qt.quick.vectorgraphics", QtWarningMsg)

QQuickGenerator::QQuickGenerator(const QString fileName, QQuickVectorGraphics::GeneratorFlags flags)
    : m_flags(flags)
    , m_fileName(fileName)
    , m_loader(nullptr)
{
}

QQuickGenerator::~QQuickGenerator()
{
    delete m_loader;
}

void QQuickGenerator::setGeneratorFlags(QQuickVectorGraphics::GeneratorFlags flags)
{
    m_flags = flags;
}

QQuickVectorGraphics::GeneratorFlags QQuickGenerator::generatorFlags()
{
    return m_flags;
}

void QQuickGenerator::generate()
{
    m_loader = new QSvgVisitorImpl(m_fileName, this);
    m_loader->traverse();
}

void QQuickGenerator::optimizePaths(const PathNodeInfo &info)
{
    QPainterPath pathCopy = info.painterPath;
    pathCopy.setFillRule(info.fillRule);

    if (m_flags.testFlag(QQuickVectorGraphics::GeneratorFlag::OptimizePaths)) {
        QQuadPath strokePath = QQuadPath::fromPainterPath(pathCopy);
        bool fillPathNeededClose;
        QQuadPath fillPath = strokePath.subPathsClosed(&fillPathNeededClose);
        const bool intersectionsFound = QSGCurveProcessor::solveIntersections(fillPath, false);
        fillPath.addCurvatureData();
        QSGCurveProcessor::solveOverlaps(fillPath);
        const bool compatibleStrokeAndFill = !fillPathNeededClose && !intersectionsFound;
        if (compatibleStrokeAndFill || m_flags.testFlag(QQuickVectorGraphics::GeneratorFlag::OutlineStrokeMode)) {
            outputShapePath(info, nullptr, &fillPath, QQuickVectorGraphics::FillAndStroke, pathCopy.boundingRect());
        } else {
            outputShapePath(info, nullptr, &fillPath, QQuickVectorGraphics::FillPath, pathCopy.boundingRect());
            outputShapePath(info, nullptr, &strokePath, QQuickVectorGraphics::StrokePath, pathCopy.boundingRect());
        }
    } else {
        outputShapePath(info, &pathCopy, nullptr, QQuickVectorGraphics::FillAndStroke, pathCopy.boundingRect());
    }
}

QT_END_NAMESPACE
