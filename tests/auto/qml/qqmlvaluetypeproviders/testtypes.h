// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef TESTTYPES_H
#define TESTTYPES_H

#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QRect>
#include <QRectF>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QQuaternion>
#include <QMatrix4x4>
#include <QFont>
#include <QColor>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <qqml.h>

struct ConstructibleValueType
{
    Q_GADGET
    Q_PROPERTY(int foo MEMBER m_foo CONSTANT)

    QML_VALUE_TYPE(constructible)
    QML_CONSTRUCTIBLE_VALUE

public:
    ConstructibleValueType() = default;
    Q_INVOKABLE ConstructibleValueType(int foo) : m_foo(foo) {}
    Q_INVOKABLE ConstructibleValueType(QObject *) : m_foo(67) {}
    Q_INVOKABLE ConstructibleValueType(const QUrl &) : m_foo(68) {}

    int foo() const { return m_foo; }

private:
    friend bool operator==(const ConstructibleValueType &a, const ConstructibleValueType &b)
    {
        return a.m_foo == b.m_foo;
    }

    int m_foo = 0;
};

struct ConstructibleFromQReal
{
    Q_GADGET
    Q_PROPERTY(qreal foo MEMBER m_foo CONSTANT)

    QML_VALUE_TYPE(constructibleFromQReal)
    QML_CONSTRUCTIBLE_VALUE

public:
    ConstructibleFromQReal() = default;
    Q_INVOKABLE ConstructibleFromQReal(qreal foo) : m_foo(foo) {}

    qreal foo() const { return m_foo; }

private:
    friend bool operator==(const ConstructibleFromQReal &a, const ConstructibleFromQReal &b)
    {
        if (qIsNaN(a.m_foo) && qIsNaN(b.m_foo))
            return true;
        if (qIsInf(a.m_foo) && qIsInf(b.m_foo))
            return (a.m_foo > 0) == (b.m_foo > 0);
        return qFuzzyCompare(a.m_foo, b.m_foo);
    }

    qreal m_foo = 0;
};

struct StructuredValueType
{
    Q_GADGET
    Q_PROPERTY(int i READ i WRITE setI)
    Q_PROPERTY(ConstructibleValueType c READ c WRITE setC)
    Q_PROPERTY(QPointF p READ p WRITE setP)
    Q_PROPERTY(QList<QSizeF> sizes READ sizes WRITE setSizes)

    QML_VALUE_TYPE(structured)
    QML_STRUCTURED_VALUE

public:
    int i() const { return m_i; }
    void setI(int newI) { m_i = newI; }

    const ConstructibleValueType &c() const { return m_c; }
    void setC(const ConstructibleValueType &newC) { m_c = newC; }

    QPointF p() const { return m_p; }
    void setP(QPointF newP) { m_p = newP; }

    const QList<QSizeF> &sizes() const { return m_sizes; }
    void setSizes(const QList<QSizeF> &sizes) { m_sizes = sizes; }

    Q_INVOKABLE QList<QSizeF> sizesDetached() const { return m_sizes; }

private:

    friend bool operator==(const StructuredValueType &a, const StructuredValueType &b)
    {
        return a.m_i == b.m_i && a.m_c == b.m_c && a.m_p == b.m_p && a.m_sizes == b.m_sizes;
    }

    int m_i = 0;
    ConstructibleValueType m_c;
    QPointF m_p;
    QList<QSizeF> m_sizes = { QSizeF(1, 1), QSizeF(2, 2) };
};

struct BarrenValueType
{
    Q_GADGET
    Q_PROPERTY(int i READ i WRITE setI)

public:
    BarrenValueType() = default;
    Q_INVOKABLE BarrenValueType(const QString &) : m_i(25) {}

    int i() const { return m_i; }
    void setI(int newI) { m_i = newI; }

private:
    friend bool operator==(const BarrenValueType &a, const BarrenValueType &b)
    {
        return a.m_i == b.m_i;
    }

    int m_i = 0;
};

struct ForeignAnonymousStructuredValueType
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(BarrenValueType)
    QML_STRUCTURED_VALUE
};

class MyTypeObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QPoint point READ point WRITE setPoint NOTIFY changed)
    Q_PROPERTY(QPointF pointf READ pointf WRITE setPointf NOTIFY changed)
    Q_PROPERTY(QPointF pointfpoint READ pointfpoint WRITE setPointfpoint NOTIFY changed)
    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY changed)
    Q_PROPERTY(QSizeF sizef READ sizef WRITE setSizef NOTIFY changed)
    Q_PROPERTY(QSizeF sizefsize READ sizefsize WRITE setSizefsize NOTIFY changed)
    Q_PROPERTY(QSize sizereadonly READ size NOTIFY changed)
    Q_PROPERTY(QRect rect READ rect WRITE setRect NOTIFY changed)
    Q_PROPERTY(QRectF rectf READ rectf WRITE setRectf NOTIFY changed)
    Q_PROPERTY(QRectF rectfrect READ rectfrect WRITE setRectfrect NOTIFY changed)
    Q_PROPERTY(QVector2D vector2 READ vector2 WRITE setVector2 NOTIFY changed)
    Q_PROPERTY(QVector3D vector READ vector WRITE setVector NOTIFY changed)
    Q_PROPERTY(QVector4D vector4 READ vector4 WRITE setVector4 NOTIFY changed)
    Q_PROPERTY(QQuaternion quaternion READ quaternion WRITE setQuaternion NOTIFY changed)
    Q_PROPERTY(QMatrix4x4 matrix READ matrix WRITE setMatrix NOTIFY changed)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY changed)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY changed)
    Q_PROPERTY(QVariant variant READ variant NOTIFY changed)
    Q_PROPERTY(ConstructibleValueType constructible READ constructible WRITE setConstructible NOTIFY constructibleChanged)
    Q_PROPERTY(StructuredValueType structured READ structured WRITE setStructured NOTIFY structuredChanged)
    Q_PROPERTY(BarrenValueType barren READ barren WRITE setBarren NOTIFY barrenChanged)

    Q_PROPERTY(QDateTime aDateTime READ aDateTime WRITE setADateTime NOTIFY aDateTimeChanged)
    Q_PROPERTY(QDate aDate READ aDate WRITE setADate NOTIFY aDateChanged)
    Q_PROPERTY(QTime aTime READ aTime WRITE setATime NOTIFY aTimeChanged)
    Q_PROPERTY(QVariant aVariant READ aVariant WRITE setAVariant NOTIFY aVariantChanged)

