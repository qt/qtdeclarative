import DuplicateImport // imports QtQml directly and indirectly via QtQuick

QtObjectWithDefaultProperty { // for default property
    ItemDerived { // item derived has compatible QtObject type
        x: 4
    }
}
