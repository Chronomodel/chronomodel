/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 202(

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

#include "QtUtilities.h"
#include "StdUtilities.h"
#include "StateKeys.h"
#include "MainWindow.h"
#include "GraphView.h"
#include "AppSettings.h"

#include <QFontMetricsF>
#include <QDataStream>

#include <QSvgGenerator>
#include <algorithm>
#include <iterator>
#include <QApplication>
#include <QScreen>
#include <QGraphicsScene>
#include <QFileDialog>

/**
 * @brief DHMS
 * @param elapsedTime in msec
 * @return
 */
QString DHMS(quint64 elapsedTime)
{
    // Cas spécial pour 0 ms
    if (elapsedTime == 0) {
        return "0 msec";
    }

    // Constantes pour améliorer la lisibilité
    const quint64 MS_PER_SECOND = 1000;
    const quint64 MS_PER_MINUTE = 60 * MS_PER_SECOND;
    const quint64 MS_PER_HOUR = 60 * MS_PER_MINUTE;
    const quint64 MS_PER_DAY = 24 * MS_PER_HOUR;

    // Extraction des différentes unités de temps
    quint64 days = elapsedTime / MS_PER_DAY;
    quint64 remainder = elapsedTime % MS_PER_DAY;

    // Si plus d'une semaine, on affiche seulement les jours
    if (days > 6) {
        return QString("%1 days").arg(days);
    }

    // Construction de la chaîne de résultat
    QString result;

    // Ajout des jours si présents
    if (days > 1) {
        result = QString("%1 days ").arg(days);
    } else if (days == 1) {
        result = "1 day ";
    }

    // Si on a au moins un jour, on n'affiche pas en dessous de l'heure
    quint64 hours = remainder / MS_PER_HOUR;
    if (days >= 1) {
        if (hours > 1) {
            result += QString("%1 hours").arg(hours);
        } else if (hours == 1) {
            result += "1 hour";
        }
        return result.trimmed();
    }

    // Ajout des heures si présentes
    if (hours > 1) {
        result += QString("%1 hours ").arg(hours);
    } else if (hours == 1) {
        result += "1 hour ";
    }

    remainder %= MS_PER_HOUR;

    // Si on a au moins une heure, on n'affiche pas en dessous de la minute
    quint64 minutes = remainder / MS_PER_MINUTE;
    if (hours >= 1) {
        if (minutes > 1) {
            result += QString("%1 minutes").arg(minutes);
        } else if (minutes == 1) {
            result += "1 minute";
        }
        return result.trimmed();
    }

    // Ajout des minutes si présentes
    if (minutes > 1) {
        result += QString("%1 minutes ").arg(minutes);
    } else if (minutes == 1) {
        result += "1 minute ";
    }

    remainder %= MS_PER_MINUTE;

    // Si on a au moins une minute, on n'affiche pas en dessous de la seconde
    quint64 seconds = remainder / MS_PER_SECOND;
    if (minutes >= 1) {
        if (seconds > 1) {
            result += QString("%1 seconds").arg(seconds);
        } else if (seconds == 1) {
            result += "1 second";
        }
        return result.trimmed();
    }

    // Ajout des secondes si présentes
    if (seconds > 1) {
        result += QString("%1 seconds ").arg(seconds);
    } else if (seconds == 1) {
        result += "1 second ";
    }

    // Si on a au moins une seconde, on ne montre pas les millisecondes
    if (seconds >= 1) {
        return result.trimmed();
    }

    // Affichage des millisecondes si nécessaire
    quint64 milliseconds = remainder % MS_PER_SECOND;
    result += QString("%1 msec").arg(milliseconds);

    return result.trimmed();
}

bool colorIsDark(const QColor& color)
{
    int sum = (color.red() + color.green() + color.blue()) / 3;
    return (sum < 128);
}

bool intLessThan(const int& v1, const int& v2)
{
    return v1 < v2;
}

void sortIntList(QList<int>& list)
{
   // qSort(list.begin(), list.end(), intLessThan);// http://doc.qt.io/qt-5/qtalgorithms-obsolete.html#qSort
    std::sort(list.begin(), list.end(), intLessThan);
}


QList<QStringList> readCSV(const QString& filePath, const QString& separator)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QList<QStringList> data;

        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if ( (line.left(1) != "#") && (line.left(1) != "/") ) {
                QStringList values = line.split(separator);
                data.append(values);
            }
        }
        file.close();
        return data;
    }
    return QList<QStringList>();
}

