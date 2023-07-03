// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
//![0]
Item {
    property int myNumber

    onMyNumberChanged: { console.log("myNumber has changed:", myNumber); }

    Component.onCompleted: myNumber = 100
}
//![0]
