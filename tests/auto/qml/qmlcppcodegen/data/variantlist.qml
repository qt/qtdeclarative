pragma Strict
import TestTypes
import QtQml

QtObject {
    property Person person: Person {}
    property var things: person.things
}