int defaultDpiX()
{
    if (qApp->testAttribute(Qt::AA_Use96Dpi))
        return 96;

    //if(!qt_is_gui_used)
      //  return 75;

    if (const QScreen* screen = QApplication::primaryScreen())
        return qRound(screen->logicalDotsPerInchX());

    //PI has not been initialized, or it is being initialized. Give a default dpi
    return 100;
}

qreal dpiScaled(qreal value)
{
#ifdef Q_OS_MAC
    // On mac the DPI is always 72 so we should not scale it
    return value;
#else
    static const qreal scale = qreal(defaultDpiX()) / 96.0;
    return value * scale;
#endif
}


bool isComment(const QString& str)
{
    QStringList commentsMarkers;
    commentsMarkers << "//" << "#" << "/*"<<"!"<<QString(char(148));
    QString strClean = str.trimmed();

    for (auto&& str : commentsMarkers) {
        if (strClean.startsWith(str))
            return true;
    }
    return false;
}

QColor getContrastedColor(const QColor& color)
{
     //complementary color
   /* int r = color.red();
    int g = color.green();
    int b = color.blue();
// (255-r, 255-g, 255-b);
 */
     QColor frontColor;
// https://en.wikipedia.org/wiki/HSL_and_HSV#/media/File:Hsl-hsv_models.svg
     /*
     // mode hsl, correspond à la roue et lightness corresopnd à la barre
    const float hslHue = color.hslHueF()*360.;
    const float hslSaturation = color.hslSaturationF();
    const float lightness = color.lightnessF();
//qDebug()<<hslHue<<hslSaturation<<lightness;
    */

//mode TSL
    const float hsvHue = color.hsvHueF()*360.;
    const float hsvSaturation = color.hsvSaturationF();
    //float saturation = color.saturationF(); //==hsvSaturation
    const float value = color.valueF(); // Correspond à la luminosité en TSL
    //qDebug()<<hsvHue<<hsvSaturation<<saturation<< value;

    if (value < 0.55) {
        frontColor = Qt::white;
    }
    else if (hsvSaturation < 0.25) {
        frontColor = Qt::black;
    }
    else if (190<hsvHue && hsvHue<360) { // du bleu au rouge final
        frontColor = Qt::white;
    }
    else if ( hsvHue<7) { // Le debut rouge
        frontColor = Qt::white;

    } else {
        frontColor = Qt::black;
    }

    return frontColor;
}

QString removeZeroAtRight(const QString &str)
{
    return QString::fromStdString(removeZeroAtRight(str.toStdString()));
}

std::vector<int> QStringToStdVectorInt(const QString &listStr, const QString &separator)
{
    std::vector<int> result;
    if (!listStr.isEmpty()) {
        QStringList list = listStr.split(separator);
        //for (const auto& str : list)
          //  result.push_back(str.toInt());
        result.reserve(list.size()); // Réserve de l'espace pour éviter des réallocations

        std::transform(list.begin(), list.end(), std::back_inserter(result),
                       [](const QString& str) { return str.toInt(); }); // Utilisation de lambda pour conversion
    }
    return result;
}


QList<int> QStringToQListInt(const QString &listStr, const QString &separator)
{
    QList<int> result;
    if (!listStr.isEmpty()) {
        QStringList list = listStr.split(separator);
        //for (const auto& str : list)
          //  result.push_back(str.toInt());
        result.reserve(list.size()); // Réserve de l'espace pour éviter des réallocations

        std::transform(list.begin(), list.end(), std::back_inserter(result),
                       [](const QString& str) { return str.toInt(); }); // Utilisation de lambda pour conversion
    }
    return result;
}

std::vector<unsigned> QStringToStdVectorUnsigned(const QString &listStr, const QString &separator)
{
    std::vector<unsigned> result;
    if (!listStr.isEmpty()) {
        QStringList list = listStr.split(separator);
        for (const auto& str : list)
            result.push_back(str.toInt());

    }
    return result;
}

QList<unsigned> QStringToQListUnsigned(const QString &listStr, const QString &separator)
{
    QList<unsigned> result;
    if (!listStr.isEmpty()) {
        QStringList list = listStr.split(separator);
        for (const auto& str : list)
            result.push_back(str.toInt());

    }
    return result;
}


QStringList QListIntToQStringList(const QList<int> &intList)
{
    QStringList list;
    for (const auto& i : intList)
        list.append(QString::number(i));
    return list;
}

QStringList StdVectorIntToQStringList(const std::vector<int> &intList)
{
    QStringList list;
    for (const auto& i : intList)
        list.append(QString::number(i));
    return list;
}

