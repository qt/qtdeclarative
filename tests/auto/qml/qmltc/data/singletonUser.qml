import QtQml
import QmltcTests 1.0

QtObject {
    property int number: SingletonThing.integerProperty
    property string message: SingletonThing.stringProperty
}
