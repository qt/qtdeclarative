// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    width: 400
    height: 400

    PathInterpolator {
        id: motionPath

        path: Path {
            startX: 0; startY: 0

            PathCubic {
                x: 350; y: 350

                control1X: 350; control1Y: 0
                control2X: 0; control2Y: 350
            }
        }

        NumberAnimation on progress { from: 0; to: 1; duration: 2000 }
    }

    Rectangle {
        width: 50; height: 50
        color: "green"

        //bind our attributes to follow the path as progress changes
        x: motionPath.x
        y: motionPath.y
        rotation: motionPath.angle
    }
}
//![0]
