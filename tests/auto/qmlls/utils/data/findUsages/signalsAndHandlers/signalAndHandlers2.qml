// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    function myHelloHandler() { let x = 32; }
    onHelloSignal: myHelloHandler

    property int helloPropertyBinding
    helloPropertyBinding: 123

    property int checkHandlers
    onCheckHandlersChanged: myHelloHandler
    onChildrenChanged: myHelloHandler
    function callChanged() {
        checkHandlersChanged()
        childrenChanged()
    }
    property int _: 48
    property int ______42: 48
    property int _123a: 48
    on_Changed: myHelloHandler
    on______42Changed: myHelloHandler
    on_123AChanged: myHelloHandler
    function weirdPropertynames() {
        _Changed()
        ______42Changed()
        _123aChanged()
    }

    TapHandler {
        onTapped: myHelloHandler
        function f() {
            tapped()
        }
    }

    function anotherF() {
        helloPropertyChanged()
    }
    onHelloPropertyChanged: myHelloHandler
}
