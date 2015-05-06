#ifndef QTUTILITIES_H
#define QTUTILITIES_H

#include <QStringList>
#include <QColor>
#include <QFileInfo>
#include "AppSettings.h"
#include "AxisTool.h"

bool colorIsDark(const QColor& color);
void sortIntList(QList<int>& list);

QList<QStringList> readCSV(const QString& filePath, const QString& separator = ",");
int defaultDpiX();
qreal dpiScaled(qreal value);
QColor getContrastedColor(const QColor& color);
QList<int> stringListToIntList(const QString& listStr, const QString& separator = ",");
QStringList intListToStringList(const QList<int>& intList);
QString intListToString(const QList<int>& intList, const QString& separator = ",");

QFileInfo saveWidgetAsImage(QObject* widget, const QRect& r, const QString& dialogTitle, const QString& defaultPath,  const AppSettings & setting,AxisTool& Axe);
bool saveWidgetAsSVG(QWidget* widget, const QRect& r, const QString& fileName);

bool isComment(const QString& str);
QString prepareTooltipText(const QString& title, const QString& text);

QString line(const QString& str);
QString textBold(const QString& str);
QString textRed(const QString& str);
QString textGreen(const QString& str);
QString textBlue(const QString& str);
QString textPurple(const QString& str);

QColor randomColor();

bool constraintIsCircular( QJsonArray constraints, const int FromId, const int ToId);


#endif