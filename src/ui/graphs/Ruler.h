#ifndef RULER_H
#define RULER_H

#include <QWidget>

class QScrollBar;


class Ruler: public QWidget
{
    Q_OBJECT
public:
    Ruler(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~Ruler();
    
    void setRange(const float min, const float max);
    void setCurrentRange(const float min, const float max);
    
    void showScrollBar(bool show);
    void showControls(bool show);
    
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
    void zoomChanged(float min, float max);
    
private:
    QScrollBar* mScrollBar;
    
    int mButtonsWidth;
    int mScrollBarHeight;
    
    QRectF mRulerRect;
    QRectF mButtonsRect;
    QRectF mZoomInRect;
    QRectF mZoomOutRect;
    QRectF mZoomDefaultRect;
    
    float mMin;
    float mMax;
    float mCurrentMin;
    float mCurrentMax;
    
    float mZoomPropStep;
    float mCurrentProp;
    
    float mStepMinWidth;
    float mStepWidth;
    float mYearsPerStep;
    
    bool mIsZoomInHovered;
    bool mIsZoomOutHovered;
    bool mIsZoomDefaultHovered;
    
    bool mShowScrollBar;
    bool mShowControls;
};

#endif
