/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
