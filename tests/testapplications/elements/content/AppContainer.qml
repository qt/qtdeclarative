// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {

    property string qmlfile: ""
    height: parent.height *.95; width: parent.width *.95; anchors.centerIn: parent; radius: 5

    onQmlfileChanged: { qmlapp.source = qmlfile; if (qmlfile != "") { starttimer.start(); } }

    Loader {
        id: qmlapp
        property int statenum: 0
        property int statecount
        statecount: qmlfile != "" ? children[0].states.length : 0
        anchors.fill: parent; focus: true
        function advance() { statenum = statenum == statecount ? 1 : ++statenum; }
    }

    Timer { id: starttimer; interval: 500; onTriggered: { qmlapp.advance(); } }

    Rectangle {
        anchors { top: parent.top; right: parent.right; topMargin: 3; rightMargin: 3 }
        height: 30; width: 30; color: "red"; radius: 5
        Text { text: "X"; anchors.centerIn: parent; font.pointSize: 12 }
        MouseArea { anchors.fill: parent; onClicked: { elementsapp.qmlfiletoload = "" } }
    }

    Text { anchors.centerIn: parent; visible: qmlapp.status == Loader.Error; text: qmlfile+" failed to load.\n"; }

}
