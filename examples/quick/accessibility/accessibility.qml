/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import "content"

/*!
    \title QtQuick Examples - Accessibility
    \example quick/accessibility
    \brief This example has accessible buttons.

    Elements in this example are augmented with meta-data for accessiblity systems.

    For example, the button identifies itself and its functionality to the accessibility system:
    \snippet examples/quick/accessibility/content/Button.qml button

    As do Text elements inside the example:
    \snippet examples/quick/accessibility/accessibility.qml text
*/

Rectangle {
    id: window

    width: 320; height: 480
    color: "white"

    Column {
        id: column
        spacing: 6
        anchors.fill: parent
        anchors.margins: 10
        width: parent.width
        Row {
            spacing: 6
            width: column.width
            Button { width: 100; height: column.h + 20; text: "Send"; onClicked : { status.text = "Send" } }
            Button { width: 100; height: column.h + 20; text: "Discard";  onClicked : { status.text = "Discard" } }
        }

        Row {
            spacing: 6
            width: column.width
            height: column.h
            Text {
                id: subjectLabel
                //! [text]
                Accessible.role: Accessible.StaticText
                Accessible.name: text
                //! [text]
                text: "Subject:"
            }
            Rectangle {
                id: subjectBorder
                Accessible.role: Accessible.EditableText
                Accessible.name: subjectEdit.text
                border.width: 1
                border.color: "black"
                height: subjectEdit.height
                width: 240
                TextInput {
                    id: subjectEdit
                    text: "Vacation plans"
                }
            }
        }
        Rectangle {
            id: textBorder
            Accessible.role: Accessible.EditableText
            property alias text : textEdit.text
            border.width: 1
            border.color: "black"
            width: parent.width - 2
            height: parent.height - (textBorder.y + column.spacing)
            TextEdit {
                id: textEdit
                text: "Hi, we're going to the Dolomites this summer. Weren't you also going to northern Italy? \n\nbest wishes, your friend Luke"
                width: parent.width
                wrapMode: TextEdit.WordWrap
            }
        }
        Text {
            id : status
            width: column.width
        }

        Row {
            spacing: 6
            width: column.width
            Checkbox { checked: false }
            Slider { value: 10 }
        }
    }
}
