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

#include "PluginUniformRefView.h"
#if USE_PLUGIN_UNIFORM

#include "GraphView.h"

#include <QtWidgets>

PluginUniformRefView::PluginUniformRefView(QWidget* parent):GraphViewRefAbstract(parent)
{
    mMeasureColor = QColor(98, 113, 123, 240);
    mGraph = new GraphView(this);

    mGraph->setXAxisSupport(AxisTool::AxisSupport::eMin_Max);
    mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

    mGraph->setXAxisMode(GraphView::AxisMode::eAllTicks);
    mGraph->setYAxisMode(GraphView::AxisMode::eAllTicks);
    mGraph->setTipXLab("t");
    mGraph->setTipYLab("age");
    mGraph->autoAdjustYScale(true);

}

PluginUniformRefView::~PluginUniformRefView()
{

}

void PluginUniformRefView::setDate(const Date& date, const StudyPeriodSettings& settings)
{
    GraphViewRefAbstract::setDate(date, settings);
    
    if (date.mOrigin == Date::eSingleDate) { // not a combination
        // nothing to do

        // Y scale and RangeY are define in graphView::zommX()

    } else {
        
        
        double tminDisplay (mTminDisplay);
        double tmaxDisplay (mTmaxDisplay);

        const double t1 = DateUtils::convertToAppSettingsFormat(mTminDisplay);
        const double t2 = DateUtils::convertToAppSettingsFormat(mTmaxDisplay);

        for (auto&& d : date.mSubDates ) {
            Date sd (d.toObject());

            if (!sd.isNull() && sd.mIsValid) {
                const double t3 = sd.getFormatedTminCalib();
                const double t4 = sd.getFormatedTmaxCalib();
    
                tminDisplay = qMin(t1, qMin(t2,t3));
                tmaxDisplay = qMax(t1, qMax(t2,t4));
            } else {
                tminDisplay = qMin(t1, t2);
                tmaxDisplay = qMax(t1, t2);
            }
        }
        GraphViewRefAbstract::drawSubDates(date.mSubDates, settings, tminDisplay, tmaxDisplay);
    }
    
}

void PluginUniformRefView::zoomX(const double min, const double max)
{
    mGraph->zoomX(min, max);
}

void PluginUniformRefView::setMarginRight(const int margin)
{
    mGraph->setMarginRight(margin);
}

void PluginUniformRefView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    mGraph->setGeometry(0, 0, width(), height());
}

#endif
