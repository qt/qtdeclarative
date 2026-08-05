// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import test

// Overrides the default from classBegin(), so the property changes while it is being executed.
ReentrantDeferredTester {
    objectProperty: null
}
