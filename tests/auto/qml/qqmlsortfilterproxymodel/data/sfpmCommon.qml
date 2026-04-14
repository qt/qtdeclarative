import QtQml
import "Utility.js" as Sfpmutility

QtObject {
   id: sfpmTestObject

   function getValue(value) {
      return Sfpmutility.getInteger(value)
   }

   component SorterRoleData: QtObject { property string display }

   // Filters
   property ValueFilter valueFilter: ValueFilter {}
   property FunctionFilter functionFilter0: FunctionFilter {
      property string expression: ""
      property bool useRegularExpression: false
      function filter(column0: string) : bool {
         if (useRegularExpression) {
            var regex = new RegExp(expression)
            return regex.test(column0)
         }
         return column0 === expression
      }
   }
   property FunctionFilter functionFilter1: FunctionFilter {
      property string expression: ""
      property bool useRegularExpression: false
      function filter(column1: string) : bool {
         if (useRegularExpression) {
            var regex = new RegExp(expression)
            return regex.test(column1)
         }
         return column1 === expression
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
