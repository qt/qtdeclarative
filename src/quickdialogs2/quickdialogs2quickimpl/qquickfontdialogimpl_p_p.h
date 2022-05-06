/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
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

#ifndef QQUICKFONTDIALOGIMPL_P_P_H
#define QQUICKFONTDIALOGIMPL_P_P_H

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

#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>

#include "qquickfontdialogimpl_p.h"

QT_BEGIN_NAMESPACE

class QQuickFontDialogImplPrivate : public QQuickDialogPrivate
{
    Q_DECLARE_PUBLIC(QQuickFontDialogImpl)
public:
    QQuickFontDialogImplPrivate();

    static QQuickFontDialogImplPrivate *get(QQuickFontDialogImpl *dialog)
    {
        return dialog->d_func();
    }

    QQuickFontDialogImplAttached *attachedOrWarn();

    void updateEnabled();

    void handleAccept() override;
    void handleClick(QQuickAbstractButton *button) override;

    QSharedPointer<QFontDialogOptions> options;

    QFont currentFont;
};

class QQuickFontDialogImplAttachedPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickFontDialogImplAttached)

    void currentFontChanged(const QFont &font);

public:
    QPointer<QQuickDialogButtonBox> buttonBox;
    QPointer<QQuickListView> familyListView;
    QPointer<QQuickListView> styleListView;
    QPointer<QQuickListView> sizeListView;
    QPointer<QQuickTextEdit> sampleEdit;
    QPointer<QQuickComboBox> writingSystemComboBox;
    QPointer<QQuickCheckBox> underlineCheckBox;
    QPointer<QQuickCheckBox> strikeoutCheckBox;
    QPointer<QQuickTextField> familyEdit;
    QPointer<QQuickTextField> styleEdit;
    QPointer<QQuickTextField> sizeEdit;
};

QT_END_NAMESPACE

#endif // QQUICKFONTDIALOGIMPL_P_P_H
