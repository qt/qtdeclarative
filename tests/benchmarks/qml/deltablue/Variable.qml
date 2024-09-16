// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
import QtQml

QtObject {
    property int value: 0
    property list<BaseConstraint> constraints
    property BaseConstraint determinedBy: null
    property int mark: 0
    property int walkStrength: Strength.WEAKEST
    property bool stay: true

    function addConstraint(c: BaseConstraint) {
        let l = constraints;
        l[l.length++] = c;
    }

    function removeConstraint(c: BaseConstraint) {
        let l = constraints;
        let index = 0;
        let skipped = 0;
        for (let i = 0, length = l.length; i < length; ++i) {
            let value = l[i] as BaseConstraint;
            if (value !== c) {
                if (index != i)
                    l[index] = value;
                ++index
            } else {
                ++skipped;
            }
        }

        l.length -= skipped;

        if (determinedBy === c)
            determinedBy = null;
    }

    function length(): int {
        return constraints.length
    }

    function constraint(i: int) : BaseConstraint {
        return constraints[i];
    }
}
