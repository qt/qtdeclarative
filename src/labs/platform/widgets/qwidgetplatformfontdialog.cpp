// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwidgetplatformfontdialog_p.h"
#include "qwidgetplatformdialog_p.h"

#include <QtWidgets/qfontdialog.h>

QT_BEGIN_NAMESPACE

QWidgetPlatformFontDialog::QWidgetPlatformFontDialog(QObject *parent)
    : m_dialog(new QFontDialog)
{
    setParent(parent);

    connect(m_dialog.data(), &QFontDialog::accepted, this, &QPlatformDialogHelper::accept);
    connect(m_dialog.data(), &QFontDialog::rejected, this, &QPlatformDialogHelper::reject);
    connect(m_dialog.data(), &QFontDialog::currentFontChanged, this, &QPlatformFontDialogHelper::currentFontChanged);
}

QWidgetPlatformFontDialog::~QWidgetPlatformFontDialog()
{
}

QFont QWidgetPlatformFontDialog::currentFont() const
{
    return m_dialog->currentFont();
}

void QWidgetPlatformFontDialog::setCurrentFont(const QFont &font)
{
    m_dialog->setCurrentFont(font);
}

void QWidgetPlatformFontDialog::exec()
{
    m_dialog->exec();
}

bool QWidgetPlatformFontDialog::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    QSharedPointer<QFontDialogOptions> options = QPlatformFontDialogHelper::options();
    m_dialog->setWindowTitle(options->windowTitle());
    m_dialog->setOptions(static_cast<QFontDialog::FontDialogOptions>(int(options->options())) | QFontDialog::DontUseNativeDialog);

    return QWidgetPlatformDialog::show(m_dialog.data(), flags, modality, parent);
}

void QWidgetPlatformFontDialog::hide()
{
    m_dialog->hide();
}

QT_END_NAMESPACE

#include "moc_qwidgetplatformfontdialog_p.cpp"
