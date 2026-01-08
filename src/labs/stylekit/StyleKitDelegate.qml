// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import Qt.labs.StyleKit

StyleKitDelegateBase {
    // These properties are bound here in QML, rather than in StyleKitDelegateBase
    // (C++), so that a custom delegate that embeds a StyleKitDelegate as a child
    // (to create overlay or underlay effects) can override them to handle how and
    // where to apply the transform.
    scale: delegateProperties.scale
    rotation: delegateProperties.rotation
}
