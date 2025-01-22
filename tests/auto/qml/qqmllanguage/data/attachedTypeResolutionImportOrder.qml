import StaticTest
import OtherModuleTest

import QtQml

QtObject {
    // Both StaticTest and OtherModuleTest provide Yep
    property string s: Yep.s()
}
