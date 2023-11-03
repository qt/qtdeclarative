// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls.Basic
import FileSystemModule

ApplicationWindow {
    id: root
    width: 650
    height: 550
    flags: Qt.Window | Qt.FramelessWindowHint
    color: Colors.surface1

    menuBar: MyMenuBar {
        id: menuBar

        dragWindow: root
        implicitHeight: 30
        infoText: "About Qt"
    }

    Image {
        id: logo

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20

        source: "../icons/qt_logo.svg"
        sourceSize.width: 80
        sourceSize.height: 80
        fillMode: Image.PreserveAspectFit

        smooth: true
        antialiasing: true
        asynchronous: true
    }

    ScrollView {
      anchors.top: logo.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      anchors.margins: 20

      TextArea {
          selectedTextColor: Colors.textFile
          selectionColor: Colors.selection
          horizontalAlignment: Text.AlignHCenter
          textFormat: Text.RichText

          text: qsTr("<h3>About Qt</h3>"
                   + "<p>This program uses Qt version %1.</p>"
                   + "<p>Qt is a C++ toolkit for cross-platform application "
                   + "development.</p>"
                   + "<p>Qt provides single-source portability across all major desktop "
                   + "operating systems. It is also available for embedded Linux and other "
                   + "embedded and mobile operating systems.</p>"
                   + "<p>Qt is available under multiple licensing options designed "
                   + "to accommodate the needs of our various users.</p>"
                   + "<p>Qt licensed under our commercial license agreement is appropriate "
                   + "for development of proprietary/commercial software where you do not "
                   + "want to share any source code with third parties or otherwise cannot "
                   + "comply with the terms of GNU (L)GPL.</p>"
                   + "<p>Qt licensed under GNU (L)GPL is appropriate for the "
                   + "development of Qt&nbsp;applications provided you can comply with the terms "
                   + "and conditions of the respective licenses.</p>"
                   + "<p>Please see <a href=\"http://%2/\">%2</a> "
                   + "for an overview of Qt licensing.</p>"
                   + "<p>Copyright (C) %3 The Qt Company Ltd and other "
                   + "contributors.</p>"
                   + "<p>Qt and the Qt logo are trademarks of The Qt Company Ltd.</p>"
                   + "<p>Qt is The Qt Company Ltd product developed as an open source "
                   + "project. See <a href=\"http://%4/\">%4</a> for more information.</p>")
                   .arg(Application.version).arg("qt.io/licensing").arg("2023").arg("qt.io")
          color: Colors.textFile
          wrapMode: Text.WordWrap
          readOnly: true
          antialiasing: true
          background: null

          onLinkActivated: function(link) {
              Qt.openUrlExternally(link)
          }
      }
    }

    ResizeButton {
        resizeWindow: root
    }
}
