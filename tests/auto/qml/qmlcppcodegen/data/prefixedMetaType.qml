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

    property Test.WindowInstance g: Test.WindowInstance {}
    property Test.WindowDerived h: Test.WindowDerived {}

    property int countG: self.g.count
    property int countH: self.h.count

    Component.onCompleted: {
        self.state = Test.WindowState.UNCALIBRATED
    }
}
