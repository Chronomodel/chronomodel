#ifndef RULER_H
#define RULER_H

#include <QWidget>

class QScrollBar;

class AxisTool
{
public:
    AxisTool();
    void updateValues(double totalPix, double minDeltaPix, double minVal, double maxVal);
    QVector<double> paint(QPainter& p, const QRectF& r, double textW);
    
public:
    bool mIsHorizontal;
    bool mShowSubs;
    bool mMinMaxOnly;
    double mDeltaVal;
    double mDeltaPix;
    double mStartVal;
    double mStartPix;
};

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
    void setCurrentRange(const double min, const double max);
    
    void showScrollBar(bool show);
    void showControls(bool show);
    
    void clearAreas();
    void addArea(double start, double end, const QColor& color);
    
protected:
    void resizeEvent(QResizeEvent* e);
    void paintEvent(QPaintEvent* e);
    
    void enterEvent(QEvent* e);
    void leaveEvent(QEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void mousePressEvent(QMouseEvent* e);
    
    void layout();
    void updateGeometry();
    void updateZoom();
    
public slots:
    void zoomIn();
    void zoomOut();
    void zoomDefault();
    
private slots:
    void setZoomPosition(int pos);
    
signals:
    void zoomChanged(double min, double max);
    
private:
    QScrollBar* mScrollBar;
    
    int mButtonsWidth;
    int mScrollBarHeight;
    
    QRectF mRulerRect;
    QRectF mButtonsRect;
    QRectF mZoomInRect;
    QRectF mZoomOutRect;
    QRectF mZoomDefaultRect;
    
    double mMin;
    double mMax;
    double mCurrentMin;
    double mCurrentMax;
    
    double mZoomPropStep;
    double mCurrentProp;
    
    double mStepMinWidth;
    double mStepWidth;
    double mYearsPerStep;
    
    AxisTool mAxisTool;
    
    bool mIsZoomInHovered;
    bool mIsZoomOutHovered;
    bool mIsZoomDefaultHovered;
    
    bool mShowScrollBar;
    bool mShowControls;
    
    QVector<RulerArea> mAreas;
};

#endif
