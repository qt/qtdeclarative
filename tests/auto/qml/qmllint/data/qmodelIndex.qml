import QtQml

QtObject {
  property ItemSelectionModel itemSelectionModel;
  function row() { return itemSelectionModel.currentIndex.row; }
}
