// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import Backend

ListView {
    id: listView

    signal pressAndHold(int index)

    width: 320
    height: 480

    focus: true
    boundsBehavior: Flickable.StopAtBounds

    section.property: "fullName"
    section.criteria: ViewSection.FirstCharacter
    section.delegate: SectionDelegate {
        width: listView.width
    }

    delegate: ContactDelegate {
        id: delegate
        width: listView.width
        onPressAndHold: listView.pressAndHold(index)
    }

    model: ContactModel {
        id: contactModel
    }

    ScrollBar.vertical: ScrollBar { }
}
