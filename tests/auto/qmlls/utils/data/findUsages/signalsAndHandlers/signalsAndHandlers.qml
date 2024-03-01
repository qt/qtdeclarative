// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: rootId
    signal helloSignal()

    function callSignals() {
        helloSignal()
        if (false) {
            helloSignal()
        } else {
        // helloSignal() // not a usage btw
            if (true)
                helloSignal()
        }
    }
    function callSignals2() {
        helloSignal()
        if (false) {
            widthChanged()
        } else {
        // helloSignal() // not a usage btw
            if (true)
                widthChanged()
            rootId.widthChanged()
        }
    }
    Item {
        function callSignalsInChild() {
            widthChanged()
            rootId.widthChanged()
        }
    }

    function myHelloHandler() { let x = 32; }
    onHelloSignal: myHelloHandler
}
