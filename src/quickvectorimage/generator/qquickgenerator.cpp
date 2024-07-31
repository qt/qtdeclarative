// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickgenerator_p.h"
#include "qsvgvisitorimpl_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qsgcurveprocessor_p.h>
#include <private/qquickshape_p.h>
#include <private/qquadpath_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickimagebase_p_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQuickVectorImage, "qt.quick.vectorimage", QtWarningMsg)

QQuickGenerator::QQuickGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags)
    : m_flags(flags)
    , m_fileName(fileName)
    , m_loader(nullptr)
{
}

QQuickGenerator::~QQuickGenerator()
{
    delete m_loader;
}

void QQuickGenerator::setGeneratorFlags(QQuickVectorImageGenerator::GeneratorFlags flags)
{
    m_flags = flags;
}

QQuickVectorImageGenerator::GeneratorFlags QQuickGenerator::generatorFlags()
{
    return m_flags;
}

bool QQuickGenerator::generate()
{
    m_loader = new QSvgVisitorImpl(m_fileName, this);
    m_generationSucceeded = m_loader->traverse();
    return m_generationSucceeded;
}

void QQuickGenerator::optimizePaths(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    QPainterPath pathCopy = info.painterPath;
    pathCopy.setFillRule(info.fillRule);

    const QRectF &boundingRect = overrideBoundingRect.isNull() ? pathCopy.boundingRect() : overrideBoundingRect;
    if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::OptimizePaths)) {
        QQuadPath strokePath = QQuadPath::fromPainterPath(pathCopy);
        bool fillPathNeededClose;
        QQuadPath fillPath = strokePath.subPathsClosed(&fillPathNeededClose);
        const bool intersectionsFound = QSGCurveProcessor::solveIntersections(fillPath, false);
        fillPath.addCurvatureData();
        QSGCurveProcessor::solveOverlaps(fillPath);
        const bool compatibleStrokeAndFill = !fillPathNeededClose && !intersectionsFound;

        if (compatibleStrokeAndFill || m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::OutlineStrokeMode)) {
            outputShapePath(info, nullptr, &fillPath, QQuickVectorImageGenerator::FillAndStroke, boundingRect);
        } else {
            outputShapePath(info, nullptr, &fillPath, QQuickVectorImageGenerator::FillPath, boundingRect);
            outputShapePath(info, nullptr, &strokePath, QQuickVectorImageGenerator::StrokePath, boundingRect);
        }
    } else {
        outputShapePath(info, &pathCopy, nullptr, QQuickVectorImageGenerator::FillAndStroke, boundingRect);
    }
}

bool QQuickGenerator::isNodeVisible(const NodeInfo &info)
{
    if (!info.isVisible || !info.isDisplayed)
        return false;

    return true;
}

QT_END_NAMESPACE
