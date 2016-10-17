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

import QtQuick 2.7
import QtQuick.Controls 2.0

//! [1]
ListView {
    width: 200
    height: 300
    clip: true
    model: ListModel {
        id: listModel
        ListElement { title: "Electricity bill" }
        ListElement { title: "Happy Birthday!" }
        ListElement { title: "FW: Cat pictures" }
        ListElement { title: "Hotel visit receipt" }
        ListElement { title: "Customer service" }
    }
    delegate: SwipeDelegate {
        id: swipeDelegate
        text: title
        width: parent.width

        onClicked: if (swipe.complete) listModel.remove(index)

        swipe.right: Rectangle {
            color: swipeDelegate.swipe.complete && swipeDelegate.pressed ? "#333" : "#444"
            width: parent.width
            height: parent.height

            Label {
                font.pixelSize: swipeDelegate.font.pixelSize
                text: qsTr("Remove")
                color: "white"
                anchors.centerIn: parent
            }
        }
    }
}
//! [1]
