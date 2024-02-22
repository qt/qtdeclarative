// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

QtObject {
    id: root
    property int good
    property var i: Item {
       property int bad
       property int myP2: root.
       bad: 43
   }
}
