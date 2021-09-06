/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "MultiCalibrationDrawing.h"
#include "AppSettings.h"
#include "GraphViewResults.h"

#include <QMouseEvent>

int ColoredBar::mWidth = 15;

ColoredBar::ColoredBar(QWidget *parent) : QWidget(parent),
mColor (Qt::green)
{

}

ColoredBar::~ColoredBar()
{
}

void ColoredBar::paintEvent(QPaintEvent *)
{
    QPainter p (this);
    p.fillRect(rect(), mColor);
}


MultiCalibrationDrawing::MultiCalibrationDrawing(QWidget *parent) : QWidget(parent),
mVerticalSpacer (5),
mGraphHeight (GraphViewResults::mHeightForVisibleAxis),
mGraphFont (font()),
mMouseOverCurve (true)
{
   setMouseTracking(true);
   mMarkerX = new Marker(this);
   mMarkerX->showMarker();

   mScrollArea = new QScrollArea(this);
   mScrollArea->setMouseTracking(true);
   mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

   mGraphWidget = new QWidget(this);
   mGraphWidget->setMouseTracking(true);

   mScrollArea->setWidget(mGraphWidget);

}

MultiCalibrationDrawing::~MultiCalibrationDrawing()
{
}


void MultiCalibrationDrawing::setGraphList(QList<GraphView*> &list)
{
    if (!mListCalibGraph.isEmpty()) {
        for(auto&& graph : mListCalibGraph)
            delete graph;
    }
    mListCalibGraph.clear();

    for (auto&& graph : list)
        graph->setParent(mGraphWidget);

    mListCalibGraph = list;

    mScrollArea->updateGeometry();
    mGraphWidget->updateGeometry();
    updateLayout();

}

void MultiCalibrationDrawing::setEventsColorList(QList<QColor> &colorList)
{
    mListEventsColor = colorList;
    for (auto&& panel : mListBar)
        delete panel;

    mListBar.clear();
    for (auto&& color : mListEventsColor) {
        ColoredBar* panel = new ColoredBar (mGraphWidget);
        panel->setColor(color);
        mListBar.append(panel);
    }

}

void MultiCalibrationDrawing::hideMarker()
{
   mMarkerX->hideMarker();;
   mMouseOverCurve = false;
   update();
}

void MultiCalibrationDrawing::showMarker()
{
    mMouseOverCurve = true;
    mMarkerX->showMarker();
    mMarkerX->resize( mMarkerX->thickness(), height());

    mMarkerX->raise();
    update();
}


void MultiCalibrationDrawing::mouseMoveEvent(QMouseEvent* e)
{
    const int x ( qBound(0, e->pos().x(), width()) );
    mMarkerX->move(x, 0);

    /*
     *  The tip was set by "mCalibGraph->setTipXLab("t")" within CalibrationView
    */
}

/*void MultiCalibrationDrawing::paintEvent(QPaintEvent * e)
{
    updateLayout();
    (void) e;
}
*/
void MultiCalibrationDrawing::updateLayout()
{
    if (mListBar.isEmpty())
        return;
    QFontMetrics fm (font());
    const bool axisVisible = (mGraphHeight >= GraphViewResults::mHeightForVisibleAxis);
    const int marginBottom = (axisVisible ? int (fm.ascent() * 2.2) : int (fm.ascent() * 0.5));
    int y = 0;
    int i = 0;
    const int graphShift = 5; // the same name and the same value as MultiCalibrationView::exportFullImage()
qDebug()<<"MultiCalibrationDrawing::updateLayout()";

    for (auto&& graph: mListCalibGraph) {
        if (mListAxisVisible.at(i))
            mListBar[i]->setGeometry(5, y, ColoredBar::mWidth, mGraphHeight - marginBottom);
        else
            mListBar[i]->setGeometry(5, y, ColoredBar::mWidth, mGraphHeight);

        mListBar[i]->setVisible(true);

        graph->showXAxisValues(axisVisible && mListAxisVisible[i]);

        if (!graph->hasCurve()) {
            graph->showInfos(true);
            graph->setNothingMessage( graph->getInfo(' ')  + " -> Not computable" );
            graph->setGeometry(ColoredBar::mWidth + graphShift, y, width() - ColoredBar::mWidth - graphShift, mGraphHeight );
            graph->setVisible(true);

         } else {           
            graph->showXAxisSubTicks(true);
            graph->setMarginBottom(marginBottom);
            graph->setYAxisMode(GraphView::eHidden);
            graph->showYAxisLine(false);
            // usefull for bound, because there is no curve named "Calibration"
            if (graph->getCurve("Calibration"))
                graph->setOverArrow(GraphView::eBothOverflow);

            else
                graph->setOverArrow(GraphView::eNone);

            graph->setFont(font());
            graph->setTipXLab("t");
            graph->setGeometry(ColoredBar::mWidth + graphShift, y, width() - ColoredBar::mWidth - graphShift, mGraphHeight );
            graph->setVisible(true);
         }
         y += mGraphHeight;
         ++i;
    }

    mGraphWidget->resize(width(), y); // Don't use setGeometry() only resize()

    if (mMouseOverCurve)
        mMarkerX->resize( mMarkerX->thickness(), y);
    else
        hideMarker();
  //  update();
}

void MultiCalibrationDrawing::resizeEvent(QResizeEvent* e)
{
    (void) e;

    mScrollArea->setGeometry(0, 0, width(), height());
   // updateLayout();
}

QPixmap MultiCalibrationDrawing::grab()
{
    return mGraphWidget->grab();
}


void MultiCalibrationDrawing::setGraphHeight(const int & height)
{
    mGraphHeight = height;
    updateLayout();
}


void MultiCalibrationDrawing::forceRefresh()
{
    const bool axisVisible = (mGraphHeight >= GraphViewResults::mHeightForVisibleAxis);
    const int marginBottom =(axisVisible ? int (fontMetrics().ascent() * 2.2) : int (fontMetrics().ascent() * 0.5));

    int y = 0;
    int i = 0;

    for (auto&& graph: mListCalibGraph) {
        mListBar[i]->setGeometry(5, y, ColoredBar::mWidth, mGraphHeight - marginBottom);

         if (!graph->hasCurve()) {
            QLabel noCalib (tr("No Calibration"), this);
            noCalib.setGeometry(ColoredBar::mWidth +5, y, width() - ColoredBar::mWidth - fontMetrics().boundingRect(noCalib.text()).width(), mGraphHeight);

         } else {
             graph->showXAxisValues(axisVisible);
             graph->forceRefresh();
         }
         y += mGraphHeight;
         ++i;
    }
}
