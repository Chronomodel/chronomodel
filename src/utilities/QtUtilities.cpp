#include "QtUtilities.h"
#include <QtWidgets>
#include <QtSvg>


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
    if(file.open(QIODevice::ReadOnly))
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
    commentsMarkers << "//" << "#";
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

QFileInfo saveWidgetAsImage(QObject* wid, const QRect& r, const QString& dialogTitle, const QString& defaultPath)
{
    QFileInfo fileInfo;
    
    QGraphicsScene* scene = 0;
    QWidget* widget = dynamic_cast<QWidget*>(wid);
    if(!widget)
    {
        scene = dynamic_cast<QGraphicsScene*>(wid);
        if(!scene)
            return fileInfo;
    }
    
    QString filter = QObject::tr("Image (*.png);;Scalable Vector Graphics (*.svg)");
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    dialogTitle,
                                                    defaultPath,
                                                    filter);
    if(!fileName.isEmpty())
    {
        fileInfo = QFileInfo(fileName);
        bool asSvg = fileName.endsWith(".svg");
        if(asSvg)
        {
            QSvgGenerator svgGen;
            svgGen.setFileName(fileName);
            svgGen.setSize(r.size());
            svgGen.setViewBox(QRect(0, 0, r.width(), r.height()));
            QPainter p(&svgGen);
            p.setRenderHint(QPainter::Antialiasing);
            
            if(widget)
                widget->render(&p, QPoint(), QRegion(r));
            else if(scene)
                scene->render(&p, r, r);
        }
        else
        {
            qreal pr = qApp->devicePixelRatio();
            qDebug() << "Saving PNG with pixel ratio : " << pr;
            QImage image(r.width() * pr, r.height() * pr, QImage::Format_ARGB32);
            image.setDevicePixelRatio(pr);
            image.fill(Qt::transparent);
            QPainter p(&image);
            p.setRenderHint(QPainter::Antialiasing);
            
            QRectF srcRect = r;
            srcRect.setX(r.x());
            srcRect.setY(r.y());
            srcRect.setWidth(r.width() * pr);
            srcRect.setHeight(r.height() * pr);
            
            if(widget)
            {
                widget->render(&p, QPoint(), QRegion(srcRect.toRect()));
            }
            else if(scene)
            {
                scene->render(&p, image.rect(), srcRect);
            }
            image.save(fileName, "PNG");
        }
    }
    return fileInfo;
}

QString prepareTooltipText(const QString& title, const QString& text)
{
    QString result = "<div style=\"margin:10px\"><p style=\"font-weight: bold; font-style: normal\">" + title + "</p><p>" + text + "</p></div>";
    return result;
}