QStringList QListUnsignedToQStringList(const QList<unsigned>& unsignedList)
{
    QStringList list;
    for (const auto& un : unsignedList)
        list.append(QString::number(un));
    return list;
}

QString QListIntToQString(const QList<int>& intList, const QString& separator)
{
    QStringList list = QListIntToQStringList(intList);
    return list.join(separator);
}

QString StdVectorIntToQString(const std::vector<int>& intList, const QString& separator)
{
    QStringList list = StdVectorIntToQStringList(intList);
    return list.join(separator);
}

QString QListUnsignedToQString(const QList<unsigned> &intList, const QString &separator)
{
    QStringList list = QListUnsignedToQStringList(intList);
    return list.join(separator);
}


// Fonction pour convertir std::map en QMap
QMap<double, double> stdMap_to_QMap(const std::map<double, double>& stdMap)
{
    QMap<double, double> qMap;
    for (const auto& pair : stdMap) {
        qMap.insert(pair.first, pair.second);
    }
    return qMap; // Retourne le QMap construit
}


QString double_to_str(const double value)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(std::numeric_limits<long double>::max_digits10 + 1) << value;

    return QString::fromStdString(stream.str());
}

QString long_double_to_str(const long double value)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(std::numeric_limits<long double>::max_digits10 + 1) << value;

    return QString::fromStdString(stream.str());
}

QFileInfo saveWidgetAsImage(QObject* wid, const QRect &r, const QString &dialogTitle, const QString &defaultPath)
{
    QFileInfo fileInfo;

    QGraphicsScene* scene = nullptr;
    QWidget* widget = dynamic_cast<QWidget*>(wid);
    GraphView* mGraph = dynamic_cast<GraphView*>(wid);

    if (!mGraph && !widget) {
        scene = dynamic_cast<QGraphicsScene*>(wid);
        if (!scene)
            return fileInfo;
    }

    const QString filter = QObject::tr("Image (*.png);;Photo (*.jpg);; Windows Bitmap (*.bmp);;Scalable Vector Graphics (*.svg)");
    const QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(), dialogTitle, defaultPath, filter);

    if (!fileName.isEmpty()) {
        fileInfo = QFileInfo(fileName);
        const QString fileExtension = fileInfo.suffix();
        const qreal heightText  = 2 * QFontMetricsF(qApp->font()).height();
        const int bottomSpace = 5;
        const QString versionStr = qApp->applicationName() + " " + qApp->applicationVersion();

        if (fileExtension == "svg") {
            if (mGraph)
                mGraph->saveAsSVG(fileName, versionStr, "GraphView", true);

            else if (scene) {
                const  QRect viewBox = QRect( r.x(), r.y(), r.width(), r.height() );

                QSvgGenerator svgGen;
                svgGen.setFileName(fileName);

                svgGen.setViewBox(viewBox.adjusted(0, 0, 0, 10* heightText));
                svgGen.setTitle(versionStr);
                svgGen.setDescription(QObject::tr("Scene drawing"));

                QPainter p;
                p.begin(&svgGen);
                scene->render(&p, r, r);
                p.setFont(qApp->font());
                p.setPen(Qt::black);
                const int wStr= p.fontMetrics().horizontalAdvance(versionStr);
                const int hStr = p.fontMetrics().height();

                p.drawText(0, int( r.height()/2 ), wStr, hStr,
                           Qt::AlignCenter,
                           versionStr);
                p.end();
            }
            else if (widget)  // export all resultView
                saveWidgetAsSVG(widget, r, fileName);

        }
        else { // save PNG

            /* -------------------------------
             *  Get preferences
             * -------------------------------*/
            //For the scene exportation pixel Ration is forced to 1
            const short pr = short (AppSettings::mPixelRatio);

            //const short dpm = appSetting.mDpm;
            const short quality = short (AppSettings::mImageQuality);

            // -------------------------------
            //  Create the image
            // -------------------------------
            QImage image(r.width() * pr, int( (r.height() + heightText + bottomSpace) * pr) , QImage::Format_ARGB32_Premultiplied);
            if (image.isNull()) {
                qDebug() << "Cannot export null image!";
                return fileInfo;
            }

            // -------------------------------
            //  Set image properties
            // -------------------------------
            //QGuiApplication::primaryScreen()->devicePixelRatio()
           // QGuiApplication::primaryScreen()->logicalDotsPerInch(); // physicalDotsPerInchX()
           //  image.setDotsPerMeterX(dpm * 11811.024 / 300.);
           //  image.setDotsPerMeterY(dpm * 11811.024 / 300.);
            image.setDevicePixelRatio(pr);

            // -------------------------------
            //  Fill background
            // -------------------------------
            if (fileExtension == "jpg")
                image.fill(Qt::white);

            else
                image.fill(Qt::transparent);


            // -------------------------------
            //  Create painter
            // -------------------------------
            QPainter p;
            p.begin(&image);
            p.setRenderHint(QPainter::Antialiasing);

            // -------------------------------
            //  If widget, draw with or without axis
            // -------------------------------
            if (widget) // exportFullImage in ResultsView
                 widget->render(&p, QPoint(0, 0), QRegion(r.x(), r.y(), r.width(), int(r.height() + heightText + 20) ));


            // -------------------------------
            //  If scene...
            // -------------------------------
            else if (scene) {
                QRectF srcRect = r;
                srcRect.setX(r.x());
                srcRect.setY(r.y());
                srcRect.setWidth(r.width() * pr);
                srcRect.setHeight(r.height() * pr);

                QRectF tgtRect = image.rect();
                tgtRect.adjust(0, 0, 0, -heightText * pr);

                scene->render(&p, tgtRect, srcRect);

            } else
                return fileInfo;


            // -------------------------------
            //  Write application and version
            // -------------------------------

            p.setFont(qApp->font());
            p.setPen(Qt::black);

            p.drawText(0, int( r.height() + bottomSpace), r.width(), heightText,

                       Qt::AlignCenter,
                       versionStr);

            p.end();

            /* -------------------------------
             *  Save file
             * ------------------------------- */
            image.save(fileName, fileExtension.toUtf8(), quality);

            //image.save(fileName, formatExt);
            /*QImageWriter writer;
             writer.setFormat("jpg");
             writer.setQuality(100);
             writer.setFileName(fileName+"_jpg");
             writer.write(image);*/
        }
    }

    return fileInfo;
}

