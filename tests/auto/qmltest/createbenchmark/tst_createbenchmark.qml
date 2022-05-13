// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtTest 1.1

TestCase {
    id: top
    name: "CreateBenchmark"

    function benchmark_create_component() {
        var component = Qt.createComponent("item.qml")
        var obj = component.createObject(top)
        //obj.destroy(100)
        component.destroy()
    }
}
