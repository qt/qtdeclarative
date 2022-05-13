// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QmlTime 1.0 as QmlTime

Item {

    QmlTime.Timer {
        component: Grid {
            width: 480
            height: width

            id: grid
            columns: 7
            rows: 6

            Repeater {
                model: 42
                Text {
                    width: grid.width / grid.columns; height: width
                    color: "blue"
                    text: index
                    font.pixelSize: 15
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                }
            }

        }
    }
}

