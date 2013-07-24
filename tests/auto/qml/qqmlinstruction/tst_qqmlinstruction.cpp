/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include "../../shared/util.h"
#include <private/qqmlcompiler_p.h>

#include <QVector3D>
#include <QVector4D>

class tst_qqmlinstruction : public QObject
{
    Q_OBJECT
public:
    tst_qqmlinstruction() {}

private slots:
    void dump();

    void point();
    void pointf();
    void size();
    void sizef();
    void rect();
    void rectf();
    void vector3d();
    void vector4d();
    void time();
};

void tst_qqmlinstruction::dump()
{
    QQmlEngine engine;
    QQmlCompiledData *data = new QQmlCompiledData(&engine);

    {
        QQmlCompiledData::Instruction::Init i;
        i.bindingsSize = 0;
        i.parserStatusSize = 3;
        i.contextCache = -1;
        i.compiledBinding = -1;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::CreateCppObject i;
        i.type = 0;
        i.data = -1;
        i.column = 10;
        data->addInstruction(i);
    }

    {
        data->primitives << "testId";

        QQmlCompiledData::Instruction::SetId i;
        i.value = data->primitives.count() - 1;
        i.index = 0;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::SetDefault i;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::CreateComponent i;
        i.count = 3;
        i.column = 4;
        i.endLine = 14;
        i.metaObject = 0;

        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreMetaObject i;
        i.aliasData = 6;
        i.propertyCache = 7;

        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreFloat i;
        i.propertyIndex = 3;
        i.value = 11.3f;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreDouble i;
        i.propertyIndex = 4;
        i.value = 14.8f;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreInteger i;
        i.propertyIndex = 5;
        i.value = 9;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreBool i;
        i.propertyIndex = 6;
        i.value = true;

        data->addInstruction(i);
    }

    {
        data->primitives << "Test String";
        QQmlCompiledData::Instruction::StoreString i;
        i.propertyIndex = 7;
        i.value = data->primitives.count() - 1;
        data->addInstruction(i);
    }

    {
        data->urls << QUrl("http://www.qt-project.org");
        QQmlCompiledData::Instruction::StoreUrl i;
        i.propertyIndex = 8;
        i.value = data->urls.count() - 1;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreColor i;
        i.propertyIndex = 9;
        i.value = 0xFF00FF00;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreDate i;
        i.propertyIndex = 10;
        i.value = 9;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreTime i;
        i.propertyIndex = 11;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreDateTime i;
        i.propertyIndex = 12;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StorePoint i;
        i.propertyIndex = 13;
        i.point.xp = 3;
        i.point.yp = 7;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StorePointF i;
        i.propertyIndex = 13;
        i.point.xp = 3;
        i.point.yp = 7;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreSize i;
        i.propertyIndex = 15;
        i.size.wd = 8;
        i.size.ht = 11;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreSizeF i;
        i.propertyIndex = 15;
        i.size.wd = 8;
        i.size.ht = 11;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreRect i;
        i.propertyIndex = 17;
        i.rect.x1 = 7;
        i.rect.y1 = 9;
        i.rect.x2 = 11;
        i.rect.y2 = 13;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreRectF i;
        i.propertyIndex = 18;
        i.rect.xp = 11.3;
        i.rect.yp = 9.8;
        i.rect.w = 3;
        i.rect.h = 2.1;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreVector3D i;
        i.propertyIndex = 19;
        i.vector.xp = 9;
        i.vector.yp = 3;
        i.vector.zp = 92;
        data->addInstruction(i);
    }

    {
        data->primitives << "color(1, 1, 1, 1)";
        QQmlCompiledData::Instruction::StoreVariant i;
        i.propertyIndex = 20;
        i.value = data->primitives.count() - 1;

        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreObject i;
        i.propertyIndex = 21;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreVariantObject i;
        i.propertyIndex = 22;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreInterface i;
        i.propertyIndex = 23;
        data->addInstruction(i);
    }

    {
        data->primitives << "console.log(1921)";

        QQmlCompiledData::Instruction::StoreSignal i;
        i.signalIndex = 2;
        i.value = data->primitives.count() - 1;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreScriptString i;
        i.propertyIndex = 24;
        i.value = 3;
        i.scope = 1;
        i.bindingId = 4;
        data->addInstruction(i);
    }

    {
        data->primitives << "mySignal";

        QQmlCompiledData::Instruction::AssignSignalObject i;
        i.signal = data->primitives.count() - 1;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::AssignCustomType i;
        i.propertyIndex = 25;
        i.primitive = 6;
        i.type = 9;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreBinding i;
        i.property.coreIndex = 26;
        i.value = 3;
        i.context = 2;
        i.owner = 0;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreV4Binding i;
        i.property = 27;
        i.value = 2;
        i.context = 4;
        i.owner = 0;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreValueSource i;
        i.property.coreIndex = 29;
        i.castValue = 4;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreValueInterceptor i;
        i.property.coreIndex = 30;
        i.castValue = -4;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::BeginObject i;
        i.castValue = 4;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreObjectQList i;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::AssignObjectList i;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::FetchAttached i;
        i.id = 23;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::FetchQList i;
        i.property = 32;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::FetchObject i;
        i.property = 33;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::FetchValueType i;
        i.property = 34;
        i.type = 6;
        i.bindingSkipList = 7;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::PopFetchedObject i;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::PopQList i;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::PopValueType i;
        i.property = 35;
        i.type = 8;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::Defer i;
        i.deferCount = 7;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::Defer i;
        i.deferCount = 7;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreImportedScript i;
        i.value = 2;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreVariantInteger i;
        i.value = 11;
        i.propertyIndex = 32;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreVariantDouble i;
        i.value = 33.7;
        i.propertyIndex = 19;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::Done i;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreTrString i;
        i.propertyIndex = 99;
        i.context = 3;
        i.text = 14;
        i.comment = 14;
        i.n = 2;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreTrIdString i;
        i.propertyIndex = 78;
        i.text = 7;
        i.n = -1;
        data->addInstruction(i);
    }

    {
        data->primitives << "color(1, 1, 1, 1)";
        QQmlCompiledData::Instruction::StoreVar i;
        i.propertyIndex = 79;
        i.value = data->primitives.count() - 1;

        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreVarObject i;
        i.propertyIndex = 80;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreVarInteger i;
        i.value = 23;
        i.propertyIndex = 81;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreVarDouble i;
        i.value = 66.3;
        i.propertyIndex = 82;
        data->addInstruction(i);
    }

    {
        QQmlCompiledData::Instruction::StoreVarBool i;
        i.value = true;
        i.propertyIndex = 83;
        data->addInstruction(i);
    }

    QStringList expect;
    expect 
        << "Index\tOperation\t\tData1\tData2\tData3\tComments"
        << "-------------------------------------------------------------------------------"
        << "0\t\tINIT\t\t\t0\t3\t-1\t-1"
        << "1\t\tCREATECPP\t\t\t0"
        << "2\t\tSETID\t\t\t0\t\t\t\"testId\""
        << "3\t\tSET_DEFAULT"
        << "4\t\tCREATE_COMPONENT\t3"
        << "5\t\tSTORE_META\t\t"
        << "6\t\tSTORE_FLOAT\t\t3\t11.3"
        << "7\t\tSTORE_DOUBLE\t\t4\t14.8"
        << "8\t\tSTORE_INTEGER\t\t5\t9"
        << "9\t\tSTORE_BOOL\t\t6\ttrue"
        << "10\t\tSTORE_STRING\t\t7\t1\t\t\"Test String\""
        << "11\t\tSTORE_URL\t\t8\t0\t\tQUrl(\"http://www.qt-project.org\") "
        << "12\t\tSTORE_COLOR\t\t9\t\t\t\"ff00ff00\""
        << "13\t\tSTORE_DATE\t\t10\t9"
        << "14\t\tSTORE_TIME\t\t11"
        << "15\t\tSTORE_DATETIME\t\t12"
        << "16\t\tSTORE_POINT\t\t13\t3\t7"
        << "17\t\tSTORE_POINTF\t\t13\t3\t7"
        << "18\t\tSTORE_SIZE\t\t15\t8\t11"
        << "19\t\tSTORE_SIZEF\t\t15\t8\t11"
        << "20\t\tSTORE_RECT\t\t17\t7\t9\t11\t13"
        << "21\t\tSTORE_RECTF\t\t18\t11.3\t9.8\t3\t2.1"
        << "22\t\tSTORE_VECTOR3D\t\t19\t9\t3\t92"
        << "23\t\tSTORE_VARIANT\t\t20\t2\t\t\"color(1, 1, 1, 1)\""
        << "24\t\tSTORE_OBJECT\t\t21"
        << "25\t\tSTORE_VARIANT_OBJECT\t22"
        << "26\t\tSTORE_INTERFACE\t\t23"
        << "27\t\tSTORE_SIGNAL\t\t2\t3"
        << "28\t\tSTORE_SCRIPT_STRING\t24\t3\t1\t4"
        << "29\t\tASSIGN_SIGNAL_OBJECT\t4"
        << "30\t\tASSIGN_CUSTOMTYPE\t25\t6\t9"
        << "31\t\tSTORE_BINDING\t26\t3\t2"
        << "32\t\tSTORE_COMPILED_BINDING\t27\t2\t4"
        << "33\t\tSTORE_VALUE_SOURCE\t29\t4"
        << "34\t\tSTORE_VALUE_INTERCEPTOR\t30\t-4"
        << "35\t\tBEGIN\t\t\t4"
        << "36\t\tSTORE_OBJECT_QLIST"
        << "37\t\tASSIGN_OBJECT_LIST"
        << "38\t\tFETCH_ATTACHED\t\t23"
        << "39\t\tFETCH_QLIST\t\t32"
        << "40\t\tFETCH\t\t\t33"
        << "41\t\tFETCH_VALUE\t\t34\t6\t7"
        << "42\t\tPOP"
        << "43\t\tPOP_QLIST"
        << "44\t\tPOP_VALUE\t\t35\t8"
        << "45\t\tDEFER\t\t\t7"
        << "46\t\tDEFER\t\t\t7"
        << "47\t\tSTORE_IMPORTED_SCRIPT\t2"
        << "48\t\tSTORE_VARIANT_INTEGER\t\t32\t11"
        << "49\t\tSTORE_VARIANT_DOUBLE\t\t19\t33.7"
        << "50\t\tDONE"
        << "51\t\tSTORE_TR_STRING\t99\t3\t14\t14\t2"
        << "52\t\tSTORE_TRID_STRING\t78\t7\t-1"
        << "53\t\tSTORE_VAR\t\t79\t5\t\t\"color(1, 1, 1, 1)\""
        << "54\t\tSTORE_VAR_OBJECT\t80"
        << "55\t\tSTORE_VAR_INTEGER\t81\t23"
        << "56\t\tSTORE_VAR_DOUBLE\t82\t66.3"
        << "57\t\tSTORE_VAR_BOOL\t\t83\ttrue"
        << "-------------------------------------------------------------------------------";

    QQmlTestMessageHandler messageHandler;

    data->dumpInstructions();

    const int messageCount = messageHandler.messages().count();
    QCOMPARE(messageCount, expect.count());
    for (int ii = 0; ii < messageCount; ++ii) {
        QCOMPARE(messageHandler.messages().at(ii), expect.at(ii));
    }

    data->release();
}

