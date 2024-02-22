// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DEBUGVISUALIZATIONCONTROLLER_H
#define DEBUGVISUALIZATIONCONTROLLER_H

#include <QObject>

class DebugVisualizationController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool showCurves READ showCurves WRITE setShowCurves NOTIFY showCurvesChanged)
    Q_PROPERTY(bool showWireframe READ showWireframe WRITE setWireframe NOTIFY showWireframeChanged)

public:
    explicit DebugVisualizationController(QObject *parent = nullptr);

    bool showCurves() const;
    void setShowCurves(bool newShowCurves);

    bool showWireframe() const;
    void setWireframe(bool newShowWireframe);

signals:
    void showCurvesChanged();
    void showWireframeChanged();
    void settingsChanged();

private:
    void update();
    bool m_showCurves = false;
    bool m_showWireframe = false;
};

#endif // DEBUGVISUALIZATIONCONTROLLER_H
