// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    id: gradientelement
    property real gradstop: 0.25
    gradient: Gradient {
         GradientStop { position: 0.0; color: "red" }
         GradientStop { position: gradstop; color: "yellow" }
         GradientStop { position: 1.0; color: "green" }
    }
    MouseArea { anchors.fill: parent; enabled: qmlfiletoload == ""; hoverEnabled: true
        onEntered: { helptext = "The gradient should show a trio of colors - red, yellow and green"+
        " - with a slow movement of the yellow up and down the view" }
        onExited: { helptext = "" }
    }
    // Animate the background gradient
    SequentialAnimation { id: gradanim; running: true; loops: Animation.Infinite
        NumberAnimation { target: gradientelement; property: "gradstop"; to: 0.88; duration: 10000; easing.type: Easing.InOutQuad }
        NumberAnimation { target: gradientelement; property: "gradstop"; to: 0.22; duration: 10000; easing.type: Easing.InOutQuad }
    }
}
