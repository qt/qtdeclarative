/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import EasingPlot 1.0

Rectangle {
    width: 775; height: 550

    function precision(n)
    {
        var str = n.toPrecision(3);
        while (str.length > 1 && (str[str.length - 1] == "0" || str[str.length - 1] == "."))
            str = str.substr(0, str.length - 1);
        return str;
    }

    function updateEasing() {
        var ini = Math.min(100, Math.max(0, Number(in_inf.text)));
        var outi = Math.min(100, Math.max(0, Number(out_inf.text)));

        var ins = Number(in_slope.text);
        var outs = Number(out_slope.text);

        var p1 = [ (ini / 100), (ini / 100) * ins ];
        var p2 = [ 1 - (outi / 100), 1 - (outi / 100) * outs ];

        text.text = "[ " + precision(p1[0]) + ", " + precision(p1[1]) + ", " + precision(p2[0]) + ", " + precision(p2[1]) + ", 1, 1 ]";
    }

    Rectangle {
        id: border
        width: 500; height: 500
        x: 25; y: 25
        border.color: "lightsteelblue"
        border.width: 3
        radius: 5
        color: "transparent"

        EasingPlot {
            id: plot

            anchors.centerIn: parent
            width: parent.width - 10
            height: parent.height - 10

            easing.type: "Bezier"
            easing.bezierCurve: eval(text.text)
        }

    }

    Text {
        text: "<u>After Effects curve</u>"
        anchors.horizontalCenter: text.horizontalCenter
        anchors.bottom: column.top
        anchors.bottomMargin: 14
    }

    Column {
        id: column

        y: 70
        anchors.right: parent.right
        anchors.rightMargin: 25
        spacing: 5
        TextField {
            id: in_inf
            focus: true
            name: "Input influence:"
            text: "33"
            anchors.right: parent.right
            KeyNavigation.tab: in_slope
            KeyNavigation.backtab: text
            onTextChanged: updateEasing();
        }
        TextField {
            id: in_slope
            name: "Input slope:"
            text: "0"
            anchors.right: parent.right
            KeyNavigation.tab: out_inf
            KeyNavigation.backtab: in_inf
            onTextChanged: updateEasing();
        }
        TextField {
            id: out_inf
            name: "Output influence:"
            text: "33"
            anchors.right: parent.right
            KeyNavigation.tab: out_slope
            KeyNavigation.backtab: in_slope
            onTextChanged: updateEasing();
        }
        TextField {
            id: out_slope
            name: "Output slope:"
            text: "0"
            anchors.right: parent.right
            KeyNavigation.tab: text
            KeyNavigation.backtab: out_info
            onTextChanged: updateEasing();
        }
    }

    Text {
        text: "<u>QML Bezier curve</u>"
        anchors.horizontalCenter: text.horizontalCenter
        anchors.bottom: text.top
        anchors.bottomMargin: 10
    }

    TextEdit {
        id: text
        x: 200
        width: 200
        height: 200

        Rectangle {
            x: -2; y: -2
            width: parent.width + 4
            height: parent.height + 4
            color: "transparent"
            border.color: text.activeFocus?"green":"lightgreen"

            border.width: 3
            radius: 5
        }

        wrapMode: "WordWrap"

        anchors.top: column.bottom
        anchors.topMargin: 50
        anchors.right: column.right
        KeyNavigation.tab: in_inf
        KeyNavigation.backtab: out_slope
    }


    Item {
        anchors.left: text.left
        anchors.top: text.bottom
        anchors.topMargin: 35
        width: text.width
        height: rect.height

        Rectangle {
            color: "gray"
            width: 50; height: 50
            id: rect

            NumberAnimation on x {
                id: animation
                running: false
                easing: plot.easing
                duration: 1000
            }

            radius: 5
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (rect.x < 5) {
                    animation.to = text.width - rect.width;
                } else {
                    animation.to = 0;
                }
                animation.start();
            }
        }

        Text {
            anchors.centerIn: parent
            text: "Click to Try"
        }
    }

    Component.onCompleted: updateEasing();
}
