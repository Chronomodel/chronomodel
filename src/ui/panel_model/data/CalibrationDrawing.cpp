#include "CalibrationDrawing.h"
#include "GraphViewRefAbstract.h"
#include "GraphView.h"
#include "Marker.h"

#include <QMouseEvent>

CalibrationDrawing::CalibrationDrawing(QWidget *parent) : QWidget(parent),
    mRefGraphView (nullptr),
    mCalibGraph (nullptr),
    mVerticalSpacer (5),
    mFont (parent->font())

{
    mTitle = new QLabel(this);
    setMouseTracking(true);
    mRefTitle = new QLabel(this);
    mRefComment = new QLabel(this);
  //  mRefGraphView = new GraphViewRefAbstract(this);

    mCalibTitle = new QLabel(this);
    mCalibComment = new QLabel(this);
  //  mCalibGraph = new GraphView(this);

    mMarkerX = new Marker(this);
    mMarkerY = new Marker(this);

    setMouseTracking(true);
}

CalibrationDrawing::~CalibrationDrawing()
{
    if (mCalibGraph)
        delete mCalibGraph;
}
void CalibrationDrawing::hideMarker()
{
    mMarkerX->hideMarker();
    mMarkerY->hideMarker();
}

void CalibrationDrawing::showMarker()
{
    mMarkerX->resize( mMarkerX->thickness(), height());
    mMarkerY->resize( width(), mMarkerY->thickness());
    mMarkerX->showMarker();
    mMarkerY->showMarker();
    mMarkerX->raise();
    mMarkerY->raise();
}

void CalibrationDrawing::setRefGraph(GraphViewRefAbstract* refGraph)
{
      mRefGraphView = refGraph;
      if (mRefGraphView) {
        mRefGraphView->setParent(this);
        mRefGraphView->setMouseTracking(true);
      }
}

void CalibrationDrawing::setCalibGraph(GraphView* calibGraph)
{
    mCalibGraph = calibGraph;
    mCalibGraph->setParent(this);
    mCalibGraph->setMouseTracking(true);
    mCalibGraph->setTipXLab("t");
}

void CalibrationDrawing::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    // drawing a background under curve
    QPainter p(this);
    p.fillRect(rect(), Qt::white);
    updateLayout();
}

void CalibrationDrawing::updateLayout()
{
    if (width()<0 || height()<0)
        return;

    const QFontMetrics fm (mFont);
    if (!mCalibGraph)
        return;

    if (!mCalibGraph->hasCurve()) {
        QLabel noCalib ("No Calibration", this);
        noCalib.setGeometry(0, mVerticalSpacer, fm.width(noCalib.text()), fm.height());
        return;
    }

    const type_data max = mCalibGraph->maximumX();
    const int marginRight = (int) floor(fm.width(locale().toString(max))/2);

    QFont topFont(mFont);
    topFont.setBold(true);
    topFont.setPointSizeF(16.);
    QFontMetrics fmTop(topFont);
    mTitle->setFont(topFont);

    QFont titleFont(mFont);
    titleFont.setBold(true);
    titleFont.setPointSizeF(12.);
    QFontMetrics fmTitle(titleFont);

    mRefTitle->setFont(titleFont);
    mCalibTitle->setFont(titleFont);

    mRefComment->setFont(mFont);
    mCalibComment->setFont(mFont);

    const int totalGraph = height() - (fmTop.height() + 2*fmTitle.height() + 2*fm.height() + 7 * mVerticalSpacer);
    const int refH = totalGraph * 2/3; // it's a divide by integer, The direction of the operation is important
    const int calibH = totalGraph - refH ;

    const int titlePosition = ((width() - fmTop.width(mTitle->text()) - 6) /2) + 3;
    mTitle->setGeometry(titlePosition, mVerticalSpacer, fmTop.width(mTitle->text()) + 3, fmTop.height());

    mRefTitle->setGeometry(20,  mTitle->y() + mTitle->height() + mVerticalSpacer, fmTitle.width(mRefTitle->text()), fmTitle.height());
    mRefComment->setGeometry(30,  mRefTitle->y() + mRefTitle->height() + mVerticalSpacer, fm.width(mRefComment->text()), fm.height());

    if (mRefGraphView) {
        mRefGraphView->setGeometry(0, mRefComment->y() + mRefComment->height() + mVerticalSpacer, width(), refH);
        mRefGraphView->setMarginRight(marginRight);

        mCalibTitle->setGeometry(20,  mRefGraphView->y() + mRefGraphView->height() + mVerticalSpacer, fmTitle.width(mCalibTitle->text()), fmTitle.height());

    } else
        mCalibTitle->setGeometry(20,  mTitle->y() + mTitle->height() + mVerticalSpacer + refH, fmTitle.width(mCalibTitle->text()), fmTitle.height());

    mCalibComment->setGeometry(30,  mCalibTitle->y() + mCalibTitle->height() + mVerticalSpacer, fm.width(mCalibComment->text()), fm.height());

    mCalibGraph->setGeometry(0, mCalibComment->y() + mCalibComment->height() + mVerticalSpacer, width(), calibH);
    mCalibGraph->setMarginRight(marginRight);

    if (mMouseOverCurve) {
        showMarker();
        mMarkerX->resize( mMarkerX->thickness(), refH + calibH + 3*mVerticalSpacer + fmTitle.height() + fm.height());
        mMarkerY->resize( width() - 2*marginRight, mMarkerY->thickness());
    } else
        hideMarker();

}

void CalibrationDrawing::mouseMoveEvent(QMouseEvent* e)
{
    const int x ( qBound(0, e->pos().x(), width()) );

    const int y ( qBound(0, e->pos().y(), height()) );

    // draw the red cross lines
    if (( mRefGraphView && mRefGraphView->geometry().contains(x, y))
            || (mCalibGraph && mCalibGraph->geometry().contains(x, y))) {

        mMouseOverCurve = true;

    } else
        mMouseOverCurve = false;


    mMarkerX->move(x, mRefComment->y() + mRefComment->height() + mVerticalSpacer);
    mMarkerY->move(0, y);


    /*
     *  The tip was set by "mCalibGraph->setTipXLab("t")" within CalibrationView
    */

    update();
}

void CalibrationDrawing::setVisible(bool visible)
{
    mTitle->setVisible(visible);
    mRefTitle->setVisible(visible);
    mRefComment->setVisible(visible);
    if (mRefGraphView)
        mRefGraphView->setVisible(visible);

    mCalibTitle->setVisible(visible);
    mCalibComment->setVisible(visible);
    if (mCalibGraph)
        mCalibGraph->setVisible(visible);

    if (visible)
        showMarker();
    else
        hideMarker();

    QWidget::setVisible(visible);
}

void CalibrationDrawing::resizeEvent(QResizeEvent* e)
{
    (void)e;

    updateLayout();
}
