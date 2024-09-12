/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#include "ModelCurve.h"
#include "StdUtilities.h"

#include <QStringList>
#include <QColor>
#include <QFileInfo>
#include <QList>
#include <QtCore/qdebug.h>

bool colorIsDark(const QColor &color);
void sortIntList(QList<int> &list);

QString DHMS(quint64 elapsedTime);

QList<QStringList> readCSV(const QString& filePath, const QString& separator = ",");
int defaultDpiX();
qreal dpiScaled(qreal value);
QColor getContrastedColor(const QColor& color);
std::vector<int> QStringToStdVectorInt(const QString& listStr, const QString& separator = ",");
QList<int> QStringToQListInt(const QString &listStr, const QString &separator = ",");

std::vector<unsigned int> QStringToStdVectorUnsigned(const QString& listStr, const QString& separator = ",");
QList<unsigned int> QStringToQListUnsigned(const QString& listStr, const QString& separator = ",");

QStringList QListIntToQStringList(const QList<int>& intList);

QStringList StdVectorIntToQStringList(const std::vector<int> &intList);
QStringList QListUnsignedToQStringList(const QList<unsigned>& unsignedList);
QString QListIntToQString(const QList<int>& intList, const QString& separator = ",");

QString StdVectorIntToQString(const std::vector<int>& intList, const QString& separator = ",");
QString QListUnsignedToQString(const QList<unsigned>& intList, const QString& separator = ",");




QString long_double_to_str(const long double value);

#ifdef DEBUG
template <typename U>
void show_QList(const QList<U> &list, QString description)
{
    qDebug() << description;

    for (auto v: list) {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(std::numeric_limits<U>::max_digits10 + 1) << v ;

       qDebug()<< QString::fromStdString(stream.str()) ;
    }


}

template <typename U, typename V>
void show_QMap(const QMap<U, V> &map, QString description)
{
    qDebug() << description;
    for (auto [key, value]: map.asKeyValueRange()) {
        qDebug()<< key << value;
    }

}
#endif

QFileInfo saveWidgetAsImage(QObject* widget, const QRect& r, const QString& dialogTitle, const QString& defaultPath);
bool saveWidgetAsSVG(QWidget* widget, const QRect& r, const QString& fileName);

bool isComment(const QString& str);
QString prepareTooltipText(const QString &title, const QString& text);

QString line(const QString& str);
QString textBold(const QString& str);
QString textBlack(const QString& str);
QString textRed(const QString& str);
QString textGreen(const QString& str);
QString textBlue(const QString& str);
QString textOrange(const QString& str);
QString textPurple(const QString& str);
QString textColor(const QString &str, const QColor &color);
QString textBackgroundColor(const QString &str, const QColor &color);

QColor randomColor();

bool constraintIsCircular(QJsonArray constraints, const int FromId, const int ToId);


QString removeZeroAtRight(const QString &str); // use StdUtilities::eraseZeroAtLeft()
//QString stringWithAppSettings(const double valueToFormat, const bool forcePrecision = false);
QString stringForGraph(const double valueToFormat);
QString stringForLocal(const double valueToFormat, const bool forcePrecision = false);
QString stringForCSV(const double valueToFormat, const bool forcePrecision = false);

bool saveCsvTo(const QList<QStringList> &data, const QString &filePath, const QString &csvSep, const bool withDateFormat = false);
bool saveAsCsv(const QList<QStringList> &data, const QString &title = QObject::tr("Save as..."));
bool save_map_as_csv(const std::map<double, double>& map, const std::pair<QString, QString> &header, const QString title = QObject::tr("Save as..."), const QString prefix = "");

/**
 * @brief getMapDataInRange
 * @param subMin
 * @param subMax
 * @return return a QMap with only data inside the range [subMin; subMax]. We must evaluate missing data for the extremum if necessarry
 */
template <typename T, typename V>
QMap<T, V> getMapDataInRange(const QMap<T, V> data, const T subMin, const  T subMax)
{
    if (data.size() == 0)
        return data;

    if (data.size() == 1) {
        if (data.firstKey()>=subMin && data.firstKey()<= subMax) {
            return data;
        }
        else
            return QMap<T, V> ();
    }


    T tBeforeSubMin (0);
    V vBeforeSubMin (0);
    bool pointBeforeSubMin =false;
    T tAfterSubMax (0);
    V vAfterSubMax (0);
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

        } else if (data.size() == 2 && data.firstKey() <= subMin && data.lastKey() >= subMax) {
            subData.insert(subMin, data.first());
            subData.insert(subMax, data.last());

        } else if (data.firstKey()<=subMin && data.lastKey()>=subMax) {
            subData[subMin] =  interpolateValueInQMap(subMin, data);
            subData[subMax] =  interpolateValueInQMap(subMax, data);

        }
        return subData;
    }
    else {
        return data;
    }
}

std::map<double, double> getMapDataInRange(const std::map<double, double> &data, const double subMin, const double subMax);

