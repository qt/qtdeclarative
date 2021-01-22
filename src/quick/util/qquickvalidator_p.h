/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#ifndef QQUICKVALIDATOR_P_H
#define QQUICKVALIDATOR_P_H

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

#include <QtGui/qvalidator.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(validator)
class Q_AUTOTEST_EXPORT QQuickIntValidator : public QIntValidator
{
    Q_OBJECT
    Q_PROPERTY(QString locale READ localeName WRITE setLocaleName RESET resetLocaleName NOTIFY localeNameChanged)
    QML_NAMED_ELEMENT(IntValidator)
public:
    QQuickIntValidator(QObject *parent = nullptr);

    QString localeName() const;
    void setLocaleName(const QString &name);
    void resetLocaleName();

Q_SIGNALS:
    void localeNameChanged();
};

class Q_AUTOTEST_EXPORT QQuickDoubleValidator : public QDoubleValidator
{
    Q_OBJECT
    Q_PROPERTY(QString locale READ localeName WRITE setLocaleName RESET resetLocaleName NOTIFY localeNameChanged)
    QML_NAMED_ELEMENT(DoubleValidator)
public:
    QQuickDoubleValidator(QObject *parent = nullptr);

    QString localeName() const;
    void setLocaleName(const QString &name);
    void resetLocaleName();

Q_SIGNALS:
    void localeNameChanged();
};
#endif

QT_END_NAMESPACE

#if QT_CONFIG(validator)
QML_DECLARE_TYPE(QValidator)
QML_DECLARE_TYPE(QQuickIntValidator)
QML_DECLARE_TYPE(QQuickDoubleValidator)
QML_DECLARE_TYPE(QRegExpValidator)
#if QT_CONFIG(regularexpression)
QML_DECLARE_TYPE(QRegularExpressionValidator)
#endif
#endif

#endif // QQUICKVALIDATOR_P_H
