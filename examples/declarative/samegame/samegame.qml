/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
import QtQuick.Particles 2.0
import "SamegameCore"
import "SamegameCore/samegame.js" as Logic

Rectangle {
    id: screen
    width: 480; height: 640

    SystemPalette { id: activePalette }

    GameArea {
        id: gameCanvas
        width: parent.width
        anchors { top: parent.top; bottom: toolBar.top }
    }

    Rectangle {
        id: toolBar
        width: parent.width; height: 80
        color: activePalette.window
        anchors.bottom: screen.bottom

        Button {
            id: newGameButton
            anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
            text: "New Game" 
            onClicked: Logic.startNewGame(gameCanvas)
        }

        Button {
            text: "Quit"
            anchors { left: newGameButton.right; leftMargin: 12; verticalCenter: parent.verticalCenter }
            onClicked: Qt.quit();
        }

        Text {
            id: score
            anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
            text: "Score: " + gameCanvas.score
            font.bold: true
            font.pixelSize: 24
            color: activePalette.windowText
        }
    }
}
