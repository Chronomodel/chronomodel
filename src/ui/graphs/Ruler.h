#ifndef RULER_H
#define RULER_H

#include <QWidget>
#include "AxisTool.h"


class QScrollBar;

struct RulerArea{
    int mStart;
    int mStop;
    QColor mColor;
};

class Ruler: public QWidget
{
    Q_OBJECT
public:
    Ruler(QWidget* parent = nullptr, Qt::WindowFlags flags =  Qt::Widget);
    ~Ruler();
    Ruler& operator=(const Ruler & origin);

    double realPosition;
    double mCurrentMin;
    double mCurrentMax;
    double mMin;
    double mMax;
    double mZoomProp;
    void setFont(const QFont &font);

    void setRange(const double min, const double max);
    void setCurrent(const double min, const double max);
    void setMarginTop (const qreal &top) {mMarginTop = top;}
    void setMarginBottom (const qreal &bottom) {mMarginBottom = bottom;}
    void setMarginRight (const qreal &right) {mMarginRight = right;}
    void setMarginLeft (const qreal &left) {mMarginLeft = left;}

     void currentChanged(const double &min, const double &max);
     void setScaleDivision (const double &major, const int &minorCount);
     void setScaleDivision (const Scale &sc);

     double getZoom();
     double getRealValue();

     void clearAreas();
     void addArea(int start, int end, const QColor& color);
     // Set value formatting functions
     void setFormatFunctX(DateConversion f);
    void updateLayout();
protected:

    void resizeEvent(QResizeEvent* e);
    void paintEvent(QPaintEvent* e);
    
    DateConversion mFormatFuncX;
    
public slots:
    void setZoom(double & prop);
    void updateScroll();
    void scrollValueChanged(double value);

    
signals:
    void positionChanged(double min, double max);
    void valueChanged(double value);
    
private:
    QScrollBar* mScrollBar;
    qreal mScrollBarHeight;
    QFont mAxisFont;
    QRectF mAxisRect;

    qreal mStepMinWidth;
    qreal mStepWidth;
    qreal mMarginLeft;
    qreal mMarginRight;
    qreal mMarginTop;
    qreal mMarginBottom;

    AxisTool mAxisTool;

    
    QVector<RulerArea> mAreas;
};

#endif
