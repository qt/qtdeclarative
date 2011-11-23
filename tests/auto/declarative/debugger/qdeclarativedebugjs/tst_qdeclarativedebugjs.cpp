/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QLibraryInfo>
#include <QtDeclarative/private/qdeclarativedebugclient_p.h>
#include <QtDeclarative/QJSEngine>

//QDeclarativeDebugTest
#include "../shared/debugutil_p.h"
#include "../../../shared/util.h"

const char *SEQ = "seq";
const char *TYPE = "type";
const char *COMMAND = "command";
const char *ARGUMENTS = "arguments";
const char *STEPACTION = "stepaction";
const char *STEPCOUNT = "stepcount";
const char *EXPRESSION = "expression";
const char *FRAME = "frame";
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
const char *CHANGEBREAKPOINT = "changebreakpoint";
const char *CLEARBREAKPOINT = "clearbreakpoint";
const char *SETEXCEPTIONBREAK = "setexceptionbreak";
const char *V8FLAGS = "v8flags";
const char *VERSION = "version";
const char *DISCONNECT = "disconnect";
const char *LISTBREAKPOINTS = "listbreakpoints";
const char *GARBAGECOLLECTOR = "gc";
//const char *PROFILE = "profile";

const char *CONNECT = "connect";
const char *INTERRUPT = "interrupt";

const char *REQUEST = "request";
const char *IN = "in";
const char *NEXT = "next";
const char *OUT = "out";

const char *FUNCTION = "function";
const char *SCRIPT = "script";

const char *ALL = "all";
const char *UNCAUGHT = "uncaught";

//const char *PAUSE = "pause";
//const char *RESUME = "resume";

const char *BLOCKMODE = "-qmljsdebugger=port:3771,block";
const char *NORMALMODE = "-qmljsdebugger=port:3771";
const char *TEST_QMLFILE = "test.qml";
const char *TEST_JSFILE = "test.js";
const char *TIMER_QMLFILE = "timer.qml";

#define VARIANTMAPINIT \
    QString obj("{}"); \
    QJSValue jsonVal = parser.call(QJSValue(), QJSValueList() << obj); \
    jsonVal.setProperty(SEQ,QJSValue(seq++)); \
    jsonVal.setProperty(TYPE,REQUEST);


