// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Qt.labs.qmlmodels

Item {
    id: root
    width: 200
    height: 200

    property alias testModel: treeModel
    property alias treeView: treeView

    function appendSubTree() {
        testModel.appendRow({
                            checked: false,
                            amount: 4,
                            fruitType: "Peach",
                            fruitName: "Princess Peach",
                            fruitPrice: 1.45,
                            color: "yellow",
                            rows: [
                                {
                                    checked: true,
                                    amount: 5,
                                    fruitType: "Strawberry",
                                    fruitName: "Perry the Berry",
                                    fruitPrice: 3.80,
                                    color: "red",
                                },
                                {
                                    checked: false,
                                    amount: 6,
                                    fruitType:"Pear",
                                    fruitName: "Bear Pear",
                                    fruitPrice: 1.50,
                                    color: "green",
                                }
                            ]
                        })
    }

    function appendInvalid() {
        testModel.appendRow({
                            checked: true,
                            amount: 1,
                            fruitType: "Pear",
                            fruitName: [],
                            fruitPrice: 1.50,
                            color: "green"
                            })
    }

    function appendWithExtraData() {
        testModel.appendRow({
                            checked: true,
                            amount: 1,
                            fruitType: "Pear",
                            fruitName: "Williams",
                            fruitPrice: 1.50,
                            color: "green",
                            extradata: "extradata"
                            })
    }

    function setRows() {
        testModel.rows = [
                            {
                                checked: true,
                                amount: 5,
                                fruitType: "Strawberry",
                                fruitName: "Perry the Berry",
                                fruitPrice: 3.80,
                                color: "red",
                            },
                            {
                                checked: false,
                                amount: 6,
                                fruitType:"Pear",
                                fruitName: "Bear Pear",
                                fruitPrice: 1.50,
                                color: "green",
                            }
                        ]
    }

    function setInvalidRowsNumber() {
        testModel.rows = 42   // should fail - type is int
    }

    function setInvalidRowsString() {
        testModel.rows = "Hello world"    // should fail - type is QString
    }

    function setInvalidRowsObject() {
        testModel.rows = ({ foo: 1 })   // should fail - type is JSObject
    }

    TreeModel {
        id: treeModel
        objectName: "testModel"

        TableModelColumn { display: "checked"; decoration: "color" }
        TableModelColumn { display: "amount"; decoration: "color" }
        TableModelColumn { display: "fruitType"; decoration: "color" }
        TableModelColumn { display: "fruitName"; decoration: "color" }
        TableModelColumn { display: "fruitPrice"; decoration: "color" }
    }

    TreeView {
        id: treeView
        anchors.fill: parent
        model: testModel
        delegate: Text {
            text: model.display
        }
    }
}
