/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

Rectangle {
    id: helpbubble
    visible: false
    property bool showtext: false
    property bool showadvance: true
    onShowadvanceChanged: { advancearrow.visible = showadvance; }

    width: 300; radius: 15; color: "white"; border.color: "black"; border.width: 3
    height: showadvance ? bubbletext.height + advancearrow.height + 25 : bubbletext.height + 25
    Behavior on height {
        SequentialAnimation {
            ScriptAction { script: { bubbletext.visible = false; advancearrow.visible = false } }
            NumberAnimation { duration: 200 }
            ScriptAction { script: { bubbletext.visible = true; advancearrow.visible = showadvance && true } }
        }
    }
    Behavior on width {
        SequentialAnimation {
            ScriptAction { script: { bubbletext.visible = false; advancearrow.visible = false } }
            NumberAnimation { duration: 200 }
            ScriptAction { script: { bubbletext.visible = true; advancearrow.visible = true } }
        }
    }
    Text { id: bubbletext; width: parent.width - 30; text: testtext; wrapMode: Text.WordWrap
            anchors { top: parent.top; topMargin: 15; left: parent.left; leftMargin: 20 }
    }
    Image { id: advancearrow; source: "pics/arrow.png"; height: 30; width: 30; visible: showadvance
        anchors { right: parent.right; bottom: parent.bottom; rightMargin: 5; bottomMargin: 5 }
        MouseArea { enabled: showadvance; anchors.fill: parent; onClicked: { advance(); } }
    }
}
