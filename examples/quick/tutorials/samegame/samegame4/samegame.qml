// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "samegame.js" as SameGame

Rectangle {
    id: screen

    width: 490; height: 720

    SystemPalette { id: activePalette }

    Item {
        width: parent.width
        anchors { top: parent.top; bottom: toolBar.top }

        Image {
            id: background
            anchors.fill: parent
            source: "pics/background.jpg"
            fillMode: Image.PreserveAspectCrop
        }

        Item {
            id: gameCanvas
            property int score: 0
            property int blockSize: 40

            anchors.centerIn: parent
            width: parent.width - (parent.width % blockSize);
            height: parent.height - (parent.height % blockSize);

            MouseArea {
                anchors.fill: parent
                onClicked: (mouse)=> SameGame.handleClick(mouse.x,mouse.y);
            }
        }
    }

    Dialog {
        id: dialog
        anchors.centerIn: parent
        z: 100
    }

    //![0]
    Dialog {
        id: nameInputDialog
        anchors.centerIn: parent
        z: 100

        onClosed: {
            if (nameInputDialog.inputText != "")
                SameGame.saveHighScore(nameInputDialog.inputText);
        }
    }
    //![0]

    Rectangle {
        id: toolBar
        width: parent.width; height: 30
        color: activePalette.window
        anchors.bottom: screen.bottom

        Button {
            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
            text: "New Game"
            onClicked: SameGame.startNewGame()
        }

        Text {
            id: score
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
            text: "Score: " + gameCanvas.score
        }
    }
}
