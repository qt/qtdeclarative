// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick 2.0

// Mirrors LogoAnimation: a VME QObject property (`payload`, like
// `property ParticleSystem particleSystem`) read by Repeater-delegated children.
// When this composite's VME meta-object is rebuilt in place, `container.payload`
// is transiently unavailable, so the delegate bindings read it as null.
Item {
    id: container
    width: 50
    height: 50

    property QtObject payload: QtObject { property int size: 8 }

    Column {
        Repeater {
            model: 2
            Item {
                width: 24
                height: 24
                property int sz: container.payload.size
            }
        }
    }
}
