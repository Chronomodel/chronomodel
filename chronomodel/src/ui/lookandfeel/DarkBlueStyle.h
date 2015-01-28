#ifndef DarkBlueStyle_H
#define DarkBlueStyle_H

#include <QProxyStyle>


class DarkBlueStyle: public QProxyStyle
{
    Q_OBJECT
public:
    DarkBlueStyle();
    ~DarkBlueStyle();
    
    void drawPrimitive(PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
    void drawControl(ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* widget = 0) const;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex* opt, QPainter* p, const QWidget* widget) const;
    
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const;
    QRect subElementRect(SubElement sr, const QStyleOption* opt, const QWidget* widget) const;
    int pixelMetric(PixelMetric m, const QStyleOption* opt, const QWidget* widget) const;
    
private:
    QStyle* mStyle;
};

#endif
