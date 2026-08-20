import QtQml
import QtQml.Models

QtObject {
    property ListModel listModel: ListModel { id: lm }

    property SortFilterProxyModel proxy: SortFilterProxyModel {
        sourceModel: lm
        sorters: FunctionSorter {
            function compare(left : int, right : int) : int{
                return left.value - right.value   // deterministic, correct
            }
        }
    }
    Component.onCompleted: {
        lm.append({ value: 1 })
        lm.append({ value: 2 })   // second row -> first real comparison -> crash
        console.log("[reproducer] survived -- proxy.rowCount:", proxy.rowCount())
    }
}

