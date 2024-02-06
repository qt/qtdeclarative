// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Shapes
import Wearable
import WearableStyle

import "fitness.js" as FitnessData

Item {

    component AngularProgressBar : Shape {
        id: apb
        preferredRendererType: Shape.CurveRenderer

        property real value: 0.5
        property int rad: 75

        ShapePath {
            id: bgR
            strokeColor: UIStyle.buttonBackground
            strokeWidth: 8
            capStyle: ShapePath.RoundCap
            fillColor: "#00000000"
            property real sA: (90 + 45) / 180 * Math.PI
            property real eA: (360 + 45) / 180 * Math.PI
            property real mA: (sA + eA) / 2
            startX: apb.rad * Math.cos(bgR.sA)
            startY: apb.rad * Math.sin(bgR.sA)
            PathArc {
                x: apb.rad * Math.cos(bgR.mA)
                y: apb.rad * Math.sin(bgR.mA)
                radiusX: apb.rad
                radiusY: apb.rad
            }
            PathArc {
                x: apb.rad * Math.cos(bgR.eA)
                y: apb.rad * Math.sin(bgR.eA)
                radiusX: apb.rad
                radiusY: apb.rad
            }
        }

        ShapePath {
            id: fgR
            strokeColor: UIStyle.buttonProgress
            strokeWidth: 8
            capStyle: ShapePath.RoundCap
            fillColor: "#00000000"
            property real sA: bgR.sA
            property real eA: sA + (bgR.eA - sA) * apb.value
            property real mA: (sA + eA) / 2
            startX: apb.rad * Math.cos(fgR.sA)
            startY: apb.rad * Math.sin(fgR.sA)
            PathArc {
                x: apb.rad * Math.cos(fgR.mA)
                y: apb.rad * Math.sin(fgR.mA)
                radiusX: apb.rad
                radiusY: apb.rad
            }
            PathArc {
                x: apb.rad * Math.cos(fgR.eA)
                y: apb.rad * Math.sin(fgR.eA)
                radiusX: apb.rad
                radiusY: apb.rad
            }
        }
    }

    QQC2.SwipeView {
        id: svFitnessContainer

        anchors.fill: parent

        SwipeViewPage {
            id: fitnessPage1

            Image {
                id: walkingDude
                anchors.centerIn: parent
                source: UIStyle.themeImagePath("fitness-man-walking")
            }

            AngularProgressBar {
                anchors.horizontalCenter: walkingDude.horizontalCenter
                anchors.bottom: walkingDude.bottom
                anchors.horizontalCenterOffset: 40
                value: FitnessData.getWalkingSteps() / FitnessData.getWalkingGoal()
            }

            Column {
                anchors.top: walkingDude.bottom
                anchors.topMargin: 15
                anchors.horizontalCenter: parent.horizontalCenter

                spacing: -7
                Item {
                    width: childrenRect.width
                    height: childrenRect.height

                    Text {
                        id: steps
                        text: FitnessData.getWalkingSteps()
                        font: UIStyle.h2
                        color: UIStyle.textColor
                    }

                    Text {
                        anchors.bottom: steps.bottom
                        anchors.left: steps.right
                        anchors.leftMargin: 5
                        text: qsTr("steps")
                        font: UIStyle.p2
                        color: UIStyle.textColor
                    }
                }

                Item {
                    width: childrenRect.width
                    height: childrenRect.height

                    Text {
                        id: time
                        text: FitnessData.getWalkingTime()
                        font: UIStyle.h2
                        color: UIStyle.textColor
                    }

                    Text {
                        anchors.bottom: time.bottom
                        anchors.left: time.right
                        anchors.leftMargin: 5
                        text: qsTr("min")
                        font: UIStyle.p2
                        color: UIStyle.textColor
                    }
                }

                Item {
                    width: childrenRect.width
                    height: childrenRect.height

                    Text {
                        id: cal
                        text: FitnessData.getWalkingCalories()
                        font: UIStyle.h2
                        color: UIStyle.textColor
                    }

                    Text {
                        anchors.bottom: cal.bottom
                        anchors.left: cal.right
                        anchors.leftMargin: 5
                        text: qsTr("cal")
                        font: UIStyle.p2
                        color: UIStyle.textColor
                    }
                }
            }
        }

        SwipeViewPage {
            id: fitnessPage2

            Image {
                id: runningDude
                anchors.centerIn: parent
                source: UIStyle.themeImagePath("fitness-man-running")
            }

            AngularProgressBar {
                anchors.horizontalCenter: runningDude.horizontalCenter
                anchors.bottom: runningDude.bottom
                anchors.horizontalCenterOffset: 40
                value: FitnessData.getRunningDistance() / FitnessData.getRunningGoal()
            }

            Row {
                anchors.top: runningDude.bottom
                anchors.horizontalCenter: runningDude.horizontalCenter
                anchors.topMargin: 15

                spacing: 10

                Item {
                    width: childrenRect.width
                    height: childrenRect.height

                    Text {
                        id: goal
                        text: qsTr("Goal")
                        font: UIStyle.p2
                        color: UIStyle.textColor
                    }
                    Text {
                        anchors.bottom: goal.bottom
                        anchors.left: goal.right
                        anchors.leftMargin: 5
                        text: FitnessData.getRunningGoal() + " km"
                        font: UIStyle.h2
                        color: UIStyle.textColor
                    }
                }

                Rectangle {
                    width: 1
                    height: 75
                    color: UIStyle.textColor
                }

                Column {
                    spacing: -7
                    Item {
                        width: childrenRect.width
                        height: childrenRect.height
                        Text {
                            id: distance
                            text: qsTr("Distance")
                            font: UIStyle.p2
                            color: UIStyle.textColor
                        }
                        Text {
                            anchors.bottom: distance.bottom
                            anchors.left: distance.right
                            anchors.leftMargin: 5
                            text: FitnessData.getRunningDistance() + " km"
                            font: UIStyle.h2
                            color: UIStyle.textColor
                        }
                    }

                    Item {
                        width: childrenRect.width
                        height: childrenRect.height
                        Text {
                            id: runningtime
                            text: FitnessData.getRunningTime()
                            font: UIStyle.h2
                            color: UIStyle.textColor
                        }
                        Text {
                            anchors.bottom: runningtime.bottom
                            anchors.left: runningtime.right
                            anchors.leftMargin: 5
                            text: "min"
                            font: UIStyle.p2
                            color: UIStyle.textColor
                        }
                    }

                    Item {
                        width: childrenRect.width
                        height: childrenRect.height
                        Text {
                            id: runningcal
                            text: FitnessData.getRunningCalories()
                            font: UIStyle.h2
                            color: UIStyle.textColor
                        }
                        Text {
                            anchors.bottom: runningcal.bottom
                            anchors.left: runningcal.right
                            anchors.leftMargin: 5
                            text: qsTr("calories")
                            font: UIStyle.p2
                            color: UIStyle.textColor
                        }
                    }
                }
            }
        }
    }

    QQC2.PageIndicator {
        count: svFitnessContainer.count
        currentIndex: svFitnessContainer.currentIndex

        anchors.bottom: svFitnessContainer.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
