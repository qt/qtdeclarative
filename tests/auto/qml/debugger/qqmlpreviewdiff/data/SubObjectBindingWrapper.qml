// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Sets an external property binding on a sub-object of the form accessed via
// alias (target.button.interval). When SubObjectBindingFormOld's CU is rebuilt,
// the Timer child gets replaced. The binding must have its targetObject updated
// before being reinstalled on the new Timer, otherwise QQmlAnyBinding::installOn
// hits the assertion: targetObject() == target.object().

import QtQuick

SubObjectBindingOuterForm {
    id: wrapper
    property int multiplier: 5

    target.button.interval: wrapper.multiplier * 200
}
