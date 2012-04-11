/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef TESTTYPES_H
#define TESTTYPES_H

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QVector3D>

class NestedObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int dummy READ dummy);
    Q_PROPERTY(int result READ result FINAL CONSTANT);

public:
    int dummy() const { return 7; }
    int result() const { return 37; }
};

class ResultObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int result READ result WRITE setResult FINAL)
    Q_PROPERTY(NestedObject *nested READ nested CONSTANT)
    Q_PROPERTY(NestedObject *nested2 READ nested2 FINAL CONSTANT)
public:
    ResultObject() : m_result(0), m_resultCounter(0) {}

    int resultCounter() const { return m_resultCounter; }
    void resetResultCounter() { m_resultCounter = 0; }

    int result() const { return m_result; }
    void setResult(int result) { m_result = result; m_resultCounter++; }

    NestedObject *nested() { return &m_nested; }
    NestedObject *nested2() { return &m_nested2; }

private:
    int m_result;
    int m_resultCounter;

    NestedObject m_nested;
    NestedObject m_nested2;
};

class ConversionObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool boolProp READ boolProp WRITE setBoolProp NOTIFY boolPropChanged)
    Q_PROPERTY(int intProp READ intProp WRITE setIntProp NOTIFY intPropChanged)
    Q_PROPERTY(float floatProp READ floatProp WRITE setFloatProp NOTIFY floatPropChanged)
    Q_PROPERTY(double doubleProp READ doubleProp WRITE setDoubleProp NOTIFY doublePropChanged)
    Q_PROPERTY(qreal qrealProp READ qrealProp WRITE setQrealProp NOTIFY qrealPropChanged)
    Q_PROPERTY(QString qstringProp READ qstringProp WRITE setQstringProp NOTIFY qstringPropChanged)
    Q_PROPERTY(QUrl qurlProp READ qurlProp WRITE setQurlProp NOTIFY qurlPropChanged)
    Q_PROPERTY(QVector3D vec3Prop READ vec3Prop WRITE setVec3Prop NOTIFY vec3PropChanged)

public:
    ConversionObject() : m_boolProp(false), m_intProp(0), m_floatProp(0.0), m_doubleProp(0.0), m_qrealProp(0.0) {}
    ~ConversionObject() {}

    bool boolProp() const { return m_boolProp; }
    void setBoolProp(bool v) { m_boolProp = v; emit boolPropChanged(); }
    int intProp() const { return m_intProp; }
    void setIntProp(int v) { m_intProp = v; emit intPropChanged(); }
    float floatProp() const { return m_floatProp; }
    void setFloatProp(float v) { m_floatProp = v; emit floatPropChanged(); }
    double doubleProp() const { return m_doubleProp; }
    void setDoubleProp(double v) { m_doubleProp = v; emit doublePropChanged(); }
    qreal qrealProp() const { return m_qrealProp; }
    void setQrealProp(qreal v) { m_qrealProp = v; emit qrealPropChanged(); }
    QString qstringProp() const { return m_qstringProp; }
    void setQstringProp(const QString& v) { m_qstringProp = v; emit qstringPropChanged(); }
    QUrl qurlProp() const { return m_qurlProp; }
    void setQurlProp(const QUrl& v) { m_qurlProp = v; emit qurlPropChanged(); }
    QVector3D vec3Prop() const { return m_vec3Prop; }
    void setVec3Prop(const QVector3D& v) { m_vec3Prop = v; emit vec3PropChanged(); }

signals:
    void boolPropChanged();
    void intPropChanged();
    void floatPropChanged();
    void doublePropChanged();
    void qrealPropChanged();
    void qstringPropChanged();
    void qurlPropChanged();
    void vec3PropChanged();

private:
    bool m_boolProp;
    int m_intProp;
    float m_floatProp;
    double m_doubleProp;
    qreal m_qrealProp;
    QString m_qstringProp;
    QUrl m_qurlProp;
    QVector3D m_vec3Prop;
};

void registerTypes();

#endif // TESTTYPES_H

