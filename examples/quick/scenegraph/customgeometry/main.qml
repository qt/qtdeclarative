// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
import QtQuick
import CustomGeometry
//! [1] //! [2]
Item {
    width: 300
    height: 200

    BezierCurve {
        id: line
        anchors.fill: parent
        anchors.margins: 20
//! [2] //! [3]
        property real t
        SequentialAnimation on t {
            NumberAnimation { to: 1; duration: 2000; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 0; duration: 2000; easing.type: Easing.InOutQuad }
            loops: Animation.Infinite
        }

        p2: Qt.point(t, 1 - t)
        p3: Qt.point(1 - t, t)
    }
//! [3] //! [4]
    Text {
        anchors.bottom: line.bottom

        x: 20
        width: parent.width - 40
        wrapMode: Text.WordWrap

        text: qsTr("This curve is a custom scene graph item, implemented using line strips")
    }
}
//! [4]
