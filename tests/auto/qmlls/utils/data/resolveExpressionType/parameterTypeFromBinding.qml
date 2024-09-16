// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import resolveExpressionType.CppTypes

Item {
    // invalid: property changed signals have no parameters!
    onColorChanged: (invalid) => console.log(invalid.r)

    WithSignal {
        onHelloSignal: (parameter, invalid) => console.log(parameter.helloData, invalid.hello)
    }
    WithSignal {
        onHelloSignal: function (parameter) {
            let xxx = 42;
            console.log(xxx)
        }
    }
    WithSignal {
        onHelloSignal: function (parameter) {
            console.log(parameter.helloData)
        }
    }
    WithFakePropertyChangedSignal {
        onMySomeTypeChanged: (someType) => console.log(someType.x)
    }
}
