// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QJSEngine myEngine;
QJSValue three = myEngine.evaluate("1 + 2");
//! [0]


//! [1]
QJSValue fun = myEngine.evaluate("(function(a, b) { return a + b; })");
QJSValueList args;
args << 1 << 2;
QJSValue threeAgain = fun.call(args);
//! [1]


//! [2]
QString fileName = "helloworld.qs";
QFile scriptFile(fileName);
if (!scriptFile.open(QIODevice::ReadOnly))
    // handle error
QTextStream stream(&scriptFile);
QString contents = stream.readAll();
scriptFile.close();
myEngine.evaluate(contents, fileName);
//! [2]


//! [3]
myEngine.globalObject().setProperty("myNumber", 123);
...
QJSValue myNumberPlusOne = myEngine.evaluate("myNumber + 1");
//! [3]


//! [4]
QJSValue result = myEngine.evaluate(...);
if (result.isError())
    qDebug()
            << "Uncaught exception at line"
            << result.property("lineNumber").toInt()
            << ":" << result.toString();
//! [4]


//! [5]
QPushButton *button = new QPushButton;
QJSValue scriptButton = myEngine.newQObject(button);
myEngine.globalObject().setProperty("button", scriptButton);

myEngine.evaluate("button.checkable = true");

qDebug() << scriptButton.property("checkable").toBool();
scriptButton.property("show").call(); // call the show() slot
//! [5]


//! [6]
QJSEngine engine;

QObject *myQObject = new QObject();
myQObject->setProperty("dynamicProperty", 3);

QJSValue myScriptQObject = engine.newQObject(myQObject);
engine.globalObject().setProperty("myObject", myScriptQObject);

qDebug() << engine.evaluate("myObject.dynamicProperty").toInt();
//! [6]


//! [7]
class MyObject : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE MyObject() {}
};
//! [7]

//! [8]
QJSValue jsMetaObject = engine.newQMetaObject(&MyObject::staticMetaObject);
engine.globalObject().setProperty("MyObject", jsMetaObject);
//! [8]

//! [9]
engine.evaluate("var myObject = new MyObject()");
//! [9]
