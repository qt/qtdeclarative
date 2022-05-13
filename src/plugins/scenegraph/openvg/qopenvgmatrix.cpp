// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopenvgmatrix.h"

QT_BEGIN_NAMESPACE

// QOpenVGMatrix: Because Qt will never have enough matrix classes
//                Internally the data is stored as column-major format
//                So this is a 3x3 version of QMatrix4x4 for optimal
//                OpenVG usage.

QOpenVGMatrix::QOpenVGMatrix()
{
    setToIdentity();
}

QOpenVGMatrix::QOpenVGMatrix(const float *values)
{
    for (int col = 0; col < 3; ++col)
        for (int row = 0; row < 3; ++row)
            m[col][row] = values[col * 3 + row];
}

const float &QOpenVGMatrix::operator()(int row, int column) const
{
    Q_ASSERT(row >= 0 && row < 4 && column >= 0 && column < 4);
    return m[column][row];
}

float &QOpenVGMatrix::operator()(int row, int column)
{
    Q_ASSERT(row >= 0 && row < 4 && column >= 0 && column < 4);
    return m[column][row];
}

bool QOpenVGMatrix::isIdentity() const
{
    if (m[0][0] != 1.0f || m[0][1] != 0.0f || m[0][2] != 0.0f)
        return false;
    if ( m[1][0] != 0.0f || m[1][1] != 1.0f)
        return false;
    if (m[1][2] != 0.0f || m[2][0] != 0.0f)
        return false;
    if (m[2][1] != 0.0f || m[2][2] != 1.0f)
        return false;

    return true;
}

void QOpenVGMatrix::setToIdentity()
{
    m[0][0] = 1.0f;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = 1.0f;
    m[1][2] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = 1.0f;
}

bool QOpenVGMatrix::isAffine() const
{
    if (m[0][2] == 0.0f && m[1][2] == 0.0f && m[2][2] == 1.0f)
        return true;

    return false;
}

QPointF QOpenVGMatrix::map(const QPointF &point) const
{
    return *this * point;
}

void QOpenVGMatrix::fill(float value)
{
    m[0][0] = value;
    m[0][1] = value;
    m[0][2] = value;
    m[1][0] = value;
    m[1][1] = value;
    m[1][2] = value;
    m[2][0] = value;
    m[2][1] = value;
    m[2][2] = value;
}

QOpenVGMatrix QOpenVGMatrix::transposed() const
{
    QOpenVGMatrix result;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col)
             result.m[col][row] = m[row][col];
    }
    return result;
}

QOpenVGMatrix &QOpenVGMatrix::operator+=(const QOpenVGMatrix &other)
{
    m[0][0] += other.m[0][0];
    m[0][1] += other.m[0][1];
    m[0][2] += other.m[0][2];
    m[1][0] += other.m[1][0];
    m[1][1] += other.m[1][1];
    m[1][2] += other.m[1][2];
    m[2][0] += other.m[2][0];
    m[2][1] += other.m[2][1];
    m[2][2] += other.m[2][2];
    return *this;
}

QOpenVGMatrix &QOpenVGMatrix::operator-=(const QOpenVGMatrix &other)
{
    m[0][0] -= other.m[0][0];
    m[0][1] -= other.m[0][1];
    m[0][2] -= other.m[0][2];
    m[1][0] -= other.m[1][0];
    m[1][1] -= other.m[1][1];
    m[1][2] -= other.m[1][2];
    m[2][0] -= other.m[2][0];
    m[2][1] -= other.m[2][1];
    m[2][2] -= other.m[2][2];
    return *this;
}

QOpenVGMatrix &QOpenVGMatrix::operator*=(const QOpenVGMatrix &other)
{
    float m0, m1;
    m0      = m[0][0] * other.m[0][0]
            + m[1][0] * other.m[0][1]
            + m[2][0] * other.m[0][2];
    m1      = m[0][0] * other.m[1][0]
            + m[1][0] * other.m[1][1]
            + m[2][0] * other.m[1][2];
    m[2][0] = m[0][0] * other.m[2][0]
            + m[1][0] * other.m[2][1]
            + m[2][0] * other.m[2][2];
    m[0][0] = m0;
    m[1][0] = m1;

    m0      = m[0][1] * other.m[0][0]
            + m[1][1] * other.m[0][1]
            + m[2][1] * other.m[0][2];
    m1      = m[0][1] * other.m[1][0]
            + m[1][1] * other.m[1][1]
            + m[2][1] * other.m[1][2];
    m[2][1] = m[0][1] * other.m[2][0]
            + m[1][1] * other.m[2][1]
            + m[2][1] * other.m[2][2];
    m[0][1] = m0;
    m[1][1] = m1;

    m0      = m[0][2] * other.m[0][0]
            + m[1][2] * other.m[0][1]
            + m[2][2] * other.m[0][2];
    m1      = m[0][2] * other.m[1][0]
            + m[1][2] * other.m[1][1]
            + m[2][2] * other.m[1][2];
    m[2][2] = m[0][2] * other.m[2][0]
            + m[1][2] * other.m[2][1]
            + m[2][2] * other.m[2][2];
    m[0][2] = m0;
    m[1][2] = m1;
    return *this;
}

QOpenVGMatrix &QOpenVGMatrix::operator*=(float factor)
{
    m[0][0] *= factor;
    m[0][1] *= factor;
    m[0][2] *= factor;
    m[1][0] *= factor;
    m[1][1] *= factor;
    m[1][2] *= factor;
    m[2][0] *= factor;
    m[2][1] *= factor;
    m[2][2] *= factor;
    return *this;
}

