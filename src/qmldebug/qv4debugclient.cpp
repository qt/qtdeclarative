/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4debugclient_p.h"
#include "qv4debugclient_p_p.h"
#include "qqmldebugconnection_p.h"

#include <private/qpacket_p.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

QT_BEGIN_NAMESPACE

const char *V8REQUEST = "v8request";
const char *V8MESSAGE = "v8message";
const char *SEQ = "seq";
const char *TYPE = "type";
const char *COMMAND = "command";
const char *ARGUMENTS = "arguments";
const char *STEPACTION = "stepaction";
const char *STEPCOUNT = "stepcount";
const char *EXPRESSION = "expression";
const char *FRAME = "frame";
const char *CONTEXT = "context";
const char *GLOBAL = "global";
const char *DISABLEBREAK = "disable_break";
const char *HANDLES = "handles";
const char *INCLUDESOURCE = "includeSource";
const char *FROMFRAME = "fromFrame";
const char *TOFRAME = "toFrame";
const char *BOTTOM = "bottom";
const char *NUMBER = "number";
const char *FRAMENUMBER = "frameNumber";
const char *TYPES = "types";
const char *IDS = "ids";
const char *FILTER = "filter";
const char *FROMLINE = "fromLine";
const char *TOLINE = "toLine";
const char *TARGET = "target";
const char *LINE = "line";
const char *COLUMN = "column";
const char *ENABLED = "enabled";
const char *CONDITION = "condition";
const char *IGNORECOUNT = "ignoreCount";
const char *BREAKPOINT = "breakpoint";
const char *FLAGS = "flags";

const char *CONTINEDEBUGGING = "continue";
const char *EVALUATE = "evaluate";
const char *LOOKUP = "lookup";
const char *BACKTRACE = "backtrace";
const char *SCOPE = "scope";
const char *SCOPES = "scopes";
const char *SCRIPTS = "scripts";
const char *SOURCE = "source";
const char *SETBREAKPOINT = "setbreakpoint";
const char *CLEARBREAKPOINT = "clearbreakpoint";
const char *CHANGEBREAKPOINT = "changebreakpoint";
const char *SETEXCEPTIONBREAK = "setexceptionbreak";
const char *VERSION = "version";
const char *DISCONNECT = "disconnect";
const char *GARBAGECOLLECTOR = "gc";

const char *CONNECT = "connect";
const char *INTERRUPT = "interrupt";

const char *REQUEST = "request";
const char *IN = "in";
const char *NEXT = "next";
const char *OUT = "out";

const char *SCRIPT = "script";
const char *SCRIPTREGEXP = "scriptRegExp";
const char *EVENT = "event";

const char *ALL = "all";
const char *UNCAUGHT = "uncaught";

#define VARIANTMAPINIT \
    Q_D(QV4DebugClient); \
    QJsonObject jsonVal; \
    jsonVal.insert(QLatin1String(SEQ), d->seq++); \
    jsonVal.insert(QLatin1String(TYPE), QLatin1String(REQUEST));

QV4DebugClient::QV4DebugClient(QQmlDebugConnection *connection)
    : QQmlDebugClient(*new QV4DebugClientPrivate(connection))
{
    QObject::connect(this, &QQmlDebugClient::stateChanged,
                     this, [this](State state) { d_func()->onStateChanged(state); });
}

QV4DebugClientPrivate::QV4DebugClientPrivate(QQmlDebugConnection *connection) :
    QQmlDebugClientPrivate(QLatin1String("V8Debugger"), connection)
{
}

void QV4DebugClient::connect()
{
    Q_D(QV4DebugClient);
    d->sendMessage(CONNECT);
}

void QV4DebugClient::interrupt()
{
    Q_D(QV4DebugClient);
    d->sendMessage(INTERRUPT);
}

