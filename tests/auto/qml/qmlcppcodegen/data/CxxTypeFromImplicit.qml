pragma Strict
import QtQml

QtObject {
    objectName: p.name
    property Person p: Person {
        id: p
        name: "guenther"
    }
}
