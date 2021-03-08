import DuplicateImport // imports QtQml directly and indirectly via QtQuick

QtObject {
    default property QtObject child

    ItemDerived { // item derived has compatible QtObject type
        x: 4
    }
}