#undef QVERIFY
#define QVERIFY(statement) \
do {\
    if (!QTest::qVerify((statement), #statement, "", __FILE__, __LINE__)) {\
        if (QTest::currentTestFailed()) \
          qDebug().nospace() << "\nDEBUGGEE OUTPUT:\n" << process->output();\
        return;\
    }\
} while (0)


class QJSDebugClient;

class tst_QDeclarativeDebugJS : public QObject
{
    Q_OBJECT

    bool init(const QString &qmlFile = QString(TEST_QMLFILE), bool blockMode = true);

private slots:
    void initTestCase();
    void cleanupTestCase();

    void cleanup();

    void getVersion();
    void getVersionWhenAttaching();

    void applyV8Flags();

    void disconnect();

    void gc();

    void listBreakpoints();

    void setBreakpointInScriptOnCompleted();
    void setBreakpointInScriptOnTimerCallback();
    void setBreakpointInScriptInDifferentFile();
    void setBreakpointInScriptOnComment();
    void setBreakpointInScriptOnEmptyLine();
    void setBreakpointInScriptWithCondition();
    //void setBreakpointInFunction(); //NOT SUPPORTED
    void setBreakpointWhenAttaching();

    void changeBreakpoint();
    void changeBreakpointOnCondition();

    void clearBreakpoint();

    void setExceptionBreak();

    void stepNext();
    void stepNextWithCount();
    void stepIn();
    void stepOut();
    void continueDebugging();

    void backtrace();

    void getFrameDetails();

    void getScopeDetails();

    void evaluateInGlobalScope();
    void evaluateInLocalScope();

    void getScopes();

    void getScripts();

    void getSource();

    //    void profile(); //NOT SUPPORTED

    //    void verifyQMLOptimizerDisabled();

private:
    QDeclarativeDebugProcess *process;
    QJSDebugClient *client;
    QDeclarativeDebugConnection *connection;
};

class QJSDebugClient : public QDeclarativeDebugClient
{
    Q_OBJECT
public:
    enum StepAction
    {
        Continue,
        In,
        Out,
        Next
    };

    enum Exception
    {
        All,
        Uncaught
    };

//    enum ProfileCommand
//    {
//        Pause,
//        Resume
//    };

    QJSDebugClient(QDeclarativeDebugConnection *connection)
        : QDeclarativeDebugClient(QLatin1String("V8Debugger"), connection),
          seq(0)
    {
        parser = jsEngine.evaluate(QLatin1String("JSON.parse"));
        stringify = jsEngine.evaluate(QLatin1String("JSON.stringify"));
    }

    void startDebugging();
    void interrupt();

    void continueDebugging(StepAction stepAction, int stepCount = 1);
    void evaluate(QString expr, bool global = false, bool disableBreak = false, int frame = -1, const QVariantMap &addContext = QVariantMap());
    void lookup(QList<int> handles, bool includeSource = false);
    void backtrace(int fromFrame = -1, int toFrame = -1, bool bottom = false);
    void frame(int number = -1);
    void scope(int number = -1, int frameNumber = -1);
    void scopes(int frameNumber = -1);
    void scripts(int types = 4, QList<int> ids = QList<int>(), bool includeSource = false, QVariant filter = QVariant());
    void source(int frame = -1, int fromLine = -1, int toLine = -1);
    void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = true, QString condition = QString(), int ignoreCount = -1);
    void changeBreakpoint(int breakpoint, bool enabled = true, QString condition = QString(), int ignoreCount = -1);
    void clearBreakpoint(int breakpoint);
    void setExceptionBreak(Exception type, bool enabled = false);
    void v8flags(QString flags);
    void version();
    //void profile(ProfileCommand command); //NOT SUPPORTED
    void disconnect();
    void gc();
    void listBreakpoints();

protected:
    //inherited from QDeclarativeDebugClient
    void statusChanged(Status status);
    void messageReceived(const QByteArray &data);

signals:
    void enabled();
    void breakpointSet();
    void result();
    void stopped();

private:
    void sendMessage(const QByteArray &);
    void flushSendBuffer();
    QByteArray packMessage(QByteArray message);

private:
    QJSEngine jsEngine;
    int seq;

    QList<QByteArray> sendBuffer;

public:
    QJSValue parser;
    QJSValue stringify;
    QByteArray response;

};

