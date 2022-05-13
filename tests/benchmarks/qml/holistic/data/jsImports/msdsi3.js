// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// This JavaScript file is a single, small, imported script.
// It imports other small scripts which are deeply nested.
// It is imported by another script.

.import "msdsi4.js" as Msdsi4

function testFunc(seedValue) {
    var retn = 0.35;
    retn *= Msdsi4.testFunc(seedValue + retn);
    return retn;
}
