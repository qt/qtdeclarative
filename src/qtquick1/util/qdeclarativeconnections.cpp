/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "QtQuick1/private/qdeclarativeconnections_p.h"

#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/private/qdeclarativeproperty_p.h>
#include <QtDeclarative/private/qdeclarativeboundsignal_p.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/private/qdeclarativecontext_p.h>
#include <QtDeclarative/qdeclarativeinfo.h>

#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE



class QDeclarative1ConnectionsPrivate : public QObjectPrivate
{
public:
    QDeclarative1ConnectionsPrivate() : target(0), targetSet(false), ignoreUnknownSignals(false), componentcomplete(true) {}

    QList<QDeclarativeBoundSignal*> boundsignals;
    QObject *target;

    bool targetSet;
    bool ignoreUnknownSignals;
    bool componentcomplete;

    QByteArray data;
};

/*!
    \qmlclass Connections QDeclarative1Connections
    \inqmlmodule QtQuick 1
    \ingroup qml-utility-elements
    \since QtQuick 1.0
    \brief A Connections element describes generalized connections to signals.

    A Connections object creates a connection to a QML signal.

    When connecting to signals in QML, the usual way is to create an
    "on<Signal>" handler that reacts when a signal is received, like this:

    \qml
    MouseArea {
        onClicked: { foo(parameters) }
    }
    \endqml

    However, it is not possible to connect to a signal in this way in some 
    cases, such as when:

    \list
        \i Multiple connections to the same signal are required
        \i Creating connections outside the scope of the signal sender
        \i Connecting to targets not defined in QML
    \endlist

    When any of these are needed, the Connections element can be used instead.

    For example, the above code can be changed to use a Connections object,
    like this:

    \qml
    MouseArea {
        Connections {
            onClicked: foo(parameters)
        }
    }
    \endqml

    More generally, the Connections object can be a child of some object other than
    the sender of the signal:

    \qml
    MouseArea {
        id: area
    }
    // ...
    \endqml
    \qml
    Connections {
        target: area
        onClicked: foo(parameters)
    }
    \endqml

    \sa QtDeclarative
*/
QDeclarative1Connections::QDeclarative1Connections(QObject *parent) :
    QObject(*(new QDeclarative1ConnectionsPrivate), parent)
{
}

QDeclarative1Connections::~QDeclarative1Connections()
{
}

/*!
    \qmlproperty Object QtQuick1::Connections::target
    This property holds the object that sends the signal.

    If this property is not set, the \c target defaults to the parent of the Connection.

    If set to null, no connection is made and any signal handlers are ignored
    until the target is not null.
*/
QObject *QDeclarative1Connections::target() const
{
    Q_D(const QDeclarative1Connections);
    return d->targetSet ? d->target : parent();
}

void QDeclarative1Connections::setTarget(QObject *obj)
{
    Q_D(QDeclarative1Connections);
    d->targetSet = true; // even if setting to 0, it is *set*
    if (d->target == obj)
        return;
    foreach (QDeclarativeBoundSignal *s, d->boundsignals) {
        // It is possible that target is being changed due to one of our signal
        // handlers -> use deleteLater().
        if (s->isEvaluating())
            s->deleteLater();
        else
            delete s;
    }
    d->boundsignals.clear();
    d->target = obj;
    connectSignals();
    emit targetChanged();
}

/*!
    \qmlproperty bool QtQuick1::Connections::ignoreUnknownSignals

    Normally, a connection to a non-existent signal produces runtime errors.

    If this property is set to \c true, such errors are ignored.
    This is useful if you intend to connect to different types of objects, handling
    a different set of signals for each object.
*/
bool QDeclarative1Connections::ignoreUnknownSignals() const
{
    Q_D(const QDeclarative1Connections);
    return d->ignoreUnknownSignals;
}

void QDeclarative1Connections::setIgnoreUnknownSignals(bool ignore)
{
    Q_D(QDeclarative1Connections);
    d->ignoreUnknownSignals = ignore;
}



QByteArray
QDeclarative1ConnectionsParser::compile(const QList<QDeclarativeCustomParserProperty> &props)
{
    QByteArray rv;
    QDataStream ds(&rv, QIODevice::WriteOnly);

    for(int ii = 0; ii < props.count(); ++ii)
    {
        QString propName = props.at(ii).name();
        if (!propName.startsWith(QLatin1String("on")) || !propName.at(2).isUpper()) {
            error(props.at(ii), QDeclarative1Connections::tr("Cannot assign to non-existent property \"%1\"").arg(propName));
            return QByteArray();
        }

        QList<QVariant> values = props.at(ii).assignedValues();

        for (int i = 0; i < values.count(); ++i) {
            const QVariant &value = values.at(i);

            if (value.userType() == qMetaTypeId<QDeclarativeCustomParserNode>()) {
                error(props.at(ii), QDeclarative1Connections::tr("Connections: nested objects not allowed"));
                return QByteArray();
            } else if (value.userType() == qMetaTypeId<QDeclarativeCustomParserProperty>()) {
                error(props.at(ii), QDeclarative1Connections::tr("Connections: syntax error"));
                return QByteArray();
            } else {
                QDeclarativeScript::Variant v = qvariant_cast<QDeclarativeScript::Variant>(value);
                if (v.isScript()) {
                    ds << propName;
                    ds << v.asScript();
                } else {
                    error(props.at(ii), QDeclarative1Connections::tr("Connections: script expected"));
                    return QByteArray();
                }
            }
        }
    }

    return rv;
}

void QDeclarative1ConnectionsParser::setCustomData(QObject *object,
                                            const QByteArray &data)
{
    QDeclarative1ConnectionsPrivate *p =
        static_cast<QDeclarative1ConnectionsPrivate *>(QObjectPrivate::get(object));
    p->data = data;
}


void QDeclarative1Connections::connectSignals()
{
    Q_D(QDeclarative1Connections);
    if (!d->componentcomplete || (d->targetSet && !target()))
        return;

    QDataStream ds(d->data);
    while (!ds.atEnd()) {
        QString propName;
        ds >> propName;
        QString script;
        ds >> script;
        QDeclarativeProperty prop(target(), propName);
        if (prop.isValid() && (prop.type() & QDeclarativeProperty::SignalProperty)) {
            QDeclarativeBoundSignal *signal =
                new QDeclarativeBoundSignal(target(), prop.method(), this);
            QDeclarativeExpression *expression = new QDeclarativeExpression(qmlContext(this), 0, script);
            QDeclarativeData *ddata = QDeclarativeData::get(this);
            if (ddata && ddata->outerContext && !ddata->outerContext->url.isEmpty())
                expression->setSourceLocation(ddata->outerContext->url.toString(), ddata->lineNumber, ddata->columnNumber);
            signal->setExpression(expression);
            d->boundsignals += signal;
        } else {
            if (!d->ignoreUnknownSignals)
                qmlInfo(this) << tr("Cannot assign to non-existent property \"%1\"").arg(propName);
        }
    }
}

void QDeclarative1Connections::classBegin()
{
    Q_D(QDeclarative1Connections);
    d->componentcomplete=false;
}

void QDeclarative1Connections::componentComplete()
{
    Q_D(QDeclarative1Connections);
    d->componentcomplete=true;
    connectSignals();
}



QT_END_NAMESPACE
