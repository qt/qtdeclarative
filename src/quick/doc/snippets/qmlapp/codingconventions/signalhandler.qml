// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
MouseArea {
    onClicked: event => { console.log(`${event.x},${event.y}`); }
}
//![0]
