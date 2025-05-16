// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import HelperWidgets
import QtQuick.Layouts

Column {
    width: parent.width

    CheckSection {
        width: parent.width
        caption: qsTr("CheckDelegate")
    }

    ItemDelegateSection {
        width: parent.width
    }

    AbstractButtonSection {
        width: parent.width
    }

    ControlSection {
        width: parent.width
    }

    FontSection {
        width: parent.width
    }

    PaddingSection {
        width: parent.width
    }
}
