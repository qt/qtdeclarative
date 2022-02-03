/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
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

#ifndef QQUICKCOLORDIALOG_P_H
#define QQUICKCOLORDIALOG_P_H

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

#include "qquickabstractdialog_p.h"

#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class Q_QUICKDIALOGS2_PRIVATE_EXPORT QQuickColorDialog : public QQuickAbstractDialog
{
    Q_OBJECT
    Q_PROPERTY(QColor selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged)
    Q_PROPERTY(QColorDialogOptions::ColorDialogOptions options READ options WRITE setOptions RESET resetOptions NOTIFY optionsChanged)
    Q_FLAGS(QColorDialogOptions::ColorDialogOptions)
    QML_NAMED_ELEMENT(ColorDialog)
    QML_ADDED_IN_VERSION(6, 4)

public:
    explicit QQuickColorDialog(QObject *parent = nullptr);

    QColor selectedColor() const;
    void setSelectedColor(const QColor &color);

    QColorDialogOptions::ColorDialogOptions options() const;
    void setOptions(QColorDialogOptions::ColorDialogOptions options);
    void resetOptions();

Q_SIGNALS:
    void selectedColorChanged();
    void optionsChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;

private:
    QSharedPointer<QColorDialogOptions> m_options;
    QColor m_selectedColor;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickColorDialog)

#endif // QQUICKCOLORDIALOG_P_H