void tst_qqmlinstruction::point()
{
    QCOMPARE(sizeof(QQmlInstruction::instr_storePoint::QPoint), sizeof(QPoint));
    QCOMPARE(Q_ALIGNOF(QQmlInstruction::instr_storePoint::QPoint), Q_ALIGNOF(QPoint));

    QQmlInstruction i;
    i.storePoint.point.xp = 8;
    i.storePoint.point.yp = 11;

    const QPoint &point = (const QPoint &)(i.storePoint.point);
    QCOMPARE(point.x(), 8);
    QCOMPARE(point.y(), 11);
}

void tst_qqmlinstruction::pointf()
{
    QCOMPARE(sizeof(QQmlInstruction::instr_storePointF::QPointF), sizeof(QPointF));
    QCOMPARE(Q_ALIGNOF(QQmlInstruction::instr_storePointF::QPointF), Q_ALIGNOF(QPointF));

    QQmlInstruction i;
    i.storePointF.point.xp = 8.7;
    i.storePointF.point.yp = 11.3;

    const QPointF &point = (const QPointF &)(i.storePointF.point);
    QCOMPARE(point.x(), 8.7);
    QCOMPARE(point.y(), 11.3);
}

void tst_qqmlinstruction::size()
{
    QCOMPARE(sizeof(QQmlInstruction::instr_storeSize::QSize), sizeof(QSize));
    QCOMPARE(Q_ALIGNOF(QQmlInstruction::instr_storeSize::QSize), Q_ALIGNOF(QSize));

    QQmlInstruction i;
    i.storeSize.size.wd = 8;
    i.storeSize.size.ht = 11;

    const QSize &size = (const QSize &)(i.storeSize.size);
    QCOMPARE(size.width(), 8);
    QCOMPARE(size.height(), 11);
}

