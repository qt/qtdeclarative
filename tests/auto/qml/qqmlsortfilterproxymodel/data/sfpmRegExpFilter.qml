import QtQml

QtObject {
    property ListModel fruitModel: ListModel {
        ListElement { label: "apple"     }
        ListElement { label: "Apricot"   }
        ListElement { label: "banana"    }
        ListElement { label: "blueberry" }
        ListElement { label: "Cherry"    }
        ListElement { label: "cranberry" }
    }

    property ListModel unicodeFruitModel: ListModel {
        ListElement { label: "apple"     }
        ListElement { label: "Apricot"   }
        ListElement { label: "banana"    }
        ListElement { label: "blueberry" }
        ListElement { label: "blåbær"    }
        ListElement { label: "Bringebær" }
        ListElement { label: "Cherry"    }
        ListElement { label: "cranberry" }
        ListElement { label: "jordbær"   }
        ListElement { label: "pære"      }
    }

    // Standalone filter used by C++ tests via property access
    property RegExpFilter regExpFilter: RegExpFilter {
        roleName: "label"
    }

    property SortFilterProxyModel sfpmProxyModel: SortFilterProxyModel {
        sourceModel: fruitModel
        filters: regExpFilter
    }

    property SortFilterProxyModel sfpmUnicodeProxyModel: SortFilterProxyModel {
        sourceModel: unicodeFruitModel
        filters: regExpFilter
    }

    // Maatches with berrys: (blueberry, cranberry)
    property SortFilterProxyModel berryModel: SortFilterProxyModel {
        sourceModel: fruitModel
        filters: RegExpFilter {
            roleName: "label"
            regExp: /berry/i
        }
    }

    // Starts with b: (banana, blueberry)
    property SortFilterProxyModel startsWithBModel: SortFilterProxyModel {
        sourceModel: fruitModel
        filters: RegExpFilter {
            roleName: "label"
            regExp: /^b/
        }
    }

    // Or: (apple, banana)
    property SortFilterProxyModel regExpWithOrModel: SortFilterProxyModel {
        sourceModel: fruitModel
        filters: RegExpFilter {
            roleName: "label"
            regExp: /apple|banana/
        }
    }

    // Match all the fruits
    property SortFilterProxyModel unicodeAllWordsModel: SortFilterProxyModel {
        sourceModel: unicodeFruitModel
        filters: RegExpFilter {
            roleName: "label"
            regExp: /^\w+$/
        }
    }

    // Maatch specific word (pære)
    property SortFilterProxyModel unicodeFourCharModel: SortFilterProxyModel {
        sourceModel: unicodeFruitModel
        filters: RegExpFilter {
            roleName: "label"
            regExp: /^\w{4}$/
        }
    }

    // Exact match
    property SortFilterProxyModel unicodeExactModel: SortFilterProxyModel {
        sourceModel: unicodeFruitModel
        filters: RegExpFilter {
            roleName: "label"
            regExp: /^blåbær$/
        }
    }

    // Ends with bær: (blåbær, Bringebær, jordbær)
    property SortFilterProxyModel unicodeEndsWithModel: SortFilterProxyModel {
        sourceModel: unicodeFruitModel
        filters: RegExpFilter {
            roleName: "label"
            regExp: /bær$/
        }
    }

    // Contains å case insensitive: (blåbær)
    property SortFilterProxyModel unicodeContainsModel: SortFilterProxyModel {
        sourceModel: unicodeFruitModel
        filters: RegExpFilter {
            roleName: "label"
            regExp: /å/i
        }
    }
}
