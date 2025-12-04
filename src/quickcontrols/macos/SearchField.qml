// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultSearchField {
    id: control

    readonly property bool __nativeSearchIndicator: searchIndicator.indicator instanceof NativeStyle.SearchField
    readonly property bool __nativeClearIndicator: clearIndicator.indicator instanceof NativeStyle.SearchField
    readonly property Item __focusFrameTarget: control

    contentItem: T.TextField {
        text: control.text

        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: Text.AlignVCenter

        readonly property Item __focusFrameControl: control
        readonly property bool __ignoreNotCustomizable: true
    }

    NativeStyle.SearchField {
        id: search
        control: control
        subControl: NativeStyle.SearchField.Search
        x: searchIndicator.indicator ? searchIndicator.indicator.x : 0
        y: searchIndicator.indicator ? searchIndicator.indicator.y : 0
        useNinePatchImage: false
    }

    searchIndicator.indicator: Item {
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: search.width
        implicitHeight: search.height

        readonly property bool __ignoreNotCustomizable: true
    }

    NativeStyle.SearchField {
        id: clear
        visible: control.text.length > 0
        control: control
        subControl: NativeStyle.SearchField.Clear
        x: clearIndicator.indicator ? clearIndicator.indicator.x : 0
        y: clearIndicator.indicator ? clearIndicator.indicator.y : 0
        useNinePatchImage: false
    }

    clearIndicator.indicator: Item {
        x: control.width - width - 5
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: clear.width
        implicitHeight: clear.height

        readonly property bool __ignoreNotCustomizable: true
    }
}
