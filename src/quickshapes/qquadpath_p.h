// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUADPATH_P_H
#define QQUADPATH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qrect.h>
#include <QtCore/qlist.h>
#include <QtCore/qdebug.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qpainterpath.h>

QT_BEGIN_NAMESPACE

class QQuadPath
{
public:
    class Element
    {
    public:
        Element ()
            : m_isSubpathStart(false), m_isSubpathEnd(false), m_isLine(false)
        {
        }

        bool isSubpathStart() const
        {
            return m_isSubpathStart;
        }

        bool isSubpathEnd() const
        {
            return m_isSubpathEnd;
        }

        bool isLine() const
        {
            return m_isLine;
        }

        bool isConvex() const
        {
            return m_curvatureFlags & Convex;
        }

        QVector2D startPoint() const
        {
            return sp;
        }

        QVector2D controlPoint() const
        {
            return cp;
        }

        QVector2D endPoint() const
        {
            return ep;
        }

        QVector2D midPoint() const
        {
            return isLine() ? 0.5f * (sp + ep) : (0.25f * sp) + (0.5f * cp) + (0.25 * ep);
        }

        int childCount() const { return m_numChildren; }

        int indexOfChild(int childNumber) const
        {
            Q_ASSERT(childNumber >= 0 && childNumber < childCount());
            return -(m_firstChildIndex + 1 + childNumber);
        }

        QVector2D pointAtFraction(float t) const;

        QVector2D tangentAtFraction(float t) const
        {
            return isLine() ? (ep - sp) : ((1 - t) * 2 * (cp - sp)) + (t * 2 * (ep - cp));
        }

        QVector2D normalAtFraction(float t) const
        {
            const QVector2D tan = tangentAtFraction(t);
            return QVector2D(-tan.y(), tan.x());
        }

        float extent() const;

        void setAsConvex(bool isConvex)
        {
            if (isConvex)
                m_curvatureFlags = Element::CurvatureFlags(m_curvatureFlags | Element::Convex);
            else
                m_curvatureFlags = Element::CurvatureFlags(m_curvatureFlags & ~Element::Convex);
        }

        bool isControlPointOnLeft() const
        {
            return isPointOnLeft(cp, sp, ep);
        }

    private:
        int intersectionsAtY(float y, float *fractions) const;

        enum CurvatureFlags : quint8 {
            CurvatureUndetermined = 0,
            FillOnRight = 1,
            Convex = 2
        };

        QVector2D sp;
        QVector2D cp;
        QVector2D ep;
        int m_firstChildIndex = 0;
        quint8 m_numChildren = 0;
        CurvatureFlags m_curvatureFlags = CurvatureUndetermined;
        quint8 m_isSubpathStart : 1;
        quint8 m_isSubpathEnd : 1;
        quint8 m_isLine : 1;
        friend class QQuadPath;
        friend QDebug operator<<(QDebug, const QQuadPath::Element &);
    };

    void moveTo(const QVector2D &to)
    {
        subPathToStart = true;
        currentPoint = to;
    }

    void lineTo(const QVector2D &to)
    {
        addElement({}, to, true);
    }

    void quadTo(const QVector2D &control, const QVector2D &to)
    {
        addElement(control, to);
    }

    Element &elementAt(int i)
    {
        return i < 0 ? m_childElements[-(i + 1)] : m_elements[i];
    }

    const Element &elementAt(int i) const
    {
        return i < 0 ? m_childElements[-(i + 1)] : m_elements[i];
    }

    int indexOfChildAt(int i, int childNumber) const
    {
        return elementAt(i).indexOfChild(childNumber);
    }

    QRectF controlPointRect() const;

    Qt::FillRule fillRule() const { return m_fillRule; }
    void setFillRule(Qt::FillRule rule) { m_fillRule = rule; }

    void reserve(int size) { m_elements.reserve(size); }
    int elementCount() const { return m_elements.size(); }
    bool isEmpty() const { return m_elements.size() == 0; }
    int elementCountRecursive() const;

    static QQuadPath fromPainterPath(const QPainterPath &path);
    QPainterPath toPainterPath() const;

    QQuadPath subPathsClosed() const;
    void addCurvatureData();
    QQuadPath flattened() const;
    QQuadPath dashed(qreal lineWidth, const QList<qreal> &dashPattern, qreal dashOffset = 0) const;
    void splitElementAt(int index);
    bool contains(const QVector2D &point) const;

    template<typename Func>
    void iterateChildrenOf(Element &e, Func &&lambda)
    {
        const int lastChildIndex = e.m_firstChildIndex + e.childCount() - 1;
        for (int i = e.m_firstChildIndex; i <= lastChildIndex; i++) {
            Element &c = m_childElements[i];
            if (c.childCount() > 0)
                iterateChildrenOf(c, lambda);
            else
                lambda(c);
        }
    }

    template<typename Func>
    void iterateChildrenOf(const Element &e, Func &&lambda) const
    {
        const int lastChildIndex = e.m_firstChildIndex + e.childCount() - 1;
        for (int i = e.m_firstChildIndex; i <= lastChildIndex; i++) {
            const Element &c = m_childElements[i];
            if (c.childCount() > 0)
                iterateChildrenOf(c, lambda);
            else
                lambda(c);
        }
    }

    template<typename Func>
    void iterateElements(Func &&lambda)
    {
        for (auto &e : m_elements) {
            if (e.childCount() > 0)
                iterateChildrenOf(e, lambda);
            else
                lambda(e);
        }
    }

    template<typename Func>
    void iterateElements(Func &&lambda) const
    {
        for (auto &e : m_elements) {
            if (e.childCount() > 0)
                iterateChildrenOf(e, lambda);
            else
                lambda(e);
        }
    }

    static QVector2D closestPointOnLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);
    static bool isPointOnLeft(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);
    static bool isPointOnLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);
    static bool isPointNearLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);

private:
    void addElement(const QVector2D &control, const QVector2D &to, bool isLine = false);
    Element::CurvatureFlags coordinateOrderOfElement(const Element &element) const;

    friend QDebug operator<<(QDebug, const QQuadPath &);

    bool subPathToStart = true;
    Qt::FillRule m_fillRule = Qt::OddEvenFill;
    QVector2D currentPoint;
    QList<Element> m_elements;
    QList<Element> m_childElements;
};

QDebug operator<<(QDebug, const QQuadPath::Element &);
QDebug operator<<(QDebug, const QQuadPath &);

QT_END_NAMESPACE

#endif
