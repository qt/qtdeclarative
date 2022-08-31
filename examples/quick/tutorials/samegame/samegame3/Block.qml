// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {
    id: block

    property int type: 0

    Image {
        id: img

        anchors.fill: parent
        source: {
            if (type == 0)
                return "pics/redStone.png";
            else if (type == 1)
                return "pics/blueStone.png";
            else
                return "pics/greenStone.png";
        }
    }
}
//![0]
