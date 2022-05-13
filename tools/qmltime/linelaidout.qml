// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QmlTime 1.0 as QmlTime

Item {

    QmlTime.Timer {
        component: Text {
            width: 480
            height: width
            text: "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42"
            color: "blue"
            font.pixelSize: 15
            font.bold: true

            onLineLaidOut: (line)=> {
                line.x = (line.number % 7) * 70
                line.y = Math.floor(line.number / 7.0) * 70
            }
        }
    }
}
