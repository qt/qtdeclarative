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

#ifndef QQMLBIND_H
#define QQMLBIND_H

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

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQmlBindPrivate;
class Q_AUTOTEST_EXPORT QQmlBind : public QObject, public QQmlPropertyValueSource, public QQmlParserStatus
{
public:
    enum RestorationMode {
        RestoreNone    = 0x0,
        RestoreBinding = 0x1,
        RestoreValue   = 0x2,
        RestoreBindingOrValue = RestoreBinding | RestoreValue
    };

private:
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlBind)
    Q_INTERFACES(QQmlParserStatus)
    Q_INTERFACES(QQmlPropertyValueSource)
    Q_PROPERTY(QObject *target READ object WRITE setObject)
    Q_PROPERTY(QString property READ property WRITE setProperty)
    Q_PROPERTY(QJSValue value READ value WRITE setValue)
    Q_PROPERTY(bool when READ when WRITE setWhen)
    Q_PROPERTY(bool delayed READ delayed WRITE setDelayed REVISION 8)
    Q_PROPERTY(RestorationMode restoreMode READ restoreMode WRITE setRestoreMode
               NOTIFY restoreModeChanged REVISION 14)
    Q_ENUM(RestorationMode)
    QML_NAMED_ELEMENT(Binding)

public:
    QQmlBind(QObject *parent=nullptr);
    ~QQmlBind();

    bool when() const;
    void setWhen(bool);

    QObject *object();
    void setObject(QObject *);

    QString property() const;
    void setProperty(const QString &);

    QJSValue value() const;
    void setValue(const QJSValue &);

    bool delayed() const;
    void setDelayed(bool);

    RestorationMode restoreMode() const;
    void setRestoreMode(RestorationMode);

Q_SIGNALS:
    void restoreModeChanged();

protected:
    void setTarget(const QQmlProperty &) override;
    void classBegin() override;
    void componentComplete() override;

private:
    void prepareEval();
    void eval();

private Q_SLOTS:
    void targetValueChanged();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQmlBind)

#endif
