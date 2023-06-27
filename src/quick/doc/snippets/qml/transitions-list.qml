// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: page
    width: 640
    height: 500

    Image {
        id: userIcon
        x: topLeftRect.x
        y: topLeftRect.y

        source: "../../images/declarative-qtlogo.png"
    }

    Rectangle {
        id: topLeftRect
        anchors { left: parent.left; top: parent.top; leftMargin: 10; topMargin: 20 }
        width: 46; height: 54
        color: "Transparent"; border.color: "Gray"; radius: 6
        // Clicking here sets state to default state, returning image to initial position
        TapHandler { onTapped: page.state = 'start' }
    }

    Rectangle {
        id: middleRightRect
        anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 20 }
        width: 46; height: 54
        color: "Transparent"; border.color: "Gray"; radius: 6
        // Clicking in here sets the state to 'middleRight'
        TapHandler { onTapped: page.state = 'middleRight' }
    }

    Rectangle {
        id: bottomLeftRect
        anchors { left: parent.left; bottom: parent.bottom; leftMargin: 10; bottomMargin: 20 }
        width: 46; height: 54
        color: "Transparent"; border.color: "Gray"; radius: 6
        // Clicking in here sets the state to 'bottomLeft'
        TapHandler { onTapped: page.state = 'bottomLeft' }
    }

    states: [
        State {
            name: "start"
            PropertyChanges {
                target: userIcon
                explicit: true
                x: topLeftRect.x
                y: topLeftRect.y
            }
        },
        State {
            name: "middleRight"
            PropertyChanges {
                target: userIcon
                explicit: true
                x: middleRightRect.x
                y: middleRightRect.y
            }
        },
        State {
            name: "bottomLeft"
            PropertyChanges {
                target: userIcon
                explicit: true
                x: bottomLeftRect.x
                y: bottomLeftRect.y
            }
        }
    ]
//! [list of transitions]
    transitions: [
        Transition {
            from: "*"; to: "middleRight"
            NumberAnimation {
                properties: "x,y";
                easing.type: Easing.InOutQuad;
                duration: 2000;
            }
        },
        Transition {
            from: "*"; to: "bottomLeft";
            NumberAnimation {
                properties: "x,y";
                easing.type: Easing.InOutQuad;
                duration: 200;
            }
        },
        //If any other rectangle is clicked, the icon will return
        //to the start position at a slow speed and bounce.
        Transition {
            from: "*"; to: "*";
            NumberAnimation {
                easing.type: Easing.OutBounce;
                properties: "x,y";
                duration: 4000;
            }
        }
    ]
//! [list of transitions]
}
