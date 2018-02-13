#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#define GRAPH_OPENGL 0

#include "GraphViewAbstract.h"
#include "GraphCurve.h"
#include "GraphZone.h"
#include "Ruler.h"
#include "DateUtils.h"

#if GRAPH_OPENGL
#include <QOpenGLWidget>
#else
#include <QWidget>
#endif

#include <QString>
#include <QFont>
#include <QColor>
#include <QPixmap>
#include <QFileInfo>

#if GRAPH_OPENGL
class GraphView: public QOpenGLWidget, public GraphViewAbstract
#else
class GraphView: public QWidget, public GraphViewAbstract
#endif
{
    Q_OBJECT
public:
    enum Rendering
    {
        eSD = 0,
        eHD = 1
    };
    enum AxisMode
    {
        eHidden = 0,
        eMinMax = 1,
        eMainTicksOnly = 2,
        eAllTicks = 3
    };
    enum OverflowDataArrowMode
    {
        eNone = 0,
        eBothOverflow = 1,
        eUnderMin = 2,
        eOverMax = 3
    };
    GraphView(QWidget* parent = nullptr);
    explicit GraphView(const GraphView &graph, QWidget *parent= nullptr);
    void setParent(QWidget *parent) {this->QWidget::setParent(parent);}
    void copyFrom(const GraphView &graph);
    virtual ~GraphView();
    
    // Options
    
    void setBackgroundColor(const QColor &color);
    QColor getBackgroundColor() const;
    
    void addInfo(const QString& info);
    QString getInfo(char sep = '|');
    void clearInfos();
    void showInfos(bool show);
    bool isShow();
    
    void setNothingMessage(const QString& message);
    void resetNothingMessage();

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

    void setOverArrow(OverflowDataArrowMode mode) { mOverflowArrowMode = mode;}
    
    void autoAdjustYScale(bool active);
    void adjustYToMaxValue(const qreal& marginProp = 0.);
    void adjustYToMinMaxValue();
    
    void setRendering(Rendering render);
    Rendering getRendering();
    void setGraphFont(const QFont& font);

    void setGraphsThickness(int value);
    void setCurvesOpacity(int value);
    void setCanControlOpacity(bool can);
    
    // Manage Curves
    
    void addCurve(const GraphCurve& curve);
    bool hasCurve() {return ((mCurves.size() != 0) || (mZones.size() != 0)) ;}
    void removeCurve(const QString& name);
    void removeAllCurves();
    void reserveCurves(const int size);
    void setCurveVisible(const QString& name, const bool visible);
    GraphCurve* getCurve(const QString& name);
    const QList<GraphCurve>& getCurves() const;
    int numCurves() const;
    


    void addZone(const GraphZone& zone);
    void removeAllZones();
    
    // Set value formatting functions
    void setFormatFunctX(DateConversion f);
    void setFormatFunctY(DateConversion f);

    void setXScaleDivision(const Scale &sc) { mAxisToolX.setScaleDivision(sc);}
    void setXScaleDivision(const double &major, const int &minorCount) { mAxisToolX.setScaleDivision(major, minorCount);}

    void setYScaleDivision(const Scale &sc) { mAxisToolY.setScaleDivision(sc);}
    void setYScaleDivision(const double &major, const int &minorCount) { mAxisToolY.setScaleDivision(major, minorCount);}
    
    // Paint
    
    void paintToDevice(QPaintDevice* device);
    void forceRefresh() {repaintGraph(true);}
    // Save
    
    bool saveAsSVG(const QString& fileName, const QString& svgTitle, const QString& svgDescrition, const bool withVersion, const int versionHeight=20);
    
    // ToolTips
    
    void setTipXLab(const QString& lab);
    void setTipYLab(const QString& lab);

signals:
    void signalCurvesThickness(int value);

public slots:
    void updateCurvesThickness(int value);
    void zoomX(const type_data min, const type_data max);
    void exportCurrentDensityCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, double step =1.) const;

    void exportCurrentVectorCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, bool writeInRows, int offset = 0) const;

    void changeXScaleDivision (const Scale &sc);
    void changeXScaleDivision (const double &major, const int & minor);

protected:
    void adaptMarginBottom();
    
    void updateGraphSize(int w, int h);

    void drawCurves(QPainter& painter);

#if GRAPH_OPENGL
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
#else
    void resizeEvent(QResizeEvent* event);
#endif
    
    void paintEvent(QPaintEvent*);
    void repaintGraph(const bool aAlsoPaintBackground);
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
    OverflowDataArrowMode mOverflowArrowMode;

    Rendering mRendering;
    
    bool mAutoAdjustYScale; 
       
    bool mShowInfos;
    QStringList mInfos;
    
    QString mNothingMessage;
    
    QColor	mBackgroundColor;
    int mThickness;
    int mOpacity;
    bool mCanControlOpacity;
    
    QRectF  mTipRect;
    qreal  mTipX;
    qreal  mTipY;
    QString  mTipXLab;
    QString  mTipYLab;
    qreal  mTipWidth;
    qreal  mTipHeight;
    bool  mTipVisible;
    bool  mUseTip;
    
    int mCurveMaxResolution;
    QList<GraphCurve> mCurves;
    QList<GraphZone> mZones;

    QPainter mPrevPainter;
    
public:
    QString mLegendX;
    QString mLegendY;

private:
     DateConversion mUnitFunctionX;
     DateConversion mUnitFunctionY;

};

#endif

