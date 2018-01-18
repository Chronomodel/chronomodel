#include "MultiCalibrationDrawing.h"
#include <QMouseEvent>

ColoredPanel::ColoredPanel(QWidget *parent) : QWidget(parent),
mColor (Qt::green)
{

}

ColoredPanel::~ColoredPanel()
{
}

void ColoredPanel::paintEvent(QPaintEvent *)
{
    QPainter p (this);
    p.fillRect(rect(), mColor);
}


MultiCalibrationDrawing::MultiCalibrationDrawing(QWidget *parent) : QWidget(parent),
mVerticalSpacer (5),
mFont (parent->font()),
mGraphHeight (70),
mMouseOverCurve (true)
{
   setMouseTracking(true);
   mMarkerX = new Marker(this);
   mMarkerX->showMarker();

   mScrollArea = new QScrollArea(this);
   mScrollArea->setMouseTracking(true);
   mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

   mGraphWidget = new QWidget();
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
    for (auto && panel : mListPanel)
        delete panel;

    mListPanel.clear();
    for (auto && color : mListEventsColor) {
        ColoredPanel* panel = new ColoredPanel (mGraphWidget);
        panel->setColor(color);
        mListPanel.append(panel);
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
    const int panelWidth (15);
    const QFontMetrics fm (font());

    const bool axisVisible = (mGraphHeight >= 100.);
    const qreal marginBottom =(axisVisible ? font().pointSizeF() + 10. : 10.);
    int y (0);
    int i (0);

    for (GraphView *graph: mListCalibGraph) {
        mListPanel[i]->setGeometry(5, y, panelWidth, mGraphHeight - marginBottom);
        mListPanel[i]->setVisible(true);

        if (!graph->hasCurve()) {
             graph->showInfos(true);
            graph->setNothingMessage( graph->getInfo(' ')  + " -> Not computable" );
            graph->setGeometry(panelWidth + 5, y, width() - panelWidth, mGraphHeight );
            graph->setVisible(true);

         } else {
            graph->showXAxisValues(axisVisible);
            graph->showXAxisSubTicks(axisVisible);
            graph->setMarginBottom(marginBottom);
            graph->setYAxisMode(GraphView::eHidden);
            graph->showYAxisLine(false);
            // usefull for bound, because there is no curve named "Calibration"
            if (graph->getCurve("Calibration"))
                graph->setOverArrow(GraphView::eBothOverflow);
            else
                graph->setOverArrow(GraphView::eNone);

            graph->setFont(font());
            graph->setTipXLab(tr("t="));
            graph->setGeometry(panelWidth + 5, y, width() - panelWidth, mGraphHeight );
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
    const int panelWidth (15);
    const QFontMetrics fm (font());

    const bool axisVisible = (mGraphHeight >= 100.);
    const qreal marginBottom =(axisVisible ? font().pointSizeF() + 10. : 10.);
    int y (0);
    int i (0);

    for (GraphView *graph: mListCalibGraph) {
        mListPanel[i]->setGeometry(5, y, panelWidth, mGraphHeight - marginBottom);
         if (!graph->hasCurve()) {
            QLabel noCalib (tr("No Calibration"), this);
            noCalib.setGeometry(panelWidth +5, y, width() - panelWidth - fm.width(noCalib.text()), mGraphHeight);

         } else {
             graph->forceRefresh();
         }
         y += mGraphHeight;
         ++i;
    }
}

