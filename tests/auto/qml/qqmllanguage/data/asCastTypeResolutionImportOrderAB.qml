import QtQml

import StaticTest as ST

import OtherModuleTest
import StaticTest

QtObject {
    property QtObject mo: ST.MyObject { }
    // Both StaticTest and OtherModuleTest provide MyObject
    property string s: (mo as MyObject).s()
}
