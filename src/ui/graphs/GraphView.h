/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include "GraphViewAbstract.h"
#include "GraphCurve.h"
#include "GraphZone.h"
#include "AxisTool.h"
#include "DateUtils.h"

#include <QWidget>
#include <QString>
#include <QFont>
#include <QColor>
#include <QPixmap>
#include <QFileInfo>
#include <QPainterPath>
#include <QStaticText>

class GraphView: public GraphViewAbstract
{
    Q_OBJECT

    Q_PROPERTY(int thickness MEMBER mThickness READ getGraphsThickness WRITE setGraphsThickness NOTIFY signalCurvesThickness);
    Q_PROPERTY(QColor backgroundColor MEMBER mBackgroundColor READ getBackgroundColor WRITE setBackgroundColor);
public:

    enum AxisMode
    {
        eHidden = 0,
        eMinMax = 1,
        eMinMaxHidden = 2,
        eMainTicksOnly = 3,
        eAllTicks = 4
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

    void copyFrom(const GraphView &graph);
    virtual ~GraphView();

    // Options

    void setBackgroundColor(const QColor &color);
    QColor getBackgroundColor() const;

    void setInfo(const QString& info) {mInfos.clear(); mInfos.append(info);};
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

    void setXAxisSupport(AxisTool::AxisSupport support) {mAxisToolX.mSupport = support;};
    void setYAxisSupport(AxisTool::AxisSupport support) {mAxisToolY.mSupport = support;};

    void setXAxisMode(AxisMode mode);
    void setYAxisMode(AxisMode mode);

    void setOverArrow(OverflowDataArrowMode mode) { mOverflowArrowMode = mode;}

    void autoAdjustYScale(bool active);
    inline bool autoAdjustY() {return mAutoAdjustYScale;};
    void adjustYScale();

   // void setRendering(Rendering render);
   // Rendering getRendering();
    void setGraphFont(const QFont& font);

    void setGraphsThickness(int value);
    int getGraphsThickness()const {return mThickness;};

    void setCurvesOpacity(int value);
    void setCanControlOpacity(bool can);

    // Manage Curves

    void add_curve(const GraphCurve& curve);
    inline bool has_curves() {return ((mCurves.size() != 0) || (mZones.size() != 0)) ;}

    void removeCurve(const QString& name);
    void removeAllCurves();
    void reserveCurves(const int size);
    void squeezeCurves() {mCurves.squeeze();};
    void setCurveVisible(const QString& name, const bool visible);
    GraphCurve* getCurve(const QString& name);
    const QList<GraphCurve>& getCurves() const;
    int numCurves() const;

    inline bool has_points() const {return (refPoints.size() != 0) ;}
    inline CurveRefPts* get_refPoint(int i) {return &refPoints[i];}
    void set_points(const std::vector<CurveRefPts> refPts) {refPoints = refPts;};
    void add_point(const CurveRefPts refPt) {refPoints.push_back(refPt);};
    inline void insert_points(const std::vector<CurveRefPts> refPts) {refPoints.insert(refPoints.end(), refPts.begin(), refPts.end());};
    void set_points_visible(const QString &name, const bool visible);

    void add_zone(const GraphZone& zone);
    void remove_all_zones();

    // Set value formatting functions
    void setFormatFunctX(DateConversion f);
    void setFormatFunctY(DateConversion f);

    void setXScaleDivision(const Scale& sc) { mAxisToolX.setScaleDivision(sc);}
    void setXScaleDivision(const double& major, const int& minorCount) { mAxisToolX.setScaleDivision(major, minorCount);}

    void setYScaleDivision(const Scale& sc) { mAxisToolY.setScaleDivision(sc);}
    void setYScaleDivision(const double& major, const int& minorCount) { mAxisToolY.setScaleDivision(major, minorCount);}

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

    void exportCurrentDensities(const QString& defaultPath, const QLocale& locale, const QString& csvSep, double step = 1.) const;
    void exportCurrentVectorCurves(const QString& defaultPath, const QLocale& locale, const QString& csvSep, bool writeInRows, int offset = 0) const;

    void exportCurrentCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, double step = 1., QString graph_title = "") const;
    void exportReferenceCurves(const QString& defaultPath, const QLocale locale = QLocale::English, const QString& csvSep = ",", double step = 1.0, QString filename = "") const;

