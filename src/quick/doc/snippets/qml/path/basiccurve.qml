// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Canvas {
    width: 400; height: 200
    contextType: "2d"

    Path {
        id: myPath
        startX: 0; startY: 100

        PathCurve { x: 75; y: 75 }
        PathCurve { x: 200; y: 150 }
        PathCurve { x: 325; y: 25 }
        PathCurve { x: 400; y: 100 }
    }

    onPaint: {
        context.strokeStyle = Qt.rgba(.4,.6,.8);
        context.path = myPath;
        context.stroke();
    }
}
//![0]
