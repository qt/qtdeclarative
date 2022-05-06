/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKPROGRESSBAR_P_H
#define QQUICKPROGRESSBAR_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickProgressBarPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickProgressBar : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(bool indeterminate READ isIndeterminate WRITE setIndeterminate NOTIFY indeterminateChanged FINAL)
    QML_NAMED_ELEMENT(ProgressBar)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickProgressBar(QQuickItem *parent = nullptr);

    qreal from() const;
    void setFrom(qreal from);

    qreal to() const;
    void setTo(qreal to);

    qreal value() const;
    void setValue(qreal value);

    qreal position() const;
    qreal visualPosition() const;

    bool isIndeterminate() const;
    void setIndeterminate(bool indeterminate);

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void valueChanged();
    void positionChanged();
    void visualPositionChanged();
    void indeterminateChanged();

protected:
    void mirrorChange() override;
    void componentComplete() override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickProgressBar)
    Q_DECLARE_PRIVATE(QQuickProgressBar)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickProgressBar)

#endif // QQUICKPROGRESSBAR_P_H
