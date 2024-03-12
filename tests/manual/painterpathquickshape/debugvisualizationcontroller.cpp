// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "debugvisualizationcontroller.h"
#include <private/qquickshapecurverenderer_p.h>

DebugVisualizationController::DebugVisualizationController(QObject *parent)
    : QObject{parent}
{
}

bool DebugVisualizationController::showCurves() const
{
    return m_showCurves;
}

void DebugVisualizationController::setShowCurves(bool newShowCurves)
{
    if (m_showCurves == newShowCurves)
        return;
    m_showCurves = newShowCurves;
    update();
    emit showCurvesChanged();
}

bool DebugVisualizationController::showWireframe() const
{
    return m_showWireframe;
}

void DebugVisualizationController::setWireframe(bool newShowWireframe)
{
    if (m_showWireframe == newShowWireframe)
        return;
    m_showWireframe = newShowWireframe;
    update();
    emit showWireframeChanged();
}

void DebugVisualizationController::update()
{
    int flags = (m_showCurves ? QQuickShapeCurveRenderer::DebugCurves : 0)
                | (m_showWireframe ? QQuickShapeCurveRenderer::DebugWireframe : 0);
    QQuickShapeCurveRenderer::setDebugVisualization(flags);
    emit settingsChanged();
}
