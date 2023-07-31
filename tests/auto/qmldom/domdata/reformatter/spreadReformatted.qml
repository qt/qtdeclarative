import QtQuick 2.15

Item {
    function foo() {
        iterableObj = [1, 2];
        obj = {
            a: 42
        };
        let x = (console.log("bla\n"), 3);
        myFunction(...iterableObj); // pass all elements of iterableObj as arguments to function myFunction
        let arr = [...iterableObj, '4', 'five', 6]; // combine two arrays by inserting all elements from iterableObj
        //let objClone = { ...obj }; // pass all key:value pairs from an object (ES2018)
        myFunction(-1, ...args, 2, ...[3]);
        console.log(Math.max(...[1, 2, 3, 4]));
    }
    function myFunction() {
    }
}
