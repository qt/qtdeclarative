/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

ListView {
    id: controlviewitem
    property string controlname: ""
    property variant controlvalue
    signal reset
    height: 40; width: 200; highlightRangeMode: ListView.StrictlyEnforceRange
    orientation: ListView.Horizontal; snapMode: ListView.SnapOneItem
    preferredHighlightBegin: 50; preferredHighlightEnd: 150;
    delegate: Rectangle { height: 40; width: 100; border.color: "black"
        Text { anchors.centerIn: parent; width: parent.width; text: model.name; elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter } }
    header: headfoot
    footer: headfoot
    Component {
        id: headfoot
        Rectangle {
            MouseArea { id: resetbutton; anchors.fill: parent; onClicked: { reset(); } }
            color: "lightgray"; height: 40; width: 50; border.color: "gray"
            Text { id: headertext; anchors.centerIn: parent; wrapMode: Text.WrapAnywhere
                rotation: -40; text: controlviewitem.controlname; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
        }
    }
}