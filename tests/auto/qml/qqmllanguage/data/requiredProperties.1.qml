import QtQuick 2.13
Item {
     property var required: 32  // required is still allowed as an identifier for properties
     function f(required) { // for javascript
         required = required + required;
         console.log(required);
    }
}
