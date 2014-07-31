/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "finalstate.h"
#include "signaltransition.h"
#include "statebase.h"
#include "statemachine.h"
#include "timeouttransition.h"

#include <QHistoryState>
#include <QQmlExtensionPlugin>
#include <qqml.h>

QT_BEGIN_NAMESPACE

class QtQmlStateMachinePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.org.Qt.QtQml.StateMachine/1.0")

public:
    void registerTypes(const char *uri)
    {
        qmlRegisterType<StateBase>(uri, 1, 0, "StateBase");
        qmlRegisterType<StateMachine>(uri, 1, 0, "StateMachine");
        qmlRegisterType<QHistoryState>(uri, 1, 0, "HistoryState");
        qmlRegisterType<FinalState>(uri, 1, 0, "FinalState");
        qmlRegisterUncreatableType<QState>(uri, 1, 0, "QState", "Don't use this, use StateBase instead");
        qmlRegisterUncreatableType<QAbstractState>(uri, 1, 0, "QAbstractState", "Don't use this, use StateBase instead");
        qmlRegisterUncreatableType<QSignalTransition>(uri, 1, 0, "QSignalTransition", "Don't use this, use SignalTransition instead");
        qmlRegisterType<SignalTransition>(uri, 1, 0, "SignalTransition");
        qmlRegisterType<TimeoutTransition>(uri, 1, 0, "TimeoutTransition");
        qmlProtectModule(uri, 1);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
