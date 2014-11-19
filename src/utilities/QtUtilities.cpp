#include "QtUtilities.h"

#include <QApplication>
#include <QScreen>
#include <QFile>
#include <QTextStream>
#include <QDebug>


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