void QJSDebugClient::startDebugging()
{
    //    { "seq"     : <number>,
    //      "type"    : "request",
    //      "command" : "connect",
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(CONNECT)));

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::interrupt()
{
    //    { "seq"     : <number>,
    //      "type"    : "request",
    //      "command" : "interrupt",
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(INTERRUPT)));

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::continueDebugging(StepAction action, int count)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "continue",
    //      "arguments" : { "stepaction" : <"in", "next" or "out">,
    //                      "stepcount"  : <number of steps (default 1)>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(CONTINEDEBUGGING)));

    if (action != Continue) {
        QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);
        switch (action) {
        case In: args.setProperty(QLatin1String(STEPACTION),QJSValue(QLatin1String(IN)));
            break;
        case Out: args.setProperty(QLatin1String(STEPACTION),QJSValue(QLatin1String(OUT)));
            break;
        case Next: args.setProperty(QLatin1String(STEPACTION),QJSValue(QLatin1String(NEXT)));
            break;
        default:break;
        }
        if (args.isValid()) {
            if (count != 1)
                args.setProperty(QLatin1String(STEPCOUNT),QJSValue(count));
            jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
        }
    }
    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::evaluate(QString expr, bool global, bool disableBreak, int frame, const QVariantMap &/*addContext*/)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "evaluate",
    //      "arguments" : { "expression"    : <expression to evaluate>,
    //                      "frame"         : <number>,
    //                      "global"        : <boolean>,
    //                      "disable_break" : <boolean>,
    //                      "additional_context" : [
    //                           { "name" : <name1>, "handle" : <handle1> },
    //                           { "name" : <name2>, "handle" : <handle2> },
    //                           ...
    //                      ]
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(EVALUATE)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);
    args.setProperty(QLatin1String(EXPRESSION),QJSValue(expr));

    if (frame != -1)
        args.setProperty(QLatin1String(FRAME),QJSValue(frame));

    if (global)
        args.setProperty(QLatin1String(GLOBAL),QJSValue(global));

    if (disableBreak)
        args.setProperty(QLatin1String(DISABLEBREAK),QJSValue(disableBreak));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::lookup(QList<int> handles, bool includeSource)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "lookup",
    //      "arguments" : { "handles"       : <array of handles>,
    //                      "includeSource" : <boolean indicating whether the source will be included when script objects are returned>,
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(LOOKUP)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

    QString arr("[]");
    QJSValue array = parser.call(QJSValue(), QJSValueList() << arr);
    int index = 0;
    foreach (int handle, handles) {
        array.setProperty(index++,QJSValue(handle));
    }
    args.setProperty(QLatin1String(HANDLES),array);

    if (includeSource)
        args.setProperty(QLatin1String(INCLUDESOURCE),QJSValue(includeSource));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::backtrace(int fromFrame, int toFrame, bool bottom)
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
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(BACKTRACE)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

    if (fromFrame != -1)
        args.setProperty(QLatin1String(FROMFRAME),QJSValue(fromFrame));

    if (toFrame != -1)
        args.setProperty(QLatin1String(TOFRAME),QJSValue(toFrame));

    if (bottom)
        args.setProperty(QLatin1String(BOTTOM),QJSValue(bottom));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::frame(int number)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "frame",
    //      "arguments" : { "number" : <frame number>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(FRAME)));

    if (number != -1) {
        QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);
        args.setProperty(QLatin1String(NUMBER),QJSValue(number));

        if (args.isValid()) {
            jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
        }
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::scope(int number, int frameNumber)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scope",
    //      "arguments" : { "number" : <scope number>
    //                      "frameNumber" : <frame number, optional uses selected frame if missing>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(SCOPE)));

    if (number != -1) {
        QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);
        args.setProperty(QLatin1String(NUMBER),QJSValue(number));

        if (frameNumber != -1)
            args.setProperty(QLatin1String(FRAMENUMBER),QJSValue(frameNumber));

        if (args.isValid()) {
            jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
        }
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::scopes(int frameNumber)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scopes",
    //      "arguments" : { "frameNumber" : <frame number, optional uses selected frame if missing>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(SCOPES)));

    if (frameNumber != -1) {
        QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);
        args.setProperty(QLatin1String(FRAMENUMBER),QJSValue(frameNumber));

        if (args.isValid()) {
            jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
        }
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::scripts(int types, QList<int> ids, bool includeSource, QVariant /*filter*/)
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
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(SCRIPTS)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);
    args.setProperty(QLatin1String(TYPES),QJSValue(types));

    if (ids.count()) {
        QString arr("[]");
        QJSValue array = parser.call(QJSValue(), QJSValueList() << arr);
        int index = 0;
        foreach (int id, ids) {
            array.setProperty(index++,QJSValue(id));
        }
        args.setProperty(QLatin1String(IDS),array);
    }

    if (includeSource)
        args.setProperty(QLatin1String(INCLUDESOURCE),QJSValue(includeSource));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::source(int frame, int fromLine, int toLine)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "source",
    //      "arguments" : { "frame"    : <frame number (default selected frame)>
    //                      "fromLine" : <from line within the source default is line 0>
    //                      "toLine"   : <to line within the source this line is not included in
    //                                    the result default is the number of lines in the script>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(SOURCE)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

    if (frame != -1)
        args.setProperty(QLatin1String(FRAME),QJSValue(frame));

    if (fromLine != -1)
        args.setProperty(QLatin1String(FROMLINE),QJSValue(fromLine));

    if (toLine != -1)
        args.setProperty(QLatin1String(TOLINE),QJSValue(toLine));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::setBreakpoint(QString type, QString target, int line, int column, bool enabled, QString condition, int ignoreCount)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setbreakpoint",
    //      "arguments" : { "type"        : <"function" or "script" or "scriptId" or "scriptRegExp">
    //                      "target"      : <function expression or script identification>
    //                      "line"        : <line in script or function>
    //                      "column"      : <character position within the line>
    //                      "enabled"     : <initial enabled state. True or false, default is true>
    //                      "condition"   : <string with break point condition>
    //                      "ignoreCount" : <number specifying the number of break point hits to ignore, default value is 0>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(SETBREAKPOINT)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

    args.setProperty(QLatin1String(TYPE),QJSValue(type));
    args.setProperty(QLatin1String(TARGET),QJSValue(target));

    if (line != -1)
        args.setProperty(QLatin1String(LINE),QJSValue(line));

    if (column != -1)
        args.setProperty(QLatin1String(COLUMN),QJSValue(column));

    args.setProperty(QLatin1String(ENABLED),QJSValue(enabled));

    if (!condition.isEmpty())
        args.setProperty(QLatin1String(CONDITION),QJSValue(condition));

    if (ignoreCount != -1)
        args.setProperty(QLatin1String(IGNORECOUNT),QJSValue(ignoreCount));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::changeBreakpoint(int breakpoint, bool enabled, QString condition, int ignoreCount)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "changebreakpoint",
    //      "arguments" : { "breakpoint"  : <number of the break point to clear>
    //                      "enabled"     : <initial enabled state. True or false, default is true>
    //                      "condition"   : <string with break point condition>
    //                      "ignoreCount" : <number specifying the number of break point hits            }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(CHANGEBREAKPOINT)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

    args.setProperty(QLatin1String(BREAKPOINT),QJSValue(breakpoint));

    args.setProperty(QLatin1String(ENABLED),QJSValue(enabled));

    if (!condition.isEmpty())
        args.setProperty(QLatin1String(CONDITION),QJSValue(condition));

    if (ignoreCount != -1)
        args.setProperty(QLatin1String(IGNORECOUNT),QJSValue(ignoreCount));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::clearBreakpoint(int breakpoint)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "clearbreakpoint",
    //      "arguments" : { "breakpoint" : <number of the break point to clear>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(CLEARBREAKPOINT)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

    args.setProperty(QLatin1String(BREAKPOINT),QJSValue(breakpoint));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::setExceptionBreak(Exception type, bool enabled)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setexceptionbreak",
    //      "arguments" : { "type"    : <string: "all", or "uncaught">,
    //                      "enabled" : <optional bool: enables the break type if true>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(SETEXCEPTIONBREAK)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

    if (type == All)
        args.setProperty(QLatin1String(TYPE),QJSValue(QLatin1String(ALL)));
    else if (type == Uncaught)
        args.setProperty(QLatin1String(TYPE),QJSValue(QLatin1String(UNCAUGHT)));

    if (enabled)
        args.setProperty(QLatin1String(ENABLED),QJSValue(enabled));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::v8flags(QString flags)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "v8flags",
    //      "arguments" : { "flags" : <string: a sequence of v8 flags just like those used on the command line>
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(V8FLAGS)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

    args.setProperty(QLatin1String(FLAGS),QJSValue(flags));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::version()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "version",
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(VERSION)));

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

