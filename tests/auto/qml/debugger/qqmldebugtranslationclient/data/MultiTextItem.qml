// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

