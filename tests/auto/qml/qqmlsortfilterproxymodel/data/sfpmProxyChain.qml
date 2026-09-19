import QtQml

QtObject {
   // The inner proxy deliberately has no source model.
   property SortFilterProxyModel innerProxyModel: SortFilterProxyModel {}

   property SortFilterProxyModel outerProxyModel: SortFilterProxyModel {
      sourceModel: innerProxyModel
      sorters: RoleSorter { roleName: "display" }
   }
}
