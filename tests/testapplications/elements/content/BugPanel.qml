// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    property string urltext
    urltext: "<a href=\"" + bugreports + bugnumber + "\">QTBUG-" + bugnumber + "</a>"
    property string bugnumber: ""
    property string bugreports: "http://bugreports.qt.io/browse/QTBUG-"
    visible: opacity != 0
    opacity: bugnumber == "" ? 0 : 1
    Behavior on opacity { NumberAnimation { duration: 1500 } }
    height: buglist.paintedHeight; width: 200; radius: 5; border.color: "lightgray"
    anchors { bottom: parent.bottom; left: parent.left; leftMargin: 15; bottomMargin: 15 }
    Text { id: buglist; text: urltext; textFormat: Text.RichText; visible: bugnumber != ""
        anchors.centerIn: parent; onLinkActivated: { Qt.openUrlExternally(link); }
    }
}
