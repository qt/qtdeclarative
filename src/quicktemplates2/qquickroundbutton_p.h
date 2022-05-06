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

#ifndef QQUICKROUNDBUTTON_P_H
#define QQUICKROUNDBUTTON_P_H

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

#include <QtQuickTemplates2/private/qquickbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickRoundButtonPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickRoundButton : public QQuickButton
{
    Q_OBJECT
    Q_PROPERTY(qreal radius READ radius WRITE setRadius RESET resetRadius NOTIFY radiusChanged FINAL)
    QML_NAMED_ELEMENT(RoundButton)
    QML_ADDED_IN_VERSION(2, 1)

public:
    explicit QQuickRoundButton(QQuickItem *parent = nullptr);

    qreal radius() const;
    void setRadius(qreal radius);
    void resetRadius();

Q_SIGNALS:
    void radiusChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    Q_DISABLE_COPY(QQuickRoundButton)
    Q_DECLARE_PRIVATE(QQuickRoundButton)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickRoundButton)

#endif // QQUICKROUNDBUTTON_P_H