    void changeXScaleDivision (const Scale& sc);
    void changeXScaleDivision (const double& major, const int& minor);

protected:
    void adaptMarginBottom();

    void updateGraphSize(qreal w, qreal h);

    QPainterPath makePath (const QMap<double, double>& map, const bool showBorder) const;

    void drawCurves(QPainter& painter);
    void drawMap(const GraphCurve& curve, QPainter& painter);
    QPainterPath makeSubShape(const QMap<double, double>& mapInf, const QMap<double, double>& mapSup) const;
    void drawShape(const GraphCurve& curve, QPainter& painter);
    void drawDensity(const GraphCurve& curve, QPainter& painter);

    void resizeEvent(QResizeEvent*);

    void paintEvent(QPaintEvent*);
    void repaintGraph(const bool aAlsoPaintBackground);
    void enterEvent(QEnterEvent* e);
    void leaveEvent(QEvent* e);
    void mouseMoveEvent(QMouseEvent* e);

protected:

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

   // Rendering mRendering;

    bool mAutoAdjustYScale;

    bool mShowInfos;
    QStringList mInfos;

    QString mNothingMessage;

    QColor mBackgroundColor;
    int mThickness;
    int mOpacity;
    bool mCanControlOpacity;

    QRectF  mTipRect;
    qreal  mTipX;
    qreal  mTipY;
    QString  mTipXLab;
    QString  mTipYLab;
    QString  mTipComment;
    qreal  mTipWidth;
    qreal  mTipHeight;
    bool  mTipVisible;
    bool  mUseTip;

    QList<GraphCurve> mCurves;
    QList<GraphZone> mZones;

    QPainter mPrevPainter;

    qreal mBottomSpacer;

public:
    QString mLegendX;
    QString mLegendY;
    std::vector<CurveRefPts> refPoints;

private:
    DateConversion mUnitFunctionX;
    DateConversion mUnitFunctionY;

};



class GraphTitle: public GraphViewAbstract
{
    Q_OBJECT
protected:
    qreal mTitleHeight;
    qreal mSubTitleHeight;

    QStaticText mTitle;
    QStaticText mCommentTitle;
    QStaticText mSubTitle;
    QColor mBackgroundColor;
    QColor mTitleBarColor;

    bool mAutoAdjustTitleHeight;
    bool mAutoAdjustSubTitleHeight;

public:
    GraphTitle(QWidget* parent = nullptr);
    virtual ~GraphTitle();

    explicit GraphTitle(QString title, QWidget* parent = nullptr);
    explicit GraphTitle(QString title, QColor titleBarColor, QWidget* parent);

    explicit GraphTitle(QString title, QString subTitle, QWidget* parent = nullptr);
    explicit GraphTitle(QString title, QString subTitle, QColor backGround, QWidget* parent = nullptr);

    explicit GraphTitle(QString title, QString commentTitle, QString subTitle, QWidget* parent = nullptr);

    void paintEvent(QPaintEvent*);
    void repaintGraph(const bool aAlsoPaintBackground);

    void setTitle(const QString& title) {mTitle.setText(title);};
    void setSubTitle(const QString& subTitle) {mSubTitle.setText(subTitle);};
    void setBackGroundColor(const QColor& color) {mBackgroundColor = color;};
    void setTitleBarColor(const QColor& color) {mTitleBarColor = color;};

    void setTitleHeight(const qreal h) {mTitleHeight = h;; mAutoAdjustTitleHeight = false;};
    qreal titleHeight() {return mTitleHeight;};

    void setSubTitleHeight(const qreal h) {mSubTitleHeight = h; mAutoAdjustSubTitleHeight = false;};
    qreal subTitleHeight() {return mSubTitleHeight;};

    void setAutoAdjustTitleHeight(bool adjust) {mAutoAdjustTitleHeight = adjust;};
    void setAutoAdjustSubTitleHeight(bool adjust) {mAutoAdjustSubTitleHeight = adjust;};
    bool autoAdjustTitleHeight() {return mAutoAdjustTitleHeight;};
    bool autoAdjustSubTitleHeight() {return mAutoAdjustSubTitleHeight;};

    qreal height();

    bool isTitle() const {return !mTitle.text().isEmpty();};
    bool withTitle() const {return !mSubTitle.text().isEmpty();};
};



#endif
