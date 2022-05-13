// Copyright (C) 2021 The Qt Company Ltd.

import QtQuick
import TestTypes

Item {
    Behavior on width { PropertyAnimation {} }

    CppBaseClass {
        id: withBehavior
        Behavior on cppProp { PropertyAnimation {} }
        Component.onCompleted: cppProp = 200
    }
    height: withBehavior.cppProp

    CppBaseClass {
        id: withoutBehavior
        Component.onCompleted: cppProp = 100
    }
    x: withoutBehavior.cppProp

    Component.onCompleted: {
        width = 200
        y = 100
    }

    CppBaseClass {
        id: qPropertyBinder
        cppProp: withoutBehavior.cppProp + withBehavior.cppProp
    }

    property int qProperty1: qPropertyBinder.cppProp
    property int qProperty2: withoutBehavior.cppProp + withBehavior.cppProp
}
