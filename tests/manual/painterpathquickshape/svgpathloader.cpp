// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "svgpathloader.h"

#include <QFile>
#include <QPainterPath>
#include <QtSvg/private/qsvgtinydocument_p.h>
#include <QtSvg/private/qsvggraphics_p.h>

SvgPathLoader::SvgPathLoader()
{
    connect(this, &SvgPathLoader::sourceChanged, this, &SvgPathLoader::loadPaths);

    loadPaths();
}

void SvgPathLoader::loadPaths()
{
    m_paths.clear();
    m_fillColors.clear();
    if (m_source.isEmpty())
        return;

    QString fileName;
    if (m_source.isLocalFile())
        fileName = m_source.toLocalFile();
    else if (m_source.scheme() == QStringLiteral("qrc"))
        fileName = QStringLiteral(":/") + m_source.fileName();

    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly)) {
        QXmlStreamReader reader(&f);
        QString fillColor = QStringLiteral("#ffffff");
        while (!reader.atEnd()) {
            reader.readNext();
            QXmlStreamAttributes attrs = reader.attributes();
            if (reader.isStartElement() && attrs.hasAttribute(QStringLiteral("fill")))
                fillColor = attrs.value(QStringLiteral("fill")).toString();
            if (reader.isStartElement() && reader.name() == QStringLiteral("path")) {
                m_fillColors.append(fillColor);
                if (attrs.hasAttribute(QStringLiteral("d")))
                    m_paths.append(attrs.value(QStringLiteral("d")).toString());
                if (attrs.hasAttribute(QStringLiteral("fill"))) {
                    m_fillColors[m_fillColors.size() - 1] = attrs.value(QStringLiteral("fill")).toString();
                } else if (attrs.hasAttribute(QStringLiteral("style"))) {
                    QString s = attrs.value(QStringLiteral("style")).toString();
                    int idx = s.indexOf(QStringLiteral("fill:"));
                    if (idx >= 0) {
                        idx = s.indexOf(QLatin1Char('#'), idx);
                        if (idx >= 0)
                            m_fillColors[m_fillColors.size() - 1] = s.mid(idx, 7);
                    }
                }
            }
        }
    } else {
        qWarning() << "Can't open file" << fileName;
    }

    emit pathsChanged();
}