public:
    MyTypeObject() :
        m_point(10, 4),
        m_pointf(11.3, -10.9),
        m_pointfpoint(10.0, 4.0),
        m_size(1912, 1913),
        m_sizef(0.1, 100923.2),
        m_sizefsize(1912.0, 1913.0),
        m_rect(2, 3, 109, 102),
        m_rectf(103.8, 99.2, 88.1, 77.6),
        m_rectfrect(2.0, 3.0, 109.0, 102.0),
        m_vector2(32.88f, 1.3f),
        m_vector(23.88f, 3.1f, 4.3f),
        m_vector4(54.2f, 23.88f, 3.1f, 4.3f),
        m_quaternion(4.3f, 54.2f, 23.88f, 3.1f),
        m_matrix(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)
    {
        m_font.setFamily("Arial");
        m_font.setBold(true);
        m_font.setWeight(QFont::DemiBold);
        m_font.setItalic(true);
        m_font.setUnderline(true);
        m_font.setOverline(true);
        m_font.setStrikeOut(true);
        m_font.setPointSize(29);
        m_font.setCapitalization(QFont::AllLowercase);
        m_font.setLetterSpacing(QFont::AbsoluteSpacing, 10.2);
        m_font.setWordSpacing(19.7);
        m_color.setRedF(0.2f);
        m_color.setGreenF(0.88f);
        m_color.setBlueF(0.6f);
        m_color.setAlphaF(0.34f);
    }

    QPoint m_point;
    QPoint point() const { return m_point; }
    void setPoint(const QPoint &v) { m_point = v; emit changed(); }

    QPointF m_pointf;
    QPointF pointf() const { return m_pointf; }
    void setPointf(const QPointF &v) { m_pointf = v; emit changed(); }

    QPointF m_pointfpoint;
    QPointF pointfpoint() const { return m_pointfpoint; }
    void setPointfpoint(const QPointF &v) { m_pointfpoint = v; emit changed(); }

    QSize m_size;
    QSize size() const { return m_size; }
    void setSize(const QSize &v) { m_size = v; emit changed(); }

    QSizeF m_sizef;
    QSizeF sizef() const { return m_sizef; }
    void setSizef(const QSizeF &v) { m_sizef = v; emit changed(); }

    QSizeF m_sizefsize;
    QSizeF sizefsize() const { return m_sizefsize; }
    void setSizefsize(const QSizeF &v) { m_sizefsize = v; emit changed(); }

    QRect m_rect;
    QRect rect() const { return m_rect; }
    void setRect(const QRect &v) { m_rect = v; emit changed(); }

    QRectF m_rectf;
    QRectF rectf() const { return m_rectf; }
    void setRectf(const QRectF &v) { m_rectf = v; emit changed(); }

    QRectF m_rectfrect;
    QRectF rectfrect() const { return m_rectfrect; }
    void setRectfrect(const QRectF &v) { m_rectfrect = v; emit changed(); }

    QVector2D m_vector2;
    QVector2D vector2() const { return m_vector2; }
    void setVector2(const QVector2D &v) { m_vector2 = v; emit changed(); }

    QVector3D m_vector;
    QVector3D vector() const { return m_vector; }
    void setVector(const QVector3D &v) { m_vector = v; emit changed(); }

    QVector4D m_vector4;
    QVector4D vector4() const { return m_vector4; }
    void setVector4(const QVector4D &v) { m_vector4 = v; emit changed(); }

    QQuaternion m_quaternion;
    QQuaternion quaternion() const { return m_quaternion; }
    void setQuaternion(const QQuaternion &v) { m_quaternion = v; emit changed(); }

    QMatrix4x4 m_matrix;
    QMatrix4x4 matrix() const { return m_matrix; }
    void setMatrix(const QMatrix4x4 &v) { m_matrix = v; emit changed(); }

    QFont m_font;
    QFont font() const { return m_font; }
    void setFont(const QFont &v) { m_font = v; emit changed(); }

    QColor m_color;
    QColor color() const { return m_color; }
    void setColor(const QColor &v) { m_color = v; emit changed(); }

    QVariant variant() const { return sizef(); }

    void emitRunScript() { emit runScript(); }

    const ConstructibleValueType &constructible() const { return m_constructible; }
    void setConstructible(const ConstructibleValueType &newConstructible)
    {
        if (m_constructible == newConstructible)
            return;
        m_constructible = newConstructible;
        emit constructibleChanged();
    }

    const StructuredValueType &structured() const { return m_structured; }
    void setStructured(const StructuredValueType &newStructured)
    {
        if (m_structured == newStructured)
            return;
        m_structured = newStructured;
        emit structuredChanged();
    }

    QDateTime aDateTime() const
    {
        return m_aDateTime;
    }
    void setADateTime(const QDateTime &newADateTime)
    {
        if (m_aDateTime == newADateTime)
            return;
        m_aDateTime = newADateTime;
        emit aDateTimeChanged();
    }

    QDate aDate() const
    {
        return m_aDate;
    }
    void setADate(const QDate &newADate)
    {
        if (m_aDate == newADate)
            return;
        m_aDate = newADate;
        emit aDateChanged();
    }

    QTime aTime() const
    {
        return m_aTime;
    }
    void setATime(const QTime &newATime)
    {
        if (m_aTime == newATime)
            return;
        m_aTime = newATime;
        emit aTimeChanged();
    }

    QVariant aVariant() const
    {
        return m_aVariant;
    }
    void setAVariant(const QVariant &newAVariant)
    {
        if (m_aVariant == newAVariant)
            return;
        m_aVariant = newAVariant;
        emit aVariantChanged();
    }

    BarrenValueType barren() const
    {
        return m_barren;
    }

    void setBarren(const BarrenValueType &newBarren)
    {
        if (m_barren == newBarren)
            return;
        m_barren = newBarren;
        emit barrenChanged();
    }

    Q_INVOKABLE void acceptConstructible(const ConstructibleValueType &a)
    {
        setAVariant(QVariant::fromValue(a));
    }

    Q_INVOKABLE int acceptConstructibles(const QList<ConstructibleValueType> &constructibles)
    {
        int result = 0;
        for (const auto &c: constructibles) {
            result += c.foo();
        }
        return result;
    }

    Q_INVOKABLE StructuredValueType acceptStructured(const StructuredValueType &a)
    {
        return a;
    }

    Q_INVOKABLE void setEffectPadding(const QRect &r)
    {
        m_hasEffectPadding = true;
        m_effectPadding = r;
    }

    bool hasEffectPadding() const { return m_hasEffectPadding; }
    QRectF effectPadding() const { return m_effectPadding; }

