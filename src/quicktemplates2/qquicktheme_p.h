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

#ifndef QQUICKTHEME_P_H
#define QQUICKTHEME_P_H

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

#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtCore/qscopedpointer.h>
#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE

class QQuickThemePrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickTheme
{
public:
    QQuickTheme();
    ~QQuickTheme();

    static QQuickTheme *instance();

    enum Scope {
        System,
        Button,
        CheckBox,
        ComboBox,
        GroupBox,
        ItemView,
        Label,
        ListView,
        Menu,
        MenuBar,
        RadioButton,
        SpinBox,
        Switch,
        TabBar,
        TextArea,
        TextField,
        ToolBar,
        ToolTip,
        Tumbler
    };

    static QFont font(Scope scope);
    static QPalette palette(Scope scope);

    void setFont(Scope scope, const QFont &font);
    void setPalette(Scope scope, const QPalette &palette);

private:
    Q_DISABLE_COPY(QQuickTheme)
    Q_DECLARE_PRIVATE(QQuickTheme)
    QScopedPointer<QQuickThemePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QQUICKTHEME_P_H
