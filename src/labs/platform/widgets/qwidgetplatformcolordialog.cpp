// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
