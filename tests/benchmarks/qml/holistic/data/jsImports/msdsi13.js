// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// This JavaScript file is a single, small, imported script.
// It imports other small scripts which are deeply nested.
// It is imported by another script.

.import "msdsi14.js" as Msdsi14

function testFunc(seedValue) {
    var retn = 0.135;
    retn *= Msdsi14.testFunc(seedValue + retn);
    return retn;
}
