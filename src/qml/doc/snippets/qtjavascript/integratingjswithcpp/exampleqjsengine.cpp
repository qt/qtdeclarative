// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![qjs-engine-example]

    QJSEngine engine;
    // We create an object with a read-only property whose getter throws an exception
    auto val = engine.evaluate("let o = { get f()  {throw 42;} }; o");
    val.property("f");
    qDebug() << engine.hasError(); // prints false

    // This time, we  construct a QJSManagedValue before accessing the property
    val = engine.evaluate("let o = { get f()  {throw 42;} }; o");
    QJSManagedValue managed(std::move(val), &engine);
    managed.property("f");
    qDebug() << engine.hasError(); // prints true

    QJSValue error = engine.catchError();
    Q_ASSERT(error.toInt(), 42);

//![qjs-engine-example]
