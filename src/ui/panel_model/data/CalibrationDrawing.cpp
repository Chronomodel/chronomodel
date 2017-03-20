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

    mRefTitle = new QLabel(this);
    mRefComment = new QLabel(this);

    mCalibTitle = new QLabel(this);
    mCalibComment = new QLabel(this);

    mMarkerX = new Marker(this);
    mMarkerY = new Marker(this);

    setMouseTracking(true);
}

CalibrationDrawing::~CalibrationDrawing()
{
    if (mRefGraphView)
        delete mRefGraphView;
    if (mCalibGraph)
        delete mCalibGraph;
}
void CalibrationDrawing::hideMarker()
{
    mMarkerX->hide();
    mMarkerY->hide();
}

void CalibrationDrawing::showMarker()
{
    mMarkerX->show();
    mMarkerY->show();
}

void CalibrationDrawing::addRefGraph(GraphViewRefAbstract* refGraph)
{
    mRefGraphView = refGraph;
    mRefGraphView->setParent(this);
    mRefGraphView->setVisible(true);
}

void CalibrationDrawing::addCalibGraph(GraphView* calibGraph)
{
    mCalibGraph = calibGraph;
    mCalibGraph->setParent(this);
    mCalibGraph->setVisible(true);
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
    const QFontMetrics fm (mFont);

    if (!mCalibGraph || !mCalibGraph->hasCurve()) {
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

    mMarkerX->resize( mMarkerX->thickness(), refH + calibH + 3*mVerticalSpacer + fmTitle.height() + fm.height());
    mMarkerY->resize( width() - 2*marginRight, mMarkerY->thickness());

}

void CalibrationDrawing::mouseMoveEvent(QMouseEvent* e)
{
    int x = e->pos().x();
    x = (x < 0) ? 0 : x;
    x = (x > width()) ? width() : x;

    int y = e->pos().y();
    y = (y < 0) ? 0 : y;
    y = (y > height()) ? height() : y;
    // draw the red cross lines
    mMarkerX->raise();
    mMarkerY->raise();
    mMarkerX->move(x, mRefGraphView->y());
    mMarkerY->move(mCalibGraph->marginRight(), y);
    /*
     *  The tip was set by "mCalibGraph->setTipXLab("t")" within CalibrationView
    */
}

void CalibrationDrawing::resizeEvent(QResizeEvent* e)
{
    updateLayout();
}
