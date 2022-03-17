pragma Strict

import QtQml
import TestTypes as Test

QtObject {
    id: self
    property int state
    property QtObject a: Test.WindowInstance {}
    property QtObject b: a as Test.WindowInstance
    property QtObject c: self as Test.WindowInstance

    property QtObject d: Test.WindowDerived {}
    property QtObject e: d as Test.WindowDerived
    property QtObject f: self as Test.WindowDerived

    Component.onCompleted: {
        self.state = Test.WindowState.UNCALIBRATED
    }
}
