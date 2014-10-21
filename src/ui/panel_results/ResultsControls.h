#ifndef ResultsControls_H
#define ResultsControls_H

#include <QWidget>
#include <vector>
#include <map>


class ResultsControls: public QWidget
{
    Q_OBJECT
public:
    ResultsControls(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ResultsControls();
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    
private:
    int mMargin;
    int mLineH;
    int mButH;
    
    QRectF mZoomRect;
    QRectF mZoomInRect;
    QRectF mZoomDefaultRect;
    QRectF mZoomOutRect;
    
    QRectF mInfosRect;
    
    QRectF mChainsRect;
    QRectF mPhasesRect;
    QRectF mEventsRect;
    QRectF mDatesRect;
    
    QRectF mDatesHistoRect;
    QRectF mDatesCalibRect;
    QRectF mDatesTraceRect;
    QRectF mDatesVarianceRect;
    
    QRectF mEventsHistoRect;
    QRectF mEventsTraceRect;
    
    QRectF mPhasesAlphaRect;
    QRectF mPhasesBetaRect;
    QRectF mPhasesPredictRect;
    
    bool mIsZoomInDown;
    bool mIsZoomDefaultDown;
    bool mIsZoomOutDown;
    
    bool mIsInfosChecked;
    
    bool mIsDatesHistoSelected;
    bool mIsDatesCalibChecked;
    bool mIsDatesTraceSelected;
    bool mIsDatesVarianceSelected;
    
    bool mIsEventsHistoSelected;
    bool mIsEventsTraceSelected;
    
    bool mIsPhasesAlphaChecked;
    bool mIsPhasesBetaChecked;
    bool mIsPhasesPredictChecked;
};

#endif
