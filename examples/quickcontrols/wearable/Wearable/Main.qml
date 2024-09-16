// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Shapes

import WearableSettings
import WearableStyle

//! [window start]
QQC2.ApplicationWindow {
    id: window
//! [window start]
    visible: true
    width: 380
    height: 380
    title: qsTr("Wearable")

    background: Shape {//Shape because Rectangle does not support diagonal gradient
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeWidth: 0
            startX: 0; startY: 0

            PathLine {
                x: window.width
                y: 0
            }
            PathLine {
                x: window.width
                y: window.height
            }
            PathLine {
                x: 0
                y: window.height
            }
            fillGradient: LinearGradient {
                x1: window.width / 3
                y1: 0
                x2: window.width
                y2: 1.3 * window.height
                GradientStop {
                    position: 0.0
                    color: UIStyle.background1
                }
                GradientStop {
                    position: 0.5
                    color: UIStyle.background2
                }
                GradientStop {
                    position: 1.0
                    color: UIStyle.background3
                }
            }
        }
    }

//! [stackview start]
    QQC2.StackView {
        id: stackView
//! [stackview start]

        anchors.fill: parent

        focus: true

//! [onLaunched connection]
        initialItem: LauncherPage {
            onLaunched: (title, page, fallback) => {
                            var createdPage = Qt.createComponent(page)
                            if (createdPage.status !== Component.Ready)
                                createdPage = Qt.createComponent(fallback)
                            stackView.push(createdPage)
                            header.title = title
                        }
        }
//! [onLaunched connection]
//! [stackview end]
    }

//! [stackview end]
    MenuHeader {
        id: header

        anchors.top: parent.top
        width: parent.width

        title: ""

        enabled: stackView.depth > 1

        onBackClicked: stackView.pop()
    }

//! [DemoMode]
    DemoMode {
        stackView: stackView
    }

//! [DemoMode]
//! [DemoModeIndicator]
    DemoModeIndicator {
        id: demoModeIndicator
        y: WearableSettings.demoMode ? header.height + 3 : -height - 5
        anchors.horizontalCenter: parent.horizontalCenter
        z: header.z + 1
    }

//! [DemoModeIndicator]
//! [MouseArea]
    MouseArea {
        enabled: WearableSettings.demoMode
        anchors.fill: parent
        onClicked: {
            // Stop demo mode and return to the launcher page.
            WearableSettings.demoMode = false
            stackView.pop(null)
        }
    }
//! [MouseArea]
//! [window end]
}
//! [window end]
