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

#ifndef QQMLCONNECTIONS_H
#define QQMLCONNECTIONS_H

#include <qqml.h>
#include <private/qqmlcustomparser_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QQmlBoundSignal;
class QQmlContext;
class QQmlConnectionsPrivate;
class Q_AUTOTEST_EXPORT QQmlConnections : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlConnections)

    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QObject *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool ignoreUnknownSignals READ ignoreUnknownSignals WRITE setIgnoreUnknownSignals)

public:
    QQmlConnections(QObject *parent=0);
    ~QQmlConnections();

    QObject *target() const;
    void setTarget(QObject *);

    bool ignoreUnknownSignals() const;
    void setIgnoreUnknownSignals(bool ignore);

Q_SIGNALS:
    void targetChanged();

private:
    void connectSignals();
    void classBegin();
    void componentComplete();
};

class QQmlConnectionsParser : public QQmlCustomParser
{
public:
    virtual QByteArray compile(const QV4::CompiledData::QmlUnit *qmlUnit, const QList<const QV4::CompiledData::Binding *> &props);
    virtual void setCustomData(QObject *, const QByteArray &, QQmlCompiledData *cdata);
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QQmlConnections)

#endif
