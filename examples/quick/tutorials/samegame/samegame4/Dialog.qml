// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Rectangle {
    id: container
//![0]

//![1]
    property string inputText: textInput.text
    signal closed

    function show(text) {
        dialogText.text = text;
        container.opacity = 1;
        textInput.opacity = 0;
    }

    function showWithInput(text) {
        show(text);
        textInput.opacity = 1;
        textInput.focus = true;
        textInput.text = ""
    }

    function hide() {
        textInput.focus = false;
        container.opacity = 0;
        container.closed();
    }
//![1]

    width: dialogText.width + textInput.width + 20
    height: dialogText.height + 20
    opacity: 0
    visible: opacity > 0

    Text {
        id: dialogText
        anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 10 }
        text: ""
    }

//![2]
    TextInput {
        id: textInput
        anchors { verticalCenter: parent.verticalCenter; left: dialogText.right }
        width: 80
        text: ""

        onAccepted: container.hide()    // close dialog when Enter is pressed
    }
//![2]

    MouseArea {
        anchors.fill: parent

        onClicked:  {
            if (textInput.text == "" && textInput.opacity > 0)
                Qt.inputMethod.show();
            else
                hide();
        }
    }

//![3]
}
//![3]
