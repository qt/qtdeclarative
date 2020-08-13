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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

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

        Flickable {
            width: 200
            height: defaultTextArea.height
            clip: true

            TextArea.flickable: TextArea {
                text: "Inside flickable - Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                + "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi "
                + "ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit "
                + "in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur "
                + "sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
                + "mollit anim id est laborum."
                wrapMode: TextEdit.WordWrap
                selectByMouse: true
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
        }
    }

    Row {
        spacing: container.rowSpacing

        TextArea {
            width: 200
            wrapMode: TextEdit.WordWrap
            selectByMouse: true
            text: "Small - Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                  + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
            property bool qqc2_style_small
        }

        TextArea {
            width: 200
            wrapMode: TextEdit.WordWrap
            selectByMouse: true
            text: "Mini - Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                  + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
            property bool qqc2_style_mini
        }

        TextArea {
            placeholderText: "Placeholder text"
            selectByMouse: true
        }
    }
}
