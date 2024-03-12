// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2

Item
{
    id: root

    width: 200
    height: 200

    signal enterFinalStage

    property bool finalStageComplete: false;

    /* What comes below is some convenience for running the .qml files s
     * standalone using qmlscene. This can be quite handy when debugging
     * issues.
     */

    onFinalStageCompleteChanged: {
        if (typeof suite == 'undefined') {
            print("-> final stage complete");
        }
    }

    Component.onCompleted: {
        if (typeof suite == 'undefined') {
            print("-> not running in testsuite, now in initial state")
            suiteFaker.running = true;
        }
    }

    Timer {
        id: suiteFaker
        running: false;
        interval: 1000
        repeat: false;
        onTriggered: {
            print("-> entering final stage")
            enterFinalStage();
        }
    }

}
