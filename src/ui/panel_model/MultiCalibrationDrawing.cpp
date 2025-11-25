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

#include "MultiCalibrationDrawing.h"
#include "GraphView.h"

#include <QMouseEvent>

int ColoredBar::mWidth = 15;

ColoredBar::ColoredBar(QWidget *parent) :
    QWidget(parent),
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


MultiCalibrationDrawing::MultiCalibrationDrawing(QWidget* parent):
    QWidget(parent),
    mVerticalSpacer (5),
    mGraphHeight (100),
    mHeightForVisibleAxis (100),
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
    for (auto&& g : mListCalibGraph) {
        delete g;
        g = nullptr;
    }
    for (auto&& panel : mListBar) {
        delete panel;
        panel = nullptr;
    }
}


void MultiCalibrationDrawing::setGraphList(QList<GraphViewAbstract*> &list)
{
    if (!mListCalibGraph.isEmpty()) {
        for(auto&& graph : mListCalibGraph) {
            delete graph;
            graph = nullptr;
        }
    }
    mListCalibGraph.clear();

    for (auto&& graph : list)
        graph->setParent(mGraphWidget);

    mListCalibGraph = list;

    mScrollArea->updateGeometry();
    mGraphWidget->updateGeometry();

}

void MultiCalibrationDrawing::setEventsColorList(QList<QColor> &colorList)
{
    mListEventsColor = colorList;
    for (auto&& panel : mListBar) {
        delete panel;
        panel = nullptr;
    }

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
    const int x ( std::clamp(e->pos().x(), 0, width()) );
    mMarkerX->move(x, 0);

    /*
     *  The tip was set by "mCalibGraph->setTipXLab("t")" within CalibrationView
    */
}

