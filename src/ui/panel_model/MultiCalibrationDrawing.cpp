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
        for(auto graph : mListCalibGraph)
            delete graph;
    }
    mListCalibGraph.clear();

    for (auto &&graph : list)
        graph->setParent(mGraphWidget);

    mListCalibGraph = list;

    mScrollArea->updateGeometry();
    mGraphWidget->updateGeometry();
    updateLayout();

}

void MultiCalibrationDrawing::setEventsColorList(QList<QColor> &colorList)
{
    mListEventsColor = colorList;
    for (auto && panel : mListBar)
        delete panel;

    mListBar.clear();
    for (auto && color : mListEventsColor) {
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


void MultiCalibrationDrawing::updateLayout()
{
    const bool axisVisible = (mGraphHeight >= GraphViewResults::mHeightForVisibleAxis);
    const int marginBottom = (axisVisible ? int (font().pointSize() * 2.2) : int (font().pointSize() * 0.5));
    int y (0);
    int i (0);
    const int graphShift (5); // the same name and the same value as MultiCalibrationView::exportFullImage()

    for (GraphView *graph: mListCalibGraph) {
        mListBar[i]->setGeometry(5, y, ColoredBar::mWidth, mGraphHeight - marginBottom);
        mListBar[i]->setVisible(true);

        if (!graph->hasCurve()) {
            graph->showInfos(true);
            graph->setNothingMessage( graph->getInfo(' ')  + " -> Not computable" );
            graph->setGeometry(ColoredBar::mWidth + graphShift, y, width() - ColoredBar::mWidth - graphShift, mGraphHeight );
            graph->setVisible(true);

         } else {
            graph->showXAxisValues(axisVisible);
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
    update();
}

void MultiCalibrationDrawing::resizeEvent(QResizeEvent* e)
{
    (void) e;
    mScrollArea->setGeometry(0, 0, width(), height());
    updateLayout();
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
    const QFontMetrics fm (font());

    const bool axisVisible = (mGraphHeight >= GraphViewResults::mHeightForVisibleAxis);
    const int marginBottom =(axisVisible ? int (font().pointSize() * 2.2) : 10);

    int y (0);
    int i (0);

    for (GraphView *graph: mListCalibGraph) {
        mListBar[i]->setGeometry(5, y, ColoredBar::mWidth, mGraphHeight - marginBottom);

         if (!graph->hasCurve()) {
            QLabel noCalib (tr("No Calibration"), this);
            noCalib.setGeometry(ColoredBar::mWidth +5, y, width() - ColoredBar::mWidth - fm.width(noCalib.text()), mGraphHeight);

         } else {
             graph->showXAxisValues(axisVisible);
             graph->forceRefresh();
         }
         y += mGraphHeight;
         ++i;
    }
}

