/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0


Item {
    id: root
    width: 360
    height: 360
    property var widthFactor: 7
    // used in the multilanguage translator
    property var screenId: "yi"

    Column {
        Text {
            text: qsTr("hello")
            width: root.width / widthFactor
            elide: Text.ElideRight
        }
        Text {
            text: qsTr("short")
            width: root.width / widthFactor
            elide: Text.ElideRight
        }
        Text {
            text: "long not translated text"
            width: root.width / widthFactor
            elide: Text.ElideRight
        }
    }
    // this is necessary to have the test working for different font sizes and dpi settings
    Text {
        id: originHelloTextToGetTheNecessaryWidth
        text: "short"
        opacity: 0
        anchors.bottom: root.bottom
        onWidthChanged: root.width = originHelloTextToGetTheNecessaryWidth.width * widthFactor
    }
}
