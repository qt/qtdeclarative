// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import Qt.labs.StyleKit

StyledItemBase {
    // These properties are bound here in QML, rather than in StyledItemBase
    // (C++), so that a custom delegate that embeds a StyledItemBase as a child
    // (to create overlay or underlay effects) can override them to handle how and
    // where to apply the transform.
    scale: delegateStyle.scale
    rotation: delegateStyle.rotation
    visible: delegateStyle.visible
}
