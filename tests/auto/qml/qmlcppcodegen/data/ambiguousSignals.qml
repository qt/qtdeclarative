pragma Strict
import TestTypes

Person {
    objectName: "tomorrow"
    onAmbiguous: function(a) { objectName = a + "foo" }
}
