pragma Strict
import QtQml
import "."

QtObject {
    objectName: p.name
    property Person p: Person {
        id: p
        name: "horst"
    }
}