bool saveWidgetAsSVG(QWidget* widget, const QRect &r, const QString &fileName)
{
    const QFontMetrics fm(widget->font());

    const int heightText= fm.descent() + fm.ascent() + 10;

    const QRect viewBox = QRect(0, 0, r.width(), r.height() + heightText);
    QSvgGenerator svgGenFile;
    svgGenFile.setFileName(fileName);
    svgGenFile.setViewBox(viewBox);

    svgGenFile.setDescription(QObject::tr("SVG widget drawing"));

    QPainter p;
    p.begin(&svgGenFile);
    p.setFont(widget->font());
    widget->render(&p);

    p.setPen(Qt::black);

    p.drawText(0, r.height() + 10, r.width(), heightText,
               Qt::AlignCenter,
               qApp->applicationName() + " " + qApp->applicationVersion());
    p.end();

    return true;
}

QString prepareTooltipText(const QString& title, const QString& text)
{
    return "<div style=\"margin:10px\"><p style=\"font-weight: bold; font-style: normal\">" + title + "</p><p>" + text + "</p></div>";
}

QString line(const QString& str)
{
    return str + "<br>";
}

QString textBlack(const QString& str)
{
    return "<span style=\"color: #000000;\">" + str + "</span>";
}

QString textBold(const QString& str)
{
    return "<b>" + str + "</b>";
}

QString textRed(const QString& str)
{
    return "<span style=\"color: red;\">" + str + "</span>";
}

QString textGreen(const QString& str)
{
    return "<span style=\"color: green;\">" + str + "</span>";
}

QString textBlue(const QString& str)
{
    return "<span style=\"color: blue;\">" + str + "</span>";
}

QString textOrange(const QString& str)
{
    return "<span style=\"color: #C95805;\">" + str + "</span>";
}

QString textPurple(const QString& str)
{
    return "<span style=\"color: #7C1190;\">" + str + "</span>";
}

QString textColor(const QString &str, const QColor &color)
{
    int red, green, blue;
    color.getRgb(&red, &green, &blue);
    return "<span style=""color:rgb("+ QString::number(red)
                                        + "," + QString::number(green)
                                        + "," + QString ::number(blue) + ");>" + str + "</span>";

}
QString textBackgroundColor(const QString &str, const QColor &color)
{
    int red, green, blue;
    color.getRgb(&red, &green, &blue);
    return "<center><p style=""background-color:rgb("+ QString::number(red)
                                        + "," + QString::number(green)
                                        + "," + QString ::number(blue) + "); >" + str + "</p></center>";
}


QColor randomColor()
{
#ifdef Q_OS_MAC
    return QColor(arc4random() % 255,
                  arc4random() % 255,
                  arc4random() % 255);
#else
    return QColor(rand() % 255,
                  rand() % 255,
                  rand() % 255);
#endif
}


