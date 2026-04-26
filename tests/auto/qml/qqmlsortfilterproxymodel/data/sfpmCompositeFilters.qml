import QtQml

QtObject {
    id: sfpmTestCompositeObject

    property ListModel colorList: ListModel {
        ListElement { name: "red1";   color: "red";   active: true  }
        ListElement { name: "red2";   color: "red";   active: false }
        ListElement { name: "blue1";  color: "blue";  active: true  }
        ListElement { name: "blue2";  color: "blue";  active: false }
        ListElement { name: "green1"; color: "green"; active: true  }
        ListElement { name: "green2"; color: "green"; active: false }
    }

    // AnyOfFilter: color = "red" OR color = "blue" (4 rows)
    property SortFilterProxyModel redOrBlueModel: SortFilterProxyModel {
        sourceModel: sfpmTestCompositeObject.colorList
        filters: [
            AnyOfFilter {
                ValueFilter { roleName: "color"; value: "red"  }
                ValueFilter { roleName: "color"; value: "blue" }
            }
        ]
    }

    // AllOfFilter: color = "red" AND active = true (1 row)
    property SortFilterProxyModel activeRedModel: SortFilterProxyModel {
        sourceModel: sfpmTestCompositeObject.colorList
        filters: [
            AllOfFilter {
                ValueFilter { roleName: "color";  value: "red"  }
                ValueFilter { roleName: "active"; value: true   }
            }
        ]
    }

    // AllOf { AnyOf { red, blue }, active = true } (2 rows)
    property SortFilterProxyModel activeRedOrBlueModel: SortFilterProxyModel {
        sourceModel: sfpmTestCompositeObject.colorList
        filters: [
            AllOfFilter {
                AnyOfFilter {
                    ValueFilter { roleName: "color"; value: "red"  }
                    ValueFilter { roleName: "color"; value: "blue" }
                }
                ValueFilter { roleName: "active"; value: true }
            }
        ]
    }

    // AllOf { AnyOf (disabled) { red, blue }, active = true } (3 rows)
    property SortFilterProxyModel activeDisabledRedOrBlueModel: SortFilterProxyModel {
        sourceModel: sfpmTestCompositeObject.colorList
        filters: [
            AllOfFilter {
                AnyOfFilter {
                    enabled: false
                    ValueFilter { roleName: "color"; value: "red"  }
                    ValueFilter { roleName: "color"; value: "blue" }
                }
                ValueFilter { roleName: "active"; value: true }
            }
        ]
    }

    // AllOf { AllOf (inverted) { red, active = true }, active = false } (3 rows)
    property SortFilterProxyModel invertedAllOfRedAndBlueModel: SortFilterProxyModel {
        sourceModel: sfpmTestCompositeObject.colorList
        filters: [
            AllOfFilter {
                AllOfFilter {
                    inverted: true
                    ValueFilter { roleName: "color"; value: "red"  }
                    ValueFilter { roleName: "active"; value: true }
                }
                ValueFilter { roleName: "active"; value: false }
            }
        ]
    }

    // AllOf (inverted) { AnyOf (inverted) { red, blue }, active = false } (5 rows)
    property SortFilterProxyModel invertedAnyOfAllOfRedOrBlueModel: SortFilterProxyModel {
        sourceModel: sfpmTestCompositeObject.colorList
        filters: [
            AllOfFilter {
                inverted: true
                AnyOfFilter {
                    inverted: true
                    ValueFilter { roleName: "color"; value: "red"  }
                    ValueFilter { roleName: "color"; value: "blue" }
                }
                ValueFilter { roleName: "active"; value: false }
            }
        ]
    }

    // AnyOf { AnyOf { }, active = true} (3 rows)
    property SortFilterProxyModel nestedEmptyAnyOf: SortFilterProxyModel {
        sourceModel: sfpmTestCompositeObject.colorList
        filters: [
            AnyOfFilter {
                AnyOfFilter { } // Shouldnt filter any data here
                ValueFilter { roleName: "active"; value: true }
            }
        ]
    }

    property ValueFilter anyOfChild1: ValueFilter {}
    property ValueFilter anyOfChild2: ValueFilter {}
    property AnyOfFilter anyOfFilter: AnyOfFilter {}

    property ValueFilter allOfChild1: ValueFilter {}
    property ValueFilter allOfChild2: ValueFilter {}
    property AllOfFilter allOfFilter: AllOfFilter {}

    // Nested: AllOf containing AnyOf
    property ValueFilter nestedAnyOfChild1: ValueFilter {}
    property ValueFilter nestedAnyOfChild2: ValueFilter {}
    property ValueFilter nestedAllOfChild: ValueFilter {}
    property AnyOfFilter nestedAnyOf: AnyOfFilter {}
    property AllOfFilter nestedAllOf: AllOfFilter {}

    property SortFilterProxyModel sfpmProxyModel: SortFilterProxyModel {}
}
