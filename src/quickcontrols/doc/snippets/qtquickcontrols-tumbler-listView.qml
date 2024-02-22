// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [contentItem]
Tumbler {
    id: tumbler

    contentItem: ListView {
        model: tumbler.model
        delegate: tumbler.delegate

        snapMode: ListView.SnapToItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        preferredHighlightBegin: height / 2 - (height / tumbler.visibleItemCount / 2)
        preferredHighlightEnd: height / 2 + (height / tumbler.visibleItemCount / 2)
        clip: true
    }
}
//! [contentItem]
