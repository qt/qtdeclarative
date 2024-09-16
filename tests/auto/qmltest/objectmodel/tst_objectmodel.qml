// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml 2.1
import QtQml.Models 2.3
import QtTest 1.1

TestCase {
    name: "ObjectModel"

    ObjectModel {
        id: model
        QtObject { id: static0 }
        QtObject { id: static1 }
        QtObject { id: static2 }
    }

    Component { id: object; QtObject { } }

    function test_attached_index() {
        compare(model.count, 3)
        compare(static0.ObjectModel.index, 0)
        compare(static1.ObjectModel.index, 1)
        compare(static2.ObjectModel.index, 2)

        var dynamic0 = object.createObject(model, {objectName: "dynamic0"})
        compare(dynamic0.ObjectModel.index, -1)
        model.append(dynamic0) // -> [static0, static1, static2, dynamic0]
        compare(model.count, 4)
        for (var i = 0; i < model.count; ++i)
            compare(model.get(i).ObjectModel.index, i)

        var dynamic1 = object.createObject(model, {objectName: "dynamic1"})
        compare(dynamic1.ObjectModel.index, -1)
        model.insert(0, dynamic1) // -> [dynamic1, static0, static1, static2, dynamic0]
        compare(model.count, 5)
        for (i = 0; i < model.count; ++i)
            compare(model.get(i).ObjectModel.index, i)

        model.move(1, 3) // -> [dynamic1, static1, static2, static0, dynamic0]
        compare(model.count, 5)
        for (i = 0; i < model.count; ++i)
            compare(model.get(i).ObjectModel.index, i)

        model.move(4, 0) // -> [dynamic0, dynamic1, static1, static2, static0]
        compare(model.count, 5)
        for (i = 0; i < model.count; ++i)
            compare(model.get(i).ObjectModel.index, i)

        model.remove(1) // -> [dynamic0, static1, static2, static0]
        compare(model.count, 4)
        compare(dynamic1.ObjectModel.index, -1)
        for (i = 0; i < model.count; ++i)
            compare(model.get(i).ObjectModel.index, i)

        model.clear()
        compare(static0.ObjectModel.index, -1)
        compare(static1.ObjectModel.index, -1)
        compare(static2.ObjectModel.index, -1)
        compare(dynamic0.ObjectModel.index, -1)
        compare(dynamic1.ObjectModel.index, -1)
    }
}
