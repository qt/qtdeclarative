// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENVGMATRIX_H
#define QOPENVGMATRIX_H

#include <QtCore/qdebug.h>
#include <QtCore/QDataStream>
#include <QtCore/QPointF>

QT_BEGIN_NAMESPACE

class QOpenVGMatrix
{
public:
    QOpenVGMatrix();
    explicit QOpenVGMatrix(const float *values);

    const float& operator()(int row, int column) const;
    float& operator()(int row, int column);

    bool isIdentity() const;
    void setToIdentity();

    bool isAffine() const;

    QPointF map(const QPointF& point) const;

    void fill(float value);

    QOpenVGMatrix transposed() const;

    QOpenVGMatrix& operator+=(const QOpenVGMatrix& other);
    QOpenVGMatrix& operator-=(const QOpenVGMatrix& other);
    QOpenVGMatrix& operator*=(const QOpenVGMatrix& other);
    QOpenVGMatrix& operator*=(float factor);
    QOpenVGMatrix& operator/=(float divisor);
    friend QOpenVGMatrix operator*(const QOpenVGMatrix& m1, const QOpenVGMatrix& m2);
    friend QPointF operator*(const QPointF& point, const QOpenVGMatrix& matrix);
    friend QPointF operator*(const QOpenVGMatrix& matrix, const QPointF& point);
#ifndef QT_NO_DEBUG_STREAM
    friend QDebug operator<<(QDebug dbg, const QOpenVGMatrix &m);
#endif
    bool operator==(const QOpenVGMatrix& other) const;
    bool operator!=(const QOpenVGMatrix& other) const;

    void copyDataTo(float *values) const;

    float *data() { return *m; }
    const float *data() const { return *m; }
    const float *constData() const { return *m; }

private:
    float m[3][3];
};

QOpenVGMatrix operator*(const QOpenVGMatrix& m1, const QOpenVGMatrix& m2);
QPointF operator*(const QPointF& point, const QOpenVGMatrix& matrix);
QPointF operator*(const QOpenVGMatrix& matrix, const QPointF& point);

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QOpenVGMatrix &m);
#endif

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &, const QOpenVGMatrix &);
QDataStream &operator>>(QDataStream &, QOpenVGMatrix &);
#endif

QT_END_NAMESPACE

#endif // QOPENVGMATRIX_H