void tst_qqmlinstruction::sizef()
{
    QCOMPARE(sizeof(QQmlInstruction::instr_storeSizeF::QSizeF), sizeof(QSizeF));
    QCOMPARE(Q_ALIGNOF(QQmlInstruction::instr_storeSizeF::QSizeF), Q_ALIGNOF(QSizeF));

    QQmlInstruction i;
    i.storeSizeF.size.wd = 8;
    i.storeSizeF.size.ht = 11;

    const QSizeF &size = (const QSizeF &)(i.storeSizeF.size);
    QCOMPARE(size.width(), (qreal)8);
    QCOMPARE(size.height(), (qreal)11);
}

void tst_qqmlinstruction::rect()
{
    QCOMPARE(sizeof(QQmlInstruction::instr_storeRect::QRect), sizeof(QRect));
    QCOMPARE(Q_ALIGNOF(QQmlInstruction::instr_storeRect::QRect), Q_ALIGNOF(QRect));

    QQmlInstruction i;
    i.storeRect.rect.x1 = 8;
    i.storeRect.rect.y1 = 11;
    i.storeRect.rect.x2 = 13;
    i.storeRect.rect.y2 = 19;

    const QRect &rect = (const QRect &)(i.storeRect.rect);
    QCOMPARE(rect.left(), 8);
    QCOMPARE(rect.top(), 11);
    QCOMPARE(rect.right(), 13);
    QCOMPARE(rect.bottom(), 19);
}

