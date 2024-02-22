// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    width: 360
    height: 360
    Component.onCompleted:  {
        console.log("console.log")
        console.count("console.count");
    }
}
