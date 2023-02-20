pragma Strict
import TestTypes
import QtQuick

Item {
    property int i: moo.data.value

    VariantMapLookupFoo {
        id: moo
    }
}
