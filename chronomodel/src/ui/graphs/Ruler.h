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
    Ruler(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~Ruler();
    
    void setRange(const double min, const double max);
    
    void clearAreas();
    void addArea(double start, double end, const QColor& color);
    
protected:
    void layout();
    void resizeEvent(QResizeEvent* e);
    void paintEvent(QPaintEvent* e);
    
public slots:
    void setZoom(int prop);
    void updateScroll();
    
signals:
    void positionChanged(double min, double max);
    
private:
    QScrollBar* mScrollBar;
    int mScrollBarHeight;
    
    QRectF mRulerRect;
    
    double mMin;
    double mMax;
    double mCurrentMin;
    double mCurrentMax;
    double mZoomProp;
    
    double mStepMinWidth;
    double mStepWidth;
    
    AxisTool mAxisTool;
    
    QVector<RulerArea> mAreas;
};

#endif
