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

    component AnnotatedRect : Rectangle {
        implicitWidth: 36
        implicitHeight: 36
        Layout.minimumWidth: 36
        Layout.minimumHeight: 36
        Layout.alignment: Qt.AlignHCenter
        property string text: ""
        border.color: "#EEE"
        border.width: hh.hovered ? 2 : 0
        HoverHandler { id: hh }
        Text {
            font.pixelSize: 18
            font.bold: true
            anchors.centerIn: parent
            text: parent.text
            color: "#333"
        }
    }

    //! [item definition]
    AnnotatedRect {
        id: contentItem
        text: "Content Item"
        Layout.fillWidth: true
        implicitHeight: 1000
        implicitWidth: 500
        gradient: Gradient {
            GradientStop { position: 0.0; color: "tomato" }
            GradientStop { position: 0.5; color: "navajowhite" }
            GradientStop { position: 1.0; color: "darkseagreen" }
        }
    }

    AnnotatedRect {
        id: a
        text: "A"
        color: "lightskyblue"
        Layout.fillWidth: true
    }

    AnnotatedRect {
        id: b
        text: "B"
        color: "lightskyblue"
        Layout.fillWidth: true
    }

    AnnotatedRect {
        id: c
        text: "C"
        color: "lightskyblue"
        Layout.fillWidth: true
    }

    AnnotatedRect {
        id: d
        text: "D"
        color: "lightskyblue"
        Layout.fillWidth: true
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
            //! [layout property on proxy]
            LayoutItemProxy{ target: a; Layout.bottomMargin: 5 }
            //! [layout property on proxy]
            LayoutItemProxy{ target: b; Layout.bottomMargin: 5 }
            LayoutItemProxy{ target: c; Layout.bottomMargin: 5 }
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
