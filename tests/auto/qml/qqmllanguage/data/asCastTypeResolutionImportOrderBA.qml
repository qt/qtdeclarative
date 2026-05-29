import QtQml

import StaticTest as ST

import StaticTest
import OtherModuleTest

QtObject {
    property QtObject mo: ST.MyObject { }
    // Both StaticTest and OtherModuleTest provide MyObject
    property string s: (mo as MyObject).s()
}
