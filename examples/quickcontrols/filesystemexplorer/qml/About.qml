// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls.Basic
import FileSystemModule

ApplicationWindow {
    id: root
    width: 500
    height: 360
    flags: Qt.Window | Qt.FramelessWindowHint
    color: Colors.surface1

    menuBar: MyMenuBar {
        id: menuBar
        implicitHeight: 20
        rootWindow: root
        infoText: "About Qt"
    }

    Image {
        id: logo
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        source: "../icons/qt_logo.svg"
        sourceSize: Qt.size(80, 80)
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
        asynchronous: true
    }

    TextArea {
        anchors.top: logo.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        antialiasing: true
        wrapMode: Text.WrapAnywhere
        color: Colors.textFile
        horizontalAlignment: Text.AlignHCenter
        readOnly: true
        selectionColor: Colors.selection
        text: qsTr("Qt Group (Nasdaq Helsinki: QTCOM) is a global software company with a strong \
presence in more than 70 industries and is the leading independent technology behind 1+ billion \
devices and applications. Qt is used by major global companies and developers worldwide, and the \
technology enables its customers to deliver exceptional user experiences and advance their digital \
transformation initiatives. Qt achieves this through its cross-platform software framework for the \
development of apps and devices, under both commercial and open-source licenses.")
        background: Rectangle {
            color: "transparent"
        }
    }
    ResizeButton {}
}