//void QJSDebugClient::profile(ProfileCommand command)
//{
////    { "seq"       : <number>,
////      "type"      : "request",
////      "command"   : "profile",
////      "arguments" : { "command"  : "resume" or "pause" }
////    }
//    VARIANTMAPINIT;
//    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(PROFILE)));

//    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

//    if (command == Resume)
//        args.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(RESUME)));
//    else
//        args.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(PAUSE)));

//    args.setProperty(QLatin1String("modules"),QJSValue(1));
//    if (args.isValid()) {
//        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
//    }

//    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
//    sendMessage(packMessage(json.toString().toUtf8()));
//}

void QJSDebugClient::disconnect()
{
    //    { "seq"     : <number>,
    //      "type"    : "request",
    //      "command" : "disconnect",
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(DISCONNECT)));

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::gc()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "gc",
    //      "arguments" : { "type" : <string: "all">,
    //                    }
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(GARBAGECOLLECTOR)));

    QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

    args.setProperty(QLatin1String(TYPE),QJSValue(QLatin1String(ALL)));

    if (args.isValid()) {
        jsonVal.setProperty(QLatin1String(ARGUMENTS),args);
    }

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::listBreakpoints()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "listbreakpoints",
    //    }
    VARIANTMAPINIT;
    jsonVal.setProperty(QLatin1String(COMMAND),QJSValue(QLatin1String(LISTBREAKPOINTS)));

    QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
    sendMessage(packMessage(json.toString().toUtf8()));
}

