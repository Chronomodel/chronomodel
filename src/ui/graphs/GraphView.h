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
    QString getInfo();
    void clearInfos();
    void showInfos(bool show);
    bool isShow();
    
    void setNothingMessage(const QString& message);
    void resetNothingMessage();
    
    void setRendering(Rendering render);
    Rendering getRendering();
    
    void showXAxisLine(bool show);
    void showXAxisArrow(bool show);
    void showXAxisTicks(bool show);
    void showXAxisSubTicks(bool show);
    void showXAxisValues(bool show);
    
    void showYAxisLine(bool show);
    void showYAxisArrow(bool show);
    void showYAxisTicks(bool show);
    void showYAxisSubTicks(bool show);
    void showYAxisValues(bool show);
    
    void setXAxisMode(AxisMode mode);
    void setYAxisMode(AxisMode mode);
    
    void autoAdjustYScale(bool active);
    void adjustYToMaxValue(const double& marginProp = 0.1);
    void adjustYToMinMaxValue();
    
    void setGraphFont(const QFont& font);
    void setCurvesThickness(int value);
    
    // Manage Curves
    
    void addCurve(const GraphCurve& curve);
    void removeCurve(const QString& name);
    void removeAllCurves();
    void setCurveVisible(const QString& name, const bool visible);
    GraphCurve* getCurve(const QString& name);
    const QList<GraphCurve>& getCurves() const;
    int numCurves() const;
    
    void addZone(const GraphZone& zone);
    void removeAllZones();
    
    // Set value formatting functions
    void setFormatFunctX(FormatFunc f);
    void setFormatFunctY(FormatFunc f);
    
    // Paint
    
    void paintToDevice(QPaintDevice* device);
 
    // Save
    
    bool saveAsSVG(const QString& fileName, const QString& svgTitle, const QString& svgDescrition, const bool withVersion, const int versionHeight=20);
    
    // ToolTips
    
    void setTipXLab(const QString& lab);
    void setTipYLab(const QString& lab);
    
public slots:
    void zoomX(const double min, const double max);
    void exportCurrentDensityCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, double step =1.) const;

    void exportCurrentVectorCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, bool writeInRows, int offset = 0) const;
    //void exportCurrentCurves_old(const QString& defaultPath, const QString& csvSep, bool writeInRows, int offset = 0) const;
    
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
    
    bool mXAxisLine;
    bool mXAxisArrow;
    bool mXAxisTicks;
    bool mXAxisSubTicks;
    bool mXAxisValues;
    
    bool mYAxisLine;
    bool mYAxisArrow;
    bool mYAxisTicks;
    bool mYAxisSubTicks;
    bool mYAxisValues;
    
    AxisMode mXAxisMode;
    AxisMode mYAxisMode;
    
    Rendering mRendering;
    
    bool mAutoAdjustYScale; 
    
    FormatFunc mFormatFuncX;
    FormatFunc mFormatFuncY;
    
    bool mShowInfos;
    QStringList mInfos;
    
    QString mNothingMessage;
    
    QColor	mBackgroundColor;
    int mThickness;
    
    QRectF  mTipRect;
    double  mTipX;
    double  mTipY;
    QString  mTipXLab;
    QString  mTipYLab;
    double  mTipWidth;
    double  mTipHeight;
    double  mTipVisible;
    double  mUseTip;
    
    int mCurveMaxResolution;
    QList<GraphCurve> mCurves;
    QList<GraphZone> mZones;
    
public:
    QString mLegendX;
    QString mLegendY;
};

#endif
