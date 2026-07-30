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
#include <QPicture>
#include <QElapsedTimer>
#include <QTimer>

class GraphView: public GraphViewAbstract
{
    Q_OBJECT

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

    inline void setInfo(const QString& info)
    {
        mInfos.clear();
        mInfos.append(info);
    };

    QString getInfo(char sep = '|');
    bool isShow();

    inline void setBackgroundColor(const QColor& color)
    {
        mBackgroundColor = color;
    }

    inline QColor getBackgroundColor() const
    {
        return mBackgroundColor;
    }

    inline void addInfo(const QString& info)
    {
        mInfos << info;
    }

    inline void clearInfos()
    {
        mInfos.clear();
    }

    inline void showInfos(bool show)
    {
        mShowInfos = show;
    }

    inline void setNothingMessage(const QString& message)
    {
        mNothingMessage = message;
    }

    inline void resetNothingMessage()
    {
        mNothingMessage = tr("Nothing to display");
    }

    // Just Setter no action
    inline void showXAxisLine(bool show)
    {
        mXAxisLine = show;

    }
    inline void showXAxisArrow(bool show)
    {
        mXAxisArrow = show;
    }
    inline void showXAxisTicks(bool show)
    {
        mXAxisTicks = show;
    }
    inline void showXAxisSubTicks(bool show)
    {
        mXAxisSubTicks = show;
    }
    inline void showXAxisValues(bool show)
    {
        mXAxisValues = show;
    }

    inline void showYAxisLine(bool show)
    {
        mYAxisLine = show;
    }
    inline void showYAxisArrow(bool show)
    {
        mYAxisArrow = show;
    }
    inline void showYAxisTicks(bool show)
    {
        mYAxisTicks = show;
    }
    inline void showYAxisSubTicks(bool show)
    {
        mYAxisSubTicks = show;
    }
    inline void showYAxisValues(bool show)
    {
        mYAxisValues = show;
    }

    void setXAxisMode(AxisMode mode)
    {
        mXAxisMode = mode;
        mAxisToolX.mShowText = (mXAxisMode != eHidden);
    }

    inline void setXAxisSupport(AxisTool::AxisSupport support)
    {
        mAxisToolX.mSupport = support;
    }
    inline void setYAxisSupport(AxisTool::AxisSupport support)
    {
        mAxisToolY.mSupport = support;
    }

    void setYAxisMode(AxisMode mode);

    inline void setOverArrow(OverflowDataArrowMode mode)
    {
        mOverflowArrowMode = mode;
    }

    /**
     * @brief If active is true, the current view automaticaly adjust Y axis on the next paint.
     */
    inline void autoAdjustYScale(bool active)
    {
        mAutoAdjustYScale = active;
    }

    inline bool autoAdjustY() const
    {
        return mAutoAdjustYScale;
    }

    void adjustYScale();

   // void setRendering(Rendering render);
   // Rendering getRendering();
    void setGraphFont(const QFont& font);

    /**
     * @brief GraphView::setCurvesThickness, set mThickness without repaint
     * @param value
    */
    inline void setCurvesThickness(int value)
    {
        mThickness = value;
    }
    void updateCurvesThickness(int value);
    inline int getGraphsThickness() const
    {
        return mThickness;
    }

    inline void setCurvesOpacity(int value)
    {
        mOpacity = value;
    }
    void updateCurvesOpacity(int value);

    inline void setCanControlOpacity(bool can)
    {
        mCanControlOpacity = can;
    }
    // Manage Curves

    void add_curve(const GraphCurve& curve);
    inline bool has_curves() const
    {
        return ((mCurves.size() != 0) || (mZones.size() != 0)) ;
    }

    void removeCurve(const QString& name);
    void removeAllCurves();
    void reserveCurves(const int size);
    void squeezeCurves() {mCurves.squeeze();};
    void setCurveVisible(const QString& name, const bool visible);
    void setCurveVisible(const QStringList& names, const bool visible);
    void setCurveVisible(std::initializer_list<const char*> names, const bool visible);

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