/*
void MultiCalibrationDrawing::updateLayout()
{
    const bool withBar = !mListEventsColor.isEmpty();
    const decltype(ColoredBar::mWidth) barWidth = withBar ? ColoredBar::mWidth : 0;

    QFontMetrics fm (font());
    const bool axisVisible = (mGraphHeight >= mHeightForVisibleAxis);

    int ay = 0;
    int graphNo = 0;
    int barNo = 0;
    constexpr int graphShift = 5; // the same name and the same value as MultiCalibrationView::exportFullImage()

    const qreal ax = withBar ? barWidth + graphShift : graphShift;
    const qreal aw = std::max(0.0, width() - ax);

    qreal ah = mGraphHeight;
std::cout<< "new layout ay=" << ay << std::endl;
    for (auto&& graphAbstract: mListCalibGraph) {

        GraphView* graph = dynamic_cast<GraphView*>(graphAbstract);
        GraphTitle* graphTitle = dynamic_cast<GraphTitle*>(graphAbstract);

        // --- Ajustement de la barre : hauteur différente selon axe visible ---
        if (withBar && barNo < mListBar.size()) {
            int barHeight = withAxis ? ah - marginBottom : ah;
            barHeight = std::max(0, barHeight);

            const QRect newGeom(graphShift, ay, barWidth, barHeight);
            if (mListBar[barNo]->geometry() != newGeom)
                mListBar[barNo]->setGeometry(newGeom);

            if (!mListBar[barNo]->isVisible())
                mListBar[barNo]->setVisible(true);

            ++barNo;
        }

        if (graphTitle) {

            graphTitle->setGeometry(ax, ay, aw, graphTitle->height() );
            graphTitle->setVisible(true);

            ay += graphTitle->height();// + (graphTitle->isTitle()? 2: 0);
            std::cout<< "add title ay=" << ay << std::endl;

        } else if (graph) {
           // newTitle = true;
            const bool withAxis = axisVisible && mListAxisVisible[graphNo];
            const int marginBottom = withAxis ? int (fm.ascent() * 2.2) : int (fm.height()/2);
//withAxis ? mGraphHeight - marginBottom : mGraphHeight;


            graph->showXAxisValues(axisVisible && mListAxisVisible[graphNo]);

            if (!graph->has_curves() && !graph->has_points()) {
                graph->showInfos(true);
                graph->setNothingMessage( graph->getInfo(' ')  + " -> Not computable" );
                //graph->setGeometry(ax, ay, aw , ah);
                //graph->setVisible(true);

            } else {
                graph->showXAxisSubTicks(true);

                // usefull for bound, because there is no curve named "Calibration"
                if (graph->getCurve("Calibration"))
                    graph->setOverArrow(GraphView::eBothOverflow);

                else
                    graph->setOverArrow(GraphView::eNone);

                graph->setFont(font());
                graph->setTipXLab("t");
                graph->setMarginBottom(marginBottom);

               // graph->setGeometry(ax, ay, aw, ah );
               // graph->setGeometry(barWidth + graphShift, y, std::max(0, width() - barWidth - 2*graphShift), mGraphHeight );
               // graph->setVisible(true);
            }

            const QRect graphGeom(ax, ay, aw, ah);
            if (graph->geometry() != graphGeom)
                graph->setGeometry(graphGeom);

            if (!graph->isVisible())
                graph->setVisible(true);


            ay += ah;//mGraphHeight;
            std::cout<< "add graph ay=" << ay << std::endl;
            ++graphNo;
        }

    }

    mGraphWidget->resize(width(), ay); // Don't use setGeometry() only resize()

    if (mMouseOverCurve)
        mMarkerX->resize( mMarkerX->thickness(), ay);
    else
        hideMarker();

    forceRefresh();
}
*/
void MultiCalibrationDrawing::updateLayout()
{
    const bool withBar = !mListEventsColor.isEmpty();
    const int barWidth = withBar ? ColoredBar::mWidth : 0;

    QFontMetrics fm(font());
    const bool axisVisible = (mGraphHeight >= mHeightForVisibleAxis);

    int ay = 0;
    int graphNo = 0;
    int barNo = 0;
    constexpr int graphShift = 5; // same as MultiCalibrationView::exportFullImage()

    const int ax = withBar ? barWidth + graphShift : graphShift;
    const int aw = std::max(0, width() - ax);

    for (auto&& graphAbstract : mListCalibGraph) {

        if (auto* graphTitle = dynamic_cast<GraphTitle*>(graphAbstract)) {
            // --- Titre ---
            if (withBar && barNo < mListBar.size()) {
                mListBar[barNo]->setGeometry(graphShift, ay, barWidth, graphTitle->height());
                // ne pas rappeler setVisible si déjà visible
                if (!mListBar[barNo]->isVisible())
                    mListBar[barNo]->setVisible(true);

                ++barNo;
            }

            graphTitle->setGeometry(ax, ay, aw, graphTitle->height());
            ay += graphTitle->height();
        }

        else if (auto* graph = dynamic_cast<GraphView*>(graphAbstract)) {
            // --- Graphe ---
            const bool withAxis = axisVisible && mListAxisVisible[graphNo];
            const int marginBottom = withAxis ? int(fm.ascent() * 2.2) : 0;
            const int ah = mGraphHeight;

            // --- Ajustement de la barre : hauteur différente selon axe visible ---
            if (withBar && barNo < mListBar.size()) {
                int barHeight = withAxis ? ah - marginBottom : ah;
                barHeight = std::max(0, barHeight);

                const QRect newGeom(graphShift, ay, barWidth, barHeight);
                if (mListBar[barNo]->geometry() != newGeom)
                    mListBar[barNo]->setGeometry(newGeom);

                if (!mListBar[barNo]->isVisible())
                    mListBar[barNo]->setVisible(true);

                ++barNo;
            }

            // --- Configuration du graphe ---
            graph->showXAxisValues(withAxis);

            if (!graph->has_curves() && !graph->has_points()) {
                graph->showInfos(true);
                graph->setNothingMessage(graph->getInfo(' ') + " -> Not computable");
            } else {
                graph->showXAxisSubTicks(true);

                if (graph->getCurve("Calibration"))
                    graph->setOverArrow(GraphView::eBothOverflow);
                else
                    graph->setOverArrow(GraphView::eNone);

                graph->setFont(font());
                graph->setTipXLab("t");
                graph->setMarginBottom(marginBottom);
            }

            const QRect graphGeom(ax, ay, aw, ah);
            if (graph->geometry() != graphGeom)
                graph->setGeometry(graphGeom);

            if (!graph->isVisible())
                graph->setVisible(true);
            ay += ah;
            ++graphNo;

        }
    }

    // --- Masquer les barres inutilisées ---
    for (int i = barNo; i < mListBar.size(); ++i) {
        if (mListBar[i]->isVisible())
            mListBar[i]->setVisible(false);
    }

    // --- Ajustement final du widget principal ---
    mGraphWidget->resize(width(), ay);

    if (mMouseOverCurve)
        mMarkerX->resize(mMarkerX->thickness(), ay);
    else
        hideMarker();

}

QList<GraphView*> MultiCalibrationDrawing::getGraphViewList() const
{
    QList<GraphView*> graphList;
    for (auto gr : mListCalibGraph) {
        GraphView* grView = dynamic_cast<GraphView*>(gr);
        if (grView)
            graphList.append(grView);
    }
    return graphList;
}

void MultiCalibrationDrawing::resizeEvent(QResizeEvent* e)
{
    (void) e;
    mScrollArea->setGeometry(0, 0, width()+2, height());
}

QPixmap MultiCalibrationDrawing::grab()
{
    return mGraphWidget->grab();
}


void MultiCalibrationDrawing::setGraphHeight(int height)
{
    mGraphHeight = height;
}


