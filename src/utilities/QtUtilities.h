#ifndef QTUTILITIES_H
#define QTUTILITIES_H

#include <QStringList>
#include <QColor>
#include <QFileInfo>
#include "AppSettings.h"
#include "AxisTool.h"

#include <QtCore/qdebug.h>

bool colorIsDark(const QColor& color);
void sortIntList(QList<int>& list);

QList<QStringList> readCSV(const QString& filePath, const QString& separator = ",");
int defaultDpiX();
qreal dpiScaled(qreal value);
QColor getContrastedColor(const QColor& color);
QList<int> stringListToIntList(const QString& listStr, const QString& separator = ",");
QStringList intListToStringList(const QList<int>& intList);
QString intListToString(const QList<int>& intList, const QString& separator = ",");

QFileInfo saveWidgetAsImage(QObject* widget, const QRect& r, const QString& dialogTitle, const QString& defaultPath,  const AppSettings & setting);
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

QColor randomColor();

bool constraintIsCircular( QJsonArray constraints, const int FromId, const int ToId);

QString formatValueToAppSettingsPrecision(const float valueToFormat);

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
    if(data.size() == 0)
        qDebug()<<"QtUtilities::getMapDataInRange data.size() == 0";

#endif
    if(data.size() == 0)
        return data;

    T tBeforeSubMin;
    V vBeforeSubMin;
    bool pointBeforeSubMin =false;
    T tAfterSubMax;
    V vAfterSubMax;
    bool pointAfterSubMax =false;
    const T min = data.firstKey();
    const T max = data.lastKey();
    if(subMin != min || subMax != max) {
        QMap<T, V> subData;
        subData.clear();
        QMapIterator<T, V> iter(data);
        while(iter.hasNext())
        {
            iter.next();
            T valueX = iter.key();
            if(valueX >= subMin && valueX <= subMax) {
                subData.insert(valueX, iter.value());
            }
            else if(valueX<subMin){
               pointBeforeSubMin =true;
               tBeforeSubMin = valueX;
               vBeforeSubMin = iter.value();
            }
            else if( valueX>subMax && !pointAfterSubMax ){
                pointAfterSubMax =true;
                tAfterSubMax = valueX;
                vAfterSubMax = iter.value();
            }
        }
         // Correct the QMap, with addition of value on the extremum tmin and tmax
        if(subData.size() > 0) {
            if(pointBeforeSubMin && subData.constFind(subMin) == subData.cend()) {
                subData[subMin] = interpolate( subMin, tBeforeSubMin, subData.firstKey(), vBeforeSubMin, subData.first() );
            }
            if(pointAfterSubMax && subData.constFind(subMax) == subData.cend()) {
                subData[subMax] = interpolate( subMax, subData.lastKey(), tAfterSubMax, subData.last(), vAfterSubMax );
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
    if(subMin != min || subMax != max)  {
        QVector<T> subData;
        subData.reserve(data.size());
        int idxStart = (int) floor(data.size() * (subMin - min) / (max - min));
        int idxEnd = (int) floor(data.size() * (subMax - min) / (max - min));
        for(int i=idxStart; i<=idxEnd; ++i)
        {
            if(i >= 0 && i < data.size())
                subData.append(data[i]);
        }
        return subData;
    }
    else {
        return data;
    }
}

#endif
