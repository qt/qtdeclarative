import QtQuick

Item {

    function f() {
        var count = 0;
        class Person {
            constructor(name){
                this._name = name;
            }
        }
        class Employee extends Person {
            constructor(name, age){
                super(name);
                this._name = name;
                this._age = age;
                ++count;
            }

            doWork(){}

            /* do we get the comment? */ get name(){
                return this._name.toUpperCase();
            }

            set name(newName){
                if (newName) {
                    this._name = newName;
                }
            }

            static get count(){
                return count;
            }
        }
    }
}
