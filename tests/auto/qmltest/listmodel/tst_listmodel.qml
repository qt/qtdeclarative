// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtTest 1.1

Item {
    id: top
    ListModel { id: emptymodel }
    ListModel { id: manyitems }
    ListModel { id: insertmodel }
    ListModel { id: move; ListElement { name: "Element0" } ListElement { name: "Element1" } }
    ListModel { id: firstmodel; ListElement { name: "FirstModelElement0" } }
    ListModel { id: secondmodel; ListElement { name: "SecondModelElement0" } ListElement { name: "SecondModelElement1" } }
    ListModel { id: altermodel; ListElement { name: "AlterModelElement0" } ListElement { name: "AlterModelElement1" } }

    property string funcResult
    ListModel {
        id: funcModel
        property string modelProp
        ListElement { friendlyText: "one"; action: function(obj) { funcResult = obj.friendlyText } }
        ListElement { friendlyText: "two"; action: function() {} }
        ListElement { friendlyText: "three"; action: function() { modelProp = "fail" } }
        ListElement { friendlyText: "four"; action: function() { funcResult = friendlyText } }
    }

    TestCase {
        name: "ListModel"

        function test_empty() {
            compare(emptymodel.count, 0)
            emptymodel.clear();
            compare(emptymodel.count, 0)
        }

        function test_multipleitems_data() {
            return [
                {
                    tag: "10items",
                    numitems: 10
                },
                {
                    tag: "100items",
                    numitems: 100
                },
                {
                    tag: "10000items",
                    numitems: 10000
                }
            ]
        }

        function test_multipleitems(row) {
            var i;
            manyitems.clear();
            compare(manyitems.count, 0)
            for (i = 0; i < row.numitems; ++i) {
                manyitems.append({"name":"Item"+i})
            }
            compare(manyitems.count, row.numitems)
        }

        function test_insert() {
            insertmodel.insert(0, {"name": "Element0"})
            compare(insertmodel.get(0).name, "Element0")
            insertmodel.insert(1, {"name": "Element1"})
            compare(insertmodel.get(1).name, "Element1")
        }

        function test_altermodeled() {
            tryCompare(altermodel, 'count', 2)
            compare(altermodel.get(0).name, "AlterModelElement0")
            compare(altermodel.get(1).name, "AlterModelElement1")
            altermodel.append({"name":"AlterModelElement2"})
            tryCompare(altermodel, 'count', 3)
            compare(altermodel.get(0).name, "AlterModelElement0")
            compare(altermodel.get(1).name, "AlterModelElement1")
            compare(altermodel.get(2).name, "AlterModelElement2")
            altermodel.insert(2,{"name":"AlterModelElement1.5"})
            tryCompare(altermodel, 'count', 4)
            compare(altermodel.get(0).name, "AlterModelElement0")
            compare(altermodel.get(1).name, "AlterModelElement1")
            compare(altermodel.get(2).name, "AlterModelElement1.5")
            compare(altermodel.get(3).name, "AlterModelElement2")
            tryCompare(altermodel, 'count', 4)
            altermodel.move(2,1,1);
            compare(altermodel.get(0).name, "AlterModelElement0")
            compare(altermodel.get(1).name, "AlterModelElement1.5")
            compare(altermodel.get(2).name, "AlterModelElement1")
            compare(altermodel.get(3).name, "AlterModelElement2")
            altermodel.remove(1,2)
            tryCompare(altermodel, 'count', 2)
            compare(altermodel.get(0).name, "AlterModelElement0")
            compare(altermodel.get(1).name, "AlterModelElement2")
            altermodel.set(1,{"name":"AlterModelElement1"})
            compare(altermodel.get(0).name, "AlterModelElement0")
            compare(altermodel.get(1).name, "AlterModelElement1")
            altermodel.setProperty(0, "name", "AlteredProperty")
            compare(altermodel.get(0).name, "AlteredProperty")
            altermodel.clear()
            tryCompare(altermodel, 'count', 0)
            compare(altermodel.get(0), undefined)
        }

        function test_functions() {
            // test different ways of calling
            funcModel.get(0).action(funcModel.get(0))
            compare(funcResult, "one")
            funcModel.get(0).action.call(this, { friendlyText: "seven" })
            compare(funcResult, "seven")

            // test different ways of setting
            funcResult = ""
            funcModel.get(1).action()
            compare(funcResult, "")

            funcModel.set(1, { friendlyText: "two", action: function() { funcResult = "set" } })
            funcModel.get(1).action()
            compare(funcResult, "set")

            funcModel.setProperty(1, "action", function() { top.funcResult = "setProperty" })
            funcModel.get(1).action()
            compare(funcResult, "setProperty")

            funcModel.get(1).action = function() { funcResult = "jsSet" }
            funcModel.get(1).action()
            compare(funcResult, "jsSet")

            // test unsupported features
            var didThrow = false
            try {
                funcModel.get(2).action()
            } catch(ex) {
                verify(ex.toString().includes("Error: Invalid write to global property"))
                didThrow = true
            }
            verify(didThrow)

            didThrow = false
            try {
                funcModel.get(3).action()
            } catch(ex) {
                verify(ex.toString().includes("ReferenceError: friendlyText is not defined"))
                didThrow = true
            }
            verify(didThrow)


        }
    }
}