void tst_qqmlinstruction::rectf()
{
    QCOMPARE(sizeof(QQmlInstruction::instr_storeRectF::QRectF), sizeof(QRectF));
    QCOMPARE(Q_ALIGNOF(QQmlInstruction::instr_storeRectF::QRectF), Q_ALIGNOF(QRectF));

    QQmlInstruction i;
    i.storeRectF.rect.xp = 8;
    i.storeRectF.rect.yp = 11;
    i.storeRectF.rect.w = 13;
    i.storeRectF.rect.h = 19;

    const QRectF &rect = (const QRectF &)(i.storeRectF.rect);
    QCOMPARE(rect.left(), (qreal)8);
    QCOMPARE(rect.top(), (qreal)11);
    QCOMPARE(rect.width(), (qreal)13);
    QCOMPARE(rect.height(), (qreal)19);
}

void tst_qqmlinstruction::vector3d()
{
    QCOMPARE(sizeof(QQmlInstruction::instr_storeVector3D::QVector3D), sizeof(QVector3D));
    QCOMPARE(Q_ALIGNOF(QQmlInstruction::instr_storeVector3D::QVector3D), Q_ALIGNOF(QVector3D));

    QQmlInstruction i;
    i.storeVector3D.vector.xp = 8.2f;
    i.storeVector3D.vector.yp = 99.3f;
    i.storeVector3D.vector.zp = 12.0;

    const QVector3D &vector = (const QVector3D &)(i.storeVector3D.vector);
    QCOMPARE(vector.x(), (qreal)(float)8.2);
    QCOMPARE(vector.y(), (qreal)(float)99.3);
    QCOMPARE(vector.z(), (qreal)(float)12.0);
}

void tst_qqmlinstruction::vector4d()
{
    QCOMPARE(sizeof(QQmlInstruction::instr_storeVector4D::QVector4D), sizeof(QVector4D));
    QCOMPARE(Q_ALIGNOF(QQmlInstruction::instr_storeVector4D::QVector4D), Q_ALIGNOF(QVector4D));

    QQmlInstruction i;
    i.storeVector4D.vector.xp = 8.2f;
    i.storeVector4D.vector.yp = 99.3f;
    i.storeVector4D.vector.zp = 12.0;
    i.storeVector4D.vector.wp = 121.1f;

    const QVector4D &vector = (const QVector4D &)(i.storeVector4D.vector);
    QCOMPARE(vector.x(), (qreal)(float)8.2);
    QCOMPARE(vector.y(), (qreal)(float)99.3);
    QCOMPARE(vector.z(), (qreal)(float)12.0);
    QCOMPARE(vector.w(), (qreal)(float)121.1);
}

void tst_qqmlinstruction::time()
{
    QCOMPARE(sizeof(QQmlInstruction::instr_storeTime::QTime), sizeof(QTime));
    QCOMPARE(Q_ALIGNOF(QQmlInstruction::instr_storeTime::QTime), Q_ALIGNOF(QTime));
}

QTEST_MAIN(tst_qqmlinstruction)

#include "tst_qqmlinstruction.moc"
