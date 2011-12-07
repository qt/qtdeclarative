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
import QtQuick.Window 2.0
import "widgets"

Rectangle {
    id: window

    width: 360; height: 300
    color: "white"

    Column {
        id: column
        spacing: 6
        anchors.fill: parent
        width: parent.width
        Row {
            spacing: 6
            width: column.width
            Button { width: 100; height: column.h + 20; text: "Send" }
            Button { width: 100; height: column.h + 20; text: "Discard" }
        }

        Row {
            spacing: 6
            width: column.width
            height: column.h
            Text {
                id: subjectLabel
                Accessible.role: Accessible.StaticText
                Accessible.name: text
                text: "Subject:"
                width: 50
            }
            Rectangle {
                id: subjectBorder
                Accessible.role: Accessible.EditableText
                Accessible.name: subjectEdit.text
                border.width: 1
                border.color: "black"
                height: subjectEdit.height
                width: 304
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
            width: parent.width
            height: textEdit.height
            TextEdit {
                id: textEdit
                text: "Hi, we're going to the Dolomites this summer. Weren't you also going to northern Italy? \n\nbest wishes, your friend Luke"
                width: parent.width
                wrapMode: TextEdit.WordWrap
            }
        }
    }
}
