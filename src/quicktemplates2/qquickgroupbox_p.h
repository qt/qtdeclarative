/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKGROUPBOX_P_H
#define QQUICKGROUPBOX_P_H

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

#include <QtQuickTemplates2/private/qquickframe_p.h>

QT_BEGIN_NAMESPACE

class QQuickGroupBoxPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickGroupBox : public QQuickFrame
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QQuickItem *label READ label WRITE setLabel NOTIFY labelChanged FINAL)
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal implicitLabelWidth READ implicitLabelWidth NOTIFY implicitLabelWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitLabelHeight READ implicitLabelHeight NOTIFY implicitLabelHeightChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "background,contentItem,label")
    QML_NAMED_ELEMENT(GroupBox)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickGroupBox(QQuickItem *parent = nullptr);
    ~QQuickGroupBox();

    QString title() const;
    void setTitle(const QString &title);

    QQuickItem *label() const;
    void setLabel(QQuickItem *label);

    // 2.5 (Qt 5.12)
    qreal implicitLabelWidth() const;
    qreal implicitLabelHeight() const;

Q_SIGNALS:
    void titleChanged();
    void labelChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void implicitLabelWidthChanged();
    Q_REVISION(2, 5) void implicitLabelHeightChanged();

protected:
    void componentComplete() override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
    void accessibilityActiveChanged(bool active) override;
#endif

private:
    Q_DISABLE_COPY(QQuickGroupBox)
    Q_DECLARE_PRIVATE(QQuickGroupBox)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickGroupBox)

#endif // QQUICKGROUPBOX_P_H