template <typename T>
QList<T> getVectorDataInRange(const QList<T> &data, const T subMin,const T subMax, const T min, const T max)
{
    Q_ASSERT(!data.isEmpty());

    if (subMin != min || subMax != max)  {
        QList<T> subData;
        subData.reserve(data.size());
        qsizetype idxStart = (qsizetype) floor(data.size() * (subMin - min) / (max - min));
        qsizetype idxEnd = (qsizetype) floor(data.size() * (subMax - min) / (max - min));

        idxStart = std::max((qsizetype)0, idxStart);
        idxEnd = std::min(idxEnd, data.size()-1);
        // we can use mid()
        for (qsizetype i=idxStart; i<=idxEnd; ++i)
                subData.append(data[i]);

        subData.squeeze();
        return subData;
    }
    else
        return data;

}

/*
template <typename T>
std::vector<T> getVectorDataInRange(const std::vector<T> &data, const T subMin,const T subMax, const T min, const T max)
{
    Q_ASSERT(!data.empty());

    if (subMin != min || subMax != max)  {
        std::vector<T> subData;
        subData.reserve(data.size());
        size_t idxStart = (size_t) floor(data.size() * (subMin - min) / (max - min));
        size_t idxEnd = (size_t) floor(data.size() * (subMax - min) / (max - min));

        idxStart = std::max((size_t)0, idxStart);
        idxEnd = std::min(idxEnd, data.size()-1);
        // we can use mid()
        for (size_t i = idxStart; i <= idxEnd; ++i)
            subData.push_back(data[i]);

        subData.shrink_to_fit();
        return subData;
    }
    else
        return data;

}*/

template <typename T>
std::vector<T> getVectorDataInRange(const std::vector<T>& data, const T subMin, const T subMax, const T min, const T max)
{
    if (data.empty()) {
        throw std::invalid_argument("Input vector is empty.");
    }

    if (subMin == min && subMax == max) {
        return data; // Return the entire vector if the ranges are equal
    }

    // Calculate indices
    size_t idxStart = static_cast<size_t>(std::floor(data.size() * (subMin - min) / (max - min)));
    size_t idxEnd = static_cast<size_t>(std::floor(data.size() * (subMax - min) / (max - min)));

    // Ensure indices are within bounds
    idxStart = std::max(static_cast<size_t>(0), idxStart);
    idxEnd = std::min(idxEnd, data.size() - 1);

    // Create a vector to hold the subrange
    std::vector<T> subData;
    subData.reserve(idxEnd - idxStart + 1); // Reserve space for the expected number of elements

    // Use std::copy to copy the range
    std::copy(data.begin() + idxStart, data.begin() + idxEnd + 1, std::back_inserter(subData));
    return subData;
}

QList<double>* load_QList_ptr(QDataStream& stream);
QList<double> load_QList(QDataStream& stream);


std::vector<double> load_std_vector(QDataStream& stream);
std::vector<bool> load_std_vector_bool(QDataStream& stream);

std::shared_ptr<std::vector<double> > load_std_vector_ptr(QDataStream& stream);
void reload_shared_ptr(const std::shared_ptr<std::vector<double> > data, QDataStream& stream);


/*template <template<typename...> class Container, class T >
void save_container(QDataStream& stream, const Container<T>& data)
{
   // qDebug()<<"[QtUtilities::save_container] "<< data.size();
    quint32 size = (quint32) data.size();
    stream << size;
    if (size > 0) {
        for (const auto& v : data)
            stream << v;
    }

}*/
template <template<typename...> class Container, class T>
void save_container(QDataStream& stream, const Container<T>& data)
{
    quint32 size = static_cast<quint32>(data.size());
    stream << size;

    if (stream.status() != QDataStream::Ok) {
        // Gérer l'erreur de flux ici
        return;
    }

    if (size > 0) {
        for (const auto& v : data) {
            stream << v;
            if (stream.status() != QDataStream::Ok) {
                // Gérer l'erreur de flux ici
                return;
            }
        }
    }
}

/*template <template<typename...> class Container, class T >
void load_container(QDataStream& stream, Container<T>& data)
{
    quint32 siz;
    T v;

    stream >> siz;
    Container<T> tmp (siz);
    std::generate_n(tmp.begin(), siz, [&stream, &v]{stream >> v; return v;});

    std::swap(data, tmp);
}*/

template <template<typename...> class Container, class T>
void load_container(QDataStream& stream, Container<T>& data)
{
    quint32 siz;
    stream >> siz;

    if (stream.status() != QDataStream::Ok) {
        // Gérer l'erreur de lecture ici
        return;
    }

    data.resize(siz);

    // Utilisation de std::generate pour remplir le conteneur
    std::generate(data.begin(), data.end(), [&stream]() {
        T v;
        stream >> v;
        if (stream.status() != QDataStream::Ok) {
            // Gérer l'erreur de lecture ici
            throw std::runtime_error("Error reading from stream");
        }
        return v;
    });
}

std::shared_ptr<Project> getProject_ptr();
std::shared_ptr<ModelCurve> getModel_ptr();
QJsonObject* getState_ptr();

#endif
