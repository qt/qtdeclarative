// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "svgpathloader.h"

#include <QFile>
#include <QPainterPath>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QStack>
#include <QLocale>
#include <QMatrix4x4>

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
    QMatrix4x4 transform;
};

void SvgPathLoader::loadPaths()
{
    m_paths.clear();
    m_fillColors.clear();
    m_strokeColors.clear();
    m_strokeWidths.clear();
    m_transforms.clear();
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

            if (attrs.hasAttribute(QStringLiteral("transform"))) {
                QString t = attrs.value(QStringLiteral("transform")).toString();
                const bool isTranslate = t.startsWith(QStringLiteral("translate"));
                const bool isScale = t.startsWith(QStringLiteral("scale"));
                const bool isMatrix = t.startsWith(QStringLiteral("matrix"));
                if (isTranslate || isScale || isMatrix) {
                    int pStart = t.indexOf(QLatin1Char('('));
                    int pEnd = t.indexOf(QLatin1Char(')'));
                    if (pStart >= 0 && pEnd > pStart + 1) {
                        t = t.mid(pStart + 1, pEnd - pStart - 1);
                        QStringList coords = t.split(QLatin1Char(','));
                        if (isMatrix && coords.size() == 6) {
                            QMatrix3x3 m;
                            m(0, 0) = coords.at(0).toDouble();
                            m(1, 0) = coords.at(1).toDouble();
                            m(2, 0) = 0.0f;

                            m(0, 1) = coords.at(2).toDouble();
                            m(1, 1) = coords.at(3).toDouble();
                            m(2, 1) = 0.0f;

                            m(0, 2) = coords.at(4).toDouble();
                            m(1, 2) = coords.at(5).toDouble();
                            m(2, 2) = 1.0f;

                            currentState.transform *= QMatrix4x4(m);
                        } else if (coords.size() == 2) {
                            qreal c1 = coords.first().toDouble();
                            qreal c2 = coords.last().toDouble();

                            if (isTranslate)
                                currentState.transform.translate(c1, c2);
                            else if (isScale)
                                currentState.transform.scale(c1, c2);
                        }
                    }
                }
            }

            if (attrs.hasAttribute(QStringLiteral("fill"))) {
                currentState.fillColor = attrs.value(QStringLiteral("fill")).toString();
                if (!currentState.fillColor.startsWith("#"))
                    currentState.fillColor = "";
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

                QString t;
                for (int i = 0; i < 4; ++i) {
                    if (i > 0)
                        t += QLatin1Char(',');
                    QVector4D row = currentState.transform.row(i);

                    QLocale c(QLocale::C);
                    t += QStringLiteral("%1, %2, %3, %4")
                            .arg(c.toString(row.x()))
                            .arg(c.toString(row.y()))
                            .arg(c.toString(row.z()))
                            .arg(c.toString(row.w()));
                }

                m_transforms.append(t);
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
