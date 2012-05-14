/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
var timer = 0

function disabled(op) {
    if (op == "." && display.text.toString().search(/\./) != -1) {
        return true
    } else if (op == window.squareRoot &&  display.text.toString().search(/-/) != -1) {
        return true
    } else {
        return false
    }
}

function doOperation(op) {
    if (op == '*')//Keyboard Aliases
        op = window.multiplication;
    if (op == '/')
        op = window.division;
    if (disabled(op)) {
        return
    }

    if (op.toString().length==1 && ((op >= "0" && op <= "9") || op==".") ) {
        if (display.text.toString().length >= 14)
            return; // No arbitrary length numbers
        if (lastOp.toString().length == 1 && ((lastOp >= "0" && lastOp <= "9") || lastOp == ".") ) {
            display.text = display.text + op.toString()
        } else {
            display.text = op
        }
        lastOp = op
        return
    }
    lastOp = op

    if (display.currentOperation.text == "+") {
        display.text = Number(display.text.valueOf()) + Number(curVal.valueOf())
    } else if (display.currentOperation.text == "-") {
        display.text = Number(curVal) - Number(display.text.valueOf())
    } else if (display.currentOperation.text == window.multiplication) {
        display.text = Number(curVal) * Number(display.text.valueOf())
    } else if (display.currentOperation.text == window.division) {
        display.text = Number(Number(curVal) / Number(display.text.valueOf())).toString()
    } else if (display.currentOperation.text == "=") {
    }

    if (op == "+" || op == "-" || op == window.multiplication || op == window.division) {
        display.currentOperation.text = op
        curVal = display.text.valueOf()
        return
    }

    curVal = 0
    display.currentOperation.text = ""

    if (op == "1/x") {
        display.text = (1 / display.text.valueOf()).toString()
    } else if (op == "x^2") {
        display.text = (display.text.valueOf() * display.text.valueOf()).toString()
    } else if (op == "Abs") {
        display.text = (Math.abs(display.text.valueOf())).toString()
    } else if (op == "Int") {
        display.text = (Math.floor(display.text.valueOf())).toString()
    } else if (op == window.plusminus) {
        display.text = (display.text.valueOf() * -1).toString()
    } else if (op == window.squareRoot) {
        display.text = (Math.sqrt(display.text.valueOf())).toString()
    } else if (op == "mc") {
        memory = 0;
    } else if (op == "m+") {
        memory += display.text.valueOf()
    } else if (op == "mr") {
        display.text = memory.toString()
    } else if (op == "m-") {
        memory = display.text.valueOf()
    } else if (op == window.leftArrow) {
        display.text = display.text.toString().slice(0, -1)
        if (display.text.length == 0) {
            display.text = "0"
        }
    } else if (op == "Off") {
        Qt.quit();
    } else if (op == "C") {
        display.text = "0"
    } else if (op == "AC") {
        curVal = 0
        memory = 0
        lastOp = ""
        display.text ="0"
    }
}

