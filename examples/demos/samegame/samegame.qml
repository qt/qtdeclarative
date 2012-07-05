/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Particles 2.0
import "content/samegame.js" as Logic
import "content"

Rectangle {
    id: root
    width: 320; height: 480
    property int acc: 0
    property int menuDelay: 500

    function loadPuzzle() {
        if (gameCanvas.mode != "")
            Logic.cleanUp();
        Logic.startNewGame(gameCanvas,"puzzle","levels/level"+acc+".qml")
    }
    function nextPuzzle() {
        acc = (acc + 1) % 10;
        loadPuzzle();
    }
    Timer {
        id: gameOverTimer
        interval: 1500
        running : gameCanvas.gameOver && gameCanvas.mode == "puzzle" //mode will be reset by cleanUp();
        repeat  : false
        onTriggered: {
            Logic.cleanUp();
            nextPuzzle();
        }
    }

    Image {
        source: "content/gfx/background.png"
        anchors.fill: parent
    }

    GameArea {
        id: gameCanvas
        z: 1
        width: parent.width

        y: 20
        height: parent.height - 64
        backgroundVisible: root.state == "in-game"
        onModeChanged: if (gameCanvas.mode != "puzzle") puzzleWon = false; //UI has stricter constraints on this variable than the game does
        Age {
            groups: ["redspots", "greenspots", "bluespots", "yellowspots"]
            enabled: root.state == ""
            system: gameCanvas.ps
        }

        onPuzzleLost: acc--;//So that nextPuzzle() reloads the current one

    }

    Item {
        id: menu
        z: 2
        width: parent.width;
        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        LogoAnimation {
            x: 64
            y: 20
            particleSystem: gameCanvas.ps
            running: root.state == ""
        }
        Row {
            x: 112
            y: 20
            Image { source: "content/gfx/logo-a.png" }
            Image { source: "content/gfx/logo-m.png" }
            Image { source: "content/gfx/logo-e.png" }
        }

        Column {
            y: 100 + 40
            spacing: 0

            Button {
                height: 64
                width: root.width
                rotatedButton: true
                imgSrc: "content/gfx/but-game-1.png"
                onClicked: {
                    if (root.state == "in-game")
                        return //Prevent double clicking
                    root.state = "in-game"
                    gameCanvas.blockFile = "Block.qml"
                    gameCanvas.background = "gfx/background.png"
                    arcadeTimer.start();
                }
                //Emitted particles don't fade out, because ImageParticle is on the GameArea
                system: gameCanvas.ps
                group: "green"
                Timer {
                    id: arcadeTimer
                    interval: menuDelay
                    running : false
                    repeat  : false
                    onTriggered: Logic.startNewGame(gameCanvas)
                }
            }

            Button {
                height: 64
                width: root.width
                rotatedButton: true
                imgSrc: "content/gfx/but-game-2.png"
                onClicked: {
                    if (root.state == "in-game")
                        return
                    root.state = "in-game"
                    gameCanvas.blockFile = "Block.qml"
                    gameCanvas.background = "gfx/background.png"
                    twopTimer.start();
                }
                system: gameCanvas.ps
                group: "green"
                Timer {
                    id: twopTimer
                    interval: menuDelay
                    running : false
                    repeat  : false
                    onTriggered: Logic.startNewGame(gameCanvas, "multiplayer")
                }
            }

            Button {
                height: 64
                width: root.width
                rotatedButton: true
                imgSrc: "content/gfx/but-game-3.png"
                onClicked: {
                    if (root.state == "in-game")
                        return
                    root.state = "in-game"
                    gameCanvas.blockFile = "SimpleBlock.qml"
                    gameCanvas.background = "gfx/background.png"
                    endlessTimer.start();
                }
                system: gameCanvas.ps
                group: "blue"
                Timer {
                    id: endlessTimer
                    interval: menuDelay
                    running : false
                    repeat  : false
                    onTriggered: Logic.startNewGame(gameCanvas, "endless")
                }
            }

            Button {
                height: 64
                width: root.width
                rotatedButton: true
                imgSrc: "content/gfx/but-game-4.png"
                group: "yellow"
                onClicked: {
                    if (root.state == "in-game")
                        return
                    root.state = "in-game"
                    gameCanvas.blockFile = "PuzzleBlock.qml"
                    gameCanvas.background = "gfx/background.png"
                    puzzleTimer.start();
                }
                Timer {
                    id: puzzleTimer
                    interval: menuDelay
                    running : false
                    repeat  : false
                    onTriggered: loadPuzzle();
                }
                system: gameCanvas.ps
            }
        }
    }

    Image {
        id: scoreBar
        source: "content/gfx/bar.png"
        width: parent.width
        z: 6
        y: -24
        height: 24
        Behavior on opacity { NumberAnimation {} }
        Text {
            id: arcadeScore
            anchors { right: parent.right; topMargin: 3; rightMargin: 11; top: parent.top}
            text: '<font color="#f7d303">P1:</font> ' + gameCanvas.score
            font.pixelSize: 14
            textFormat: Text.StyledText
            color: "white"
            opacity: gameCanvas.mode == "arcade" ? 1 : 0
            Behavior on opacity { NumberAnimation {} }
        }
        Text {
            id: arcadeHighScore
            anchors { left: parent.left; topMargin: 3; leftMargin: 11; top: parent.top}
            text: '<font color="#f7d303">Highscore:</font> ' + gameCanvas.highScore
            font.pixelSize: 14
            color: "white"
            textFormat: Text.StyledText
            opacity: gameCanvas.mode == "arcade" ? 1 : 0
            Behavior on opacity { NumberAnimation {} }
        }
        Text {
            id: p1Score
            anchors { right: parent.right; topMargin: 3; rightMargin: 11; top: parent.top}
            text: '<font color="#f7d303">P1:</font> ' + gameCanvas.score
            textFormat: Text.StyledText
            font.pixelSize: 14
            color: "white"
            opacity: gameCanvas.mode == "multiplayer" ? 1 : 0
            Behavior on opacity { NumberAnimation {} }
        }
        Text {
            id: p2Score
            anchors { left: parent.left; topMargin: 3; leftMargin: 11; top: parent.top}
            text: '<font color="#f7d303">P2:</font> ' + gameCanvas.score2
            textFormat: Text.StyledText
            font.pixelSize: 14
            color: "white"
            opacity: gameCanvas.mode == "multiplayer" ? 1 : 0
            Behavior on opacity { NumberAnimation {} }
            rotation: 180
        }
        Text {
            id: puzzleMoves
            anchors { left: parent.left; topMargin: 3; leftMargin: 11; top: parent.top}
            text: '<font color="#f7d303">Moves:</font> ' + gameCanvas.moves
            textFormat: Text.StyledText
            font.pixelSize: 14
            color: "white"
            opacity: gameCanvas.mode == "puzzle" ? 1 : 0
            Behavior on opacity { NumberAnimation {} }
        }
        Text {
            Image {
                source: "content/gfx/icon-time.png"
                x: -20
            }
            id: puzzleTime
            anchors { topMargin: 3; top: parent.top; horizontalCenter: parent.horizontalCenter; horizontalCenterOffset: 20}
            text: "00:00"
            font.pixelSize: 14
            color: "white"
            opacity: gameCanvas.mode == "puzzle" ? 1 : 0
            Behavior on opacity { NumberAnimation {} }
            Timer {
                interval: 1000
                repeat: true
                running: gameCanvas.mode == "puzzle" && !gameCanvas.gameOver
                onTriggered: {
                    var elapsed = Math.floor((new Date() - Logic.gameDuration)/ 1000.0);
                    var mins = Math.floor(elapsed/60.0);
                    var secs = (elapsed % 60);
                    puzzleTime.text =  (mins < 10 ? "0" : "") + mins + ":" + (secs < 10 ? "0" : "") + secs;
                }
            }
        }
        Text {
            id: puzzleScore
            anchors { right: parent.right; topMargin: 3; rightMargin: 11; top: parent.top}
            text: '<font color="#f7d303">Score:</font> ' + gameCanvas.score
            textFormat: Text.StyledText
            font.pixelSize: 14
            color: "white"
            opacity: gameCanvas.mode == "puzzle" ? 1 : 0
            Behavior on opacity { NumberAnimation {} }
        }
    }

    Image {
        id: bottomBar
        width: parent.width
        height: 44
        source: "content/gfx/bar.png"
        y: parent.height - 44
        z: 2
        function selectButtons() {
            menuButton.visible = (root.state == "in-game");
            nextButton.visible = (root.state == "in-game");
            againButton.visible = (root.state == "in-game");
        }
        Button {
            id: quitButton
            imgSrc: "content/gfx/but-quit.png"
            onClicked: {Qt.quit(); }
            anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 11 }
        }
        Button {
            id: menuButton
            imgSrc: "content/gfx/but-menu.png"
            visible: false
            onClicked: {root.state = ""; Logic.cleanUp(); gameCanvas.mode = ""}
            anchors { left: quitButton.right; verticalCenter: parent.verticalCenter; leftMargin: 0 }
        }
        Button {
            id: againButton
            imgSrc: "content/gfx/but-game-new.png"
            visible: false
            opacity: gameCanvas.gameOver && (gameCanvas.mode == "arcade" || gameCanvas.mode == "multiplayer") ? 1 : 0
            Behavior on opacity{ NumberAnimation {} }
            onClicked: {if (gameCanvas.gameOver) Logic.startNewGame(gameCanvas, gameCanvas.mode);}
            anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 11 }
        }
        Button {
            id: nextButton
            imgSrc: "content/gfx/but-puzzle-next.png"
            visible: false
            opacity: gameCanvas.puzzleWon ? 1 : 0
            Behavior on opacity{ NumberAnimation {} }
            onClicked: {if (gameCanvas.puzzleWon) nextPuzzle();}
            anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 11 }
        }
    }

    Connections {
        target: root
        onStateChanged: stateChangeAnim.running = true
    }
    SequentialAnimation {
        id: stateChangeAnim
        ParallelAnimation {
            NumberAnimation { target: bottomBar; property: "y"; to: root.height; duration: menuDelay/2; easing.type: Easing.OutQuad }
            NumberAnimation { target: scoreBar; property: "y"; to: -24; duration: menuDelay/2; easing.type: Easing.OutQuad }
        }
        ScriptAction { script: bottomBar.selectButtons(); }
        ParallelAnimation {
            NumberAnimation { target: bottomBar; property: "y"; to: root.height - 44; duration: menuDelay/2; easing.type: Easing.OutBounce}
            NumberAnimation { target: scoreBar; property: "y"; to: root.state == "" ? -24 : 0; duration: menuDelay/2; easing.type: Easing.OutBounce}
        }
    }

    states: [
        State {
            name: "in-game"
            PropertyChanges {
                target: menu
                opacity: 0
                visible: false
            }
        }
    ]

    transitions: [
        Transition {
            NumberAnimation {properties: "x,y,opacity"}
        }
    ]

    //"Debug mode"
    focus: true
    Keys.onAsteriskPressed: Logic.nuke();
    Keys.onSpacePressed: gameCanvas.puzzleWon = true;
}
