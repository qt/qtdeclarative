// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.1
import QtTest 1.1
import "data"

Item {
    id: top

    ListView {
        id: emptylist
        height: 20
        width: 50
    }

    ListView {
        id: singleElementList
        height: 20
        width: 50
        model: 1
        property real heightForDelegate: 100
        property real contentHeightOnDelegateResize
        delegate: Rectangle {
            height: singleElementList.heightForDelegate
            onHeightChanged: {
                singleElementList.forceLayout();
                singleElementList.contentHeightOnDelegateResize = singleElementList.contentHeight;
            }
        }
    }

    ListView {
        id: viewmanyitems
        model: manyitems
        delegate: Text { text: model.name }
    }

    ListView {
        id: modelchange
        width: 100
        height: 100
        model: firstmodel
        delegate: Text { text: model.name }
    }

    ListView {
        id: modelalter
        width: 100
        height: 100
        model: altermodel
        delegate: Text { text: model.name }
    }

    ListView {
        id: asyncLoaderCurrentIndexListView
        width: 360
        height: 360
        model: asyncLoaderCurrentIndexListModel

        currentIndex: 0

        delegate: Loader {
            width: asyncLoaderCurrentIndexListView.width
            height: asyncLoaderCurrentIndexListView.height

            source: component
            asynchronous: true
        }
    }

    ListView {
        id: asyncListViewLoaderView
        width: 360
        height: 360
        model: asyncListViewLoaderModel

        currentIndex: 0

        delegate: Loader {
            width: asyncListViewLoaderView.width
            height: asyncListViewLoaderView.height

            source: component
            asynchronous: true
        }
    }

    ListView {
        id: listViewDelegateModelAfterCreate
        anchors.fill: parent
        property int createdDelegates: 0
    }

    ListView
    {
        id: listInteractiveCurrentIndexEnforce
        width: 600
        height: 600

        snapMode: ListView.SnapOneItem
        orientation: ListView.Horizontal
        interactive: !currentItem.moving
        highlightRangeMode: ListView.StrictlyEnforceRange

        model: 4

        focus: true
        Keys.onPressed: if (event.key == Qt.Key_K) currentIndex = currentIndex + 1;

        delegate: Flickable {
            width: 600
            height: 600
            contentWidth: 600
            contentHeight: 1200

            MouseArea { anchors.fill: parent }
            Rectangle { anchors.fill: parent; color: index == 0 ? "red" : index == 1 ? "green" : index == 2 ? "blue" : "white" }
        }
    }

    ListView {
        id: viewWithActionModel
        property string funcResult
        model: ListModel { ListElement { friendlyText: "one"; action: function(text) { viewWithActionModel.funcResult = text } } }
        delegate: Item {
            Component.onCompleted: action(model.friendlyText)
        }
    }

    Component {
        id: delegateModelAfterCreateComponent
        Rectangle {
            width: 140
            height: 140
            border.color: "black"
            color: "red"
            Component.onCompleted: listViewDelegateModelAfterCreate.createdDelegates++;
        }
    }

    ListModel { id: emptymodel }
    ListModel { id: manyitems }
    ListModel { id: firstmodel; ListElement { name: "FirstModelElement0" } }
    ListModel { id: secondmodel; ListElement { name: "SecondModelElement0" } ListElement { name: "SecondModelElement1" } }
    ListModel { id: altermodel; ListElement { name: "AlterModelElement0" } ListElement { name: "AlterModelElement1" } }
    ListModel {
        id: asyncLoaderCurrentIndexListModel
        ListElement { component: "data/asyncloadercurrentindex.qml" }
        ListElement { component: "data/asyncloadercurrentindex.qml" }
        ListElement { component: "data/asyncloadercurrentindex.qml" }
        ListElement { component: "data/asyncloadercurrentindex.qml" }
        ListElement { component: "data/asyncloadercurrentindex.qml" }
        ListElement { component: "data/asyncloadercurrentindex.qml" }
    }
    ListModel {
        id: asyncListViewLoaderModel
        ListElement { component: "data/asynclistviewloader.qml" }
        ListElement { component: "data/asynclistviewloader.qml" }
        ListElement { component: "data/asynclistviewloader.qml" }
        ListElement { component: "data/asynclistviewloader.qml" }
        ListElement { component: "data/asynclistviewloader.qml" }
    }

    MultiDelegate {
        id: multiDelegate
    }

    MultiDelegate2 {
        id: multiDelegate2
    }

    MultiDelegate3 {
        id: multiDelegate3
    }

    TestCase {
        name: "ListView"
        when: windowShown

        function test_empty() {
            compare(emptylist.count, 0)
            emptylist.model = emptymodel;
            compare(emptylist.count, 0)
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
            tryCompare(viewmanyitems, 'count', row.numitems)

        }

        function test_modelchange() {
            tryCompare(modelchange, 'count', 1)
            modelchange.currentIndex = 0;
            compare(modelchange.currentItem.text, "FirstModelElement0")
            modelchange.model = secondmodel;
            tryCompare(modelchange, 'count', 2)
            modelchange.currentIndex = 0;
            compare(modelchange.currentItem.text, "SecondModelElement0")
            modelchange.currentIndex = 1;
            compare(modelchange.currentItem.text, "SecondModelElement1")
        }

        function test_modelaltered() {
            tryCompare(modelalter, 'count', 2)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            altermodel.append({"name":"AlterModelElement2"})
            tryCompare(modelalter, 'count', 3)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            modelalter.currentIndex = 2;
            compare(modelalter.currentItem.text, "AlterModelElement2")
            altermodel.insert(2,{"name":"AlterModelElement1.5"})
            tryCompare(modelalter, 'count', 4)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            modelalter.currentIndex = 2;
            compare(modelalter.currentItem.text, "AlterModelElement1.5")
            modelalter.currentIndex = 3;
            compare(modelalter.currentItem.text, "AlterModelElement2")
            altermodel.move(2,1,1);
            tryCompare(modelalter, 'count', 4)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1.5")
            modelalter.currentIndex = 2;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            modelalter.currentIndex = 3;
            compare(modelalter.currentItem.text, "AlterModelElement2")
            altermodel.remove(1,2)
            tryCompare(modelalter, 'count', 2)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement2")
            altermodel.set(1,{"name":"AlterModelElement1"})
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            altermodel.clear()
            modelalter.forceLayout()
            tryCompare(modelalter, 'count', 0)
            compare(modelalter.currentItem, null)
        }

        function test_asyncLoaderCurrentIndexChange() {
            skip("more details in QTBUG-53780")
            for (var i = 0; i < 500; i++) {
                asyncLoaderCurrentIndexListView.currentIndex = 0;
                asyncLoaderCurrentIndexListView.currentIndex = 1;
                asyncLoaderCurrentIndexListView.currentIndex = 2;
                asyncLoaderCurrentIndexListView.currentIndex = 3;
                asyncLoaderCurrentIndexListView.currentIndex = 4;
                asyncLoaderCurrentIndexListView.currentIndex = 5;
            }
            wait(1000)
        }

        function test_asyncListViewLoader() {
            skip("more details in QTBUG-53780")
            for (var i = 0; i < 50; i++) {
                wait(10);
                asyncListViewLoaderView.currentIndex = 0;
                asyncListViewLoaderView.currentIndex = 1;
                asyncListViewLoaderView.currentIndex = 2;
                asyncListViewLoaderView.currentIndex = 3;
                asyncListViewLoaderView.currentIndex = 4;
            }
        }

        function test_set_delegate_model_after_list_creation() {
            listViewDelegateModelAfterCreate.delegate = delegateModelAfterCreateComponent;
            listViewDelegateModelAfterCreate.model = 40;
            verify(listViewDelegateModelAfterCreate.createdDelegates > 0);
        }

        function test_listInteractiveCurrentIndexEnforce() {
            mousePress(listInteractiveCurrentIndexEnforce, 10, 50);
            wait(1); // because Flickable pays attention to velocity, we need some time between movements
            mouseMove(listInteractiveCurrentIndexEnforce, 10, 40);
            wait(1);
            mouseMove(listInteractiveCurrentIndexEnforce, 10, 30);
            wait(1);
            mouseMove(listInteractiveCurrentIndexEnforce, 10, 20);
            wait(1);
            mouseMove(listInteractiveCurrentIndexEnforce, 10, 10);
            compare(listInteractiveCurrentIndexEnforce.interactive, false);
            mouseRelease(listInteractiveCurrentIndexEnforce, 10, 10);
            tryCompare(listInteractiveCurrentIndexEnforce, "interactive", true);
            keyClick("k");
            compare(listInteractiveCurrentIndexEnforce.currentIndex, 1);
            tryCompare(listInteractiveCurrentIndexEnforce, "contentX", listInteractiveCurrentIndexEnforce.width);
        }

        function test_forceLayoutForContentHeight() {
            singleElementList.heightForDelegate = 200;
            compare(singleElementList.contentHeightOnDelegateResize, 200);
        }

        function test_viewWithAction() {
            compare(viewWithActionModel.funcResult, "one")
        }

        function test_multipleDelegates_data() {
            return [
                { y: 25, type: "Rectangle", value: "red" },
                { y: 75, type: "Image", value: "logo.png" },
                { y: 125, type: "Text", value: "Hello" },
                { y: 175, type: "Text", value: "World" },
                { y: 225, type: "Rectangle", value: "green" },
                { y: 275, type: "Image", value: "logo.png" },
                { y: 325, type: "Rectangle", value: "blue" },
                { y: 375, type: "Item", value: "" }
            ]
        }

        function test_multipleDelegates(row) {
            var delegate = multiDelegate.itemAt(10, row.y)
            verify(delegate.toString().includes(row.type))
            switch (row.type) {
                case "Rectangle": verify(Qt.colorEqual(delegate.color, row.value)); break
                case "Text": compare(delegate.text, row.value); break
                case "Image": compare(delegate.source, row.value); break
                case "Item": break
            }
        }

        function test_multipleDelegates2_data() {
            return [
                { y: 25, type: "1" },
                { y: 75, type: "2" },
                { y: 125, type: "3" },
                { y: 175, type: "4" },
                { y: 225, type: "4" },
                { y: 275, type: "3" },
                { y: 325, type: "4" },
                { y: 375, type: "4" }
            ]
        }

        function test_multipleDelegates2(row) {
            var delegate = multiDelegate2.itemAt(10, row.y)
            compare(delegate.choiceType, row.type)
        }

        function test_multipleDelegates3_data() {
            return [
                { y: 25, type: "Rectangle", property: "color", value: "#ff0000" },
                { y: 75, type: "Text", property: "text", value: "Hello world" }
            ]
        }

        function test_multipleDelegates3(row) {
            var delegate = multiDelegate3.itemAt(10, row.y)
            verify(delegate.toString().includes(row.type))
            compare(delegate[row.property], row.value)
        }
    }
}
