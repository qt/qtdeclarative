/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


var curVal = 0
var memory = 0
var lastOp = ""
var previousOperator = ""
var digits = ""

function disabled(op) {
    if (op == "." && digits.toString().search(/\./) != -1) {
        return true
    } else if (op == window.squareRoot &&  digits.toString().search(/-/) != -1) {
        return true
    } else {
        return false
    }
}

function digitPressed(op)
{
    if (disabled(op))
        return
    if (digits.toString().length >= 14)
        return
    if (lastOp.toString().length == 1 && ((lastOp >= "0" && lastOp <= "9") || lastOp == ".") ) {
        digits = digits + op.toString()
        display.appendDigit(op.toString())
    } else {
        digits = op
        display.appendDigit(op.toString())
    }
    lastOp = op
}

function operatorPressed(op)
{
    if (disabled(op))
        return
    lastOp = op

    if (previousOperator == "+") {
        digits = Number(digits.valueOf()) + Number(curVal.valueOf())
    } else if (previousOperator == "−") {
        digits = Number(curVal) - Number(digits.valueOf())
    } else if (previousOperator == "×") {
        digits = Number(curVal) * Number(digits.valueOf())
    } else if (previousOperator == "÷") {
        digits = Number(curVal) / Number(digits.valueOf())
    } else if (previousOperator == "=") {
    }

    if (op == "+" || op == "−" || op == "×" || op == "÷") {
        previousOperator = op
        curVal = digits.valueOf()
        display.displayOperator(previousOperator)
        return
    }

    if (op == "=") {
        display.newLine("=", digits.toString())
    }

    curVal = 0
    previousOperator = ""

    if (op == "1/x") {
        digits = (1 / digits.valueOf()).toString()
    } else if (op == "x^2") {
        digits = (digits.valueOf() * digits.valueOf()).toString()
    } else if (op == "Abs") {
        digits = (Math.abs(digits.valueOf())).toString()
    } else if (op == "Int") {
        digits = (Math.floor(digits.valueOf())).toString()
    } else if (op == "±") {
        digits = (digits.valueOf() * -1).toString()
    } else if (op == "√") {
        digits = (Math.sqrt(digits.valueOf())).toString()
    } else if (op == "mc") {
        memory = 0;
    } else if (op == "m+") {
        memory += digits.valueOf()
    } else if (op == "mr") {
        digits = memory.toString()
    } else if (op == "m-") {
        memory = digits.valueOf()
    } else if (op == window.leftArrow) {
        digits = digits.toString().slice(0, -1)
        if (digits.length == 0) {
            digits = "0"
        }
    } else if (op == "Off") {
        Qt.quit();
    } else if (op == "C") {
        display.clear()
    } else if (op == "AC") {
        curVal = 0
        memory = 0
        lastOp = ""
        digits ="0"
    }


}