    // Set value formatting functions
    inline void setFormatFunctX(DateConversion f)
    {
        mUnitFunctionX = f;
    }

    inline void setFormatFunctY(DateConversion f)
    {
        mUnitFunctionY = f;
    }

    inline void setXScaleDivision(const Scale& sc)
    {
        mAxisToolX.setScaleDivision(sc);
    }
    inline void setXScaleDivision(const double& major, const int& minorCount)
    {
        mAxisToolX.setScaleDivision(major, minorCount);
    }

    inline void setYScaleDivision(const Scale& sc)
    {
        mAxisToolY.setScaleDivision(sc);
    }
    inline void setYScaleDivision(const double& major, const int& minorCount)
    {
        mAxisToolY.setScaleDivision(major, minorCount);
    }

    // Paint

    void paintToDevice(QPaintDevice* device);
    inline void forceRefresh() {
        repaintGraph();
    }
    // Save

    bool saveAsSVG(const QString& fileName, const QString& svgTitle, const QString& svgDescrition, const bool withVersion, const int versionHeight = 20);

    // ToolTips

    void setTipXLab(const QString& lab);
    void setTipYLab(const QString& lab);

public slots:

    void zoomX(const type_data min, const type_data max);

    void exportCurrentDensities(const QString& defaultPath, const QLocale& locale, const QString& csvSep, double step = 1.0) const;
    void exportCurrentVectorCurves(const QString& defaultPath, const QLocale& locale, const QString& csvSep, bool writeInRows, int offset = 0) const;

    void exportCurrentCurves(const QString& defaultPath, const QLocale locale, const QString& csvSep, double step = 1.0, QString graph_title = "") const;
    void exportReferenceCurves(const QString& defaultPath, const QLocale locale = QLocale::English, const QString& csvSep = ",", double step = 1.0, QString filename = "", const double threshold = 95.0, bool isHPD = false) const;

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

    void resizeEvent(QResizeEvent*) override;

    void paintEvent(QPaintEvent*) override;
    void repaintGraph() override;
    void updateRasterCache();
    void drawTooltip();

    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;

    void autoUpdate(); // Méthode d'update

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

    QPicture mVectorCache;      // Cache vectoriel (qualité)
    QPixmap mRasterCache;       // Cache raster (performance)

    QElapsedTimer mLastFastPaint;
    bool mIsScrolling = false;
    bool mCacheValid = false;  // Indique si mRasterCache est à jour
    QTimer* mUpdateTimer; // Timer pour la mise à jour automatique

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
    void repaintGraph();

    inline void setTitle(const QString& title)
    {
        mTitle.setText(title);
    };
    inline void setSubTitle(const QString& subTitle)
    {
        mSubTitle.setText(subTitle);
    };
    inline void setBackGroundColor(const QColor& color)
    {
        mBackgroundColor = color;
    };
    inline void setTitleBarColor(const QColor& color)
    {
        mTitleBarColor = color;
    };

    inline void setTitleHeight(const qreal h)
    {
        mTitleHeight = h;
        mAutoAdjustTitleHeight = false;
    };
    inline qreal titleHeight() const
    {
        return mTitleHeight;
    };

    inline void setSubTitleHeight(const qreal h) {
        mSubTitleHeight = h;
        mAutoAdjustSubTitleHeight = false;
    };
    inline qreal subTitleHeight() const
    {
        return mSubTitleHeight;
    }

    void setAutoAdjustTitleHeight(bool adjust) {mAutoAdjustTitleHeight = adjust;};
    void setAutoAdjustSubTitleHeight(bool adjust) {mAutoAdjustSubTitleHeight = adjust;};
    bool autoAdjustTitleHeight() {return mAutoAdjustTitleHeight;};
    inline bool autoAdjustSubTitleHeight()const
    {
        return mAutoAdjustSubTitleHeight;
    };

    qreal height();

    inline bool isTitle() const
    {
        return !mTitle.text().isEmpty();
    };
    inline bool withTitle() const
    {
        return !mSubTitle.text().isEmpty();
    };
};



#endif
