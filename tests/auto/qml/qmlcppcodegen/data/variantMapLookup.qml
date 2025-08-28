pragma Strict
import TestTypes
import QtQuick

Item {
    property int i: moo.data.value
    property int j: moo.many[1].value

    VariantMapLookupFoo {
        id: moo
    }
}
