// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import QtQuick.Controls.Fusion

QtObject {
    function torture() {
        Fusion.highlight(null);
        Fusion.highlightedText(null);
        Fusion.outline(null);
        Fusion.highlightedOutline(null);
        Fusion.tabFrameColor(null);
        Fusion.buttonColor(null);
        Fusion.buttonOutline(null);
        Fusion.grooveColor(null);
    }
}
