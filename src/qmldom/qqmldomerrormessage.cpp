// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldomitem_p.h"
#include "qqmldomstringdumper_p.h"
#include "qqmldomattachedinfo_p.h"

#include <QtCore/QCborMap>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

Q_LOGGING_CATEGORY(domLog, "qt.qmldom", QtWarningMsg);

enum {
    FatalMsgMaxLen=511
};

/*!
\internal
\macro NewErrorGroup

\param groupId a double qouted string giving the groupId for this group

\brief convenience macro creating a new ErrorGroup and registering its groupId as translatable string
*/

/*!
\internal
\class QQmlJS::Dom::ErrorGroup
\brief Represents a tag grouping a set of related error messages, it can be used to disable them

Every group has a unique string identifying it (the \l{groupId}), and it should be a string that can
be translated to get the local name. The best way to acheive this is to create new groups using
the NewErrorGroup macro.
 */
void ErrorGroup::dump(Sink sink) const
{
    sink(u"[");
    sink(groupName());
    sink(u"]");
}

void ErrorGroup::dumpId(Sink sink) const
{
    sink(u"[");
    sink(QString(groupId()));
    sink(u"]");
}

QLatin1String ErrorGroup::groupId() const
{
    return QLatin1String(m_groupId);
}

QString ErrorGroup::groupName() const
{
    return tr(m_groupId);
}

/*!
\internal
\class QQmlJS::Dom::ErrorGroups
\brief Represents a set of tags grouping a set of related error messages

The simplest way to create new ErrorMessages is to have an ErrorGroups instance,
and use it to create new ErrorMessages using its debug, warning, error,... methods
 */

void ErrorGroups::dump(Sink sink) const
{
    for (int i = 0; i < groups.size(); ++i)
        groups.at(i).dump(sink);
}

void ErrorGroups::dumpId(Sink sink) const
{
    for (int i = 0; i < groups.size(); ++i)
        groups.at(i).dumpId(sink);
}

QCborArray ErrorGroups::toCbor() const
{
    QCborArray res;
    for (int i = 0; i < groups.size(); ++i)
        res.append(QCborValue(groups.at(i).groupId()));
    return res;
}

/*!
\internal
\class QQmlJS::Dom::ErrorMessage
\brief Represents an error message connected to the dom

The error messages *should* be translated, but they do not need to be pre registered.
To give a meaningful handling of error messages ErrorMessages have "tags" (ErrorGroup) that are
grouped toghether in ErrorGroups.

To create an ErrorMessage from scratch the best way is to use one of the methods provided by
an ErrorGroups object.
For example create an ErrorGroups called myErrors and use it to create all your errors.
\code
static ErrorGroups myErrors(){
    static ErrorGroups res({NewErrorGroup("StaticAnalysis"), NewErrorGroup("FancyDetector")});
    return res;
}
\endcode

You can  preregister the errors giving them a unique name (reverse dns notation is encouraged) with
the msg function.
This unique name (errorId) is a const char* (QLatin1String) to integrate better with the tr function.
Ideally you create variables to store the errorId either by creating variables with plain strings
that you use to initialize the error messages
\code
// in .h file
constexpr const char *myError0 = "my.company.error0";
// in some initialization function
ErrorMessage::msg(myError0, myErrors().warning(tr("Error number 0")));
\endcode
or using the result of the msg function
\code
// in cpp file
static auto myError1 = ErrorMessage::msg("my.company.error1", myErrors().warning(tr("Error number 1")));
static auto myError2 = ErrorMessage::msg("my.company.error2", myErrors().error(tr("Error number 2 on %1")));
\endcode
and then use them like this
\code
ErrorMessage::load(myError2, QLatin1String("extra info")).handle(errorHandler);
\endcode
or using directly the string (more error prone)
\code
errorHandler(ErrorMessage::load(QLatin1String("my.company.error1")));
\endcode

The \l{withItem} method can be used to set the path file and location if not aready set.
 */