std::map<double,double> getMapDataInRange(const std::map<double,double>& data, double subMin, double subMax) {
    std::map<double,double> subData;
    if (data.empty() || subMin > subMax) return subData;

    auto itLow = data.lower_bound(subMin);   // premier >= subMin
    auto itPrev = (itLow == data.begin()) ? data.end() : std::prev(itLow);

    // point d'interpolation avant subMin si nécessaire
    if (itLow == data.end()) {
        // tous les éléments < subMin
        return subData;
    }
    if (itLow->first > subMin && itPrev != data.end()) {
        subData[subMin] = interpolate(subMin, itPrev->first, itLow->first, itPrev->second, itLow->second);
    } else if (itLow->first == subMin) {
        subData[itLow->first] = itLow->second;
    }

    // copier les éléments entre itLow (ou next si itLow==exact) et itHigh
    for (auto it = itLow; it != data.end() && it->first <= subMax; ++it) {
        subData[it->first] = it->second;
    }

    auto itAfter = data.upper_bound(subMax); // premier > subMax
    if (itAfter != data.begin() && itAfter != data.end()) {
        auto itBefore = std::prev(itAfter);
        if (itAfter->first > subMax) {
            subData[subMax] = interpolate(subMax, itBefore->first, itAfter->first, itBefore->second, itAfter->second);
        }
    } else if (itAfter == data.begin()) {
        // tout > subMax -> rien (déjà géré plus haut)
    } else if (itAfter == data.end()) {
        // subMax >= dernier point : si dernier clé < subMax, on peut décider d'ignorer ou d'extrapoler
    }

    // cas spécial si data.size()==1
    if (subData.empty() && data.size() == 1) {
        auto p = *data.begin();
        if (p.first >= subMin && p.first <= subMax) subData.emplace(p);
    }

    return subData;
}

bool constraintIsCircular(QJsonArray constraints, const int fromId, const int toId)
{
    for (int i = 0; i<constraints.size(); ++i) {
        const QJsonObject &constraint = constraints.at(i).toObject();

        // Detect circularity
        if (constraint.value(STATE_CONSTRAINT_BWD_ID).toInt() == toId && constraint.value(STATE_CONSTRAINT_FWD_ID).toInt() == fromId)
            return true;

        // If the constraint follows the one we are trying to create,
        // follow it to check the circularity !
        else if (constraint.value(STATE_CONSTRAINT_BWD_ID).toInt() == toId){
                int nextToId =  constraint.value(STATE_CONSTRAINT_FWD_ID).toInt();
                if (constraintIsCircular(constraints, fromId, nextToId))
                    return true;

        }
    }
    return false;
}

//QString stringWithAppSettings(const double valueToFormat, const bool forcePrecision)

QString stringForGraph(const double valueToFormat)
{
    char fmt = 'f';
    QLocale locale = QLocale();
    const int precision = AppSettings::mPrecision;

    if (std::abs(valueToFormat) > 1E+06)
        fmt = 'G';

    if (std::abs(valueToFormat) > 1E-6)
        if (precision > 0) {
            return removeZeroAtRight(locale.toString( valueToFormat, fmt, precision));// + 1)); // if appSettings precision is 0, we need a decimal
        } else {
            return locale.toString( valueToFormat, fmt, precision);
        }
    else
        return "0";
}


QString stringForLocal(const double valueToFormat, const bool forcePrecision)
{
    char fmt = 'f';
    QLocale locale = QLocale();

    if (forcePrecision)
        return locale.toString(valueToFormat, fmt, 19);

    const int precision = AppSettings::mPrecision;

    if (std::abs(valueToFormat)>1E+06)
        fmt = 'G';

      return locale.toString(valueToFormat, fmt, precision);

}

QString stringForCSV(const double valueToFormat, const bool forcePrecision)
{
    char fmt = 'f';
    QLocale locale = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    if (forcePrecision)
        return locale.toString(valueToFormat, fmt, 9);

    if (std::abs(valueToFormat)>1E+06)
        fmt = 'G';

    const int precision = AppSettings::mPrecision;

    return locale.toString(valueToFormat, fmt, precision);

}


