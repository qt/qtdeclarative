/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:FDL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Free Documentation License Usage
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of
** this file. Please review the following information to ensure
** the GNU Free Documentation License version 1.3 requirements
** will be met: http://www.gnu.org/copyleft/fdl.html.
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.6
import Qt.labs.controls 1.0

ListView {
    id: listView
    width: 100
    height: 120

    model: ListModel {
        ListElement { name: "Apple" }
        ListElement { name: "Orange" }
        ListElement { name: "Pear" }
    }

    delegate: SwipeDelegate {
        id: rootDelegate
        width: listView.width
        text: modelData

        ListView.onRemove: SequentialAnimation {
            PropertyAction {
                target: rootDelegate
                property: "ListView.delayRemove"
                value: true
            }
            NumberAnimation {
                target: rootDelegate
                property: "height"
                to: 0
                easing.type: Easing.InOutQuad
            }
            PropertyAction {
                target: rootDelegate
                property: "ListView.delayRemove"
                value: false
            }
        }

        onClicked: if (exposure.active) ListView.view.model.remove(index)

        Component {
            id: removeComponent

            Rectangle {
                color: rootDelegate.exposed && rootDelegate.pressed ? "#333" : "#444"
                width: parent.width
                height: parent.height

                Label {
                    text: "Remove"
                    color: "white"
                    anchors.centerIn: parent
                }
            }
        }

        exposure.left: removeComponent
        exposure.right: removeComponent
    }
}
