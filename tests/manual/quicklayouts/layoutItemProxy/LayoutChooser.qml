// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls

Item {
    id: __layoutChooser
    property var chosenLayout: undefined
    property list<bool> criteria: []
    property list<var> layoutChoices: []

    implicitWidth: chosenLayout.implicitWidth
    implicitHeight: chosenLayout.implicitHeight

    onCriteriaChanged: {
        console.log("Criterias:", criteria)
        showAndHide()
    }

    onLayoutChoicesChanged: {
        console.log("Layouts:", layoutChoices)
        showAndHide()
    }

    function showAndHide() {
        const oldLayout = chosenLayout

        let i = 0
        for (; i < criteria.length; i++) {
            if (criteria[i])
                break
        }

        console.log("Choosing layout", i)

        if (i < layoutChoices.length)
            chosenLayout = layoutChoices[i]
        else if (layoutChoices.length > 0)
            chosenLayout = layoutChoices[0]
        else
            return

        for (i = 0; i < layoutChoices.length; i++) {
            layoutChoices[i].visible = false
        }
        chosenLayout.visible = true
        chosenLayout.ensurePolished()
    }
}