void QV4DebugClient::continueDebugging(StepAction action)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "continue",
    //      "arguments" : { "stepaction" : <"in", "next" or "out">,
    //                      "stepcount"  : <number of steps (default 1)>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(CONTINEDEBUGGING));

    if (action != Continue) {
        QJsonObject args;
        switch (action) {
        case In:
            args.insert(QLatin1String(STEPACTION), QLatin1String(IN));
            break;
        case Out:
            args.insert(QLatin1String(STEPACTION), QLatin1String(OUT));
            break;
        case Next:
            args.insert(QLatin1String(STEPACTION), QLatin1String(NEXT));
            break;
        default:
            break;
        }
        jsonVal.insert(QLatin1String(ARGUMENTS), args);
    }

    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::evaluate(const QString &expr, int frame, int context)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "evaluate",
    //      "arguments" : { "expression"    : <expression to evaluate>,
    //                      "frame"         : <number>,
    //                      "context"       : <object ID>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(EVALUATE));

    QJsonObject args;
    args.insert(QLatin1String(EXPRESSION), expr);

    if (frame != -1)
        args.insert(QLatin1String(FRAME), frame);

    if (context != -1)
        args.insert(QLatin1String(CONTEXT), context);

    jsonVal.insert(QLatin1String(ARGUMENTS), args);

    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::lookup(const QList<int> &handles, bool includeSource)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "lookup",
    //      "arguments" : { "handles"       : <array of handles>,
    //                      "includeSource" : <boolean indicating whether the source will be included when script objects are returned>,
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND),(QLatin1String(LOOKUP)));

    QJsonObject args;
    QJsonArray array;

    for (int handle : handles)
        array.append(handle);

    args.insert(QLatin1String(HANDLES), array);

    if (includeSource)
        args.insert(QLatin1String(INCLUDESOURCE), includeSource);

    jsonVal.insert(QLatin1String(ARGUMENTS), args);

    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::backtrace(int fromFrame, int toFrame, bool bottom)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "backtrace",
    //      "arguments" : { "fromFrame" : <number>
    //                      "toFrame" : <number>
    //                      "bottom" : <boolean, set to true if the bottom of the stack is requested>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(BACKTRACE));

    QJsonObject args;

    if (fromFrame != -1)
        args.insert(QLatin1String(FROMFRAME), fromFrame);

    if (toFrame != -1)
        args.insert(QLatin1String(TOFRAME), toFrame);

    if (bottom)
        args.insert(QLatin1String(BOTTOM), bottom);

    jsonVal.insert(QLatin1String(ARGUMENTS), args);
    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::frame(int number)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "frame",
    //      "arguments" : { "number" : <frame number>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(FRAME));

    if (number != -1) {
        QJsonObject args;
        args.insert(QLatin1String(NUMBER), number);
        jsonVal.insert(QLatin1String(ARGUMENTS), args);
    }

    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::scope(int number, int frameNumber)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scope",
    //      "arguments" : { "number" : <scope number>
    //                      "frameNumber" : <frame number, optional uses selected frame if missing>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(SCOPE));

    if (number != -1) {
        QJsonObject args;
        args.insert(QLatin1String(NUMBER), number);

        if (frameNumber != -1)
            args.insert(QLatin1String(FRAMENUMBER), frameNumber);

        jsonVal.insert(QLatin1String(ARGUMENTS), args);
    }

    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::scripts(int types, const QList<int> &ids, bool includeSource)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scripts",
    //      "arguments" : { "types"         : <types of scripts to retrieve
    //                                           set bit 0 for native scripts
    //                                           set bit 1 for extension scripts
    //                                           set bit 2 for normal scripts
    //                                         (default is 4 for normal scripts)>
    //                      "ids"           : <array of id's of scripts to return. If this is not specified all scripts are requrned>
    //                      "includeSource" : <boolean indicating whether the source code should be included for the scripts returned>
    //                      "filter"        : <string or number: filter string or script id.
    //                                         If a number is specified, then only the script with the same number as its script id will be retrieved.
    //                                         If a string is specified, then only scripts whose names contain the filter string will be retrieved.>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(SCRIPTS));

    QJsonObject args;
    args.insert(QLatin1String(TYPES), types);

    if (ids.count()) {
        QJsonArray array;
        for (int id : ids)
            array.append(id);

        args.insert(QLatin1String(IDS), array);
    }

    if (includeSource)
        args.insert(QLatin1String(INCLUDESOURCE), includeSource);

    jsonVal.insert(QLatin1String(ARGUMENTS), args);
    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::setBreakpoint(const QString &target, int line, int column, bool enabled,
                                   const QString &condition, int ignoreCount)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setbreakpoint",
    //      "arguments" : { "type"        : "scriptRegExp"
    //                      "target"      : <function expression or script identification>
    //                      "line"        : <line in script or function>
    //                      "column"      : <character position within the line>
    //                      "enabled"     : <initial enabled state. True or false, default is true>
    //                      "condition"   : <string with break point condition>
    //                      "ignoreCount" : <number specifying the number of break point hits to ignore, default value is 0>
    //                    }
    //    }

    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(SETBREAKPOINT));

    QJsonObject args;

    args.insert(QLatin1String(TYPE), QLatin1String(SCRIPTREGEXP));
    args.insert(QLatin1String(TARGET), target);

    if (line != -1)
        args.insert(QLatin1String(LINE), line);

    if (column != -1)
        args.insert(QLatin1String(COLUMN), column);

    args.insert(QLatin1String(ENABLED), enabled);

    if (!condition.isEmpty())
        args.insert(QLatin1String(CONDITION), condition);

    if (ignoreCount != -1)
        args.insert(QLatin1String(IGNORECOUNT), ignoreCount);

    jsonVal.insert(QLatin1String(ARGUMENTS),args);
    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::clearBreakpoint(int breakpoint)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "clearbreakpoint",
    //      "arguments" : { "breakpoint" : <number of the break point to clear>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(CLEARBREAKPOINT));

    QJsonObject args;
    args.insert(QLatin1String(BREAKPOINT), breakpoint);
    jsonVal.insert(QLatin1String(ARGUMENTS),args);

    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::changeBreakpoint(int breakpoint, bool enabled)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "changebreakpoint",
    //      "arguments" : { "breakpoint" : <number of the break point to change>
    //                      "enabled" : <bool: enables the break type if true, disables if false>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(CHANGEBREAKPOINT));

    QJsonObject args;
    args.insert(QLatin1String(BREAKPOINT), breakpoint);
    args.insert(QLatin1String(ENABLED), enabled);

    jsonVal.insert(QLatin1String(ARGUMENTS), args);
    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::setExceptionBreak(Exception type, bool enabled)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setexceptionbreak",
    //      "arguments" : { "type"    : <string: "all", or "uncaught">,
    //                      "enabled" : <optional bool: enables the break type if true>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(SETEXCEPTIONBREAK));

    QJsonObject args;

    if (type == All)
        args.insert(QLatin1String(TYPE), QLatin1String(ALL));
    else if (type == Uncaught)
        args.insert(QLatin1String(TYPE), QLatin1String(UNCAUGHT));

    if (enabled)
        args.insert(QLatin1String(ENABLED), enabled);

    jsonVal.insert(QLatin1String(ARGUMENTS), args);
    d->sendMessage(V8REQUEST, jsonVal);
}

