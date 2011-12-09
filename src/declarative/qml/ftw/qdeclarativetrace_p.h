/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#ifndef QDECLARATIVETRACE_P_H
#define QDECLARATIVETRACE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <private/qdeclarativepool_p.h>

// #define QML_ENABLE_TRACE

#if defined(QML_ENABLE_TRACE) && defined(Q_OS_MAC)
#include <mach/mach_time.h>
#endif

QT_BEGIN_NAMESPACE

class QUrl;
class QDeclarativeTrace
{
public:
    inline QDeclarativeTrace(const char *desc);
    inline ~QDeclarativeTrace();

    inline void addDetail(const char *);
    inline void addDetail(const char *, int);
    inline void addDetail(const char *, const QString &);
    inline void addDetail(const char *, const QUrl &);

    inline void event(const char *desc);

#ifdef QML_ENABLE_TRACE

#ifdef Q_OS_MAC
    typedef uint64_t TimeType;
#else
    typedef timespec TimeType;
#endif

    struct Entry : public QDeclarativePool::POD {
        enum Type { Null, RangeStart, RangeEnd, Detail, IntDetail, StringDetail, UrlDetail, Event };
        inline Entry();
        inline Entry(Type);
        Type type;
        Entry *next;
    };
    struct RangeEnd : public Entry {
        inline RangeEnd();
        TimeType time;
    };
    struct RangeStart : public Entry {
        inline RangeStart();
        const char *description;
        TimeType time;
        QDeclarativeTrace::RangeEnd *end;
    };
    struct Detail : public Entry {
        inline Detail();
        inline Detail(Type t);
        const char *description;
    };
    struct IntDetail : public Detail {
        inline IntDetail();
        int value;
    };
    struct StringDetail : public Detail {
        inline StringDetail();
        QString *value;
    };
    struct UrlDetail : public Detail {
        inline UrlDetail();
        QUrl *value;
    };
    struct Event : public Entry {
        inline Event();
        const char *description;
        TimeType time;
        QDeclarativeTrace::RangeStart *start;
    };

    struct Pool : public QDeclarativePool {
        Pool();
        ~Pool();
    };

    static Pool logPool;
    static Entry *first;
    static Entry *last;

private:
    RangeStart *start;

    static TimeType gettime() {
#ifdef Q_OS_MAC
        return mach_absolute_time();
#else
        TimeType ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts;
#endif
    }
#endif
};

#ifdef QML_ENABLE_TRACE
QDeclarativeTrace::Entry::Entry()
: type(Null), next(0)
{
}

QDeclarativeTrace::Entry::Entry(Type type)
: type(type), next(0)
{
    QDeclarativeTrace::last->next = this;
    QDeclarativeTrace::last = this;
}

QDeclarativeTrace::RangeEnd::RangeEnd()
: QDeclarativeTrace::Entry(QDeclarativeTrace::Entry::RangeEnd),
  time(gettime())
{
}

QDeclarativeTrace::RangeStart::RangeStart()
: QDeclarativeTrace::Entry(QDeclarativeTrace::Entry::RangeStart),
  description(0), time(gettime())
{
}

QDeclarativeTrace::Detail::Detail()
: QDeclarativeTrace::Entry(QDeclarativeTrace::Entry::Detail),
  description(0)
{
}

QDeclarativeTrace::Detail::Detail(Type type)
: QDeclarativeTrace::Entry(type), description(0)
{
}

QDeclarativeTrace::IntDetail::IntDetail()
: QDeclarativeTrace::Detail(QDeclarativeTrace::Entry::IntDetail),
  value(0)
{
}

QDeclarativeTrace::StringDetail::StringDetail()
: QDeclarativeTrace::Detail(QDeclarativeTrace::Entry::StringDetail),
  value(0)
{
}

QDeclarativeTrace::UrlDetail::UrlDetail()
: QDeclarativeTrace::Detail(QDeclarativeTrace::Entry::UrlDetail),
  value(0)
{
}

QDeclarativeTrace::Event::Event()
: QDeclarativeTrace::Entry(QDeclarativeTrace::Entry::Event),
  description(0), time(gettime()), start(0)
{
}
#endif

QDeclarativeTrace::QDeclarativeTrace(const char *desc)
{
#ifdef QML_ENABLE_TRACE
    RangeStart *e = logPool.New<RangeStart>();
    e->description = desc;
    e->end = 0;
    start = e;
#else
    Q_UNUSED(desc);
#endif
}

QDeclarativeTrace::~QDeclarativeTrace()
{
#ifdef QML_ENABLE_TRACE
    RangeEnd *e = logPool.New<RangeEnd>();
    start->end = e;
#endif
}

void QDeclarativeTrace::addDetail(const char *desc)
{
#ifdef QML_ENABLE_TRACE
    Detail *e = logPool.New<Detail>();
    e->description = desc;
#else
    Q_UNUSED(desc);
#endif
}

void QDeclarativeTrace::addDetail(const char *desc, int v)
{
#ifdef QML_ENABLE_TRACE
    IntDetail *e = logPool.New<IntDetail>();
    e->description = desc;
    e->value = v;
#else
    Q_UNUSED(desc);
    Q_UNUSED(v);
#endif
}

void QDeclarativeTrace::addDetail(const char *desc, const QString &v)
{
#ifdef QML_ENABLE_TRACE
    StringDetail *e = logPool.New<StringDetail>();
    e->description = desc;
    e->value = logPool.NewString(v);
#else
    Q_UNUSED(desc);
    Q_UNUSED(v);
#endif
}

void QDeclarativeTrace::addDetail(const char *desc, const QUrl &v)
{
#ifdef QML_ENABLE_TRACE
    UrlDetail *e = logPool.New<UrlDetail>();
    e->description = desc;
    e->value = logPool.NewUrl(v);
#else
    Q_UNUSED(desc);
    Q_UNUSED(v);
#endif
}

void QDeclarativeTrace::event(const char *desc)
{
#ifdef QML_ENABLE_TRACE
    Event *e = logPool.New<Event>();
    e->start = start;
    e->description = desc;
#else
    Q_UNUSED(desc);
#endif
}

QT_END_NAMESPACE

#endif // QDECLARATIVETRACE_P_H
