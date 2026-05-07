import QtQml
import "Utility.js" as Sfpmutility

QtObject {
   id: sfpmTestObject

   function getValue(value) {
      return Sfpmutility.getInteger(value)
   }

   component SorterRoleData: QtObject { property string display }
   component FilterRoleData0: QtObject { property string column0 }
   component FilterRoleData1: QtObject { property string column1 }

   // Filters
   property ValueFilter valueFilter: ValueFilter {}
   property FunctionFilter functionFilter0: FunctionFilter {
      property string expression: ""
      function filter(data: FilterRoleData0) : bool {
         return eval(expression)
      }
   }
   property FunctionFilter functionFilter1: FunctionFilter {
      property string expression: ""
      function filter(data: FilterRoleData1) : bool {
         return eval(expression)
      }
   }

   // Sorters
   property RoleSorter roleSorter: RoleSorter {}
   property StringSorter stringSorter: StringSorter {}
   property FunctionSorter functionSorter: FunctionSorter {
      property string expression: ""
      function compare(lhsData: SorterRoleData, rhsData: SorterRoleData) : int {
         return eval(expression)
      }
   }

   property SortFilterProxyModel sfpmProxyModel: SortFilterProxyModel {}
}
