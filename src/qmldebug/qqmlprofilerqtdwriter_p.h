// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROFILERQTDWRITER_P_H
#define QQMLPROFILERQTDWRITER_P_H

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

#include <private/qqmlprofilerclientdefinitions_p.h>
#include <private/qqmlprofilereventlocation_p.h>
#include <private/qqmlprofilereventreceiver_p.h>

#include <QObject>

QT_BEGIN_NAMESPACE

class QQmlProfilerQtdWriterPrivate;
class QQmlProfilerQtdWriter : public QQmlProfilerEventReceiver
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlProfilerQtdWriter)
public:
    explicit QQmlProfilerQtdWriter(QObject *parent = nullptr);
    ~QQmlProfilerQtdWriter();

    qsizetype numLoadedEventTypes() const final;
    qsizetype numLoadedEvents() const final;
    void addEventType(const QQmlProfilerEventType &type) final;
    void addEvent(const QQmlProfilerEvent &event) final;
    bool save(const QString &filename) final;
    void clear() final;

    void startTrace(qint64 time, const QList<int> &engineIds) final;
    void endTrace(qint64 time, const QList<int> &engineIds) final;
    void complete(qint64 maximumTime) final;

    QList<QQmlProfilerEventType> eventTypes() const;
    QList<QQmlProfilerEvent> events() const;
};

QT_END_NAMESPACE

#endif // QQMLPROFILERQTDWRITER_P_H
