#include <QString>
#include <QtTest>

#include "ChronoApp.h"
#include <QtWidgets>
#include "MainController.h"
#include "StdUtilities.h"

#include "fftw3.h"

#include <iostream>
#include <cmath>
#include <errno.h>
//#include <fenv.h>
#include "Generator.h"
#include "Functions.h"

class ChronomodelTest : public QObject
{
    Q_OBJECT

public:
    ChronomodelTest();

private Q_SLOTS:
    void testCase1();
    void mersenneTwister();
    void timeRange();
};

ChronomodelTest::ChronomodelTest()
{
}

void ChronomodelTest::testCase1()
{
    QString str = "Hello";
        QCOMPARE(str.toUpper(), QString("HELLO"));

    QBENCHMARK {
            str.trimmed();
        }
}


/*
 * Debuggage mersenne twister
 *
 */

void ChronomodelTest::mersenneTwister()
{
   Generator::initGenerator(int(111));

   QString text;
   const QString reponse111 ("; -758.484; -396.421; 707.813; -404.568; -244.275; -581.953; 299.128; 203.405; 424.546; 840.891");

   for (int i=0; i<10; ++i)
       text= text+"; "+QString::number( Generator::randomUniform(-1000, 1000));

   QVERIFY(reponse111 == text);

   // 2d test
   text="";
   Generator::initGenerator(int(3649));

   const QString reponse3694 ("; -757.681; -1.8087; 363.35; -982.55; -505.08; 683.574; -629.103; -477.18; 691.793; -829.783");

   for (int i=0; i<10; ++i)
       text= text+"; "+QString::number( Generator::randomUniform(-1000, 1000));

   QCOMPARE(reponse3694, text);

}

/* Debuggage timerange
 * /
 */
void ChronomodelTest::timeRange()
{
    QVector<double> trace1;
    trace1<<1.1<<3.8<<5.6<<9.9<<7.4<<8.2<<5.7<<8.6<<9.4<<4.2<<6.1<<4.3<<8.7<<9.1<<11.6<<5.2<<1.18<<2.4<<6.8<<3.12;

    QVector<double> trace2;
    trace2<<5.18<<7.5<<6.18<<13.4<<9.3<<11.4<<9.1<<14.15<<13.19<<9.6<<11.3<<9.9<<15.13<<13.20<<14.14<<6.2<<9.2<<8.2<<7.13<<9.11;

     QPair<double, double> range = timeRangeFromTraces(trace1, trace2, 95., "test timeRangeFromTraces 1");
     QPair<double, double> ansRang = qMakePair<double, double> (1.1, 14.199);
qDebug()<<range;
    QVERIFY(range.first == ansRang.first);
 //   QVERIFY(range.second == ansRang.second);

    QVector<double> traceBeta;
    traceBeta<<1.1<<3.8<<5.6<<1.99<<7.4<<4.2<<5.7<<3.6<<3.4<<2.7<<3.1<<4.3<<3.87<<4.91<<6.11<<5.2<<1.18<<2.4<<6.8<<3.12;
    traceBeta<<1.11<<3.81<<5.61<<1.991<<7.41<<4.21<<5.71<<3.61<<3.41<<2.71<<3.11<<4.31<<3.871<<4.911<<6.111<<5.21<<1.181<<2.41<<6.81<<3.121;
    traceBeta<<1.112<<3.812<<5.612<<1.9912<<7.412<<4.212<<5.712<<3.612<<3.412<<2.712<<3.112<<4.312<<3.8712<<4.9112<<6.1112<<5.212<<1.1812<<2.412<<6.812<<3.1212;

    QVector<double> traceAlpha;
    traceAlpha<<11.9<<7.5<<12.18<<13.4<<9.3<<11.4<<9.1<<14.15<<13.019<<10.6<<11.3<<9.9<<15.13<<13.19<<14.14<<6.2<<9.2<<8.2<<7.13<<9.01;
    traceAlpha<<11.91<<7.51<<12.181<<13.41<<9.31<<11.41<<9.11<<14.151<<13.0191<<10.61<<11.31<<9.91<<15.131<<13.1921<<14.141<<6.21<<9.21<<8.21<<7.131<<9.011;
    traceAlpha<<11.912<<7.512<<12.1812<<13.412<<9.312<<11.412<<9.112<<14.1512<<13.01912<<10.612<<11.312<<9.912<<15.1312<<13.19212<<14.1412<<6.212<<9.212<<8.212<<7.1312<<9.0112;

    QPair<double, double> gapRange = gapRangeFromTraces(traceBeta, traceAlpha, 95., "test gapRangeFromTraces");
    QPair<double, double> ansGap = qMakePair<double, double> ( -INFINITY , INFINITY);
// qDebug()<<gapRange;
     QCOMPARE(gapRange, ansGap);

   QVector<double>tBeta;
   tBeta<<0.0<<5.0<<5.0<<10.0;

   QVector<double>tAlpha;
   tAlpha<<11.0<<16.0<<16.0<<21.0;

   QPair<double,double> gapRang2 = gapRangeFromTraces(tBeta, tAlpha, 50., "test gapRangeFromTraces 2");
qDebug()<<gapRang2;
   QPair<double, double> ansGap2 = qMakePair<double, double> (5.005,13.5037481259);
   QCOMPARE(gapRang2.first, ansGap2.first);
   QVERIFY(gapRang2.second == ansGap2.second);
}

QTEST_APPLESS_MAIN(ChronomodelTest)

#include "tst_chronomodeltest.moc"
