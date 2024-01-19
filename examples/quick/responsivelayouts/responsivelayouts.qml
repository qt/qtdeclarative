// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: appWindow

    visible: true
    title: qsTr("Responsive layouts with LayoutItemProxy")

    minimumHeight: 500
    minimumWidth: 250

    //! [item definition]
    Rectangle {
        id: contentItem
        Layout.fillWidth: true
        implicitHeight: grid.implicitHeight
        implicitWidth: grid.implicitWidth
        color: "#00414A"

        GridLayout {
            anchors.fill: parent
            id: grid
            columns: 2
            anchors.margins: 8
            Repeater {
                model: 18
                Rectangle {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    implicitWidth: 200
                    implicitHeight: width
                    radius: width / 10
                    gradient: Gradient {
                        GradientStop { position: -0.2; color: "#2CDE85" }
                        GradientStop { position: 1.2; color: "#00414A" }
                    }
                    Text {
                        color: "#ffffff"
                        font.pointSize: 22
                        anchors.centerIn: parent
                        text: index
                    }
                }
            }
        }
    }

    Button {
        id: a
        text: "Text"
        icon.source: "./icons/text.svg"
        Layout.fillWidth: true
        Layout.margins: 3
    }

    Button {
        id: b
        text: "Grid 1"
        icon.source: "./icons/grid.svg"
        Layout.fillWidth: true
        Layout.margins: 3
    }

    Button {
        id: c
        text: "Grid 2"
        icon.source: "./icons/grid.svg"
        Layout.fillWidth: true
        Layout.margins: 3
    }

    Button {
        id: d
        text: "Settings"
        icon.source: "./icons/settings.svg"
        Layout.fillWidth: true
        Layout.margins: 3
    }
    //! [item definition]

    //! [first layout]
    ColumnLayout {
        id: smallLayout
        anchors.fill: parent

        //! [proxy in flickable]
        Flickable {
            Layout.fillHeight: true
            Layout.fillWidth: true
            contentWidth: width
            contentHeight: gl.implicitHeight
            clip: true
            ScrollIndicator.vertical: ScrollIndicator { }
            LayoutItemProxy {
                id: gl
                width: parent.width
                height: implicitHeight
                target: contentItem
            }
        }
        //! [proxy in flickable]

        //! [proxy in layout]
        RowLayout {
            Layout.fillHeight: false
            Layout.fillWidth: true
            Layout.margins: 5
            //! [layout property on proxy]
            LayoutItemProxy{ target: a; }
            //! [layout property on proxy]
            LayoutItemProxy{ target: b; }
            LayoutItemProxy{ target: c; }
        }
        //! [proxy in layout]
    }
    //! [first layout]

    //! [second layout]
    RowLayout {
        id: largeLayout
        anchors.fill: parent
        ColumnLayout {
            Layout.minimumWidth: 100
            Layout.margins: 2
            LayoutItemProxy{ target: a }
            LayoutItemProxy{ target: b }
            LayoutItemProxy{ target: c }
            //! [spacer item]
            Item { Layout.fillHeight: true }
            //! [spacer item]
            LayoutItemProxy{ target: d }
        }

        LayoutItemProxy {
            Layout.fillHeight: true
            Layout.fillWidth: false
            target: contentItem
        }
    }
    //! [second layout]

    //! [setting the layout]
    function setFittingLayout() {
        if (width < 450) {
            smallLayout.visible = true
            largeLayout.visible = false
        } else {
            smallLayout.visible = false
            largeLayout.visible = true
        }
    }
    onWidthChanged: setFittingLayout()
    Component.onCompleted: setFittingLayout()
    //! [setting the layout]
}
