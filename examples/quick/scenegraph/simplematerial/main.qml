/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

//! [1]
import QtQuick 2.0
import SimpleMaterial 1.0

Rectangle {
    width: 640
    height: 360

    gradient: Gradient {
        GradientStop { position: 0; color: "#00ffff" }
        GradientStop { position: 1; color: "#00ff00" }
    }

//! [1] //! [2]
    SimpleMaterialItem {

        anchors.fill: parent
        SequentialAnimation on scale {
            NumberAnimation { to: 100; duration: 60000; easing.type: Easing.InCubic }
            NumberAnimation { to: 1; duration: 60000; easing.type: Easing.OutCubic }
            loops: Animation.Infinite
        }

        rotation: scale * 10 - 10
    }
//! [2] //! [3]
    Rectangle {
        color: Qt.rgba(0, 0, 0, 0.8)
        radius: 10
        border.width: 1
        border.color: "black"
        anchors.fill: label
        anchors.margins: -10
    }

    Text {
        id: label
        color: "white"
        wrapMode: Text.WordWrap
        text: "The background here is implemented as one QSGGeometryNode node which uses QSGSimpleMaterial to implement a mandlebrot fractal fill"
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }
}
//! [3]
