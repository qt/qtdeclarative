// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.9
import QtQuick.Layouts 1.3

Item {
    objectName: "qtbug51927-container"
    visible: true

    default property alias _contents: customContent.data

    ColumnLayout {
        id: customContent
        objectName: "qtbug51927-columnLayout"
        anchors.fill: parent
    }
}
