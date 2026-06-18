// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.HeaderViewDelegate {
    id: control

    // same as AbstractButton.qml
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 8

    highlighted: selected

    readonly property var view: control.headerView.syncView
    readonly property bool indicatorVisible: control.headerView.showSortIndicator
                                             && view
                                             && model
                                             && view.sortColumn === model.index

    contentItem: IconLabel {
        spacing: 5

        mirrored: control.mirrored
        display: control.indicatorVisible ? IconLabel.TextBesideIcon : IconLabel.TextOnly

        icon.width: 16
        icon.height: 16
        icon.color: control.palette.text
        icon.source: control.indicatorVisible
                     ? control.view.sortOrder === Qt.AscendingOrder
                       ? "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/sort-ascending.png"
                       : "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/sort-descending.png"
                     : ""
        defaultIconColor: control.palette.text

        text: model[control.headerView.textRole]
        color: control.palette.windowText
        font: control.font
    }

    background: Rectangle {
        border.color: Qt.styleHints.accessibility.contrastPreference === Qt.HighContrast ?
                      control.palette.windowText : control.palette.midlight
        color: control.palette.light
    }
}
