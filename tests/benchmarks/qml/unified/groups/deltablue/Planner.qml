// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
pragma ComponentBehavior: Bound

import QtQml

QtObject {
    id: planner

    component Remover: QtObject {
        property list<Constraint> unsatisfied
        property list<Variable> toRemove

        function removeOne(v: Variable) {
            let vDeterminedBy = v.determinedBy;
            for (let i = 0, length = v.length(); i < length; ++i) {
                let next = v.constraint(i) as Constraint;
                if (next.satisfaction === Satisfaction.NONE) {
                    let u = unsatisfied;
                    u[u.length++] = next;
                } else if (next !== vDeterminedBy) {
                    next.recalculate();
                    let t = toRemove;
                    t[t.length++] = next.output;
                }
            }
        }

        function run(c: Constraint) {
            c.satisfaction = Satisfaction.NONE;
            c.removeFromGraph();

            let out = c.output as Variable;
            out.determinedBy = null;
            out.walkStrength = Strength.WEAKEST;
            out.stay = true;

            removeOne(out);
            let t = toRemove;
            while (t.length > 0) {
                let one = t[t.length - 1];
                --t.length;
                removeOne(one);
            }

            let u = unsatisfied;
            let ss = Strength;
            let sat = satisfier
            for (let s = ss.REQUIRED, end = ss.WEAKEST; s !== end; s = ss.nextWeaker(s)) {
                for (let i = 0, length = u.length; i < length; ++i) {
                    let uu = u[i];
                    if (uu.strength === s)
                        sat.run(uu);
                }
            }

            u.length = 0;
        }
    }

    component Satisfier: QtObject {
        property list<Constraint> todo

        function addPropagate(c: Constraint, mark: int) : bool {
            let d = c;
            while (true) {
                let output = d.output as Variable;

                if (output.mark === mark) {
                    remover.run(c);
                    todo.length = 0;
                    return false;
                }
                d.recalculate();

                let outputDeterminedBy = output.determinedBy;
                for (let i = 0, length = output.length();  i < length; ++i) {
                    let dd = output.constraint(i) as Constraint;
                    if (dd !== outputDeterminedBy && dd.satisfaction !== Satisfaction.NONE)
                        todo[todo.length++] = dd;
                }

                let todoLength = todo.length;
                if (todoLength === 0)
                    break;

                d = todo[todoLength - 1] as Constraint;
                --todo.length;
            }
            return true;
        }

        function satisfy(c: Constraint, mark: int) : Constraint {
            c.chooseMethod(mark);
            if (c.satisfaction === Satisfaction.NONE) {
                if (c.strength === Strength.REQUIRED)
                    console.error("Could not satisfy a required constraint!");
                return null;
            }

            c.markInputs(mark);
            let out = c.output as Variable;
            let overridden = out.determinedBy as Constraint;
            if (overridden !== null)
                overridden.satisfaction = Satisfaction.NONE;

            out.determinedBy = c;
            if (!addPropagate(c, mark))
                console.error("Cycle encountered");

            out.mark = mark;
            return overridden;
        }

        function run(c: Constraint) {
            let mark = planner.newMark();
            let overridden = satisfy(c, mark);
            while (overridden !== null)
                overridden = satisfy(overridden, mark);
        }
    }

    component Populator: QtObject {
        property list<Constraint> constraints

        function run(initial: Constraint, plan: Plan) {
            // TODO: If we pass initial as a list here, we cannot assign to constraints!
            let l = constraints;
            l.length = 1;
            l[0] = initial;
            plan.clear();
            for (let i = 0; i < l.length; ++i) {
                let c = l[i];
                if (!c.isInput || c.satisfaction === Satisfaction.NONE) {
                    // Not eligible for inclusion. Remove
                    let last = l[l.length - 1];
                    if (i < l.length--)
                        l[i] = last;
                }
            }

            let mark = planner.newMark();
            while (l.length > 0) {
                let c = l[l.length - 1];
                --l.length;

                let output = c.output as Variable;
                if (output.mark !== mark && c.inputsKnown(mark)) {
                    plan.addConstraint(c);
                    output.mark = mark;
                    addConstraintsConsumingTo(output);
                }
            }
        }

        function addConstraintsConsumingTo(v: Variable) {
            let vDeterminedBy = v.determinedBy;
            let l = constraints;
            for (let i = 0, length = v.length(); i < length; ++i) {
                let c = v.constraint(i) as Constraint;
                if (c !== vDeterminedBy && c.satisfaction !== Satisfaction.NONE)
                    l[l.length++] = c;
            }
        }
    }

    property Remover remover: Remover { id: remover }
    property Satisfier satisfier: Satisfier { id: satisfier }
    property Populator populator: Populator { id: populator }

    property int currentMark: 0


    function incrementalAdd(c : Constraint) {
        c.addToGraph();
        satisfier.run(c);
    }

    function incrementalRemove(c : Constraint) {
        remover.run(c);
    }

    // TODO: If we pass a list here and modify it in place, the behavior differs between
    //       interpreter and compiler.
    function populatePlanFromConstraint(c: Constraint, plan: Plan) {
        populator.run(c, plan)
    }

    function newMark() : int {
        return ++currentMark;
    }
}
