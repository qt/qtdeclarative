// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

.pragma library

var generatedValue = 5;

function generateNextValue() {
    generatedValue += 1;
    return generatedValue;
}
