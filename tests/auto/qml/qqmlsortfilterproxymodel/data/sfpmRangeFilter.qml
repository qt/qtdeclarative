import QtQml

QtObject {
    property ListModel productModel: ListModel {
        ListElement { name: "Budget";   price: 10 }
        ListElement { name: "Basic";    price: 20 }
        ListElement { name: "Standard"; price: 30 }
        ListElement { name: "Premium";  price: 40 }
        ListElement { name: "Luxury";   price: 50 }
    }

    property RangeFilter rangeFilter: RangeFilter {
        roleName: "price"
    }

    property SortFilterProxyModel sfpmProxyModel: SortFilterProxyModel {
        sourceModel: productModel
        filters: [ rangeFilter ]
    }

    // minimum=20 inclusive — prices 20, 30, 40, 50
    property SortFilterProxyModel minimumInclusiveModel: SortFilterProxyModel {
        sourceModel: productModel
        filters: RangeFilter {
            roleName: "price"
            minimum: 20
        }
    }

    // maximum=30 inclusive — prices 10, 20, 30
    property SortFilterProxyModel maximumInclusiveModel: SortFilterProxyModel {
        sourceModel: productModel
        filters: RangeFilter {
            roleName: "price"
            maximum: 30
        }
    }

    // minimum=20, maximum=40, both inclusive — prices 20, 30, 40
    property SortFilterProxyModel bothInclusiveModel: SortFilterProxyModel {
        sourceModel: productModel
        filters: RangeFilter {
            roleName: "price"
            minimum: 20
            maximum: 40
        }
    }

    // minimum=exclusive(20), maximum=40 — prices 30, 40
    property SortFilterProxyModel exclusiveMinimumModel: SortFilterProxyModel {
        sourceModel: productModel
        filters: RangeFilter {
            roleName: "price"
            minimum: exclusive(20)
            maximum: 40
        }
    }

    // minimum=20, maximum=exclusive(40) — prices 20, 30
    property SortFilterProxyModel exclusiveMaximumModel: SortFilterProxyModel {
        sourceModel: productModel
        filters: RangeFilter {
            roleName: "price"
            minimum: 20
            maximum: exclusive(40)
        }
    }

    // minimum=exclusive(20), maximum=exclusive(40) — price 30 only
    property SortFilterProxyModel bothExclusiveModel: SortFilterProxyModel {
        sourceModel: productModel
        filters: RangeFilter {
            roleName: "price"
            minimum: exclusive(20)
            maximum: exclusive(40)
        }
    }
}
