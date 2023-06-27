// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//![0]
// MyItem.qml
Item {
    function readValues(anArray, anObject) {
        for (var i=0; i<anArray.length; i++)
            console.log("Array item:", anArray[i])

        for (var prop in anObject) {
            console.log("Object item:", prop, "=", anObject[prop])
        }
    }
}
//![0]