// CSV File
bool saveCsvTo(const QList<QStringList>& data, const QString& filePath, const QString& csvSep, const bool withDateFormat)
{
    QFile file(filePath);
    if (file.open(QFile::WriteOnly | QFile::Truncate))  {
        QTextStream output(&file);
        const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
        output << "# " + version + "\r";
        const QString projectName = MainWindow::getInstance()->getNameProject();

        output << "# " + projectName + "\r";
        if (withDateFormat)
            output<<"# Date Format : "+ DateUtils::getAppSettingsFormatStr() +"\r";

        else
            output << "# Date Format : BC/AD\r";

        for (int i = 0; i < data.size(); ++i)  {
            output << data.at(i).join(csvSep);
            output << "\r";
        }
        file.close();
        return true;
    }
    return false;
}

bool saveAsCsv(const QList<QStringList>& data, const QString& title)
{
    const QString csvSep = AppSettings::mCSVCellSeparator;

    const QString currentPath = MainWindow::getInstance()->getCurrentPath();
    const QString filter = "CSV (*.csv)";
    QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    title,
                                                    currentPath,
                                                    filter);
    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream output(&file);
        for (int i = 0; i<data.size(); ++i)  {
            output << data.at(i).join(csvSep);
            output << "\r";
        }
        file.close();
        return true;
    }
    return false;
}

bool save_map_as_csv(const std::map<double, double>& map, const std::pair<QString, QString>& header, const QString title, const QString prefix)
{
    const QString csvSep = AppSettings::mCSVCellSeparator;
    QLocale csvLocal = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
    csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);

    const QString currentPath = MainWindow::getInstance()->getCurrentPath();
    QString filename;

    if (!prefix.isEmpty()) {
        const QString fiName = MainWindow::getInstance()->getNameProject();
        const QString defaultFilename = currentPath + "/" + fiName.mid(0, fiName.size()-4) + "_" + prefix;

        filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                              title,
                                                              defaultFilename, "CSV (*.csv)");

    } else {
        filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                              title,
                                                              currentPath,
                                                              "CSV (*.csv)");
    }


    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream output(&file);
        output << header.first + csvSep + header.second<< "\r";
        for (auto m : map)  {
            output << csvLocal.toString(m.first) + csvSep + csvLocal.toString(m.second);
            output << "\r";
        }
        file.close();
        return true;
    }
    return false;
}


#pragma mark save_container

void save_container(QDataStream& stream, const std::vector<long long>& data)
{
    quint32 size = static_cast<quint32>(data.size());

    // Écrire la taille
    stream << size;
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::save_container] erreur écriture taille" << stream.device()->errorString();
        throw std::runtime_error("Error writing to stream (size)");
    }

    // Écrire les éléments
    for (quint32 i = 0; i < size; ++i) {
        qint64 v = static_cast<qint64>(data[i]);  // Conversion pour QDataStream
        stream << v;

        if (stream.status() != QDataStream::Ok) {
            qDebug() << "[QtUtilities::save_container] erreur écriture élément" << i << stream.device()->errorString();
            throw std::runtime_error("Error writing to stream (element)");
        }
    }
}

void save_container(QDataStream& stream, const std::vector<bool>& data)
{
    quint32 size = static_cast<quint32>(data.size());

    // Écrire la taille
    stream << size;
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::save_container] erreur écriture taille" << stream.device()->errorString();
        throw std::runtime_error("Error writing to stream (size)");
    }

    // Écrire les éléments
    for (quint32 i = 0; i < size; ++i) {
        bool v = data[i];  // std::vector<bool> retourne une référence proxy
        stream << v;

        if (stream.status() != QDataStream::Ok) {
            qDebug() << "[QtUtilities::save_container] erreur écriture élément" << i << stream.device()->errorString();
            throw std::runtime_error("Error writing to stream (element)");
        }
    }
}

void save_container(QDataStream& stream, const std::vector<double>& data)
{
    quint32 size = static_cast<quint32>(data.size());

    // Écrire la taille
    stream << size;
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::save_container] erreur écriture taille" << stream.device()->errorString();
        throw std::runtime_error("Error writing to stream (size)");
    }

    // Écrire les éléments
    for (quint32 i = 0; i < size; ++i) {
        double v = data[i];
        stream << v;  // QDataStream gère nativement double

        if (stream.status() != QDataStream::Ok) {
            qDebug() << "[QtUtilities::save_container] erreur écriture élément" << i << stream.device()->errorString();
            throw std::runtime_error("Error writing to stream (element)");
        }
    }
}

