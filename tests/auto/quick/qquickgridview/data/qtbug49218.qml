// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick 2.4

Item {
    width: 500
    height: 160
    visible: true

    property var model1: ["1","2","3","4","5","6","7","8","9","10",
    "11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30",
    "31","32","33","34","a"]
    property var model2: ["a","b","c","d","e","f","g","h","i","j","k","l","m","1"]
    property bool useModel1: true

    function changeModel() {
        useModel1 = !useModel1
        grid.loadModel(useModel1 ? model1 : model2)
    }

    function scrollToTop() {
        grid.contentY = grid.originY;
    }

    GridView {
        id: grid
        anchors.fill: parent

        model: ListModel {
        }

        onCurrentIndexChanged: {
            positionViewAtIndex(currentIndex, GridView.Contain)
        }

        Component.onCompleted: {
            loadModel(model1)
            grid.currentIndex = 34
            grid.positionViewAtIndex(34, GridView.Contain)
        }

        function loadModel(m) {
            var remove = {};
            var add = {};
            var i;
            for (i=0; i < model.count; ++i)
                remove[model.get(i).name] = true;
            for (i=0; i < m.length; ++i)
                if (remove[m[i]])
                    delete remove[m[i]];
                else
                    add[m[i]] = true;

            for (i=model.count-1; i>= 0; --i)
                if (remove[model.get(i).name])
                    model.remove(i, 1);

            for (i=0; i<m.length; ++i)
                if (add[m[i]])
                    model.insert(i, { "name": m[i] })
        }

        delegate: Rectangle {
            height: grid.cellHeight
            width: grid.cellWidth
            color: GridView.isCurrentItem ? "gray" : "white"
            Text {
                anchors.fill: parent
                text: name
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    grid.currentIndex = index
                }
            }
        }
    }
}
