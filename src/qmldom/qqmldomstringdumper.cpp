// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomstringdumper_p.h"
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

/*!
 * \internal
 * \tn QQmlJS::Dom::Sink
 * \brief A Sink is a function that accepts a QStringView as input
 *
 * A Sink it the core element of a text based stream oriented output.
 * It is simply a function accepting a QStringView as input.
 */

/*!
 * \internal
 * \tn QQmlJS::Dom::sinkInt
 * \brief writes an integer to a sink without any axtra heap allocation
 * \param sink where to write
 * \param i the integer to write out
 *
 * mainly for debugging/fatal errors
 */

/*!
 * \internal
 * \class QQmlJS::Dom::Dumper
 * \brief Helper class to accept eithe a string or a dumper (a function that writes to a sink)
 *
 * Using a Dumper as input parameter one always obtains a dumper (i.e. a
 * function_ref<void(function_ref<void(QStringView)>)> , but can pass in any
 * object accepted by QStringView, and it is automatically converted to a dumper.
 */

/*!
 * \internal
 * \brief Converts a dumper to a string
 * \param writer The dumper convert to a string
 */
QString dumperToString(Dumper writer)
{
    QString s;
    QTextStream d(&s);
    writer([&d](QStringView s){ d << s; });
    d.flush();
    return s;
}

/*!
 * \internal
 * \brief dumps a string as quoted string (escaping things like quotes or newlines)
 * \param sink The sink to write the quoted string to
 * \param s The string to sink
 * \param options If quotes should be outputted around the string (defaults to yes)
 */
void sinkEscaped(Sink sink, QStringView s, EscapeOptions options) {
    if (options == EscapeOptions::OuterQuotes)
        sink(u"\"");
    int it0=0;
    for (int it = 0; it < s.size();++it) {
        QChar c=s[it];
        bool noslash = c != QLatin1Char('\\');
        bool noquote = c != QLatin1Char('"');
        bool nonewline = c != QLatin1Char('\n');
        bool noreturn = c != QLatin1Char('\r');
        if (noslash && noquote && nonewline && noreturn)
            continue;
        sink(s.mid(it0, it - it0));
        it0 = it + 1;
        if (!noslash)
            sink(u"\\\\");
        else if (!noquote)
            sink(u"\\\"");
        else if (!nonewline)
            sink(u"\\n");
        else if (!noreturn)
            sink(u"\\r");
        else
            Q_ASSERT(0);
    }
    sink(s.mid(it0, s.size() - it0));
    if (options == EscapeOptions::OuterQuotes)
        sink(u"\"");
}

/*!
 * \internal
 * \brief Dumps a string describing the given error level (ErrorLevel::Error -> Error,...)
 * \param s the sink to write to
 * \param level the level to describe
 */
void dumpErrorLevel(Sink s, ErrorLevel level)
{
    switch (level) {
    case ErrorLevel::Debug:
        s(u"Debug");
        break;
    case ErrorLevel::Info:
        s(u"Info");
        break;
    case ErrorLevel::Warning:
        s(u"Warning");
        break;
    case ErrorLevel::Error:
        s(u"Error");
        break;
    case ErrorLevel::Fatal:
        s(u"Fatal");
        break;
    }

}

void dumperToQDebug(Dumper dumper, QDebug debug)
{
    QDebug & d = debug.noquote().nospace();
    dumper([&d](QStringView s){
        d << s;
    });
}

/*!
 * \internal
 * \brief writes the dumper to the QDebug object corrsponding to the given error level
 * \param level the error level of the message
 * \param dumper the dumper that writes a message
 */
void dumperToQDebug(Dumper dumper, ErrorLevel level)
{
    QDebug d = qDebug().noquote().nospace();
    switch (level) {
    case ErrorLevel::Debug:
        break;
    case ErrorLevel::Info:
        d = qInfo().noquote().nospace();
        break;
    case ErrorLevel::Warning:
        d = qWarning().noquote().nospace();
        break;
    case ErrorLevel::Error:
    case ErrorLevel::Fatal: // should be handled differently (avoid allocations...), we try to catch them before ending up here
        d = qCritical().noquote().nospace();
        break;
    }
    dumper([&d](QStringView s){
        d << s;
    });
}

/*!
 * \internal
 * \brief sinks the requested amount of spaces
 */
void sinkIndent(Sink s, int indent)
{
    if (indent > 0) {
        QStringView spaces = u"                     ";
        while (indent > spaces.size()) {
            s(spaces);
            indent -= spaces.size();
        }
        s(spaces.left(indent));
    }
}

/*!
 * \internal
 * \brief sinks a neline and indents by the given amount
 */
void sinkNewline(Sink s, int indent)
{
    s(u"\n");
    if (indent > 0)
        sinkIndent(s, indent);
}

/*!
 * \internal
 * \fn QQmlJS::Dom::devNull
 * \brief A sink that ignores whatever it receives
 */

QDebug operator<<(QDebug d, Dumper dumper)
{
    QDebug dd = d.noquote().nospace();
    dumper([&dd](QStringView s) { dd << s; });
    return d;
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
