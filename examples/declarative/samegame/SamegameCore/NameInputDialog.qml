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

Dialog {
    id: nameInputDialog

    property int initialWidth: 0
    property alias name: nameInputText.text

    anchors.centerIn: parent
    z: 22;

    Behavior on width {
        NumberAnimation {} 
        enabled: nameInputDialog.initialWidth != 0
    }

    signal accepted(string name)
    onClosed: {
        if (nameInputText.text != "")
            accepted(name);
    }
    Text {
        id: dialogText
        anchors { left: nameInputDialog.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
        text: "You won! Please enter your name: "
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (nameInputText.text == "")
                nameInputText.openSoftwareInputPanel();
            else
                nameInputDialog.forceClose();
        }
    }

    TextInput {
        id: nameInputText
        anchors { verticalCenter: parent.verticalCenter; left: dialogText.right }
        focus: visible
        autoScroll: false
        maximumLength: 24
        onTextChanged: {
            var newWidth = nameInputText.width + dialogText.width + 40;
            if ( (newWidth > nameInputDialog.width && newWidth < screen.width) 
                    || (nameInputDialog.width > nameInputDialog.initialWidth) )
                nameInputDialog.width = newWidth;
        }
        onAccepted: {
            nameInputDialog.forceClose();
        }
    }
}
