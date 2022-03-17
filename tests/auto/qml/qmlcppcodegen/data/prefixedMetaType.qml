pragma Strict

import QtQml
import TestTypes as Test

QtObject {
    id: self
    property int state
    Component.onCompleted: {
        self.state = Test.WindowState.UNCALIBRATED
    }
}
