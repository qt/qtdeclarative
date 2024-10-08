// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQml
import QtCore
import QtQuick

Item {
    component ParentSettings: Settings {
        category: "Parent"
        property string overriddenProperty: "parentOriginal"
    }

    component ChildSettings: ParentSettings {
        category: "Child"
        overriddenProperty: "childOriginal"
    }

    ParentSettings {
        id: parentSettings
    }

    ChildSettings {
        id: childSettings
    }

    Component.onCompleted: {
        parentSettings.overriddenProperty = "parentChanged"
        childSettings.overriddenProperty = "childChanged"
    }
}
