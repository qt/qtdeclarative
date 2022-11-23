pragma Strict
import TestTypes
import QtQml

QtObject {
    objectName: f1.location + " and " + f2.location
    property Foreign1 f1: Foreign1 { location: "not here" }
    property Foreign2 f2: Foreign2 { location: "not there" }

}
