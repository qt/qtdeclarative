// Copyright (C) 2015 Canonical Limited and/or its subsidiary(-ies)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "ImageResponseProviderCore"

Column {
    Image { source: "image://async/slow" }
    Image { source: "image://async/fast" }
}

