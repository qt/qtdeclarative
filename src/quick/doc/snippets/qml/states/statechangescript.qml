// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
//! [state and transition]
State {
    name: "state1"
    StateChangeScript {
        name: "myScript"
        script: doStateStuff();
    }
    // ...
}
// ...
Transition {
    to: "state1"
    SequentialAnimation {
        NumberAnimation { /* ... */ }
        ScriptAction { scriptName: "myScript" }
        NumberAnimation { /* ... */ }
    }
}
//! [state and transition]
}
