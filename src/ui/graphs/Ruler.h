#ifndef RULER_H
#define RULER_H

#include <QWidget>
#include "AxisTool.h"


class QScrollBar;

struct RulerArea{
    double mStart;
    double mStop;
    QColor mColor;
};

class Ruler: public QWidget
{
    Q_OBJECT
public:
    Ruler(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
    ~Ruler();
    
    void setFont(const QFont &font);
    void setRange(const double min, const double max);
    void setCurrent(const double min, const double max);
    void currentChanged(const double min, const double max);
    double getZoom();
    double getRealValue();
    
    void clearAreas();
    void addArea(double start, double end, const QColor& color);
    // Set value formatting functions
    void setFormatFunctX(FormatFunc f);
    
    double realPosition;
    double mCurrentMin;
    double mCurrentMax;
    double mMin;
    double mMax;
    double mZoomProp;
    
    qreal mMarginLeft;
    qreal mMarginRight;
    
    AxisTool mAxisTool;
   // QFont mFont;
    
protected:
    void layout();
    void resizeEvent(QResizeEvent* e);
    void paintEvent(QPaintEvent* e);
    
    FormatFunc mFormatFuncX;
    
public slots:
    void setZoom(int prop);
    void updateScroll();
    void scrollValueChanged(int value);

    
signals:
    void positionChanged(double min, double max);
    void valueChanged(int value);
    
private:
    QScrollBar* mScrollBar;
    qreal mScrollBarHeight;
    
    QRectF mRulerRect;
        
    qreal mStepMinWidth;
    qreal mStepWidth;

    
    QVector<RulerArea> mAreas;
};

#endif
