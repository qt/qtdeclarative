// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

import Graph

Item {
    width: 800
    height: 400

    Graph {
        id: graph
        anchors.fill: parent
        anchors.margins: 100

        function newSample(i) {
            return (Math.sin(i / 100.0 * Math.PI * 2) + 1) * 0.4 + Math.random() * 0.05;
        }

        Component.onCompleted: {
            for (var i=0; i<100; ++i)
                appendSample(newSample(i));
        }

        property int offset: 100;
    }

    Timer {
        id: timer
        interval: 500
        repeat: true
        running: true
        onTriggered: {
            graph.removeFirstSample();
            graph.appendSample(graph.newSample(++graph.offset));
        }

    }

    Rectangle {
        anchors.fill: graph
        color: "transparent"
        border.color: "black"
        border.width: 2
    }

}