void save_container(QDataStream& stream, const std::shared_ptr<std::vector<double>>& data)
{
    // Vérifier que le shared_ptr n'est pas null
    if (!data) {
        qDebug() << "[QtUtilities::save_container] shared_ptr est null";
        throw std::runtime_error("Cannot save null shared_ptr");
    }

    quint32 size = static_cast<quint32>(data->size());

    // Écrire la taille
    stream << size;
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::save_container] erreur écriture taille" << stream.device()->errorString();
        throw std::runtime_error("Error writing to stream (size)");
    }

    // Écrire les éléments
    for (quint32 i = 0; i < size; ++i) {
        double v = (*data)[i];
        stream << v;

        if (stream.status() != QDataStream::Ok) {
            qDebug() << "[QtUtilities::save_container] erreur écriture élément" << i << stream.device()->errorString();
            throw std::runtime_error("Error writing to stream (element)");
        }
    }
}
// Version avec support des pointeurs null (optionnelle)
void save_container_nullable(QDataStream& stream, const std::shared_ptr<std::vector<double>>& data)
{
    bool isNull = (data == nullptr);
    stream << isNull;

    if (!isNull) {
        save_container(stream, data);  // Appel à la version normale
    }
}

#pragma mark load_container
// Spécialisation pour std::vector<long long>
void load_container(QDataStream& stream, std::vector<long long>& data)
{
    quint32 size;
    stream >> size;

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_container] erreur lecture taille " << stream.device()->errorString();
        throw std::runtime_error("Error reading from stream (size)");
    }

    data.clear();
    data.reserve(size);  // Optimisation pour std::vector

    for (quint32 i = 0; i < size; ++i) {
        qint64 v;  // QDataStream utilise qint64 pour long long
        stream >> v;

        if (stream.status() != QDataStream::Ok) {
            qDebug() << "[QtUtilities::load_container] erreur lecture élément" << i << stream.device()->errorString();
            throw std::runtime_error("Error reading from stream (element)");
        }

        data.push_back(static_cast<long long>(v));
    }
}

// Spécialisation pour std::vector<bool>
void load_container(QDataStream& stream, std::vector<bool> &data)
{
    quint32 size;
    stream >> size;

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_container] erreur lecture taille" << stream.device()->errorString();
        throw std::runtime_error("Error reading from stream (size)");
    }

    data.clear();
    data.reserve(size);  // Optimisation pour std::vector

    for (quint32 i = 0; i < size; ++i) {
        bool v;
        stream >> v;

        if (stream.status() != QDataStream::Ok) {
            qDebug() << "[QtUtilities::load_container] erreur lecture élément" << i << stream.device()->errorString();
            throw std::runtime_error("Error reading from stream (element)");
        }

        data.push_back(v);
    }
}

// Spécialisation pour std::vector<double>
void load_container(QDataStream& stream, std::vector<double>& data)
{
    quint32 size;
    stream >> size;

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_container] erreur lecture taille" << stream.device()->errorString();
        throw std::runtime_error("Error reading from stream (size)");
    }

    data.clear();
    data.reserve(size);  // Optimisation pour std::vector

    for (quint32 i = 0; i < size; ++i) {
        double v;  // QDataStream gère nativement double
        stream >> v;

        if (stream.status() != QDataStream::Ok) {
            qDebug() << "[QtUtilities::load_container] erreur lecture élément" << i << stream.device()->errorString();
            throw std::runtime_error("Error reading from stream (element)");
        }

        data.push_back(v);
    }
}

// Surcharge pour std::shared_ptr<std::vector<double>>
void load_container(QDataStream& stream, std::shared_ptr<std::vector<double>>& data)
{
    quint32 size;
    stream >> size;

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "[QtUtilities::load_container] erreur lecture taille" << stream.device()->errorString();
        throw std::runtime_error("Error reading from stream (size)");
    }

    // Créer ou réinitialiser le shared_ptr
    if (!data) {
        data = std::make_shared<std::vector<double>>();
    } else {
        data->clear();
    }

    data->reserve(size);  // Optimisation pour std::vector

    for (quint32 i = 0; i < size; ++i) {
        double v;
        stream >> v;

        if (stream.status() != QDataStream::Ok) {
            qDebug() << "[QtUtilities::load_container] erreur lecture élément" << i << stream.device()->errorString();
            throw std::runtime_error("Error reading from stream (element)");
        }

        data->push_back(v);
    }
}

// Version avec support des pointeurs null (optionnelle)
void load_container_nullable(QDataStream& stream, std::shared_ptr<std::vector<double>>& data)
{
    bool isNull;
    stream >> isNull;

    if (isNull) {
        data = nullptr;  // Définit le pointeur comme null si le flag est vrai
    }
    else {
        // Si ce n'est pas null, crée ou réinitialise le pointeur
        if (!data) {
            data = std::make_shared<std::vector<double>>();
        }
        else {
            data->clear();  // Nettoie le conteneur existant avant de charger
        }

        // Charge les données dans le conteneur
        load_container(stream, *data);
    }
}

