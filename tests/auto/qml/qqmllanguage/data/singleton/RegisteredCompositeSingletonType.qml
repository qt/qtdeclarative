// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0
pragma Singleton

Item {
     id: singletonId

     property int testProp1: 925
     property int testProp2: 825
     property int testProp3: 755

     width: 25; height: 25

     Rectangle {
         id: rectangle
         border.color: "white"
         anchors.fill: parent
     }
}
