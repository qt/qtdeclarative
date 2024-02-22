// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "TextAreas"

    Row {
        spacing: container.rowSpacing

        TextArea {
            id: defaultTextArea
            width: 200
            wrapMode: TextEdit.WordWrap
            selectByMouse: true
            text: "Default - Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                  + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
        }

        TextArea {
            enabled: false
            width: 200
            wrapMode: TextEdit.WordWrap
            selectByMouse: true
            text: "Disabled - Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                  + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
        }

        ScrollView {
            id: scrollView
            width: 200
            height: defaultTextArea.height

            TextArea {
                text: "Inside ScrollView - Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                + "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi "
                + "ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit "
                + "in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur "
                + "sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
                + "mollit anim id est laborum."
                wrapMode: TextEdit.WordWrap
                selectByMouse: true
            }
        }
    }

    Row {
        spacing: container.rowSpacing

        Frame {
            id: frame
            contentWidth: textArea.width
            contentHeight: textArea.height

            TextArea {
                id: textArea
                width: 200
                height: 80
                wrapMode: TextEdit.WrapAnywhere
                selectByMouse: true
                text: "Inside frame - Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                      + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
            }
        }

        Frame {
            contentWidth: 200
            contentHeight: 100
            ScrollView {
                id: scrollView2
                anchors.fill: parent

                TextArea {
                    id: area2
                    text: "Inside Frame and ScrollView - Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                          + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                          + "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi "
                          + "ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit "
                          + "in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur "
                          + "sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
                          + "mollit anim id est laborum."
                    wrapMode: TextEdit.WordWrap
                    selectByMouse: true
                }
            }
        }

        TextArea {
            placeholderText: "Placeholder text"
            selectByMouse: true
        }
    }
}
