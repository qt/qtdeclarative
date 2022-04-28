/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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

#include "qwidgetplatformcolordialog_p.h"
#include "qwidgetplatformdialog_p.h"

#include <QtWidgets/qcolordialog.h>

QT_BEGIN_NAMESPACE

QWidgetPlatformColorDialog::QWidgetPlatformColorDialog(QObject *parent)
    : m_dialog(new QColorDialog)
{
    setParent(parent);

    connect(m_dialog.data(), &QColorDialog::accepted, this, &QPlatformDialogHelper::accept);
    connect(m_dialog.data(), &QColorDialog::rejected, this, &QPlatformDialogHelper::reject);
    connect(m_dialog.data(), &QColorDialog::currentColorChanged, this, &QPlatformColorDialogHelper::currentColorChanged);
}

QWidgetPlatformColorDialog::~QWidgetPlatformColorDialog()
{
}

QColor QWidgetPlatformColorDialog::currentColor() const
{
    return m_dialog->currentColor();
}

void QWidgetPlatformColorDialog::setCurrentColor(const QColor &color)
{
    m_dialog->setCurrentColor(color);
}

void QWidgetPlatformColorDialog::exec()
{
    m_dialog->exec();
}

bool QWidgetPlatformColorDialog::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    QSharedPointer<QColorDialogOptions> options = QPlatformColorDialogHelper::options();
    m_dialog->setWindowTitle(options->windowTitle());
    m_dialog->setOptions(static_cast<QColorDialog::ColorDialogOptions>(int(options->options())) | QColorDialog::DontUseNativeDialog);

    return QWidgetPlatformDialog::show(m_dialog.data(), flags, modality, parent);
}

void QWidgetPlatformColorDialog::hide()
{
    m_dialog->hide();
}

QT_END_NAMESPACE

#include "moc_qwidgetplatformcolordialog_p.cpp"