void QJSDebugClient::statusChanged(Status status)
{
    if (status == Enabled) {
        flushSendBuffer();
        emit enabled();
    }
}

void QJSDebugClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    QByteArray command;
    ds >> command;

    if (command == "V8DEBUG") {
        ds >> response;
        QString jsonString(response);
        QVariantMap value = parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();
        QString type = value.value("type").toString();

        if (type == "response") {

            if (!value.value("success").toBool()) {
//                qDebug() << "Error: The test case will fail since no signal is emitted";
                return;
            }

            QString debugCommand(value.value("command").toString());
            if (debugCommand == "backtrace" ||
                    debugCommand == "lookup" ||
                    debugCommand == "setbreakpoint" ||
                    debugCommand == "evaluate" ||
                    debugCommand == "listbreakpoints" ||
                    debugCommand == "version" ||
                    debugCommand == "v8flags" ||
                    debugCommand == "disconnect" ||
                    debugCommand == "gc" ||
                    debugCommand == "changebreakpoint" ||
                    debugCommand == "clearbreakpoint" ||
                    debugCommand == "frame" ||
                    debugCommand == "scope" ||
                    debugCommand == "scopes" ||
                    debugCommand == "scripts" ||
                    debugCommand == "source" ||
                    debugCommand == "setexceptionbreak" /*||
                    debugCommand == "profile"*/) {
                emit result();

            } else {
                // DO NOTHING
            }

        } else if (type == "event") {
            QString event(value.value("event").toString());

            if (event == "break" ||
                    event == "exception") {
                emit stopped();
            }
        }
    }
}

void QJSDebugClient::sendMessage(const QByteArray &msg)
{
    if (status() == Enabled) {
        QDeclarativeDebugClient::sendMessage(msg);
    } else {
        sendBuffer.append(msg);
    }
}

void QJSDebugClient::flushSendBuffer()
{
    foreach (const QByteArray &msg, sendBuffer)
        QDeclarativeDebugClient::sendMessage(msg);
    sendBuffer.clear();
}

QByteArray QJSDebugClient::packMessage(QByteArray message)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "V8DEBUG";
    rs << cmd << message;
    return reply;
}

void tst_QDeclarativeDebugJS::initTestCase()
{
    process = 0;
    client = 0;
    connection = 0;
}

void tst_QDeclarativeDebugJS::cleanupTestCase()
{
    if (process) {
        process->stop();
        delete process;
    }

    if (client)
        delete client;

    if (connection)
        delete connection;
}

