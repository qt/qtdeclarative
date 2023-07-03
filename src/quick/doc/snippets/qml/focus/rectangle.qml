// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//! [simple key event]
Rectangle {
    width: 100; height: 100
    focus: true
    Keys.onPressed: (event)=> {
        if (event.key == Qt.Key_A) {
            console.log('Key A was pressed');
            event.accepted = true;
        }
    }
//! [simple key event]

//! [active focus]
    Text {
        text: activeFocus ? "I have active focus!" : "I do not have active focus"
    }
//! [active focus]

//! [simple key event end]
}
//! [simple key event end]
