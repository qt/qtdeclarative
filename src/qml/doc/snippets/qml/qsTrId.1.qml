// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
//![0]
    Text {
        //% "hello"
        text: qsTrId("hello_id")
    }
//![0]

//![1]
    Text {
        /*% "hello" */
        text: qsTrId("hello_id")
    }
//![1]
}
