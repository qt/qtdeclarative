// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
import QtQml

QtObject {
    property list<Constraint> v

    function clear() {
        v.length = 0;
    }

    function addConstraint(c: Constraint) {
        let constraints = v;
        constraints[constraints.length++] = c;
    }

    function execute() {
        let constraints = v;
        for (let i = 0, length = constraints.length; i < length; ++i) {
            let c = constraints[i];
            if (c.input)
                c.output.value = c.evaluate();
        }
    }
}
