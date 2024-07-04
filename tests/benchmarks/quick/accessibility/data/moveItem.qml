// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Item {
    id: root
    width: 400
    height: 400
    Item {
        Item {
            Item {
                Item {
                    Item {
                        Rectangle {
                            objectName: "Cancel"
                            color: "green"
                            width: 100
                            height: 50
                            border.width: 2
                            radius: 5
                            Text {
                                id: txt
                                anchors.centerIn: parent
                                text: parent.objectName
                            }
                        }
                    }
                }
            }
        }
    }
}
