// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.14
import QtQuick.Shapes 1.14

Item {
    id: item
    width: 200
    height: 150

    Shape {
        vendorExtensionsEnabled: false
        objectName: "shape"
        id: shape
        anchors.fill: parent

        ShapePath {
            strokeWidth: 4
            strokeColor: "red"
            scale: Qt.size(shape.width - 1, shape.height - 1)
            fillGradient: LinearGradient {
                x1: 20; y1: 20
                x2: 180; y2: 130
                GradientStop { position: 0; color: "blue" }
                GradientStop { position: 0.2; color: "green" }
                GradientStop { position: 0.4; color: "red" }
                GradientStop { position: 0.6; color: "yellow" }
                GradientStop { position: 1; color: "cyan" }
            }
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 4 ]
            PathMultiline {
                paths: [[Qt.point(20.0  / (item.width - 1.0), 20.0  / (item.height - 1.0)),
                         Qt.point(180.0 / (item.width - 1.0), 130.0 / (item.height - 1.0)),
                         Qt.point(20.0  / (item.width - 1.0), 130.0 / (item.height - 1.0)),
                         Qt.point(20.0  / (item.width - 1.0), 20.0  / (item.height - 1.0)) ],
                        [Qt.point( 0.45 , 0.67 ),
                         Qt.point( 0.414906666468 , 0.573581858547 ),
                         Qt.point( 0.32604722665 , 0.522278837048 ),
                         Qt.point( 0.225 , 0.540096189432 ),
                         Qt.point( 0.159046106882 , 0.618696978501 ),
                         Qt.point( 0.159046106882 , 0.721303021499 ),
                         Qt.point( 0.225 , 0.799903810568 ),
                         Qt.point( 0.32604722665 , 0.817721162952 ),
                         Qt.point( 0.414906666468 , 0.766418141453 ),
                         Qt.point( 0.45 , 0.67 ),
                        ],
                        [Qt.point( 0.69 , 0.75 ),
                         Qt.point( 0.668943999881 , 0.692149115128 ),
                         Qt.point( 0.61562833599 , 0.661367302229 ),
                         Qt.point( 0.555 , 0.672057713659 ),
                         Qt.point( 0.515427664129 , 0.719218187101 ),
                         Qt.point( 0.515427664129 , 0.780781812899 ),
                         Qt.point( 0.555 , 0.827942286341 ),
                         Qt.point( 0.61562833599 , 0.838632697771 ),
                         Qt.point( 0.668943999881 , 0.807850884872 ),
                         Qt.point( 0.69 , 0.75 ),
                        ]]
            }
        }
    }
}
