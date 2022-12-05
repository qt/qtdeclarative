// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

Item {
    width: 100
    height: 100

    //! [1]
    Flickable {
        anchors.fill: parent

        contentWidth: parent.width * 2
        contentHeight: parent.height * 2

        ScrollIndicator.horizontal: ScrollIndicator { id: hbar; active: vbar.active }
        ScrollIndicator.vertical: ScrollIndicator { id: vbar; active: hbar.active }
    }
    //! [1]
}
