// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKPATHINTERPOLATED_P_H
#define QQUICKPATHINTERPOLATED_P_H

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

#include <QStringList>
#include <QPainterPath>
#include <private/qquickpath_p.h>
#include <QtQuickVectorImageHelpers/qtquickvectorimagehelpersexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEHELPERS_EXPORT QQuickPathInterpolated : public QQuickCurve
{
    Q_OBJECT
    Q_PROPERTY(qreal factor READ factor WRITE setFactor NOTIFY factorChanged)
    Q_PROPERTY(QStringList svgPaths READ svgPaths WRITE setSvgPaths NOTIFY svgPathsChanged)

    QML_NAMED_ELEMENT(PathInterpolated)
    QML_ADDED_IN_VERSION(6, 11)
public:
    explicit QQuickPathInterpolated(QObject *parent = nullptr);
    qreal factor() const;
    void setFactor(qreal newFactor);
    QStringList svgPaths() const;
    void setSvgPaths(const QStringList &newSvgPaths);

    void addToPath(QPainterPath &path, const QQuickPathData &) override;

Q_SIGNALS:
    void factorChanged();
    void svgPathsChanged();

private:
    QStringList m_svgPaths;
    QList<QPainterPath> m_paths;
    qreal m_factor = 0;
    bool m_dirty = false;
};

QT_END_NAMESPACE

#endif // QQUICKPATHINTERPOLATED_P_H
