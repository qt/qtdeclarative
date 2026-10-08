import QtQml
import TestTypes

DetachedReferences {
    id: self

    property Component c: QtObject { objectName: "collectable" }

    // We inject JavaScript-owned objects into the containers stored in C++.
    // This is usually a recipe for disaster.
    map: ({ hello: "world", self: self, collectable: c.createObject() })
    list: [map, c.createObject()]

    // But then we produce detached versions of the same containers, which cause the collectables
    // to be marked, so that they are not actually collected.
    property var markedMap: map

    // 6.8 has this quirk of not actually detaching the list if we just assign it here.
    // Therefore, assign it piecemeal.
    property list<var> markedList: [list[0], list[1]]
}
