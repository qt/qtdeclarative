// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.5
import QtQuick.Layouts 1.2

Item {
    objectName: "qtbug51927-window"
    visible: true

    default property alias _contents: customContent.data

    ColumnLayout {
        id: customContent
        objectName: "qtbug51927-columnLayout"
        anchors.fill: parent
    }
}