bool tst_QDeclarativeDebugJS::init(const QString &qmlFile, bool blockMode)
{
    connection = new QDeclarativeDebugConnection();
    process = new QDeclarativeDebugProcess(QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmlscene");
    client = new QJSDebugClient(connection);

    if (blockMode)
        process->start(QStringList() << QLatin1String(BLOCKMODE) << TESTDATA(qmlFile));
    else
        process->start(QStringList() << QLatin1String(NORMALMODE) << TESTDATA(qmlFile));

    if (!process->waitForSessionStart()) {
        return false;
    }

    connection->connectToHost("127.0.0.1", 3771);
    if (!connection->waitForConnected())
        return false;

    return QDeclarativeDebugTest::waitForSignal(client, SIGNAL(enabled()));
}

void tst_QDeclarativeDebugJS::cleanup()
{
    if (process) {
        process->stop();
        delete process;
    }

    if (client)
        delete client;

    if (connection)
        delete connection;

    process = 0;
    client = 0;
    connection = 0;
}

void tst_QDeclarativeDebugJS::getVersion()
{
    //void version()

    QVERIFY(init());
    client->interrupt();
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->version();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

void tst_QDeclarativeDebugJS::getVersionWhenAttaching()
{
    //void version()

    QVERIFY(init(QLatin1String(TIMER_QMLFILE), false));
    client->interrupt();
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->version();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

void tst_QDeclarativeDebugJS::applyV8Flags()
{
    //void v8flags(QString flags)

    QVERIFY(init());
    client->interrupt();
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->v8flags(QString());
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

void tst_QDeclarativeDebugJS::disconnect()
{
    //void disconnect()

    QVERIFY(init());
    client->interrupt();
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->disconnect();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

void tst_QDeclarativeDebugJS::gc()
{
    //void gc()

    QVERIFY(init());
    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_JSFILE), 43, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->gc();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

void tst_QDeclarativeDebugJS::listBreakpoints()
{
    //void listBreakpoints()

    int sourceLine1 = 57;
    int sourceLine2 = 60;
    int sourceLine3 = 67;

    QVERIFY(init());
    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine1, -1, true);
    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine2, -1, true);
    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_JSFILE), sourceLine3, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->listBreakpoints();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QList<QVariant> breakpoints = value.value("body").toMap().value("breakpoints").toList();

    QCOMPARE(breakpoints.count(), 3);
}

void tst_QDeclarativeDebugJS::setBreakpointInScriptOnCompleted()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)

    int sourceLine = 49;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_QMLFILE));
}

void tst_QDeclarativeDebugJS::setBreakpointInScriptOnTimerCallback()
{
    int sourceLine = 49;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)
    sourceLine = 67;
    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->evaluate("timer.running = true");
    client->continueDebugging(QJSDebugClient::Continue);

    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_QMLFILE));
}

void tst_QDeclarativeDebugJS::setBreakpointInScriptInDifferentFile()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)

    int sourceLine = 43;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_JSFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_JSFILE));
}

void tst_QDeclarativeDebugJS::setBreakpointInScriptOnComment()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)

    int sourceLine = 48;
    int actualLine = 50;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_JSFILE), sourceLine, -1, true);
    client->startDebugging();
    QEXPECT_FAIL("", "Relocation of breakpoints is disabled right now", Abort);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), actualLine);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_JSFILE));
}

void tst_QDeclarativeDebugJS::setBreakpointInScriptOnEmptyLine()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)

    int sourceLine = 49;
    int actualLine = 50;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_JSFILE), sourceLine, -1, true);
    client->startDebugging();
    QEXPECT_FAIL("", "Relocation of breakpoints is disabled right now", Abort);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), actualLine);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_JSFILE));
}

void tst_QDeclarativeDebugJS::setBreakpointInScriptWithCondition()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)

    int out = 10;
    int sourceLine = 51;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_JSFILE), sourceLine, -1, true, QLatin1String("out > 10"));
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    //Get the frame index
    QString jsonString = client->response;
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    int frameIndex = body.value("index").toInt();

    //Verify the value of 'result'
    client->evaluate(QLatin1String("out"),frameIndex);

    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    jsonString = client->response;
    value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    body = value.value("body").toMap();

    QVERIFY(body.value("value").toInt() > out);
}

void tst_QDeclarativeDebugJS::setBreakpointWhenAttaching()
{
    int sourceLine = 49;
    QVERIFY(init(QLatin1String(TIMER_QMLFILE), false));

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TIMER_QMLFILE), sourceLine);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));
}

//void tst_QDeclarativeDebugJS::setBreakpointInFunction()
//{
//    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)

//    int actualLine = 31;

//    client->startDebugging();
//    client->setBreakpoint(QLatin1String(FUNCTION), QLatin1String("doSomethingElse"), -1, -1, true);

//    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

//    QString jsonString(client->response);
//    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

//    QVariantMap body = value.value("body").toMap();

//    QCOMPARE(body.value("sourceLine").toInt(), actualLine);
//    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(QMLFILE));
//}

