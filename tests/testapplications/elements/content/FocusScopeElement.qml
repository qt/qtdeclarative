// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: focusscopeelementtest
    anchors.fill: parent
    property string testtext: ""

    FocusScope {
        id: firstfocusscopeelement
    }
    FocusScope {
        id: secondfocusscopeelement
    }

    BugPanel { id: bugpanel }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: focusscopeelementtest
                testtext: "This test uses a FocusScope element. There should be two boxes, "+
                "the first showing a red border to represent it having focus.\n"+
                "Next, let's press a key to see which has focus." }
        }
    ]

}
