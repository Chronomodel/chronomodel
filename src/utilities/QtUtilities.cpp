#include "QtUtilities.h"
#include "StdUtilities.h"
#include "StateKeys.h"
#include "MainWindow.h"
#include <QtWidgets>
#include <QtSvg>

#include "GraphView.h"
#include "AppSettings.h"


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
    qSort(list.begin(), list.end(), intLessThan);
}


QList<QStringList> readCSV(const QString& filePath, const QString& separator)
{
    QFile file(filePath);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QList<QStringList> data;
        
        QTextStream stream(&file);
        while(!stream.atEnd())
        {
            QString line = stream.readLine();
            if( (line.left(1) != "#") && (line.left(1) != "/") )
            {
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
    if(qApp->testAttribute(Qt::AA_Use96Dpi))
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
    
    for(int i=0; i<commentsMarkers.size(); ++i)
    {
        if(strClean.startsWith(commentsMarkers[i]))
            return true;
    }
    return false;
}

QColor getContrastedColor(const QColor& color)
{
    QColor frontColor = Qt::white;
    qreal s = color.saturationF();
    if(s < 0.4)
    {
        qreal l = color.lightnessF();
        if(l >= 0.5)
            frontColor = Qt::black;
    }
    return frontColor;
}

QList<int> stringListToIntList(const QString& listStr, const QString& separator)
{
    QList<int> result;
    if(!listStr.isEmpty())
    {
        QStringList list = listStr.split(separator);
        for(int i=0; i<list.size(); ++i)
            result.append(list[i].toInt());
    }
    return result;
}

QStringList intListToStringList(const QList<int>& intList)
{
    QStringList list;
    for(int i=0; i<intList.size(); ++i)
        list.append(QString::number(intList[i]));
    return list;
}

QString intListToString(const QList<int>& intList, const QString& separator)
{
    QStringList list = intListToStringList(intList);
    return list.join(separator);
}
# pragma mark Save Widget

QFileInfo saveWidgetAsImage(QObject* wid, const QRect& r, const QString& dialogTitle, const QString& defaultPath, const AppSettings & appSetting)
{
    QFileInfo fileInfo;
    
    QGraphicsScene* scene = 0;
    QWidget* widget = dynamic_cast<QWidget*>(wid);
    GraphView* mGraph = dynamic_cast<GraphView*>(wid);
    
    if(!mGraph && !widget)
    {
        scene = dynamic_cast<QGraphicsScene*>(wid);
        if(!scene)
            return fileInfo;
    }
    
    QString filter = QObject::tr("Image (*.png);;Photo (*.jpg);; Windows Bitmap (*.bmp);;Scalable Vector Graphics (*.svg)");
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    dialogTitle,
                                                    defaultPath,
                                                    filter);
    if(!fileName.isEmpty())
    {
        fileInfo = QFileInfo(fileName);
        QString fileExtension = fileInfo.suffix();
        
        //QString fileExtension = fileName.(".svg");
       // bool asSvg = fileName.endsWith(".svg");
       // if(asSvg)
        QFontMetrics fm((scene ? qApp->font() : widget->font()));
        
        int heightText = fm.height() + 30;
        
        if(fileExtension == "svg")
        {
            if(mGraph)
            {
                mGraph->saveAsSVG(fileName, "Title", "Description",true);
            }
            else if(scene)
            {
                QSvgGenerator svgGen;
                svgGen.setFileName(fileName);
                svgGen.setSize(r.size());
                svgGen.setViewBox(QRect(0, 0, r.width(), r.height()));
                svgGen.setDescription(QObject::tr("SVG scene drawing "));
                //qDebug()<<"export scene as SVG";
                
                QPainter p;
                p.begin(&svgGen);
                scene->render(&p, r, r);
                p.end();
            }
            else if(widget)
            {
                saveWidgetAsSVG(widget, r, fileName);
            }
        }
        else
        { // save PNG
            
            //int versionHeight = 20;
            //qreal pr = 1;//qApp->devicePixelRatio();
           /* qreal prh=  32000. / ( r.height() + versionHeight) ; // QImage axes are limited to 32767x32767 pixels
           
            qreal prw=  32000. / r.width() ;                  qreal pr = (prh<prw)? prh : prw;
            if (pr>4) {
                pr=4;
            }
            */
            
            // -------------------------------
            //  Get preferences
            // -------------------------------
            short pr = appSetting.mPixelRatio;
            short dpm = appSetting.mDpm;
            short quality = appSetting.mImageQuality;
            
            // -------------------------------
            //  Create the image
            // -------------------------------
            QImage image(r.width() * pr, (r.height() + heightText) * pr , QImage::Format_ARGB32_Premultiplied);
            if(image.isNull()){
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
            if (fileExtension == "jpg") {
                image.fill(Qt::white);
            }
            else {
                image.fill(Qt::transparent);
            }
            
            // -------------------------------
            //  Create painter
            // -------------------------------
            QPainter p;
            p.begin(&image);
            p.setRenderHint(QPainter::Antialiasing);
            
            // -------------------------------
            //  If widget, draw with or without axis
            // -------------------------------
            if(widget){
                //p.setFont(widget->font());
                widget->render(&p, QPoint(0, 0), QRegion(r.x(), r.y(), r.width(), r.height()));
            }
            
            // -------------------------------
            //  If scene...
            // -------------------------------
            else if(scene){
                QRectF srcRect = r;
                srcRect.setX(r.x());
                srcRect.setY(r.y());
                srcRect.setWidth(r.width() * pr);
                srcRect.setHeight(r.height() * pr);
                
                QRectF tgtRect = image.rect();
                tgtRect.adjust(0, 0, 0, -heightText * pr);
                
                scene->render(&p, tgtRect, srcRect);
            }
            
            // -------------------------------
            //  Write application and version
            // -------------------------------
            p.setPen(Qt::black);
            p.drawText(0, r.height(), r.width(), heightText,
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
   // qDebug()<<"QFileInfo saveWidgetAsImage image.save"<<fileName;
    
    return fileInfo;
}

bool saveWidgetAsSVG(QWidget* widget, const QRect& r, const QString& fileName)
{
    QFontMetrics fm(widget->font());
    
    int heightText= fm.height()+10;
    

    //int versionHeight = 20;
    //int heightAxe = 0;
    //if (Axe.mShowSubs) heightAxe = 20;
    
    
    QSvgGenerator svgGenFile;
    svgGenFile.setFileName(fileName);
    svgGenFile.setViewBox(r);
    svgGenFile.setDescription(QObject::tr("SVG widget drawing "));
    
    QPainter p;
    p.begin(&svgGenFile);
    p.setFont(widget->font());
    widget->render(&p);
  
    p.setPen(Qt::black);
   
    //p.drawText(0, r.height()+heightAxe+versionHeight, r.width(), versionHeight,
    //           Qt::AlignCenter,
    //           qApp->applicationName() + " " + qApp->applicationVersion());
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
    return "<span style=\"color: #000000;\">" + str + "</span>";
}
QString textBlue(const QString& str)
{
    return "<span style=\"color: blue;\">" + str + "</span>";
}
QString textPurple(const QString& str)
{
    return "<span style=\"color: #C95805;\">" + str + "</span>";
}

QColor randomColor()
{
    return QColor(rand() % 255,
                  rand() % 255,
                  rand() % 255);
}

bool constraintIsCircular(QJsonArray constraints, const int fromId, const int toId)
{    
    for(int i=0; i<constraints.size(); ++i)
    {
        QJsonObject constraint = constraints[i].toObject();
        
        // Detect circularity
        if(constraint[STATE_CONSTRAINT_BWD_ID].toInt() == toId && constraint[STATE_CONSTRAINT_FWD_ID].toInt() == fromId)
        {
            return true;
        }
        // If the constraint follows the one we are trying to create,
        // follow it to check the circularity !
        else if(constraint[STATE_CONSTRAINT_BWD_ID].toInt() == toId){
            int nextToId =  constraint[STATE_CONSTRAINT_FWD_ID].toInt();
            if(constraintIsCircular(constraints, fromId, nextToId))
            {
                return true;
            };
        }
    }
    return false;
}
/**
 * @todo add a precision in the preferences setting
 */
QString formatValueToAppSettingsPrecision(const double valueToFormat)
{
    //const AppSettings& s = MainWindow::getInstance()->getAppSettings();
    //int precision=3;
    char fmt = 'f';
    if (std::fabs(valueToFormat)>250000){
        fmt = 'G';
    }
    if (std::fabs(valueToFormat)<1E-6) {
        return "0";
    }
    else
        return QString::number(valueToFormat, fmt, 3);;
}

#pragma mark CSV File
bool saveCsvTo(const QList<QStringList>& data, const QString& filePath, const QString& csvSep)
{
    QFile file(filePath);
    if(file.open(QFile::WriteOnly | QFile::Truncate))
    {
        QTextStream output(&file);
        for(int i=0; i<data.size(); ++i)
        {
            output << data[i].join(csvSep);
            output << "\n";
        }
        file.close();
        return true;
    }
    return false;
}

bool saveAsCsv(const QList<QStringList>& data, const QString& title)
{
    AppSettings settings = MainWindow::getInstance()->getAppSettings();
    QString csvSep = settings.mCSVCellSeparator;
    
    QString currentPath = MainWindow::getInstance()->getCurrentPath();
    QString filter = QObject::tr("CSV (*.csv)");
    QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    title,
                                                    currentPath,
                                                    filter);
    QFile file(filename);
    if(file.open(QFile::WriteOnly | QFile::Truncate))
    {
        QTextStream output(&file);
        for(int i=0; i<data.size(); ++i)
        {
            output << data[i].join(csvSep);
            output << "\n";
        }
        file.close();
        return true;
    }
    return false;
}
