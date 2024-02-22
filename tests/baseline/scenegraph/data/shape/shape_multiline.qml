// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.14
import QtQuick.Shapes 6.6

Item {
    id: root
    width: 320
    height: 480

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    Column {
        Repeater {
            model: renderers
            Item {
                x: 20
                width: root.width
                height: root.height / 2

                Shape {
                    anchors.fill: parent
                    preferredRendererType: renderer

                    ShapePath {
                        strokeWidth: 1
                        strokeColor: "red"
                        fillColor: Qt.rgba(1,0,0,0.3)
                        scale: Qt.size(root.height / 2, root.height / 2)
                        PathMultiline {
                            paths: [
                                [Qt.point(0.5,     0.06698),
                                 Qt.point(1,       0.93301),
                                 Qt.point(0,       0.93301),
                                 Qt.point(0.5,     0.06698)],

                                [Qt.point(0.5,     0.12472),
                                 Qt.point(0.95,    0.90414),
                                 Qt.point(0.05,    0.90414),
                                 Qt.point(0.5,     0.12472)],

                                [Qt.point(0.47131, 0.32986),
                                 Qt.point(0.36229, 0.64789),
                                 Qt.point(0.51492, 0.58590),
                                 Qt.point(0.47563, 0.76014),
                                 Qt.point(0.44950, 0.73590),
                                 Qt.point(0.46292, 0.83392),
                                 Qt.point(0.52162, 0.75190),
                                 Qt.point(0.48531, 0.76230),
                                 Qt.point(0.57529, 0.53189),
                                 Qt.point(0.41261, 0.59189),
                                 Qt.point(0.53001, 0.32786),
                                 Qt.point(0.47131, 0.32986)]
                            ]
                        }
                    }
                }
            }
        }
    }
}