std::shared_ptr<Project> getProject_ptr()
{
    return MainWindow::getInstance()->getProject();
}

std::shared_ptr<ModelCurve> getModel_ptr()
{
    auto project = getProject_ptr();
    return project ? project->mModel : nullptr;
}

QJsonObject* getState_ptr()
{
    auto project = getProject_ptr();
    return project ? &project->mState : nullptr;
}


/**
 * Structure pour représenter un point temporel avec la fonction suivante downsampleLTTB
 */
struct Point {
    double x; // temps ou index
    double y; // valeur

    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

/**
 * LTTB (Largest Triangle Three Buckets)
 * Algorithme optimal pour réduire un signal temporel
 * Préserve la forme visuelle du signal de manière intelligente
 * Signal original : ___/\___/\/\/\___
                        ↑    ↑↑↑↑↑

 * LTTB sélectionne :  ___/\___/\/\/\___
                          •  • •••••• •
                     (plus de points où ça varie)

 * Uniforme aurait :   ___/\___/\/\/\___
                       •   •   •   •   •
                     (peut manquer les pics)
 * @param data Vecteur original de m valeurs
 * @param n Nombre de points souhaités en sortie (doit être >= 3)
 * @return Vecteur réduit de taille exactement n
 * Version combinée : retourne les valeurs ET les indices
 * @return pair<valeurs, indices> des points sélectionnés
 * S. Steinarsson, "Downsampling time series for visual representation,"
M.S. thesis, Faculty of Physical Sciences, University of Iceland,
Reykjavik, Iceland, 2013.


## Citation format APA :

Steinarsson, S. (2013). Downsampling time series for visual representation
[Master's thesis, University of Iceland].
 */
std::pair<std::vector<double>, std::vector<size_t>> downsampleLTTBWithIndices(
    const std::vector<double>& data, size_t n)
{

    size_t m = data.size();

    if (m == 0) return {{}, {}};
    if (n >= m) {
        std::vector<size_t> indices(m);
        for (size_t i = 0; i < m; ++i) indices[i] = i;
        return {data, indices};
    }
    if (n < 3) throw std::invalid_argument("n doit être >= 3 pour LTTB");

    std::vector<double> values;
    std::vector<size_t> indices;
    values.reserve(n);
    indices.reserve(n);

    std::vector<Point> points(m);
    for (size_t i = 0; i < m; ++i) {
        points[i] = Point(static_cast<double>(i), data[i]);
    }

    // Premier point
    values.push_back(points[0].y);
    indices.push_back(0);

    double bucketSize = static_cast<double>(m - 2) / (n - 2);
    size_t pointIndex = 0;

    for (size_t bucket = 0; bucket < n - 2; ++bucket) {
        double bucketStart = bucket * bucketSize + 1;
        double bucketEnd = (bucket + 1) * bucketSize + 1;

        size_t rangeStart = static_cast<size_t>(bucketStart);
        size_t rangeEnd = static_cast<size_t>(bucketEnd);
        if (rangeEnd > m) rangeEnd = m;

        double avgX = 0, avgY = 0;
        size_t nextBucketStart = rangeEnd;
        size_t nextBucketEnd = static_cast<size_t>((bucket + 2) * bucketSize + 1);
        if (nextBucketEnd > m) nextBucketEnd = m;
        if (bucket == n - 3) nextBucketEnd = m;

        size_t avgCount = nextBucketEnd - nextBucketStart;
        for (size_t i = nextBucketStart; i < nextBucketEnd; ++i) {
            avgX += points[i].x;
            avgY += points[i].y;
        }
        if (avgCount > 0) {
            avgX /= avgCount;
            avgY /= avgCount;
        }

        double maxArea = -1;
        size_t maxAreaIndex = rangeStart;
        Point prev = points[pointIndex];

        for (size_t i = rangeStart; i < rangeEnd; ++i) {
            double area = std::abs(
                              (prev.x - avgX) * (points[i].y - prev.y) -
                              (prev.x - points[i].x) * (avgY - prev.y)
                              ) * 0.5;

            if (area > maxArea) {
                maxArea = area;
                maxAreaIndex = i;
            }
        }

        values.push_back(points[maxAreaIndex].y);
        indices.push_back(maxAreaIndex);
        pointIndex = maxAreaIndex;
    }

    // Dernier point
    values.push_back(points[m - 1].y);
    indices.push_back(m - 1);

    return {values, indices};
}
