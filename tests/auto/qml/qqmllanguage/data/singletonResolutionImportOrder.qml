import StaticTest
import OtherModuleTest

import QtQml

QtObject {
    // Both StaticTest and OtherModuleTest provide YepSingleton
    property string s: YepSingleton.s()
}
