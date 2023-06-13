/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#include <QtWidgets>
#include <QFontMetricsF>
#include <QtSvg>
#include <algorithm>

/**
 * @brief DHMS
 * @param elapsedTime in msec
 * @return
 */
QString DHMS( quint64 elapsedTime)
{
    if (elapsedTime == 0)
        return "";

    QString str;
    quint64 day = 0;
    quint64 hour = 0;
    quint64 minute = 0;
    quint64 second =0;

    if (elapsedTime >= (24*3600*1000))
        day = elapsedTime / (24*3600*1000);


    if (day > 6)
        return QString("%1 days ").arg(QString::number(day));

    else if (day > 1)
        str = QString("%1 days ").arg(QString::number(day));
    else if (day == 1)
        str = QString("1 day ");

    elapsedTime -= day * 24*3600*1000;
    hour = elapsedTime / (3600*1000);

    if (hour > 1)
        str += QString("%1 hours ").arg(QString::number(hour));
    else if (hour == 1)
        str += QString("1 hour ");

    // If we have more than one day, we do not display below the hour
    if (day >= 1)
        return str;

    elapsedTime -= hour*3600*1000;
    minute = elapsedTime / (60*1000);

    if (minute > 1)
        str += QString("%1 minutes ").arg(QString::number(minute));
    else if (minute == 1)
        str += QString("1 minute ");

    // If we have more than one hour, we do not display below the second
    if (hour >= 1)
        return str;

    elapsedTime -= minute*60*1000;
    second = elapsedTime / 1000;

    if (second > 1)
        str += QString("%1 seconds ").arg(QString::number(second));
    else if (second == 1)
        str += QString("1 second ");

    // If we have more than one minute, we do not display below the second
    if (minute >= 1)
        return str;

    if (second >= 1)
        return str;

    elapsedTime -= second*1000;

    str += QString("%3 msec").arg(QString::number(elapsedTime));

    return str;
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

    if (const QScreen* screen = QGuiApplication::primaryScreen())
        return qRound(screen->logicalDotsPerInchX());

    //PI has not been initialised, or it is being initialised. Give a default dpi
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
    QColor frontColor = Qt::white;
    qreal s = color.saturationF();
    if (s < 0.4)  {
        qreal l = color.lightnessF();
        if (l >= 0.5)
            frontColor = Qt::black;
    }
    return frontColor;
}

QString removeZeroAtRight(QString str)
{
    return QString::fromStdString(removeZeroAtRight(str.toStdString()));
}

QList<int> stringListToIntList(const QString& listStr, const QString& separator)
{
    QList<int> result;
    if (!listStr.isEmpty()) {
        QStringList list = listStr.split(separator);
        for (auto& str : list)
            result.append(str.toInt());

    }
    return result;
}
QList<unsigned> stringListToUnsignedList(const QString& listStr, const QString& separator)
{
    QList<unsigned> result;
    if (!listStr.isEmpty()) {
        QStringList list = listStr.split(separator);
        for (auto& str : list)
            result.append(str.toInt());

    }
    return result;
}

QStringList intListToStringList(const QList<int>& intList)
{
    QStringList list;
    for (auto& i : intList)
        list.append(QString::number(i));
    return list;
}

QStringList unsignedListToStringList(const QList<unsigned>& unsignedList)
{
    QStringList list;
    for (auto& un : unsignedList)
        list.append(QString::number(un));
    return list;
}

QString intListToString(const QList<int>& intList, const QString& separator)
{
    QStringList list = intListToStringList(intList);
    return list.join(separator);
}

QString unsignedListToString(const QList<unsigned> &intList, const QString &separator)
{
    QStringList list = unsignedListToStringList(intList);
    return list.join(separator);
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

    const QRect viewBox = QRect( 0, 0,r.width(), r.height() + heightText );
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

bool constraintIsCircular(QJsonArray constraints, const int fromId, const int toId)
{
    for (int i = 0; i<constraints.size(); ++i) {
        QJsonObject constraint = constraints.at(i).toObject();

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

    if (forcePrecision)
        return locale.toString(valueToFormat, fmt, 9);

    if (std::abs(valueToFormat)>1E+06)
        fmt = 'G';

    const int precision = AppSettings::mPrecision;

    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    return locale.toString(valueToFormat, fmt, precision);

}


// CSV File
bool saveCsvTo(const QList<QStringList>& data, const QString& filePath, const QString& csvSep, const bool withDateFormat)
{
    QFile file(filePath);
    if (file.open(QFile::WriteOnly | QFile::Truncate))  {
        QTextStream output(&file);
        const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
        output<<"# " +version+"\r";
        const QString projectName = MainWindow::getInstance()->getNameProject();

        output<<"# " +projectName+ "\r";
        if (withDateFormat)
            output<<"# Date Format : "+ DateUtils::getAppSettingsFormatStr() +"\r";

        else
            output<<"# Date Format : BC/AD\r";

        for (int i=0; i<data.size(); ++i)  {
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
        for (int i=0; i<data.size(); ++i)  {
            output << data.at(i).join(csvSep);
            output << "\r";
        }
        file.close();
        return true;
    }
    return false;
}
