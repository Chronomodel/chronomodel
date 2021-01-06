/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

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

#ifndef GRAPHVIEWREFABSTRACT_H
#define GRAPHVIEWREFABSTRACT_H

#include "ProjectSettings.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "Date.h"
#include "CalibrationCurve.h"
#include "MainWindow.h"
#include "Project.h"
#include "Painting.h"

#include <QWidget>

class GraphViewRefAbstract: public QWidget
{
    Q_OBJECT
public:
    explicit GraphViewRefAbstract(QWidget* parent = nullptr):QWidget(parent),
        mMeasureColor(56, 120, 50)
    {
        setMouseTracking(true);
    }
    explicit GraphViewRefAbstract(const GraphViewRefAbstract& graph, QWidget* parent = nullptr):QWidget(parent),
        mMeasureColor(graph.mMeasureColor)
    {
        setMouseTracking(true);
        mSettings = graph.mSettings;
        mFormatFuncX = graph.mFormatFuncX;

        mTminCalib = graph.mTminCalib;
        mTmaxCalib = graph.mTmaxCalib;

        mTminDisplay = graph.mTminDisplay;
        mTmaxDisplay = graph.mTmaxDisplay;

        mTminRef = graph.mTminRef;
        mTmaxRef = graph.mTmaxRef;

    }

    void copyFrom(const GraphViewRefAbstract& graph)
    {
        mMeasureColor = graph.mMeasureColor;
        setMouseTracking(true);
        mSettings = graph.mSettings;
        mFormatFuncX = graph.mFormatFuncX;

        mTminCalib = graph.mTminCalib;
        mTmaxCalib = graph.mTmaxCalib;

        mTminDisplay = graph.mTminDisplay;
        mTmaxDisplay = graph.mTmaxDisplay;

        mTminRef = graph.mTminRef;
        mTmaxRef = graph.mTmaxRef;

    }

    virtual ~GraphViewRefAbstract() {
        mGraph->removeAllCurves();
        mGraph=nullptr;
    }


    virtual void setDate(const Date& date, const ProjectSettings& settings)
    {
        mSettings = settings;

        if (date.mCalibration && date.mIsValid) {
            mTminCalib = date.mCalibration->mTmin;
            mTmaxCalib = date.mCalibration->mTmax;

            mTminDisplay = qMin(mTminCalib, double (mSettings.mTmin));
            mTmaxDisplay = qMax(mTmaxCalib, double (mSettings.mTmax));

        } else {
            mTminCalib = - HUGE_VAL;
            mTmaxCalib = + HUGE_VAL;

            mTminDisplay = double (mSettings.mTmin);
            mTmaxDisplay = double (mSettings.mTmax);
        }

        mTminRef = date.getTminRefCurve();
        mTmaxRef = date.getTmaxRefCurve();
    }
    
    void drawSubDates(const QJsonArray& subDates, const ProjectSettings& settings, const double tminDisplay, const double tmaxDisplay)
    {
        mSettings = settings;
        mGraph->removeAllCurves();
        
        for (int i(0); i< subDates.size(); ++i) {
            QJsonObject subDate = subDates.at(i).toObject();
            Date sd (subDate);

            GraphCurve gCurve;
            gCurve.mName = sd.mName;

            QColor curveColor( i<210 ? Painting::chainColors[i] : randomColor());

            gCurve.mPen.setColor(Painting::mainColorDark);
            
            curveColor.setAlpha(100);
            gCurve.mBrush = QBrush(curveColor);

            gCurve.mIsVertical = false;
            gCurve.mIsHisto = false;
            gCurve.mIsRectFromZero = true;
            
            Project* project = MainWindow::getInstance()->getProject();

            QString toFind;
            if (sd.mDeltaType != Date::eDeltaNone) {
                toFind = sd.mUUID;

            } else {
                toFind = "WID::" + sd.mUUID;
            }

            QMap<QString, CalibrationCurve>::iterator it = project->mCalibCurves.find (toFind);

            if ( it != project->mCalibCurves.end())
                sd.mCalibration = & it.value();

            QMap<double, double> calib;
            if (sd.mDeltaType != Date::eDeltaNone) {
                calib = normalize_map(getMapDataInRange(sd.getFormatedWiggleCalibMap(), tminDisplay, tmaxDisplay));

            } else {
                calib = normalize_map(getMapDataInRange(sd.getFormatedCalibMap(), tminDisplay, tmaxDisplay));
            }

            gCurve.mData = calib;
            
            mGraph->addCurve(gCurve);
            mGraph->setRangeX(tminDisplay, tmaxDisplay);
            mGraph->setCurrentX(tminDisplay, tmaxDisplay);
        }
    
       
        
    }
    void setFormatFunctX(DateConversion f)
    {
        mFormatFuncX = f;
    }

    void setMarginLeft(const qreal &aMarginLeft) { mGraph->setMarginLeft(aMarginLeft);}
    void setMarginRight(const qreal &aMarginRight) { mGraph->setMarginRight(aMarginRight);}

public slots:
    virtual void zoomX(const double min, const double max)
    {
        Q_UNUSED(min);
        Q_UNUSED(max);
    }
    virtual void setMarginRight(const int margin)
    {
        Q_UNUSED(margin);
    }
    void changeXScaleDivision (const double &major, const int & minor) {mGraph->changeXScaleDivision(major, minor);}

protected:
    ProjectSettings mSettings;
    QColor mMeasureColor;
    DateConversion mFormatFuncX;

    double mTminCalib;
    double mTmaxCalib;

    double mTminRef;
    double mTmaxRef;

    double mTminDisplay;
    double mTmaxDisplay;

public:
    GraphView* mGraph;
};

#endif
