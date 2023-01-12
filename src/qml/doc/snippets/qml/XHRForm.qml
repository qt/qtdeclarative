// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "request.js" as XHR

ApplicationWindow {
      width: 640
      height: 640
      visible: true

      ColumnLayout {
           anchors.fill: parent

           RowLayout {
               Layout.fillWidth: true

               TextField {
                   id: urlTextField
                   text: "https://www.example.com/index.html"
                   Layout.fillWidth: true
               }
               Button {
                   text: qsTr("Send!")
                   onClicked: XHR.sendRequest(urlTextField.text, function(response) {
                       statusTextField.text = response.status;
                       let isPlainText = response.contentType.length === 0

                       contentTypeTextField.text = isPlainText ? "text" : response.contentType;

                       if (isPlainText)
                           contentTextArea.text = response.content;
                   });
               }
           }

           GridLayout {
               columns: 2

               Layout.fillWidth: true

               Label {
                   text: qsTr("Status code")

                   Layout.fillWidth: true
               }
               Label {
                   text: qsTr("Response type")

                   Layout.fillWidth: true
               }
               TextField {
                    id: statusTextField

                    Layout.fillWidth: true
               }
               TextField {
                    id: contentTypeTextField

                    Layout.fillWidth: true
               }
           }
           Flickable {
               clip: true
               contentWidth: contentTextArea.width
               contentHeight: contentTextArea.height
               Text {
                    id: contentTextArea
               }

               Layout.fillWidth: true
               Layout.fillHeight: true
               ScrollBar.vertical: ScrollBar {}
               ScrollBar.horizontal: ScrollBar {}
           }
      }
}

//![0]