void tst_QDeclarativeDebugJS::changeBreakpoint()
{
    //void changeBreakpoint(int breakpoint, bool enabled = false, QString condition = QString(), int ignoreCount = -1)

    int sourceLine1 = 78;
    int sourceLine2 = 79;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine1, -1, true);
    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine2, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    //Will hit 1st brakpoint, change this breakpoint enable = false
    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();
    QList<QVariant> breakpointsHit = body.value("breakpoints").toList();

    int breakpoint = breakpointsHit.at(0).toInt();
    client->changeBreakpoint(breakpoint,false);

    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    //Continue with debugging
    client->continueDebugging(QJSDebugClient::Continue);
    //Hit 2nd breakpoint
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    // start timer
    client->evaluate("timer.running = true");

    //Continue with debugging
    client->continueDebugging(QJSDebugClient::Continue);
    //Should stop at 2nd breakpoint
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    jsonString = client->response;
    value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine2);
}

void tst_QDeclarativeDebugJS::changeBreakpointOnCondition()
{
    //void changeBreakpoint(int breakpoint, bool enabled = false, QString condition = QString(), int ignoreCount = -1)

    int sourceLine1 = 56;
    int sourceLine2 = 60;
    int result = 0;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine1, -1, true);
    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine2, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    //Will hit 1st brakpoint, change this breakpoint enable = false
    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();
    QList<QVariant> breakpointsHit = body.value("breakpoints").toList();

    int breakpoint = breakpointsHit.at(0).toInt();
    client->changeBreakpoint(breakpoint,false,QLatin1String("a = 0"));

    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    //Continue with debugging
    client->continueDebugging(QJSDebugClient::Continue);
    //Hit 2nd breakpoint
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));
    // start timer
    client->evaluate("timer.running = true");
    //Continue with debugging
    client->continueDebugging(QJSDebugClient::Continue);
    //Should stop at 2nd breakpoint
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    jsonString = client->response;
    value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine2);

    client->frame();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    //Get the frame index
    jsonString = client->response;
    value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    body = value.value("body").toMap();

    int frameIndex = body.value("index").toInt();

    //Verify the value of 'result'
    client->evaluate(QLatin1String("root.result"),frameIndex);

    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    jsonString = client->response;
    value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    body = value.value("body").toMap();

    QVERIFY(body.value("value").toInt() > result);
}

void tst_QDeclarativeDebugJS::clearBreakpoint()
{
    //void clearBreakpoint(int breakpoint);

    int sourceLine1 = 78;
    int sourceLine2 = 79;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine1, -1, true);
    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine2, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    //Will hit 1st brakpoint, change this breakpoint enable = false
    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();
    QList<QVariant> breakpointsHit = body.value("breakpoints").toList();

    int breakpoint = breakpointsHit.at(0).toInt();
    client->changeBreakpoint(breakpoint,false,QLatin1String("result > 5"));

    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    //Continue with debugging
    client->continueDebugging(QJSDebugClient::Continue);
    //Hit 2nd breakpoint
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));
    // start timer
    client->evaluate("timer.running = true");
    //Continue with debugging
    client->continueDebugging(QJSDebugClient::Continue);
    //Should stop at 2nd breakpoint
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    jsonString = client->response;
    value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine2);
}

void tst_QDeclarativeDebugJS::setExceptionBreak()
{
    //void setExceptionBreak(QString type, bool enabled = false);

    int sourceLine = 49;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->setExceptionBreak(QJSDebugClient::All,true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));
    client->evaluate("root.raiseException = true");
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
    client->continueDebugging(QJSDebugClient::Continue);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped()), 10000));
}

void tst_QDeclarativeDebugJS::stepNext()
{
    //void continueDebugging(StepAction stepAction, int stepCount = 1);

    int sourceLine = 57;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->continueDebugging(QJSDebugClient::Next);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped()), 10000));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine + 1);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_QMLFILE));
}

void tst_QDeclarativeDebugJS::stepNextWithCount()
{
    //void continueDebugging(StepAction stepAction, int stepCount = 1);

    int sourceLine = 59;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->continueDebugging(QJSDebugClient::Next,2);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped()), 10000));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine + 2);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_QMLFILE));
}

