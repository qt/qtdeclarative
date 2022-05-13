// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![qjs-engine]

    QJSEngine engine;
    QJSValue object =  engine.newObject();
    object.setProperty("num", 42);
    QJSValue function = engine.evaluate("(o) => o.num *= 2 ");
    QJSValueList args = { object };
    QJSValue result = function.call(args);
    QJSValue expected = "84";
    Q_ASSERT(result.equals(expected) && !result.strictlyEquals(expected));

//![qjs-engine]

