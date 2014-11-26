#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include "GraphViewAbstract.h"
#include "GraphCurve.h"
#include "GraphZone.h"

#include <QWidget>
#include <QString>
#include <QFont>
#include <QColor>
#include <QPixmap>

#include <vector>
#include <map>

class Ruler;


class GraphView: public QWidget, public GraphViewAbstract
{
    Q_OBJECT
public:
    explicit GraphView(QWidget *parent = 0);
    virtual ~GraphView();
    
    void setRangeX(const float min, const float max);
    
public slots:
    // Change the ruler => emit signal => change graph
    void zoomX(const float min, const float max);
    // Change graph only, not ruler :
    void setCurrentRangeX(const float min, const float max);
    
public:
    
    // Options
    
    void setBackgroundColor(const QColor& aColor);

    void addInfo(const QString& info);
    void clearInfos();
    
    void showInfos(bool show);
    void showAxis(bool show);
    void showScrollBar(bool show);
    void showYValues(bool show, bool keepMargin = false);
    void showGrid(bool show);
    
    // Manage Curves
    
    void addCurve(const GraphCurve& curve);
    void removeCurve(const QString& name);
    void removeAllCurves();
    GraphCurve* getCurve(const QString& name);
    int numCurves() const;
    
    void addZone(const GraphZone& zone);
    void removeAllZones();
    
    // SVG export
    
    void paint(QPainter& painter, int w, int h);
    
protected:
    void adaptMarginBottom();
    
    void updateGraphSize(int w, int h);
    void repaintGraph(const bool aAlsoPaintBackground, const bool aAlsoPaintGraphs = true);
    void drawCurves(QPainter& painter);

    void resizeEvent(QResizeEvent* aEvent);
    void paintEvent(QPaintEvent* aEvent);
    void enterEvent(QEvent* e);
    void leaveEvent(QEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    
    void drawBackground(QPainter& painter);
    void drawXAxis(QPainter& painter);
    void drawYAxis(QPainter& painter);
    void drawYValues(QPainter& painter);
    
    
protected:
    Ruler* mRuler;
    
    QPixmap	mBufferedImage;
    QPixmap mBufferedImageWithGraphs;
    QColor	mBackgroundColor;
    
    QRectF  mTipRect;
    double  mTipX;
    double  mTipY;
    double  mTipWidth;
    double  mTipHeight;
    double  mTipVisible;
    double  mUseTip;
    
    QStringList mInfos;
    
    bool mShowInfos;
    bool mShowScrollBar;
    bool mShowAxis;
    bool mShowYValues;
    bool mShowGrid;
    
    int mCurveMaxResolution;
    QList<GraphCurve> mCurves;
    QList<GraphZone> mZones;
    
    float mStepXWidth;
    
    float mStepYHeight;
    float mDyPerStep;
    float mStepYMinHeight;
};

#endif
