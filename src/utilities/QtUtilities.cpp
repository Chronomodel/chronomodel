#include "QtUtilities.h"
#include "StateKeys.h"
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
            if(line.left(1) != "#")
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

QFileInfo saveWidgetAsImage(QObject* wid, const QRect& r, const QString& dialogTitle, const QString& defaultPath, const AppSettings & appSetting, AxisTool& Axe)
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
       
        
        
        if (fileExtension == "svg") {
           
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
            {;
                saveWidgetAsSVG(widget, r, fileName);
                
            }
           
        }
        else
        { // save PNG
            
            int versionHeight = 20;
            //qreal pr = 1;//qApp->devicePixelRatio();
           /* qreal prh=  32000. / ( r.height() + versionHeight) ; // QImage axes are limited to 32767x32767 pixels
           
            qreal prw=  32000. / r.width() ;                  qreal pr = (prh<prw)? prh : prw;
            if (pr>4) {
                pr=4;
            }
            */
          
            short pr =  appSetting.mPixelRatio;// 4.;//0.005;
            qDebug()<<" pr="<<pr;
            if(widget)
            {
                //QSize wSize = widget->size();
                int heightAxe = 0;
                if (Axe.mDeltaPix>0) heightAxe = 20;
                QImage image(r.width() * pr, (r.height() + versionHeight + heightAxe+20) * pr , QImage::Format_ARGB32_Premultiplied); //Format_ARGB32_Premultiplied //Format_ARGB32
              //  QImage image(int(r.width() * pr), int((r.height() + versionHeight) * pr), QImage::Format_ARGB32_Premultiplied);
                //QImage image(wSize, QImage::Format_ARGB32_Premultiplied);
               // qDebug()<<"r.width() * pr"<< (r.width() * pr)<<" (r.height() + versionHeight) * pr"<<(r.height() + versionHeight) * pr;
                //qDebug()<<" wSize.width()"<< wSize.width()<<" wSize.height()"<<wSize.height();
                if (image.isNull() ) {
                    qDebug()<< " image width = 0";
                    return fileInfo;
                }
                image.setDevicePixelRatio(pr);
                image.fill(Qt::transparent);
                
                QPainter p;
                p.begin(&image);
                p.setRenderHint(QPainter::Antialiasing);
                //QRectF tgtRect = image.rect();
             //   tgtRect.adjust(0, 0, 0, -versionHeight * pr);
                widget->render(&p, QPoint(0, 0), QRegion(r.x(), r.y(), r.width(), r.height()));
                //widget->render(&p,QPoint(0, 0), QRegion(r.x(), r.y(), r.width(), r.height()/2));
                //widget->render(&p,QPoint(0, r.height()/2), QRegion(r.x(), r.y()+r.height()/2, r.width(), r.height()));
               
                //Keep it in memory : mMarginLeft(50), mMarginRight(10), mMarginTop(5), mMarginBottom(15), in graphViewAbstract
               
                if (Axe.mDeltaPix>0){
                    
                    //Axe.updateValues(r.width()-10-50 , Axe.mDeltaPix, Axe.mStartVal, Axe.mEndVal);
                    Axe.updateValues(r.width()-10-50 , 50, Axe.mStartVal, Axe.mEndVal);
                    Axe.mMinMaxOnly = false;
                    Axe.mShowSubs = true;
                    Axe.mShowSubSubs = true;
                    Axe.mShowText = true;
                    Axe.paint(p, QRectF(50, r.height()+heightAxe, r.width()-10-50 ,  heightAxe), 5);
                }
                
                p.setPen(Qt::black);
                p.drawText(0, r.height()+heightAxe+versionHeight, r.width(), versionHeight,
                           Qt::AlignCenter,
                           qApp->applicationName() + " " + qApp->applicationVersion());
                
                //mAxisToolX.paint(p, QRectF(mMarginLeft, mMarginTop + mGraphHeight, mGraphWidth ,  mMarginBottom), 5);
              
                p.end();
                //image.save(fileName, "PNG");
               // char formatExt[];
                if (fileExtension=="png") {
                 //   formatExt[] = "png";
                     image.save(fileName, "png");
                }
                else if (fileExtension == "jpg") {
                    //formatExt[5] = "jpg";
                     image.save(fileName, "jpg",50);
                }
                else if (fileExtension == "bmp") {
                   
                    image.save(fileName, "bmp");
                }
                    
                //image.save(fileName, formatExt);
                /*QImageWriter writer;
                writer.setFormat("jpg");
                writer.setQuality(100);
                writer.setFileName(fileName+"_jpg");
                writer.write(image);*/
               
            }
            else if(scene)
            {
                int versionHeight = 20;
                
                //qreal pr = 4;//qApp->devicePixelRatio();
                //qDebug() << "Saving PNG with pixel ratio : " << pr;
                QImage image(r.width() * pr, (r.height() + versionHeight) * pr, QImage::Format_ARGB32);
                image.setDevicePixelRatio(pr);
                image.fill(Qt::transparent);
                
                QPainter p;
                p.begin(&image);
                p.setRenderHint(QPainter::Antialiasing);
                
                QRectF srcRect = r;
                srcRect.setX(r.x());
                srcRect.setY(r.y());
                srcRect.setWidth(r.width() * pr);
                srcRect.setHeight(r.height() * pr);
               
                QRectF tgtRect = image.rect();
                tgtRect.adjust(0, 0, 0, -versionHeight * pr);
                
                scene->render(&p, tgtRect, srcRect);
                p.setPen(Qt::black);
                p.drawText(0, r.height(), r.width(), versionHeight,
                           Qt::AlignCenter,
                           qApp->applicationName() + " " + qApp->applicationVersion());
                p.end();
               image.save(fileName, "PNG");
            }
           
        }
    }
   // qDebug()<<"QFileInfo saveWidgetAsImage image.save"<<fileName;
    
    return fileInfo;
}