ErrorMessage ErrorGroups::errorMessage(Dumper msg, ErrorLevel level, Path element, QString canonicalFilePath, SourceLocation location) const
{
    if (level == ErrorLevel::Fatal)
        fatal(msg, element, canonicalFilePath, location);
    return ErrorMessage(dumperToString(msg), *this, level, element, canonicalFilePath, location);
}

ErrorMessage ErrorGroups::errorMessage(const DiagnosticMessage &msg, Path element, QString canonicalFilePath) const
{
    ErrorMessage res(*this, msg, element, canonicalFilePath);
    if (res.location == SourceLocation()
        && (res.location.startLine != 0 || res.location.startColumn != 0)) {
        res.location.offset = -1;
        res.location.length = 1;
    }
    return res;
}

void ErrorGroups::fatal(Dumper msg, Path element, QStringView canonicalFilePath, SourceLocation location) const
{
    enum { FatalMsgMaxLen = 1023 };
    char buf[FatalMsgMaxLen+1];
    int ibuf = 0;
    auto sink = [&ibuf, &buf](QStringView s) {
        int is = 0;
        while (ibuf < FatalMsgMaxLen && is < s.size()) {
            QChar c = s.at(is);
            if (c == QChar::fromLatin1('\n') || c == QChar::fromLatin1('\r') || (c >= QChar::fromLatin1(' ') && c <= QChar::fromLatin1('~')))
                buf[ibuf++] = c.toLatin1();
            else
                buf[ibuf++] = '~';
            ++is;
        }
    };
    if (!canonicalFilePath.isEmpty()) {
        sink(canonicalFilePath);
        sink(u":");
    }
    if (location.length) {
        sinkInt(sink, location.startLine);
        sink(u":");
        sinkInt(sink, location.startColumn);
        sink(u":");
    }
    dump(sink);
    msg(sink);
    if (element.length()>0) {
        sink(u" for ");
        element.dump(sink);
    }
    buf[ibuf] = 0;
    qFatal("%s", buf);
}

ErrorMessage ErrorGroups::debug(QString message) const
{
    return ErrorMessage(message, *this, ErrorLevel::Debug);
}

ErrorMessage ErrorGroups::debug(Dumper message) const
{
    return ErrorMessage(dumperToString(message), *this, ErrorLevel::Debug);
}

ErrorMessage ErrorGroups::info(QString message) const
{
    return ErrorMessage(message, *this, ErrorLevel::Info);
}

ErrorMessage ErrorGroups::info(Dumper message) const
{
    return ErrorMessage(dumperToString(message), *this, ErrorLevel::Info);
}

ErrorMessage ErrorGroups::warning(QString message) const
{
    return ErrorMessage(message, *this, ErrorLevel::Warning);
}

ErrorMessage ErrorGroups::warning(Dumper message) const
{
    return ErrorMessage(dumperToString(message), *this, ErrorLevel::Warning);
}

ErrorMessage ErrorGroups::error(QString message) const
{
    return ErrorMessage(message, *this, ErrorLevel::Error);
}

ErrorMessage ErrorGroups::error(Dumper message) const
{
    return ErrorMessage(dumperToString(message), *this, ErrorLevel::Error);
}

int ErrorGroups::cmp(const ErrorGroups &o1, const ErrorGroups &o2)
{
    auto &g1 = o1.groups;
    auto &g2 = o2.groups;
    if (g1.size() < g2.size())
        return -1;
    if (g1.size() < g2.size())
        return 1;
    for (int i = 0; i < g1.size(); ++i) {
        int c = std::strcmp(g1.at(i).groupId().data(), g2.at(i).groupId().data());
        if (c != 0)
            return c;
    }
    return 0;
}

