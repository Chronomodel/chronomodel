#ifndef Painting_H
#define Painting_H

#include <QList>
#include <QtWidgets>

class Painting
{
public:
    static void init();
    
    static QColor mainColorLight;
    static QColor mainColorDark;
    static QColor mainColorGrey;
    static QColor mainGreen;
    static QList<QColor> chainColors;
    static QColor greyedOut;
};






double pointSize(double size);

void drawButton(QPainter& painter, const QRectF& r, bool hover, bool isEnabled = true, const QString& text = QString(), const QIcon& icon = QIcon());
void drawButton2(QPainter& painter, const QRectF& r, bool hover, bool isEnabled = true, const QString& text = QString(), const QIcon& icon = QIcon(), bool isFlat = false);
void drawBox(QPainter& painter, const QRectF& r, const QString& text);
void drawRadio(QPainter& painter, const QRectF& r, const QString& text, bool toggled);
void drawCheckbox(QPainter &painter, const QRectF &r, const QString& text, Qt::CheckState state);
void drawCheckBoxBox(QPainter& painter, const QRectF& r, Qt::CheckState state, const QColor& back, const QColor& border);


#endif