QOpenVGMatrix &QOpenVGMatrix::operator/=(float divisor)
{
    m[0][0] /= divisor;
    m[0][1] /= divisor;
    m[0][2] /= divisor;
    m[1][0] /= divisor;
    m[1][1] /= divisor;
    m[1][2] /= divisor;
    m[2][0] /= divisor;
    m[2][1] /= divisor;
    m[2][2] /= divisor;
    return *this;
}

bool QOpenVGMatrix::operator==(const QOpenVGMatrix &other) const
{
    return m[0][0] == other.m[0][0] &&
           m[0][1] == other.m[0][1] &&
           m[0][2] == other.m[0][2] &&
           m[1][0] == other.m[1][0] &&
           m[1][1] == other.m[1][1] &&
           m[1][2] == other.m[1][2] &&
           m[2][0] == other.m[2][0] &&
           m[2][1] == other.m[2][1] &&
           m[2][2] == other.m[2][2];
}

bool QOpenVGMatrix::operator!=(const QOpenVGMatrix &other) const
{
    return m[0][0] != other.m[0][0] ||
           m[0][1] != other.m[0][1] ||
           m[0][2] != other.m[0][2] ||
           m[1][0] != other.m[1][0] ||
           m[1][1] != other.m[1][1] ||
           m[1][2] != other.m[1][2] ||
           m[2][0] != other.m[2][0] ||
           m[2][1] != other.m[2][1] ||
           m[2][2] != other.m[2][2];
}

void QOpenVGMatrix::copyDataTo(float *values) const
{
    // Row-Major?
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col)
            values[row * 3 + col] = float(m[col][row]);
    }
}

QOpenVGMatrix operator*(const QOpenVGMatrix &m1, const QOpenVGMatrix &m2)
{
    QOpenVGMatrix matrix;
    matrix.m[0][0] = m1.m[0][0] * m2.m[0][0]
                   + m1.m[1][0] * m2.m[0][1]
                   + m1.m[2][0] * m2.m[0][2];
    matrix.m[0][1] = m1.m[0][1] * m2.m[0][0]
                   + m1.m[1][1] * m2.m[0][1]
                   + m1.m[2][1] * m2.m[0][2];
    matrix.m[0][2] = m1.m[0][2] * m2.m[0][0]
                   + m1.m[1][2] * m2.m[0][1]
                   + m1.m[2][2] * m2.m[0][2];

    matrix.m[1][0] = m1.m[0][0] * m2.m[1][0]
                   + m1.m[1][0] * m2.m[1][1]
                   + m1.m[2][0] * m2.m[1][2];
    matrix.m[1][1] = m1.m[0][1] * m2.m[1][0]
                   + m1.m[1][1] * m2.m[1][1]
                   + m1.m[2][1] * m2.m[1][2];
    matrix.m[1][2] = m1.m[0][2] * m2.m[1][0]
                   + m1.m[1][2] * m2.m[1][1]
                   + m1.m[2][2] * m2.m[1][2];

    matrix.m[2][0] = m1.m[0][0] * m2.m[2][0]
                   + m1.m[1][0] * m2.m[2][1]
                   + m1.m[2][0] * m2.m[2][2];
    matrix.m[2][1] = m1.m[0][1] * m2.m[2][0]
                   + m1.m[1][1] * m2.m[2][1]
                   + m1.m[2][1] * m2.m[2][2];
    matrix.m[2][2] = m1.m[0][2] * m2.m[2][0]
                   + m1.m[1][2] * m2.m[2][1]
                   + m1.m[2][2] * m2.m[2][2];
    return matrix;
}

QPointF operator*(const QPointF& point, const QOpenVGMatrix& matrix)
{
    float xin = point.x();
    float yin = point.y();
    float x = xin * matrix.m[0][0] +
              yin * matrix.m[0][1] +
              matrix.m[0][2];
    float y = xin * matrix.m[1][0] +
              yin * matrix.m[1][1] +
              matrix.m[1][2];
    float w = xin * matrix.m[2][0] +
              yin * matrix.m[2][1] +
              matrix.m[2][2];
    if (w == 1.0f) {
        return QPointF(float(x), float(y));
    } else {
        return QPointF(float(x / w), float(y / w));
    }
}

QPointF operator*(const QOpenVGMatrix& matrix, const QPointF& point)
{
    float xin = point.x();
    float yin = point.y();
    float x = xin * matrix.m[0][0] +
              yin * matrix.m[1][0] +
              matrix.m[2][0];
    float y = xin * matrix.m[0][1] +
              yin * matrix.m[1][1] +
              matrix.m[2][1];
    float w = xin * matrix.m[0][2] +
              yin * matrix.m[1][2] +
              matrix.m[2][2];
    if (w == 1.0f) {
        return QPointF(float(x), float(y));
    } else {
        return QPointF(float(x / w), float(y / w));
    }
}

QDebug operator<<(QDebug dbg, const QOpenVGMatrix &m)
{
    QDebugStateSaver saver(dbg);
    // Output in row-major order because it is more human-readable.
    dbg.nospace() << "QOpenVGMatrix:(" << Qt::endl
                  << qSetFieldWidth(10)
                  << m(0, 0) << m(0, 1) << m(0, 2) << Qt::endl
                  << m(1, 0) << m(1, 1) << m(1, 2) << Qt::endl
                  << m(2, 0) << m(2, 1) << m(2, 2) << Qt::endl
                  << qSetFieldWidth(0) << ')';
    return dbg;
}

QDataStream &operator<<(QDataStream &stream, const QOpenVGMatrix &matrix)
{
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            stream << matrix(row, col);
    return stream;
}


QDataStream &operator>>(QDataStream &stream, QOpenVGMatrix &matrix)
{
    float x;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            stream >> x;
            matrix(row, col) = x;
        }
    }
    return stream;
}


QT_END_NAMESPACE
