/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV8PROFILERSERVICE_P_H
#define QV8PROFILERSERVICE_P_H

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

#include <private/qqmldebugservice_p.h>

QT_BEGIN_NAMESPACE


struct Q_AUTOTEST_EXPORT QV8ProfilerData
{
    int messageType;
    QString filename;
    QString functionname;
    int lineNumber;
    double totalTime;
    double selfTime;
    int treeLevel;

    QByteArray toByteArray() const;
};

class QQmlEngine;
class QV8ProfilerServicePrivate;

class Q_AUTOTEST_EXPORT QV8ProfilerService : public QQmlDebugService
{
    Q_OBJECT
public:
    enum MessageType {
        V8Entry,
        V8Complete,
        V8SnapshotChunk,
        V8SnapshotComplete,
        V8Started,

        V8MaximumMessage
    };

    QV8ProfilerService(QObject *parent = 0);
    ~QV8ProfilerService();

    static QV8ProfilerService *instance();
    static void initialize();

public slots:
    void startProfiling(const QString &title);
    void stopProfiling(const QString &title);
    void takeSnapshot();
    void deleteSnapshots();

    void sendProfilingData();

protected:
    void stateAboutToBeChanged(State state);
    void messageReceived(const QByteArray &);

private:
    Q_DISABLE_COPY(QV8ProfilerService)
    Q_DECLARE_PRIVATE(QV8ProfilerService)
};

QT_END_NAMESPACE

#endif // QV8PROFILERSERVICE_P_H
