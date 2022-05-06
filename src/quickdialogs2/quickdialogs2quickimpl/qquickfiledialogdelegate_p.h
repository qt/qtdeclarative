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

#ifndef QQUICKFILEDIALOGDELEGATE_P_H
#define QQUICKFILEDIALOGDELEGATE_P_H

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

#include <QtQuickTemplates2/private/qquickitemdelegate_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickFileDialogImpl;
class QQuickFileDialogDelegatePrivate;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickFileDialogDelegate : public QQuickItemDelegate
{
    Q_OBJECT
    Q_PROPERTY(QQuickFileDialogImpl *fileDialog READ fileDialog WRITE setFileDialog NOTIFY fileDialogChanged)
    Q_PROPERTY(QUrl file READ file WRITE setFile NOTIFY fileChanged)
    QML_NAMED_ELEMENT(FileDialogDelegate)
    QML_ADDED_IN_VERSION(6, 2)

public:
    explicit QQuickFileDialogDelegate(QQuickItem *parent = nullptr);

    QQuickFileDialogImpl *fileDialog() const;
    void setFileDialog(QQuickFileDialogImpl *fileDialog);

    QUrl file() const;
    void setFile(const QUrl &file);

Q_SIGNALS:
    void fileDialogChanged();
    void fileChanged();

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickFileDialogDelegate)
    Q_DECLARE_PRIVATE(QQuickFileDialogDelegate)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFileDialogDelegate)

#endif // QQUICKFILEDIALOGDELEGATE_P_H
