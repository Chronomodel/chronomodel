#include "QtUtilities.h"
#include "StdUtilities.h"
#include "StateKeys.h"
#include "MainWindow.h"
#include <QtWidgets>
#include <QtSvg>

#include "GraphView.h"
#include "AppSettings.h"
#include <algorithm>


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
    std::sort(list.begin(),list.end(),intLessThan);
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
    commentsMarkers << "//" << "#" << "/*";
    QString strClean = str.trimmed();
    
    for (int i=0; i<commentsMarkers.size(); ++i) {
        if (strClean.startsWith(commentsMarkers[i]))
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
        for (auto str : list)
            result.append(str.toInt());

    }
    return result;
}

QStringList intListToStringList(const QList<int>& intList)
{
    QStringList list;
    for (int i=0; i<intList.size(); ++i)
        list.append(QString::number(intList[i]));
    return list;
}

QString intListToString(const QList<int>& intList, const QString& separator)
{
    QStringList list = intListToStringList(intList);
    return list.join(separator);
}



QFileInfo saveWidgetAsImage(QObject* wid, const QRect& r, const QString& dialogTitle, const QString& defaultPath, const AppSettings & appSetting)
{
    QFileInfo fileInfo;
    
    QGraphicsScene* scene = nullptr;//dynamic_cast<QGraphicsScene*>(wid);
    QWidget* widget = dynamic_cast<QWidget*>(wid);
    GraphView* mGraph = dynamic_cast<GraphView*>(wid);
    
    if (!mGraph && !widget) {
        scene = dynamic_cast<QGraphicsScene*>(wid);
        if (!scene)
            return fileInfo;
    }
    
    const QString filter = QObject::tr("Image (*.png);;Photo (*.jpg);; Windows Bitmap (*.bmp);;Scalable Vector Graphics (*.svg)");
    const QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    dialogTitle,
                                                    defaultPath,
                                                    filter);
    if (!fileName.isEmpty()) {
        fileInfo = QFileInfo(fileName);
        const QString fileExtension = fileInfo.suffix();
        const qreal heightText = r.height()/50.;

        if (fileExtension == "svg") {
            if (mGraph)
                mGraph->saveAsSVG(fileName, "Title", "Description",true);

            else if (scene) {
                const  QRect viewBox = QRect( r.x(), r.y(), r.width(), r.height() );

                QSvgGenerator svgGen;
                svgGen.setFileName(fileName);

                svgGen.setViewBox(viewBox);
                svgGen.setDescription(QObject::tr("SVG scene drawing "));

                QPainter p;
                p.begin(&svgGen);
                scene->render(&p, r, r);
                p.end();
            }
            else if (widget)  // export all resultView
                saveWidgetAsSVG(widget, r, fileName);

        }
        else { // save PNG

            // -------------------------------
            //  Get preferences
            // -------------------------------
            const short pr = appSetting.mPixelRatio;
            const short dpm = appSetting.mDpm;
            const short quality = appSetting.mImageQuality;
            
            // -------------------------------
            //  Create the image
            // -------------------------------
            QImage image(r.width() * pr, (r.height() + heightText) * pr , QImage::Format_ARGB32_Premultiplied);
            if (image.isNull()) {
                qDebug() << "Cannot export null image!";
                return fileInfo;
            }
            
            // -------------------------------
            //  Set image properties
            // -------------------------------
            image.setDotsPerMeterX(dpm * 11811.024 / 300.);
            image.setDotsPerMeterY(dpm * 11811.024 / 300.);
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
            if (widget) // exportFullImage in ReseultsView
                 widget->render(&p, QPoint(0, 0), QRegion(r.x(), r.y(), r.width(), r.height()+heightText +20));

            
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
            QFont ft =  QFont() ;
            ft.setPixelSize(heightText);
            
            p.setFont(ft);
            p.setPen(Qt::black);
            
            p.drawText(0, r.height() - 5 , r.width(), heightText,

                       Qt::AlignCenter,
                       qApp->applicationName() + " " + qApp->applicationVersion());

            p.end();
            
            // -------------------------------
            //  Save file
            // -------------------------------
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

bool saveWidgetAsSVG(QWidget* widget, const QRect& r, const QString& fileName)
{
    const QFontMetrics fm(widget->font());
    
    const int heightText= fm.height()+10;
    

    const  QRect viewBox = QRect( 0, 0,r.width(), r.height() + heightText );
    QSvgGenerator svgGenFile;
    svgGenFile.setFileName(fileName);
    svgGenFile.setViewBox(viewBox);
    //svgGenFile.setSize(QSize(widget->width(),widget->height() + heightText));
    svgGenFile.setDescription(QObject::tr("SVG widget drawing "));
    
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
    QString result = "<div style=\"margin:10px\"><p style=\"font-weight: bold; font-style: normal\">" + title + "</p><p>" + text + "</p></div>";
    return result;
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

QString textPurple(const QString& str)
{
    return "<span style=\"color: #C95805;\">" + str + "</span>";
}

QString textColor(const QString &str,const QColor &color)
{
    int red, green, blue;
    color.getRgb(&red, &green, &blue);
    const QString text ="<span style=""color:rgb("+ QString::number(red)
                                        + "," + QString::number(green)
                                        + "," + QString ::number(blue) + ");>" + str + "</span>";

   return text;
}
QString textBackgroundColor(const QString &str, const QColor &color)
{
    int red, green, blue;
    color.getRgb(&red, &green, &blue);
    const QString text ="<center><p style=""background-color:rgb("+ QString::number(red)
                                        + "," + QString::number(green)
                                        + "," + QString ::number(blue) + "); >" + str + "</p></center>";

   return text;
}


QColor randomColor()
{
    return QColor(rand() % 255,
                  rand() % 255,
                  rand() % 255);
}

bool constraintIsCircular(QJsonArray constraints, const int fromId, const int toId)
{    
    for (int i=0; i<constraints.size(); ++i) {
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

QString stringWithAppSettings(const double valueToFormat, const bool forCSV)
{
    if (std::abs(valueToFormat)<1E-6)
        return "0";

    int precision = MainWindow::getInstance()->getAppSettings().mPrecision;

    char fmt = 'f';
    if (std::abs(valueToFormat)>250000)
        fmt = 'G';

    if (forCSV) {
        QLocale locale = MainWindow::getInstance()->getAppSettings().mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
        locale.setNumberOptions(QLocale::OmitGroupSeparator);
        return locale.toString(valueToFormat, fmt, precision);

    } else {
        QLocale locale = QLocale();
        if (precision >0)
             return removeZeroAtRight(locale.toString(valueToFormat, fmt, precision));
        else
            return locale.toString(valueToFormat, fmt, precision);
   }
}

//#pragma mark CSV File
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
    const AppSettings settings = MainWindow::getInstance()->getAppSettings();
    const QString csvSep = settings.mCSVCellSeparator;
    
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
