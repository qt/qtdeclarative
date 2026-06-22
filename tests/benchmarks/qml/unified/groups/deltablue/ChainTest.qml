// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound
pragma Strict
import QtQml

QtObject {
    id: chainTest
    property int length: 100

    property Planner planner: Planner { id: planner }
    property Plan plan: Plan { id: plan }

    property Component variable: Variable {}
    property list<Variable> variables: new Array(length)
    property Variable first: variables[0]
    property Variable last: variables[length - 1]
    function setVariable(i: int, v: Variable) { variables[i] = v; }

    property Component constraint: Constraint {}

    property Constraint stayConstraint: Constraint {
        id: stayConstraint
        myOutput: chainTest.last
        strength: Strength.STRONG_DEFAULT
    }

    property Constraint editConstraint: Constraint {
        id: editConstraint
        isInput: true
        myOutput: chainTest.first
        strength: Strength.PREFERRED
    }

    Component.onCompleted: {
        for (let i = 0, end = length; i < end; ++i)
            setVariable(i, variable.createObject(this, {objectName: "v" + i}) as Variable);

        let v = variables;
        let p = planner;
        for (let i = 0, end = length - 1; i < end; ++i) {
            p.incrementalAdd(constraint.createObject(this, {
                 myInput: v[i],
                 myOutput: v[i + 1],
                 strength: Strength.REQUIRED
            }) as Constraint);
        }

        planner.incrementalAdd(stayConstraint)
        planner.incrementalAdd(editConstraint)
    }

    function verify(i: int) {
        if (last.value !== i)
            console.error("Chain test failed.", last.value, i, first.value);
    }

    function run() {
        planner.populatePlanFromConstraint(editConstraint, plan);
        for (let i = 0; i < 100; ++i) {
            first.value = i;
            plan.execute();
            verify(i);
        }
    }
}