void tst_QDeclarativeDebugJS::stepIn()
{
    //void continueDebugging(StepAction stepAction, int stepCount = 1);

    int sourceLine = 61;
    int actualLine = 78;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->continueDebugging(QJSDebugClient::In);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped()), 10000));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), actualLine);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_QMLFILE));
}

void tst_QDeclarativeDebugJS::stepOut()
{
    //void continueDebugging(StepAction stepAction, int stepCount = 1);

    int sourceLine = 56;
    int actualLine = 49;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->continueDebugging(QJSDebugClient::Out);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped()), 10000));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), actualLine);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_QMLFILE));
}

void tst_QDeclarativeDebugJS::continueDebugging()
{
    //void continueDebugging(StepAction stepAction, int stepCount = 1);

    int sourceLine1 = 56;
    int sourceLine2 = 60;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine1, -1, true);
    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine2, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->continueDebugging(QJSDebugClient::Continue);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped()), 10000));

    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine2);
    QCOMPARE(QFileInfo(body.value("script").toMap().value("name").toString()).fileName(), QLatin1String(TEST_QMLFILE));
}

void tst_QDeclarativeDebugJS::backtrace()
{
    //void backtrace(int fromFrame = -1, int toFrame = -1, bool bottom = false);

    int sourceLine = 60;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->backtrace();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

void tst_QDeclarativeDebugJS::getFrameDetails()
{
    //void frame(int number = -1);

    int sourceLine = 60;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->frame();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

void tst_QDeclarativeDebugJS::getScopeDetails()
{
    //void scope(int number = -1, int frameNumber = -1);

    int sourceLine = 60;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->scope();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

void tst_QDeclarativeDebugJS::evaluateInGlobalScope()
{
    //void evaluate(QString expr, bool global = false, bool disableBreak = false, int frame = -1, const QVariantMap &addContext = QVariantMap());

    int sourceLine = 49;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->evaluate(QLatin1String("print('Hello World')"),true);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    //Verify the value of 'print'
    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    QCOMPARE(body.value("text").toString(),QLatin1String("undefined"));
}

void tst_QDeclarativeDebugJS::evaluateInLocalScope()
{
    //void evaluate(QString expr, bool global = false, bool disableBreak = false, int frame = -1, const QVariantMap &addContext = QVariantMap());

    int sourceLine = 60;
    QVERIFY(init());

    client->setBreakpoint(QLatin1String(SCRIPT), QLatin1String(TEST_QMLFILE), sourceLine, -1, true);
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->frame();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    //Get the frame index
    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    QVariantMap body = value.value("body").toMap();

    int frameIndex = body.value("index").toInt();

    client->evaluate(QLatin1String("root.someValue"),frameIndex);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));

    //Verify the value of 'root.someValue'
    jsonString = client->response;
    value = client->parser.call(QJSValue(), QJSValueList() << QJSValue(jsonString)).toVariant().toMap();

    body = value.value("body").toMap();

    QCOMPARE(body.value("value").toInt(),10);
}

void tst_QDeclarativeDebugJS::getScopes()
{
    //void scopes(int frameNumber = -1);

    QVERIFY(init());

    client->interrupt();
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->scopes();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

void tst_QDeclarativeDebugJS::getScripts()
{
    //void scripts(int types = -1, QList<int> ids = QList<int>(), bool includeSource = false, QVariant filter = QVariant());

    QVERIFY(init(QLatin1String(TIMER_QMLFILE), true));

    client->interrupt();
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->scripts();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
    QString jsonString(client->response);
    QVariantMap value = client->parser.call(QJSValue(),
                                            QJSValueList()
                                            << QJSValue(jsonString)).toVariant().toMap();

    QList<QVariant> scripts = value.value("body").toList();

    QCOMPARE(scripts.count(), 2);
}

void tst_QDeclarativeDebugJS::getSource()
{
    //void source(int frame = -1, int fromLine = -1, int toLine = -1);

    QVERIFY(init());

    client->interrupt();
    client->startDebugging();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(stopped())));

    client->source();
    QVERIFY(QDeclarativeDebugTest::waitForSignal(client, SIGNAL(result())));
}

QTEST_MAIN(tst_QDeclarativeDebugJS)

#include "tst_qdeclarativedebugjs.moc"

