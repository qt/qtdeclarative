// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Item {
    // Don't show the item when the StackView that contains us
    // is being popped off the stack, as we use an x animation
    // and hence would show pages that we shouldn't since we
    // also don't have our own background.
    visible: SwipeView.isCurrentItem || (SwipeView.view.contentItem.moving && (SwipeView.isPreviousItem || SwipeView.isNextItem))
}