ErrorMessage::ErrorMessage(QString msg, ErrorGroups errorGroups, Level level, Path element, QString canonicalFilePath, SourceLocation location, QLatin1String errorId):
    errorId(errorId), message(msg), errorGroups(errorGroups), level(level), path(element), file(canonicalFilePath), location(location)
{
    if (level == Level::Fatal) // we should not end up here, it should have been handled at a higher level already
        errorGroups.fatal(msg, element, canonicalFilePath, location);
}

ErrorMessage::ErrorMessage(ErrorGroups errorGroups, const DiagnosticMessage &msg, Path element,
                           QString canonicalFilePath, QLatin1String errorId):
    errorId(errorId), message(msg.message), errorGroups(errorGroups),
    level(errorLevelFromQtMsgType(msg.type)), path(element), file(canonicalFilePath), location(msg.loc)
{
    if (level == Level::Fatal) // we should not end up here, it should have been handled at a higher level already
        errorGroups.fatal(msg.message, element, canonicalFilePath, location);
}


static QBasicMutex *registryMutex()
{
    static QBasicMutex rMutex{};
    return &rMutex;
}


static ErrorGroups myErrors()
{
    static ErrorGroups g = {{NewErrorGroup("ErrorMessage")}};
    return g;
}

struct StorableMsg {
    StorableMsg():
        msg(QStringLiteral(u"dummy"), myErrors(), ErrorLevel::Error)
    {}

    StorableMsg(const ErrorMessage &e):
        msg(e)
    {}

    ErrorMessage msg;
};

static QHash<QLatin1String, StorableMsg> &registry()
{
    static QHash<QLatin1String, StorableMsg> r;
    return r;
}

QLatin1String ErrorMessage::msg(const char *errorId, ErrorMessage err)
{
    return msg(QLatin1String(errorId), err);
}

QLatin1String ErrorMessage::msg(QLatin1String errorId, ErrorMessage err)
{
    bool doubleRegister = false;
    ErrorMessage old = myErrors().debug(u"dummy");
    {
        QMutexLocker l(registryMutex());
        auto &r = registry();
        if (r.contains(err.errorId)) {
            old = r[err.errorId].msg;
            doubleRegister = true;
        }
        r[errorId] = StorableMsg{err.withErrorId(errorId)};
    }
    if (doubleRegister)
        defaultErrorHandler(myErrors().warning(tr("Double registration of error %1: (%2) vs (%3)").arg(errorId, err.withErrorId(errorId).toString(), old.toString())));
    return errorId;
}

void ErrorMessage::visitRegisteredMessages(function_ref<bool (ErrorMessage)> visitor)
{
    QHash<QLatin1String, StorableMsg> r;
    {
        QMutexLocker l(registryMutex());
        r = registry();
    }
    auto it = r.cbegin();
    auto end = r.cend();
    while (it != end) {
        visitor(it->msg);
        ++it;
    }
}

ErrorMessage ErrorMessage::load(QLatin1String errorId)
{
    ErrorMessage res = myErrors().error([errorId](Sink s){
            s(u"Unregistered error ");
            s(QString(errorId)); });
    {
        QMutexLocker l(registryMutex());
        res = registry().value(errorId,res).msg;
    }
    return res;
}

ErrorMessage ErrorMessage::load(const char *errorId)
{
    return load(QLatin1String(errorId));
}

ErrorMessage &ErrorMessage::withErrorId(QLatin1String errorId)
{
    this->errorId = errorId;
    return *this;
}

ErrorMessage &ErrorMessage::withPath(const Path &path)
{
    this->path = path;
    return *this;
}

ErrorMessage &ErrorMessage::withFile(QString f)
{
    file=f;
    return *this;
}

ErrorMessage &ErrorMessage::withFile(QStringView f)
{
    file = f.toString();
    return *this;
}

ErrorMessage &ErrorMessage::withLocation(SourceLocation loc)
{
    location = loc;
    return *this;
}

