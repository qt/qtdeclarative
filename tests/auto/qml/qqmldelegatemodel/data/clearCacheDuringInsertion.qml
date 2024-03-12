// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

Window {
    id: root
    width: 640
    height: 480
    visible: true
    color: "#111111"

    Column {
        spacing: 1
        Repeater {
            model: 1000

            Rectangle {
                width: 100
                height: 100
                color: "grey"
                DelegateModel {
                    id: delegateModel
                    delegate: Rectangle {
                        width: 100
                        height: 20
                        color: "black"
                        Text {
                            anchors.centerIn: parent
                            text: "Name: " + model.name
                            color: "white"
                        }
                    }

                    property int length: 0
                    property var filterAcceptsItem: function(item) { return true; }

                    model: ListModel {
                        id: myModel

                        ListElement {
                            name: "tomato"
                            classifications: [
                                ListElement { classification: "fruit" },
                                ListElement { classification: "veg" }
                            ]
                            nutritionFacts: [
                                ListElement {
                                    calories:  "45"
                                }
                            ]
                        }
                    
                        ListElement {
                            name: "apple"
                            classifications: [
                                ListElement { classification: "fruit" }
                            ]
                            nutritionFacts: [
                                ListElement {
                                    calories:  "87"
                                }
                            ]
                        }

                        ListElement {
                            name: "broccoli"
                            classifications: [
                                ListElement { classification: "veg" }
                            ]
                            nutritionFacts: [
                                ListElement {
                                    calories:  "12"
                                }
                            ]
                        }

                        ListElement {
                            name: "squash"
                            classifications: [
                                ListElement { classification: "veg" }
                            ]
                            nutritionFacts: [
                                ListElement {
                                    calories:  "112"
                                }
                            ]
                        }
                    }

                    groups: [
                        DelegateModelGroup { id: visibleItems; name: "visible" },
                        DelegateModelGroup { name: "veg" },
                        DelegateModelGroup { name: "fruit"; includeByDefault: true }
                    ]

                    function update() {
                        
                        // Step 1: Filter items
                        var visible = [];
                        for (var i = 0; i < items.count; ++i) {
                            var item = items.get(i);
                            if (filterAcceptsItem(item.model)) {
                                visible.push(item);
                            }
                        }

                        // Step 2: Add all items to the visible group:
                        for (i = 0; i < visible.length; ++i) {
                            items.insert(visible[i], delegateModel.filterOnGroup)
                        }
                        delegateModel.length = visible.length
                    }

                    items.onChanged: update()
                    onFilterAcceptsItemChanged: update()

                    filterOnGroup: "visible"
                    Component.onCompleted: {
                        for(var i = 0; i < myModel.count; i++) {
                            var temp = 0;
                            var entry = myModel.get(i);

                            for (var j = 0; j < entry.classifications.count; j++) {
                                temp = entry.classifications.get(j)
                                items.insert(entry, temp.classification)
                            }
                        }
                    }
                }
            }
        }
    }
}