bool saveWidgetAsSVG(QWidget* widget, const QRect& r, const QString& fileName)
{
    
    int versionHeight = 20;
  
    
    QSvgGenerator svgGenFile;
    svgGenFile.setFileName(fileName);
    svgGenFile.setViewBox(r);
    svgGenFile.setDescription(QObject::tr("SVG widget drawing "));
    
    QPainter p;
    p.begin(&svgGenFile);
    widget->render(&p);//, QPoint(0, 0), QRegion(r));
  
    p.setPen(Qt::black);
    p.drawText(0, r.height(), r.width(), versionHeight,
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
    return "<span style=\"color: green;\">" + str + "</span>";
}
QString textBlue(const QString& str)
{
    return "<span style=\"color: blue;\">" + str + "</span>";
}
QString textPurple(const QString& str)
{
    return "<span style=\"color: #8E60AB;\">" + str + "</span>";
}

QColor randomColor()
{
    return QColor(rand() % 255,
                  rand() % 255,
                  rand() % 255);
}

bool constraintIsCircular( QJsonArray constraints, const int FromId, const int ToId)
{    
    /*bool isCircular = false;
   qDebug() << "entre dans constraintIsCircular true FromId="<< FromId <<"  ToId="<<ToId ;*/
    
    for(int i=0; i<constraints.size(); ++i)
    {
        QJsonObject constraint = constraints[i].toObject();
        
      /* qDebug() << "------------------------------";
       qDebug() << "comparaison constraint[STATE_CONSTRAINT_BWD_ID].toInt()=="<< constraint[STATE_CONSTRAINT_BWD_ID].toInt()<<"  constraint[STATE_CONSTRAINT_FWD_ID].toInt()=="<<constraint[STATE_CONSTRAINT_FWD_ID].toInt() ;*/
        
        //Interdit l'inversion
        if(constraint[STATE_CONSTRAINT_BWD_ID].toInt() == ToId && constraint[STATE_CONSTRAINT_FWD_ID].toInt() == FromId)
        {
            /*isCircular = true;
            qDebug() << "constraintIsCircular true" ;*/
            return true;
        }
        else if (constraint[STATE_CONSTRAINT_BWD_ID].toInt() == ToId) {
                int ToIdFellower =  constraint[STATE_CONSTRAINT_FWD_ID].toInt();
                if(constraintIsCircular(constraints, FromId ,ToIdFellower))
                {
                   /* isCircular = true;
                    qDebug() << "constraintIsCircular true" ;*/
                    return true;
                };
            }
    }
    
    return false;//isCircular;
}