ErrorMessage &ErrorMessage::withItem(DomItem el)
{
    if (path.length() == 0)
        path = el.canonicalPath();
    if (file.isEmpty())
        file = el.canonicalFilePath();
    if (location == SourceLocation()) {
        if (const FileLocations *fLocPtr = FileLocations::fileLocationsOf(el)) {
            location = fLocPtr->regions.value(QString(), fLocPtr->fullRegion);
        }
    }
    return *this;
}

ErrorMessage ErrorMessage::handle(const ErrorHandler &errorHandler)
{
    if (errorHandler)
        errorHandler(*this);
    else
        defaultErrorHandler(*this);
    return *this;
}

void ErrorMessage::dump(Sink sink) const
{
    if (!file.isEmpty()) {
        sink(file);
        sink(u":");
    }
    if (location.length) {
        sinkInt(sink, location.startLine);
        sink(u":");
        sinkInt(sink, location.startColumn);
        sink(u": ");
    }
    errorGroups.dump(sink);
    sink(u" ");
    dumpErrorLevel(sink, level);
    if (! errorId.isEmpty()) {
        sink(u" ");
        sink(QString(errorId));
    }
    sink(u": ");
    sink(message);
    if (path.length()>0) {
        sink(u" for ");
        if (!file.isEmpty() && path.length() > 3 && path.headKind() == Path::Kind::Root)
            path.mid(3).dump(sink);
        else
            path.dump(sink);
    }
}

QString ErrorMessage::toString() const
{
    return dumperToString([this](Sink sink){ this->dump(sink); });
}

QCborMap ErrorMessage::toCbor() const
{
    return QCborMap({
                        {QStringLiteral(u"errorId"),errorId},
                        {QStringLiteral(u"message"), message},
                        {QStringLiteral(u"errorGroups"), errorGroups.toCbor()},
                        {QStringLiteral(u"level"), int(level)},
                        {QStringLiteral(u"path"), path.toString()},
                        {QStringLiteral(u"file"), file},
                        {QStringLiteral(u"location"), QCborMap({
                             {QStringLiteral(u"offset"),location.offset},
                             {QStringLiteral(u"length"),location.length},
                             {QStringLiteral(u"startLine"),location.startLine},
                             {QStringLiteral(u"startColumn"),location.startColumn}})}
                    });
}

/*!
 * \internal
 * \brief writes an ErrorMessage to QDebug
 * \param error the error to write
 */
void errorToQDebug(const ErrorMessage &error)
{
    dumperToQDebug([&error](Sink s){ error.dump(s); }, error.level);
}

/*!
 * \internal
 * \brief Error handler that ignores all errors (excluding fatal ones)
 */
void silentError(const ErrorMessage &)
{
}

void errorHandlerHandler(const ErrorMessage &msg, ErrorHandler *h = nullptr)
{
    static ErrorHandler handler = &errorToQDebug;
    if (h) {
        handler = *h;
    } else {
        handler(msg);
    }
}

/*!
 * \internal
 * \brief Calls the default error handler (by default errorToQDebug)
 */
void defaultErrorHandler(const ErrorMessage &error)
{
    errorHandlerHandler(error);
}

/*!
 * \internal
 * \brief Sets the default error handler
 */
void setDefaultErrorHandler(ErrorHandler h)
{
    errorHandlerHandler(ErrorMessage(QString(), ErrorGroups({})), &h);
}

ErrorLevel errorLevelFromQtMsgType(QtMsgType msgType)
{
    switch (msgType) {
    case QtFatalMsg:
        return ErrorLevel::Fatal;
    case QtCriticalMsg:
        return ErrorLevel::Error;
    case QtWarningMsg:
        return ErrorLevel::Warning;
    case QtInfoMsg:
        return ErrorLevel::Info;
    case QtDebugMsg:
        return ErrorLevel::Debug;
    default:
        return ErrorLevel::Error;
    }
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE

#include "moc_qqmldomerrormessage_p.cpp"
