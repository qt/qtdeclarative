// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//![0]
Text {
    text: applicationData.getCurrentDateTime()

    Connections {
        target: applicationData
        onDataChanged: console.log("The application data changed!")
    }
}
//![0]

