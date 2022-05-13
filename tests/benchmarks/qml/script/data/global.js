// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

var incVar = 1;
var total;

function doSomething() {
    for (var i = 0; i < 10000; ++i)
        Math.sin(90);
}

function doIncrement() {
    total = 0;
    for (var i = 0; i < 100000; ++i)
        total += incVar;
}
