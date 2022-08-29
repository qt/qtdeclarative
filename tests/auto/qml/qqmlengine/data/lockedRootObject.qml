import QtQml

QtObject {
    property string myErrorName: {
        var e = new Error;
        try {
            e.name = "MyError1";
        } finally {
            return e.name;
        }
    }

    property string errorName: {
        var e = new Error;
        try {
            Error.prototype.name = "MyError2";
        } finally {
            return e.name
        }
    }

    property int mathMax: {
        // Cannot change methods of builtins
        try {
            Math.max = function(a, b) { return 10 };
        } finally {
            return Math.max(3, 4)
        }
    }

    property int extendGlobal: {
        // Can add new methods to globals
        try {
            Array.prototype.myMethod = function() { return 32 }
        } finally {
            return (new Array).myMethod()
        }
    }

    property string prototypeTrick: {
        // Cannot change prototypes of locked objects
        try {
            SyntaxError.prototype.setPrototypeOf({
                toLocaleString : function() { return "not a SyntaxError"}
            });
        } finally {
            return (new SyntaxError).toLocaleString();
        }
    }

    property string shadowMethod1: {
        // Can override Object.prototype methods meant to be changed
        try {
            TypeError.prototype.toLocaleString = function() { return "not a TypeError"};
        } finally {
            return (new TypeError).toLocaleString();
        }
    }

    property bool shadowMethod2: {
        // Cannot override Object.prototype methods not meant to be changed
        try {
            TypeError.prototype.hasOwnProperty = function() { return true };
        } finally {
            return (new TypeError).hasOwnProperty("foobar");
        }
    }

    property string changeObjectProto1: {
        // Can change Object.prototype methods meant to be changed
        try {
            Object.prototype.toLocaleString = function() { return "not an Object"};
        } finally {
            return (new Object).toLocaleString();
        }
    }

    property bool changeObjectProto2: {
        // Cannot change Object.prototype methods not meant to be changed
        try {
            Object.prototype.hasOwnProperty = function() { return true };
        } finally {
            return (new Object).hasOwnProperty("foobar");
        }
    }

    property string defineProperty1: {
        // Can define a property that shadows an existing one meant to be changed
        try {
            Object.defineProperty(URIError.prototype, "toLocaleString", {
                value: function() { return "not a URIError" }
            })
        } finally {
            return (new URIError).toLocaleString();
        }
    }

    property bool defineProperty2: {
        // Cannot define a property that shadows an existing one not meant to be changed
        try {
            Object.defineProperty(URIError.prototype, "hasOwnProperty", {
                value: function() { return true }
            })
        } finally {
            return (new URIError).hasOwnProperty("foobar");
        }
    }
}
