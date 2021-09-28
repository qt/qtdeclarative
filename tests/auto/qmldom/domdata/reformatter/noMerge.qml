import QtQuick

Item{
  property var arrayNoMerge
  property var array2: [1,2,3]
  property list<Item> array: [Item {}, Item{ nr: 2 }]
  default property list<Item> nonMergeable
  property list<Item> singleObjList
  singleObjList: Item{
      el: "alone"
  }
  nonMergeable: [Item {}, Item{ nr: "II" }]
  arrayNoMerge: [Item {}, Item{ nr: 2 }]
}
