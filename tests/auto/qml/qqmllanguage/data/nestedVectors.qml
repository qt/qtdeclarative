import Test
import QtQml

NestedVectors {
    id: self

    property var list1

    Component.onCompleted: {
        list1 = self.getList()

        let list2 = []
        let data1 = []
        data1.push(2)
        data1.push(3)
        data1.push(4)

        let data2 = []
        data2.push(5)
        data2.push(6)

        list2.push(data1)
        list2.push(data2)

        self.setList(list2)
    }
}
