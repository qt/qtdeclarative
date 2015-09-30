/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKCHECKABLE_P_H
#define QQUICKCHECKABLE_P_H

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

#include <QtLabsTemplates/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickCheckablePrivate;

class Q_LABSTEMPLATES_EXPORT QQuickCheckable : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged FINAL)
    Q_PROPERTY(QQuickItem *indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)

public:
    explicit QQuickCheckable(QQuickItem *parent = Q_NULLPTR);

    bool isChecked() const;
    void setChecked(bool checked);

    bool isExclusive() const;
    void setExclusive(bool exclusive);

    QQuickItem *indicator() const;
    void setIndicator(QQuickItem *indicator);

public Q_SLOTS:
    void toggle();

Q_SIGNALS:
    void checkedChanged();
    void indicatorChanged();

protected:
    QQuickCheckable(QQuickCheckablePrivate &dd, QQuickItem *parent);

    void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void classBegin() Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QQuickCheckable)
    Q_DECLARE_PRIVATE(QQuickCheckable)
};

Q_DECLARE_TYPEINFO(QQuickCheckable, Q_COMPLEX_TYPE);

QT_END_NAMESPACE

#endif // QQUICKCHECKABLE_P_H
