/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativetrace_p.h"

#ifdef QML_ENABLE_TRACE
#include <stdio.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef QML_ENABLE_TRACE

QDeclarativeTrace::Pool QDeclarativeTrace::logPool;
QDeclarativeTrace::Entry *QDeclarativeTrace::first = 0;
QDeclarativeTrace::Entry *QDeclarativeTrace::last = 0;

static qint64 toNsecs(QDeclarativeTrace::TimeType time)
{
#ifdef Q_OS_MAC
    static mach_timebase_info_data_t info = {0,0};
    if (info.denom == 0)
        mach_timebase_info(&info);
    return time * info.numer / info.denom;
#else
    qint64 rv = time.tv_sec * 1000000000 + time.tv_nsec;
    return rv;
#endif
}

QDeclarativeTrace::Pool::Pool()
{
    first = New<Entry>();
    last = first;
}

QDeclarativeTrace::Pool::~Pool()
{
    char buffer[128];
    sprintf(buffer, "qml.%d.log", ::getpid());
    FILE *out = fopen(buffer, "w");
    if (!out) {
        fprintf (stderr, "QML Log: Could not open %s\n", buffer);
        return;
    } else {
        fprintf (stderr, "QML Log: Writing log to %s\n", buffer);
    }

    QDeclarativeTrace::Entry *cur = QDeclarativeTrace::first;
    QByteArray indent;
    int depth = -1;

    qint64 firstTime = -1;

    while (cur) {

        switch (cur->type) {
        case QDeclarativeTrace::Entry::RangeStart: {
            RangeStart *rs = static_cast<QDeclarativeTrace::RangeStart *>(cur);

            qint64 nsecs = toNsecs(rs->time);

            if (firstTime == -1)
                firstTime = nsecs;

            nsecs -= firstTime;

            depth++;
            indent = QByteArray(depth * 4, ' ');
            fprintf(out, "%s%s @%lld (%lld ns)\n", indent.constData(),
                    rs->description, nsecs, toNsecs(rs->end->time) - nsecs - firstTime);
            } break;
        case QDeclarativeTrace::Entry::RangeEnd:
            depth--;
            indent = QByteArray(depth * 4, ' ');
            break;
        case QDeclarativeTrace::Entry::Detail:
            fprintf(out, "%s  %s\n", indent.constData(),
                    static_cast<QDeclarativeTrace::Detail *>(cur)->description);
            break;
        case QDeclarativeTrace::Entry::IntDetail:
            fprintf(out, "%s  %s: %d\n", indent.constData(),
                    static_cast<QDeclarativeTrace::Detail *>(cur)->description,
                    static_cast<QDeclarativeTrace::IntDetail *>(cur)->value);
            break;
        case QDeclarativeTrace::Entry::StringDetail: {
            QByteArray vLatin1 = static_cast<QDeclarativeTrace::StringDetail *>(cur)->value->toLatin1();
            fprintf(out, "%s  %s: %s\n", indent.constData(),
                    static_cast<QDeclarativeTrace::Detail *>(cur)->description,
                    vLatin1.constData());
            } break;
        case QDeclarativeTrace::Entry::UrlDetail: {
            QByteArray vLatin1 = static_cast<QDeclarativeTrace::UrlDetail *>(cur)->value->toString().toLatin1();
            fprintf(out, "%s  %s: %s\n", indent.constData(),
                    static_cast<QDeclarativeTrace::Detail *>(cur)->description,
                    vLatin1.constData());
            } break;
        case QDeclarativeTrace::Entry::Event: {
            Event *ev = static_cast<QDeclarativeTrace::Event *>(cur);
            qint64 nsecs = toNsecs(ev->time) - firstTime;
            fprintf(out, "%s  + %s @%lld +%lld ns\n", indent.constData(),
                    ev->description, nsecs, nsecs - (toNsecs(ev->start->time) - firstTime));
            } break;
        case QDeclarativeTrace::Entry::Null:
        default:
            break;
        }
        cur = cur->next;
    }
    fclose(out);
}

#endif

QT_END_NAMESPACE

