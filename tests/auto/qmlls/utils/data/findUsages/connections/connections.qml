// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
Item {

    MouseArea {
        id: area1
        onClicked: foo
        property int insideMouseArea1
    }

    MouseArea {
        id: area2
        Connections {
            function onClicked(mouse) {
                area1.clicked()
                area3.clicked()
            }
        }
        property int insideMouseArea2

        MouseArea {id: area3}
    }

    property Connections c: Connections {
        target: area3
        onClicked: function(mouse) {
             //
        }
    }
    function useMouseAreas() {
        area1.clicked()
        area2.clicked()
        area3.clicked()
    }
}
