// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [contentItem]
Tumbler {
    id: tumbler

    contentItem: PathView {
        id: pathView
        model: tumbler.model
        delegate: tumbler.delegate
        clip: true
        pathItemCount: tumbler.visibleItemCount + 1
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        dragMargin: width / 2

        path: Path {
            startX: pathView.width / 2
            startY: -pathView.delegateHeight / 2
            PathLine {
                x: pathView.width / 2
                y: pathView.pathItemCount * pathView.delegateHeight - pathView.delegateHeight / 2
            }
        }

        property real delegateHeight: tumbler.availableHeight / tumbler.visibleItemCount
    }
}
//! [contentItem]
