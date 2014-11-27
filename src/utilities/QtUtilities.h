#ifndef QTUTILITIES_H
#define QTUTILITIES_H

#include <QStringList>
#include <QColor>
#include <QFileInfo>

bool colorIsDark(const QColor& color);
void sortIntList(QList<int>& list);

QList<QStringList> readCSV(const QString& filePath, const QString& separator = ",");
int defaultDpiX();
qreal dpiScaled(qreal value);
QColor getContrastedColor(const QColor& color);
QList<int> stringListToIntList(const QString& listStr, const QString& separator = ",");
QFileInfo saveWidgetAsImage(QObject* widget, const QRect& r, const QString& dialogTitle, const QString& defaultPath);
bool isComment(const QString& str);

#endif