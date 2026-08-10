// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import MyModule

QtObject {
    property var myItem: MyItem { id: child }

    property int good
    property var i: Item {
       property int bad
       property int myP: child.
       bad: 43
   }
}
