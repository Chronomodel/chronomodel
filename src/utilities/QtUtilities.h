/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#ifndef QTUTILITIES_H
#define QTUTILITIES_H

#include "AppSettings.h"
#include "AxisTool.h"

#include <QStringList>
#include <QColor>
#include <QFileInfo>
#include <QtCore/qdebug.h>

extern double ro;

bool colorIsDark(const QColor& color);
void sortIntList(QList<int>& list);

QList<QStringList> readCSV(const QString& filePath, const QString& separator = ",");
int defaultDpiX();
qreal dpiScaled(qreal value);
QColor getContrastedColor(const QColor& color);
QList<int> stringListToIntList(const QString& listStr, const QString& separator = ",");
QStringList intListToStringList(const QList<int>& intList);
QString intListToString(const QList<int>& intList, const QString& separator = ",");

QFileInfo saveWidgetAsImage(QObject* widget, const QRect& r, const QString& dialogTitle, const QString& defaultPath);
bool saveWidgetAsSVG(QWidget* widget, const QRect& r, const QString& fileName);

bool isComment(const QString& str);
QString prepareTooltipText(const QString& title, const QString& text);

QString line(const QString& str);
QString textBold(const QString& str);
QString textBlack(const QString& str);
QString textRed(const QString& str);
QString textGreen(const QString& str);
QString textBlue(const QString& str);
QString textPurple(const QString& str);
QString textColor(const QString &str, const QColor &color);
QString textBackgroundColor(const QString &str, const QColor &color);

QColor randomColor();

bool constraintIsCircular( QJsonArray constraints, const int FromId, const int ToId);


QString removeZeroAtRight(QString str); // use StdUtilities::eraseZeroAtLeft()
//QString stringWithAppSettings(const double valueToFormat, const bool forcePrecision = false);
QString stringForGraph(const double valueToFormat);
QString stringForLocal(const double valueToFormat, const bool forcePrecision = true);
QString stringForCSV(const double valueToFormat, const bool forcePrecision = true);

bool saveCsvTo(const QList<QStringList>& data, const QString& filePath, const QString& csvSep, const bool withDateFormat = false);
bool saveAsCsv(const QList<QStringList>& data, const QString& title = QObject::tr("Save as..."));

/**
 * @brief getMapDataInRange
 * @param subMin
 * @param subMax
 * @return return a QMap with only data inside the range [subMin; subMax]. We must evaluate missing data for the extremum if necessarry
 */
template <typename T, typename V>
QMap<T, V> getMapDataInRange(const QMap<T, V> &data, const T subMin, const  T subMax)
{
#ifdef DEBUG
  //  if (data.size() == 0)
  //      qDebug()<<"QtUtilities::getMapDataInRange data.size() == 0";

#endif
    if (data.size() == 0)
        return data;

    T tBeforeSubMin;
    V vBeforeSubMin;
    bool pointBeforeSubMin =false;
    T tAfterSubMax;
    V vAfterSubMax;
    bool pointAfterSubMax =false;
    const T min = data.firstKey();
    const T max = data.lastKey();
    if (subMin != min || subMax != max) {
        QMap<T, V> subData;
        subData.clear();
        QMapIterator<T, V> iter(data);
        while (iter.hasNext()) {
            iter.next();
            T valueT = iter.key();
            if (valueT >= subMin && valueT <= subMax)
                subData.insert(valueT, iter.value());

            else if (valueT<subMin) {
               pointBeforeSubMin = true;
               tBeforeSubMin = valueT;
               vBeforeSubMin = iter.value();
            }
            else if ( valueT>subMax && !pointAfterSubMax ){
                pointAfterSubMax = true;
                tAfterSubMax = valueT;
                vAfterSubMax = iter.value();
            }
        }
         // Correct the QMap, with addition of value on the extremum tmin and tmax
        if (subData.size() > 0) {
            if (pointBeforeSubMin && subData.constFind(subMin) == subData.cend()) {
                V subDataFirst = subData.first();
                subData[subMin] = interpolate( subMin, tBeforeSubMin, (T)subData.firstKey(), vBeforeSubMin, subDataFirst );
            }
            if (pointAfterSubMax && subData.constFind(subMax) == subData.cend()) {
                V subDataLast = subData.last();
                subData[subMax] = interpolate( subMax, (T)subData.lastKey(), tAfterSubMax, subDataLast, vAfterSubMax );
            }
        }
        return subData;
    }
    else {
        return data;
    }
}

template <typename T>
QVector<T> getVectorDataInRange(const QVector<T>& data, const T subMin,const T subMax, const T min, const T max)
{
    Q_ASSERT(!data.isEmpty());

    if (subMin != min || subMax != max)  {
        QVector<T> subData;
        subData.reserve(data.size());
        int idxStart = (int) floor(data.size() * (subMin - min) / (max - min));
        int idxEnd = (int) floor(data.size() * (subMax - min) / (max - min));

        idxStart = qMax(0, idxStart);
        idxEnd = qMin(idxEnd, data.size()-1);
        // we can use mid()
        for (int i=idxStart; i<=idxEnd; ++i)
                subData.append(data[i]);

        subData.squeeze();
        return subData;
    }
    else
        return data;

}

#endif
