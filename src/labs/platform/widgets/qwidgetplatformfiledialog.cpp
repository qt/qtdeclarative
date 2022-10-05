// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwidgetplatformfiledialog_p.h"
#include "qwidgetplatformdialog_p.h"

#include <QtWidgets/qfiledialog.h>

QT_BEGIN_NAMESPACE

QWidgetPlatformFileDialog::QWidgetPlatformFileDialog(QObject *parent)
    : m_dialog(new QFileDialog)
{
    setParent(parent);

    connect(m_dialog.data(), &QDialog::accepted, this, &QPlatformDialogHelper::accept);
    connect(m_dialog.data(), &QDialog::rejected, this, &QPlatformDialogHelper::reject);

    connect(m_dialog.data(), &QFileDialog::fileSelected, [this](const QString &file) {
        emit fileSelected(QUrl::fromLocalFile(file));
    });
    connect(m_dialog.data(), &QFileDialog::filesSelected, [this](const QList<QString> &files) {
        QList<QUrl> urls;
        urls.reserve(files.size());
        for (const QString &file : files)
            urls += QUrl::fromLocalFile(file);
        emit filesSelected(urls);
    });
    connect(m_dialog.data(), &QFileDialog::currentChanged, [this](const QString &path) {
        emit currentChanged(QUrl::fromLocalFile(path));
    });
    connect(m_dialog.data(), &QFileDialog::directoryEntered, this, &QWidgetPlatformFileDialog::directoryEntered);
    connect(m_dialog.data(), &QFileDialog::filterSelected, this, &QWidgetPlatformFileDialog::filterSelected);
}

QWidgetPlatformFileDialog::~QWidgetPlatformFileDialog()
{
}

bool QWidgetPlatformFileDialog::defaultNameFilterDisables() const
{
    return false; // TODO: ?
}

void QWidgetPlatformFileDialog::setDirectory(const QUrl &directory)
{
    m_dialog->setDirectory(directory.toLocalFile());
}

QUrl QWidgetPlatformFileDialog::directory() const
{
    return m_dialog->directoryUrl();
}

void QWidgetPlatformFileDialog::selectFile(const QUrl &filename)
{
    m_dialog->selectUrl(filename);
}

QList<QUrl> QWidgetPlatformFileDialog::selectedFiles() const
{
    return m_dialog->selectedUrls();
}

void QWidgetPlatformFileDialog::setFilter()
{
    // TODO: ?
}

void QWidgetPlatformFileDialog::selectNameFilter(const QString &filter)
{
    m_dialog->selectNameFilter(filter);
}

QString QWidgetPlatformFileDialog::selectedNameFilter() const
{
    return m_dialog->selectedNameFilter();
}

void QWidgetPlatformFileDialog::exec()
{
    m_dialog->exec();
}

bool QWidgetPlatformFileDialog::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    QSharedPointer<QFileDialogOptions> options = QPlatformFileDialogHelper::options();
    m_dialog->setWindowTitle(options->windowTitle());
    m_dialog->setAcceptMode(static_cast<QFileDialog::AcceptMode>(options->acceptMode()));
    m_dialog->setFileMode(static_cast<QFileDialog::FileMode>(options->fileMode()));
    m_dialog->setOptions(static_cast<QFileDialog::Options>(int(options->options())) | QFileDialog::DontUseNativeDialog);
    m_dialog->setNameFilters(options->nameFilters());
    m_dialog->setDefaultSuffix(options->defaultSuffix());
    if (options->isLabelExplicitlySet(QFileDialogOptions::Accept))
        m_dialog->setLabelText(QFileDialog::Accept, options->labelText(QFileDialogOptions::Accept));
    if (options->isLabelExplicitlySet(QFileDialogOptions::Reject))
        m_dialog->setLabelText(QFileDialog::Reject, options->labelText(QFileDialogOptions::Reject));

    return QWidgetPlatformDialog::show(m_dialog.data(), flags, modality, parent);
}

void QWidgetPlatformFileDialog::hide()
{
    m_dialog->hide();
}

QT_END_NAMESPACE

#include "moc_qwidgetplatformfiledialog_p.cpp"
