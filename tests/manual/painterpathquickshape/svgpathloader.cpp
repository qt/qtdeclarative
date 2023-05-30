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

struct SvgState
{
    QString fillColor = {};
    QString strokeColor = {};
    QString strokeWidth = {};
};

void SvgPathLoader::loadPaths()
{
    m_paths.clear();
    m_fillColors.clear();
    m_strokeColors.clear();
    m_strokeWidths.clear();
    if (m_source.isEmpty())
        return;

    QStack<SvgState> states;
    states.push({});

    QString fileName;
    if (m_source.isLocalFile())
        fileName = m_source.toLocalFile();
    else if (m_source.scheme() == QStringLiteral("qrc"))
        fileName = QStringLiteral(":/") + m_source.fileName();

    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly)) {
        QXmlStreamReader reader(&f);
        SvgState currentState = {};
        while (!reader.atEnd()) {
            reader.readNext();
            QXmlStreamAttributes attrs = reader.attributes();
            if (reader.isStartElement())
                states.push(currentState);
            if (attrs.hasAttribute(QStringLiteral("fill"))) {
                currentState.fillColor = attrs.value(QStringLiteral("fill")).toString();
            } else if (attrs.hasAttribute(QStringLiteral("style"))) {
                QString s = attrs.value(QStringLiteral("style")).toString();
                int idx = s.indexOf(QStringLiteral("fill:"));
                if (idx >= 0) {
                    idx = s.indexOf(QLatin1Char('#'), idx);
                    if (idx >= 0)
                        currentState.fillColor = s.mid(idx, 7);
                }
            }
            if (currentState.fillColor == QStringLiteral("none"))
                currentState.fillColor = {};
            if (attrs.hasAttribute(QStringLiteral("stroke"))) {
                currentState.strokeColor = attrs.value(QStringLiteral("stroke")).toString();
                if (currentState.strokeColor == QStringLiteral("none"))
                    currentState.strokeColor = {};
            }
            if (attrs.hasAttribute(QStringLiteral("stroke-width")))
                currentState.strokeWidth = attrs.value(QStringLiteral("stroke-width")).toString();
            if (reader.isStartElement() && reader.name() == QStringLiteral("path")) {
                m_fillColors.append(currentState.fillColor);
                m_strokeColors.append(currentState.strokeColor);
                m_strokeWidths.append(currentState.strokeWidth);
                if (attrs.hasAttribute(QStringLiteral("d"))) {
                    m_paths.append(attrs.value(QStringLiteral("d")).toString());
                }
            }
            if (reader.isEndElement()) {
                currentState = states.pop();
            }
        }
    } else {
        qWarning() << "Can't open file" << fileName;
    }

    emit pathsChanged();
}
