/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQUICKWORKERSCRIPT_P_H
#define QQUICKWORKERSCRIPT_P_H

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

#include "qqml.h"
#include "qqmlparserstatus.h"

#include <QtCore/qthread.h>
#include <QtQml/qjsvalue.h>
#include <QtCore/qurl.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QQuickWorkerScript;
class QQuickWorkerScriptEnginePrivate;
class QQuickWorkerScriptEngine : public QThread
{
Q_OBJECT
public:
    QQuickWorkerScriptEngine(QQmlEngine *parent = 0);
    virtual ~QQuickWorkerScriptEngine();

    int registerWorkerScript(QQuickWorkerScript *);
    void removeWorkerScript(int);
    void executeUrl(int, const QUrl &);
    void sendMessage(int, const QByteArray &);

protected:
    virtual void run();

private:
    QQuickWorkerScriptEnginePrivate *d;
};

class QQmlV8Function;
class QQmlV8Handle;
class Q_AUTOTEST_EXPORT QQuickWorkerScript : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)

    Q_INTERFACES(QQmlParserStatus)
public:
    QQuickWorkerScript(QObject *parent = 0);
    virtual ~QQuickWorkerScript();

    QUrl source() const;
    void setSource(const QUrl &);

public slots:
    void sendMessage(QQmlV8Function*);

signals:
    void sourceChanged();
    void message(const QQmlV8Handle &messageObject);

protected:
    virtual void classBegin();
    virtual void componentComplete();
    virtual bool event(QEvent *);

private:
    QQuickWorkerScriptEngine *engine();
    QQuickWorkerScriptEngine *m_engine;
    int m_scriptId;
    QUrl m_source;
    bool m_componentComplete;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickWorkerScript)

QT_END_HEADER

#endif // QQUICKWORKERSCRIPT_P_H
