// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QGuiApplication>
#include <QSettings>
#include <QRegularExpression>
#include "bridge.h"
#include "stylegenerator.h"

Bridge::Bridge()
{
    QSettings settings;
    m_targetDirectory = settings.value("targetDirectory").toString();
    m_figmaUrlOrId = settings.value("figmaUrlOrId").toString();
    m_figmaToken = settings.value("figmaToken").toString();
    m_overwriteQml = settings.value("overwriteQml").toBool();
    m_selectedControls = settings.value("selectedControls", availableControls()).toStringList();
    m_selectedImageFormats = settings.value("selectedImageFormats", QStringList() << "png@1x" << "png@2x").toStringList();
}

Bridge::~Bridge()
{
    QSettings settings;
    settings.setValue("targetDirectory", m_targetDirectory);
    settings.setValue("figmaUrlOrId", m_figmaUrlOrId);
    settings.setValue("figmaToken", m_figmaToken);
    settings.setValue("overwriteQml", m_overwriteQml);
    settings.setValue("selectedControls", m_selectedControls);
    settings.setValue("selectedImageFormats", m_selectedImageFormats);
}

void Bridge::generate()
{
    Q_ASSERT(!m_generatorThread || !m_generatorThread->isRunning());

    // If m_figmaUrlOrId is a URL, extract the file/branch ID from it.
    QRegularExpression re(R"(file/(.*?)/(branch/(.*?)/)?)");
    QRegularExpressionMatch match = re.match(m_figmaUrlOrId);
    const QString fileId = match.captured(1);
    const QString branchId = match.captured(3);

    if (!branchId.isEmpty())
        m_fileId = branchId;
    else if (!fileId.isEmpty())
        m_fileId = fileId;
    else
        m_fileId = m_figmaUrlOrId;

    // Run the generator in a separate thread to not block the UI
    m_generator = std::make_unique<StyleGenerator>(this);
    m_generatorThread = std::make_unique<QThread>();
    m_generator->moveToThread(m_generatorThread.get());
    connect(m_generatorThread.get(), &QThread::started, this, &Bridge::started);
    connect(m_generatorThread.get(), &QThread::started, m_generator.get(), &StyleGenerator::generateStyle);
    connect(m_generatorThread.get(), &QThread::finished, this, &Bridge::finished);
    m_generatorThread->start();
}

void Bridge::stop()
{
    Q_ASSERT(m_generator);
    m_generator->m_abort = true;
}

QString Bridge::toLocalFile(const QUrl &url) const
{
    return QUrl(url).toLocalFile();
}

QString Bridge::howToText()
{
    QFile file(":/howto.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QString txt = file.readAll();
    txt.replace("\n\n", "<p>");
    txt.replace("\n", "<br>");
    txt.replace("<code>", "<i>");
    txt.replace("</code>", "</i>");
    return txt;
}

QStringList Bridge::availableControls() const
{
    return StyleGenerator(const_cast<Bridge *>(this)).availableControls();
}

QStringList Bridge::selectedControls() const
{
   return m_selectedControls;
}

void Bridge::selectControl(const QString &control, bool select)
{
    const bool alreadySelected = m_selectedControls.contains(control);
    if (select == alreadySelected)
        return;

    if (select)
        m_selectedControls.append(control);
    else
        m_selectedControls.removeOne(control);
}

bool Bridge::isImageFormatSelected(const QString &format) const
{
    return m_selectedImageFormats.contains(format);
}

void Bridge::selectImageFormat(const QString &format, bool select)
{
    const bool alreadySelected = m_selectedImageFormats.contains(format);
    if (select == alreadySelected)
        return;

    if (select)
        m_selectedImageFormats.append(format);
    else
        m_selectedImageFormats.removeOne(format);
}
