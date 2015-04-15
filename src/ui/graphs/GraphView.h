#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include "GraphViewAbstract.h"
#include "GraphCurve.h"
#include "GraphZone.h"
#include "Ruler.h"

#include <QWidget>
#include <QString>
#include <QFont>
#include <QColor>
#include <QPixmap>
#include <QFileInfo>


class GraphView: public QWidget, public GraphViewAbstract
{
    Q_OBJECT
public:
    enum Rendering{
        eSD = 0,
        eHD = 1
    };
    enum AxisMode{
        eHidden = 0,
        eMinMax = 1,
        eMainTicksOnly = 2,
        eAllTicks = 3
    };
    
    GraphView(QWidget* parent = 0);
    virtual ~GraphView();
    
    // Options
    
    void setBackgroundColor(const QColor& aColor);
    QColor getBackgroundColor() const;
    
    void addInfo(const QString& info);
    void clearInfos();
    void showInfos(bool show);
    
    void setNothingMessage(const QString& message);
    void resetNothingMessage();
    
    void setRendering(Rendering render);
    Rendering getRendering();
    
    void showAxisArrows(bool show);
    void showAxisLines(bool show);
    void showVertGrid(bool show);
    void showHorizGrid(bool show);
    void setXAxisMode(AxisMode mode);
    void setYAxisMode(AxisMode mode);
    void autoAdjustYScale(bool active);
    
    void setGraphFont(const QFont& font);
    void setCurvesThickness(int value);
    
    // Manage Curves
    
    void addCurve(const GraphCurve& curve);
    void removeCurve(const QString& name);
    void removeAllCurves();
    GraphCurve* getCurve(const QString& name);
    int numCurves() const;
    
    void addZone(const GraphZone& zone);
    void removeAllZones();
    
    // Paint
    
    void paintToDevice(QPaintDevice* device);//, QPaintEvent* e);// HL

    
    // Save
    
    bool saveAsSVG(const QString& fileName, const QString svgTitle, const QString svgDescrition, const bool withVersion, int const versionHeight=20);
    
public slots:
    void zoomX(const double min, const double max);
    void exportCurrentCurves(const QString& defaultPath, const QString& csvSep, bool writeInRows, int offset = 0) const;
    
protected:
    void adaptMarginBottom();
    
    void updateGraphSize(int w, int h);
    void repaintGraph(const bool aAlsoPaintBackground);
    void drawCurves(QPainter& painter);

    void resizeEvent(QResizeEvent* event);
    void paintEvent(QPaintEvent*);
   
    void enterEvent(QEvent* e);
    void leaveEvent(QEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    
protected:
    QPixmap	mBufferBack;
    
    AxisTool mAxisToolX;
    AxisTool mAxisToolY;
    qreal mStepMinWidth;
    
    Rendering mRendering;
    bool mShowAxisArrows;
    bool mShowAxisLines;
    bool mShowVertGrid;
    bool mShowHorizGrid;
    AxisMode mXAxisMode;
    AxisMode mYAxisMode;
    bool mAutoAdjustYScale;
    
    bool mShowInfos;
    QStringList mInfos;
    
    QString mNothingMessage;
    
    QColor	mBackgroundColor;
    int mThickness;
    
    QRectF  mTipRect;
    double  mTipX;
    double  mTipY;
    double  mTipWidth;
    double  mTipHeight;
    double  mTipVisible;
    double  mUseTip;
    
    int mCurveMaxResolution;
    QList<GraphCurve> mCurves;
    QList<GraphZone> mZones;
};

#endif
