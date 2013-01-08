/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.1

Item {
    height: label.implicitHeight * 2
    width: Math.max(label.implicitWidth * 1.2, height * 2.5);
    anchors.verticalCenter: parent.verticalCenter
    property alias text: label.text
    property string tooltip
    signal clicked
    SystemPalette { id: palette }
    Rectangle {
        antialiasing: true
        border.color: mouseArea.pressed ? palette.highlight : palette.light
        color: "transparent"
        anchors.fill: parent
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        radius: 3
    }
    Rectangle {
        border.color: palette.dark
        anchors.fill: parent
        anchors.leftMargin: 1
        anchors.topMargin: 1
        radius: 3
    }
    Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: mouseArea.pressed ? palette.dark : palette.light }
            GradientStop { position: 0.2; color: palette.button }
            GradientStop { position: 0.8; color: palette.button }
            GradientStop { position: 1.0; color: mouseArea.pressed ? palette.light : palette.dark }
        }
        anchors.fill: parent
        anchors.margins: 1
        radius: 3
    }
    Text {
        id: label
        anchors.centerIn: parent
        color: palette.buttonText
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}
