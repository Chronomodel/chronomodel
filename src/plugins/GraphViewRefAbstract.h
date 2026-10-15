/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef GRAPHVIEWREFABSTRACT_H
#define GRAPHVIEWREFABSTRACT_H

#include "StudyPeriodSettings.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "Date.h"
#include "CalibrationCurve.h"
#include "MainWindow.h"
#include "Project.h"
#include "Painting.h"
#include "GraphView.h"

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


    virtual void setDate(const Date& date, const StudyPeriodSettings& settings)
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
    
    void drawSubDates(const QJsonArray& subDates, const StudyPeriodSettings& settings, const double tminDisplay, const double tmaxDisplay)
    {
        mSettings = settings;
        mGraph->removeAllCurves();
        
        for (int i= 0; i < subDates.size(); ++i) {
            QJsonObject subDate = subDates.at(i).toObject();
            Date sd (subDate);

            GraphCurve gCurve;
            gCurve.mVisible = true;
            gCurve.mName = sd.getQStringName();

            QColor curveColor( i<210 ? Painting::chainColors[i] : randomColor());

            gCurve.mPen.setColor(Painting::mainColorDark);
            
            curveColor.setAlpha(100);
            gCurve.mBrush = QBrush(curveColor);

            //gCurve.mIsVertical = false;
            //gCurve.mIsHisto = false;
            gCurve.mIsRectFromZero = true;
            
            const auto &project = MainWindow::getInstance()->getProject();

            std::string toFind;
            if (sd.mDeltaType == Date::eDeltaNone) {
                toFind = sd.mUUID;

            } else {
                toFind = "WID::" + sd.mUUID;
            }

            std::map<std::string, CalibrationCurve>::iterator it = project->mCalibCurves.find(toFind);

            if ( it != project->mCalibCurves.end())
                sd.mCalibration = & it->second;

            std::map<double, double> calib;
            if (sd.mDeltaType != Date::eDeltaNone) {
                calib = normalize_map(getMapDataInRange(sd.getFormatedWiggleCalibMap(), tminDisplay, tmaxDisplay));

            } else {
                calib = normalize_map(getMapDataInRange(sd.getFormatedCalibMap(), tminDisplay, tmaxDisplay));
            }

            gCurve.mData = QMap(calib);
            gCurve.mType = GraphCurve::CurveType::eDensityData;
            mGraph->add_curve(gCurve);
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
    StudyPeriodSettings mSettings;
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