void QV4DebugClient::version()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "version",
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(VERSION));
    d->sendMessage(V8REQUEST, jsonVal);
}

QV4DebugClient::Response QV4DebugClient::response() const
{
    Q_D(const QV4DebugClient);
    const QJsonObject value = QJsonDocument::fromJson(d->response).object();
    return {
        value.value(QLatin1String(COMMAND)).toString(),
        value.value(QLatin1String("body"))
    };
}

void QV4DebugClient::disconnect()
{
    //    { "seq"     : <number>,
    //      "type"    : "request",
    //      "command" : "disconnect",
    //    }
    VARIANTMAPINIT;
    jsonVal.insert(QLatin1String(COMMAND), QLatin1String(DISCONNECT));
    d->sendMessage(DISCONNECT, jsonVal);
}

void QV4DebugClientPrivate::onStateChanged(QQmlDebugClient::State state)
{
    if (state == QQmlDebugClient::Enabled)
        flushSendBuffer();
}

void QV4DebugClient::messageReceived(const QByteArray &data)
{
    Q_D(QV4DebugClient);
    QPacket ds(connection()->currentDataStreamVersion(), data);
    QByteArray command;
    ds >> command;

    if (command == "V8DEBUG") {
        QByteArray type;
        ds >> type >> d->response;

        if (type == CONNECT) {
            emit connected();

        } else if (type == INTERRUPT) {
            emit interrupted();

        } else if (type == V8MESSAGE) {
            const QJsonObject value = QJsonDocument::fromJson(d->response).object();
            QString type = value.value(QLatin1String(TYPE)).toString();

            if (type == QLatin1String("response")) {

                if (!value.value(QLatin1String("success")).toBool()) {
                    emit failure();
                    qDebug() << "Received success == false response from application:"
                             << value.value(QLatin1String("message")).toString();
                    return;
                }

                QString debugCommand(value.value(QLatin1String(COMMAND)).toString());
                if (debugCommand == QLatin1String(BACKTRACE) ||
                        debugCommand == QLatin1String(LOOKUP) ||
                        debugCommand == QLatin1String(SETBREAKPOINT) ||
                        debugCommand == QLatin1String(EVALUATE) ||
                        debugCommand == QLatin1String(VERSION) ||
                        debugCommand == QLatin1String(DISCONNECT) ||
                        debugCommand == QLatin1String(GARBAGECOLLECTOR) ||
                        debugCommand == QLatin1String(CHANGEBREAKPOINT) ||
                        debugCommand == QLatin1String(CLEARBREAKPOINT) ||
                        debugCommand == QLatin1String(FRAME) ||
                        debugCommand == QLatin1String(SCOPE) ||
                        debugCommand == QLatin1String(SCOPES) ||
                        debugCommand == QLatin1String(SCRIPTS) ||
                        debugCommand == QLatin1String(SOURCE) ||
                        debugCommand == QLatin1String(SETEXCEPTIONBREAK)) {
                    emit result();
                } else {
                    // DO NOTHING
                }

            } else if (type == QLatin1String(EVENT)) {
                QString event(value.value(QLatin1String(EVENT)).toString());

                if (event == QLatin1String("break") || event == QLatin1String("exception"))
                    emit stopped();
            }
        }
    }
}

void QV4DebugClientPrivate::sendMessage(const QByteArray &command, const QJsonObject &args)
{
    Q_Q(QV4DebugClient);
    const QByteArray msg = packMessage(command, args);
    if (q->state() == QQmlDebugClient::Enabled) {
        q->sendMessage(msg);
    } else {
        sendBuffer.append(msg);
    }
}

void QV4DebugClientPrivate::flushSendBuffer()
{
    foreach (const QByteArray &msg, sendBuffer)
        sendMessage(msg);
    sendBuffer.clear();
}

QByteArray QV4DebugClientPrivate::packMessage(const QByteArray &type, const QJsonObject &object)
{
    QPacket rs(connection->currentDataStreamVersion());
    QByteArray cmd = "V8DEBUG";
    rs << cmd << type << QJsonDocument(object).toJson(QJsonDocument::Compact);
    return rs.data();
}

QT_END_NAMESPACE
