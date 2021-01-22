/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLCONNECTIONS_H
#define QQMLCONNECTIONS_H

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
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged REVISION 3)
    Q_PROPERTY(bool ignoreUnknownSignals READ ignoreUnknownSignals WRITE setIgnoreUnknownSignals)
    QML_NAMED_ELEMENT(Connections)

public:
    QQmlConnections(QObject *parent=nullptr);
    ~QQmlConnections();

    QObject *target() const;
    void setTarget(QObject *);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool ignoreUnknownSignals() const;
    void setIgnoreUnknownSignals(bool ignore);

Q_SIGNALS:
    void targetChanged();
    Q_REVISION(3) void enabledChanged();

private:
    void connectSignals();
    void connectSignalsToMethods();
    void connectSignalsToBindings();

    void classBegin() override;
    void componentComplete() override;
};

// TODO: Drop this class as soon as we can
class QQmlConnectionsParser : public QQmlCustomParser
{
public:
    void verifyBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &props) override;
    void applyBindings(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &bindings) override;
};

// TODO: We won't need Connections to be a custom type anymore once we can drop the
//       automatic signal handler inference from undeclared properties.
template<>
inline QQmlCustomParser *qmlCreateCustomParser<QQmlConnections>()
{
    return new QQmlConnectionsParser;
}

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQmlConnections)

#endif
