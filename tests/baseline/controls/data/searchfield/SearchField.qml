import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    spacing: 10
    height: 300

    // TO-DO: Add a test case for autoSuggest property
    // SearchField {
    //     autoSuggest: true
    // }

    SearchField {
        searchIndicator.indicator: null
    }

    SearchField {
        clearIndicator.indicator: null
        text: "Search"
    }

    SearchField {
        text: "Type to search"
    }

    SearchField {
        placeholderText: qsTr("Enter text")
    }

    SortFilterProxyModel {
        id: modelFilter
        sourceModel: ListModel {
            ListElement { color: "blue" }
            ListElement { color: "green" }
            ListElement { color: "red" }
            ListElement { color: "yellow" }
            ListElement { color: "orange" }
            ListElement { color: "purple" }
        }
        sorters: [
            RoleSorter {
                roleName: "color"
            }
        ]
        filters: [
            FunctionFilter {
                property var regExp: new RegExp(searchField.text, "i")
                onRegExpChanged: invalidate()
                function filter(color: string): bool {
                   return regExp.test(color);
                }
            }
        ]
    }

    SearchField {
        id: searchField
        suggestionModel: modelFilter
    }
}
