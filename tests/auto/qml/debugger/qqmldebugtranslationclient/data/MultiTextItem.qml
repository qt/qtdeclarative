/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick Designer Components.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import QtQuick.Layouts 1.3

StackLayout {
    id: root
    width: childrenRect.width
    height: childrenRect.height


    property int maxIndex: {
        var ret = 0
        for (var i = 0; i < root.data.length; i++)
        {
            if (root.data[i].text !== undefined)
             ret++
        }

        return ret

    }

    property int stringIndex: 0

    onStringIndexChanged: {
       setupText()
    }

    Component.onCompleted: setupText()

    function setupText() {
        var textArray = []

        for (var i = 0; i < root.data.length; i++)
        {
            if (root.data[i].text !== undefined)
                 textArray.push(root.data[i].text)
        }

    }

    property string textModel: {
        var textArray = ""

        for (var i = 0; i < root.data.length; i++)
        {
            if (root.data[i].text !== undefined) {
                 if (textArray === "")
                     textArray = textArray + root.data[i].text
                 else
                     textArray = textArray +  'e\u001f' + 'e\u001d' + root.data[i].text
            }
        }

        return textArray
    }

    property string testString: {

        var textArray = ""

        for (var i = 0; i < root.data.length; i++)
        {
            if (root.data[i].text !== undefined)
                textArray = textArray + (root.data[i].text)
        }

        return textArray
    }




}