signals:
    void changed();
    void runScript();

    void constructibleChanged();
    void structuredChanged();

    void aDateTimeChanged();
    void aDateChanged();
    void aTimeChanged();
    void aVariantChanged();

    void barrenChanged();

public slots:
    QSize method() { return QSize(13, 14); }
private:
    ConstructibleValueType m_constructible;
    StructuredValueType m_structured;

    QDateTime m_aDateTime;
    QDate m_aDate;
    QTime m_aTime;
    QVariant m_aVariant;
    BarrenValueType m_barren;
    QRectF m_effectPadding;
    bool m_hasEffectPadding = false;
};

class Padding
{
    Q_GADGET

    Q_PROPERTY(int left READ left WRITE setLeft)
    Q_PROPERTY(int right READ right WRITE setRight)

    QML_VALUE_TYPE(padding)
    QML_STRUCTURED_VALUE

public:
    enum LogType {
        DefaultCtor,
        CopyCtor,
        MoveCtor,
        CopyAssign,
        MoveAssign,
        InvokableCtor,
        CustomCtor,
        Invalid,
    };

    Q_ENUM(LogType);

    struct LogEntry {
        LogType type = Invalid;
        int left = 0;
        int right = 0;

        friend QDebug operator<<(QDebug &debug, const LogEntry &self)
        {
            return debug << self.type << " " << self.left << " " << self.right;
        }
    };

    static QList<LogEntry> log;

    void doLog(LogType type) {
        log.append({
            type, m_left, m_right
        });
    }

    Padding()
    {
        doLog(DefaultCtor);
    }

    Padding(const Padding &other)
        : m_left(other.m_left)
        , m_right(other.m_right)
    {
        doLog(CopyCtor);
    }

    Padding(Padding &&other)
        : m_left(other.m_left)
        , m_right(other.m_right)
    {
        doLog(MoveCtor);
    }

    Padding(int left, int right)
        : m_left( left )
        , m_right( right )
    {
        doLog(CustomCtor);
    }

    Padding &operator=(const Padding &other) {
        if (this != &other) {
            m_left = other.m_left;
            m_right = other.m_right;
        }
        doLog(CopyAssign);
        return *this;
    }

    Padding &operator=(Padding &&other) {
        if (this != &other) {
            m_left = other.m_left;
            m_right = other.m_right;
        }
        doLog(MoveAssign);
        return *this;
    }

    Q_INVOKABLE Padding(int padding )
        : m_left( padding )
        , m_right( padding )
    {
        doLog(InvokableCtor);
    }

    void setLeft(int padding) { m_left = padding; }
    int left() const { return m_left; }

    void setRight(int padding) { m_right = padding; }
    int right() const { return m_right; }

private:
    int m_left = 0;
    int m_right = 0;
};

class MyItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Padding padding READ padding WRITE setPadding NOTIFY paddingChanged)
    QML_ELEMENT

public:
    void setPadding(const Padding &padding)
    {
        if (padding.left() == m_padding.left() && padding.right() == m_padding.right())
            return;

        m_padding = padding;
        emit paddingChanged();
    }

    const Padding &padding() const{ return m_padding; }

signals:
    void paddingChanged();

private:
    Padding m_padding{ 17, 17 };
};

void registerTypes();

#endif // TESTTYPES_H
