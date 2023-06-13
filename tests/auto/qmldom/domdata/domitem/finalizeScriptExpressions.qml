// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

Item {
    property var binding
    binding: 42

    property var bindingInPropertyDefinition: 123

    function return42(aa: Item = 33, bb: string = "Hello", cc = binding): int {
        return 42
    }
    function empty(aa: Item, bb: string, cc) {}
    function full(aa: Item, bb: string, cc)
    {
        const x = 42;
        const formula = (x + 5) * 2 + 10 - x
        return formula;
    }

    id: idBinding

}
