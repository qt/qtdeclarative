// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QmltcQProcessTests

Item {
    Component {
        id: mycomp

        Item {
            // This introduces an inner required property
            // without a binding that cannot be set later and should
            // thus block the compilation.
            TypeWithRequiredProperty {}
        }
    }
}
