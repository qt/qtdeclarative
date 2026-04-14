// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import Qt.labs.qmlmodels

ApplicationWindow {
    width: 700
    height: 500
    visible: true
    title: "SearchField RTL test"

    LayoutMirroring.enabled: rtlSwitch.checked
    LayoutMirroring.childrenInherit: true

    ListModel {
        id: rtlModel
        ListElement { text: "مرحبا" }
        ListElement { text: "שלום" }
        ListElement { text: "مرحبا 123" }
        ListElement { text: "abc مرحبا" }
        ListElement { text: "test שלום" }
        ListElement { text: "بحث" }
        ListElement { text: "موز" }
        ListElement { text: "تفاح" }
        ListElement { text: "ظطذد" }
        ListElement { text: "apple" }
        ListElement { text: "Berlin" }
    }

    SortFilterProxyModel {
        id: rtlFilter
        model: rtlModel
        sorters: RoleSorter { roleName: "text" }

        filters: FunctionFilter {
            property var regExp: new RegExp(searchField.text, "i")
            onRegExpChanged: invalidate()

            function filter(text: string): bool {
                return regExp.test(text)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        Frame {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                RowLayout {
                    spacing: 12

                    Switch {
                        id: rtlSwitch
                        text: checked ? "UI: RTL" : "UI: LTR"
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "This switch changes the application's visual direction (LTR/RTL). Use it to compare the alignment of the SearchField input and the suggestion popup"
                        wrapMode: Text.Wrap
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: "rtlSwitch.checked = " + rtlSwitch.checked
                              + " | SearchField.mirrored = " + searchField.mirrored
                }
            }
        }

        Frame {
            Layout.fillWidth: true

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: "Expected behavior:\n"
                      + "- When the switch is off, the UI is treated as LTR, so the popup follows an LTR layout.\n"
                      + "- The text in the input field may still align according to the natural direction of the typed text.\n"
                      + "- When the switch is on, the UI is treated as RTL, so the popup follows an RTL layout.\n"
            }
        }

        SearchField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: rtlSwitch.checked ? "ابحث هنا" : "Search here"
            suggestionModel: rtlFilter
            textRole: "text"
        }
    }
}
