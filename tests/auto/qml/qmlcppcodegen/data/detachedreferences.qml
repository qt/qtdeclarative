import QtQml
import TestTypes

DetachedReferences {
    id: self

    property Component c: QtObject { objectName: "collectable" }

    // We inject JavaScript-owned objects into the containers stored in C++.
    // This is usually a recipe for disaster.
    map: ({ hello: "world", self: self, collectable: c.createObject(), hash: hash })
    hash: ({ world: "hello", self: self, collectable: c.createObject()})

    // But then we produce detached versions of the same containers, which cause the collectables
    // to be marked, so that they are not actually collected.
    property var markedMap: map
    property var markedHash: hash
}
