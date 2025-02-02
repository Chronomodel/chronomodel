/* ---------------------------------------------------------------------
Copyright or © or Copr. CNRS	2014 - 2023

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

#include "CalibrationDrawing.h"
#include "GraphViewRefAbstract.h"
#include "GraphView.h"
#include "Marker.h"
#include "QtUtilities.h"

#include <QMouseEvent>

CalibrationDrawing::CalibrationDrawing(QWidget *parent) : QWidget(parent),
    mRefGraphView (nullptr),
    mCalibGraph (nullptr),
    mVerticalSpacer (5),
    mFont (parent->font())
{
    setMouseTracking(true);

    QPalette palette_BW;
    palette_BW.setColor(QPalette::Base, Qt::white);
    palette_BW.setColor(QPalette::Text, Qt::black);
    palette_BW.setColor(QPalette::Window, Qt::white);
    palette_BW.setColor(QPalette::WindowText, Qt::black);

    mTitle = new QLabel(this);
    mTitle->setPalette(palette_BW);

    mRefTitle = new QLabel(this);
    mRefTitle->setPalette(palette_BW);
    mRefComment = new QLabel(this);
    mRefComment->setPalette(palette_BW);

    mCalibTitle = new QLabel(this);
    mCalibTitle->setPalette(palette_BW);
    mCalibComment = new QLabel(this);
    mCalibComment->setPalette(palette_BW);

    mMarkerX = new Marker(this);
    mMarkerY = new Marker(this);
}

CalibrationDrawing::~CalibrationDrawing()
{
    if (mCalibGraph) {
        delete mCalibGraph;
        mCalibGraph = nullptr;
    }
}

void CalibrationDrawing::hideMarker()
{
   mMarkerX->hideMarker();
   mMarkerY->hideMarker();
   mMouseOverCurve = false;
   update();
}

void CalibrationDrawing::showMarker()
{
    mMouseOverCurve = true;
    mMarkerX->showMarker();
    mMarkerY->showMarker();
    mMarkerX->resize( mMarkerX->thickness(), height());
    mMarkerY->resize( width(), mMarkerY->thickness());

    mMarkerX->raise();
    mMarkerY->raise();
    update();
}

void CalibrationDrawing::setRefGraph(GraphViewRefAbstract* refGraph)
{
    Q_ASSERT(refGraph);
    if (refGraph) {
        mRefGraphView = refGraph;
        mRefGraphView->setParent(this);
        mRefGraphView->setMouseTracking(true);
        if (mRefGraphView->mGraph) {
            mRefGraphView->mGraph->setMouseTracking(true);
            mRefGraphView->mGraph->setTipXLab("t Ref");
            mRefGraphView->update();
        }
    }
    setMouseTracking(true);
}

void CalibrationDrawing::setCalibGraph(GraphView* calibGraph)
{
    mCalibGraph = calibGraph;
    mCalibGraph->setParent(this);
    mCalibGraph->setMouseTracking(true);
    mCalibGraph->setTipXLab("t Calib");
}

void CalibrationDrawing::paintEvent(QPaintEvent* e)
{
    (void) e;
    // drawing a background under curve
    QPainter p(this);
    p.fillRect(rect(), Qt::white);

}

void CalibrationDrawing::updateLayout()
{
    if (width()<0 || height()<0)
        return;
    mFont = font();
    const QFontMetrics fm (mFont);

    if (!mCalibGraph)
        return;

    if (!mCalibGraph->has_curves()) {
        QLabel noCalib ("No Calibration", this);
        noCalib.setGeometry(0, mVerticalSpacer, fm.horizontalAdvance(noCalib.text()), fm.height());
        return;
    }

    const type_data max = mCalibGraph->maximumX();
    const int marginRight = int (floor(fm.horizontalAdvance(stringForGraph(max))/2));


    QFont topFont(mFont);
    topFont.setBold(true);
    topFont.setPointSizeF(mFont.pointSizeF() + 2);

    QFontMetrics fmTop(topFont);
    mTitle->setFont(topFont);

    const int titlePosition = ((width() - fmTop.horizontalAdvance(mTitle->text()) - 6) /2) + 3;
    mTitle->setGeometry(titlePosition, mVerticalSpacer, fmTop.horizontalAdvance(mTitle->text()) + 3, fmTop.height());

    QFont titleFont(mFont);
    titleFont.setBold(true);
    //titleFont.setPointSizeF(12.);
    QFontMetrics fmTitle(titleFont);

    mRefTitle->setFont(titleFont);
    mRefTitle->setGeometry(20,  mTitle->y() + mTitle->height() + mVerticalSpacer, fmTitle.horizontalAdvance(mRefTitle->text()), fmTitle.height());

    mCalibTitle->setFont(titleFont);

    mRefComment->setFont(mFont);
    mCalibComment->setFont(mFont);

    const int totalGraph = height() - (fmTop.height() + 2*fmTitle.height() + 2*fm.height() + 7 * mVerticalSpacer);
    const int refH = totalGraph * 2/3; // it's a divide by integer, The direction of the operation is important
    const int calibH = totalGraph - refH ;

    mRefComment->setGeometry(30,  mRefTitle->y() + mRefTitle->height() + mVerticalSpacer, fm.horizontalAdvance(mRefComment->text()), fm.height());

    if (mRefGraphView) {
        /* must be before setGeometry, because setGeometry is connected to resize.
         *  So to update the curve we need marginRight parameter
         */
        mRefGraphView->setMarginRight(marginRight);
        mRefGraphView->setGeometry(0, mRefComment->y() + mRefComment->height() + mVerticalSpacer, width(), refH);

        if (mRefGraphView->mGraph) {
            mRefGraphView->mGraph->setFont(mFont);
            mRefGraphView->mGraph->setMouseTracking(true);
            mRefGraphView->mGraph->setTipXLab("t Ref");
            mRefGraphView->mGraph->setMarginBottom(fm .ascent() * 2.2);
        }

        mCalibTitle->setGeometry(20,  mRefGraphView->y() + mRefGraphView->height() + mVerticalSpacer, fmTitle.horizontalAdvance(mCalibTitle->text()), fmTitle.height());

    } else
        mCalibTitle->setGeometry(20,  mTitle->y() + mTitle->height() + mVerticalSpacer + refH, fmTitle.horizontalAdvance(mCalibTitle->text()), fmTitle.height());

    mCalibComment->setGeometry(30,  mCalibTitle->y() + mCalibTitle->height() + mVerticalSpacer, fm.horizontalAdvance(mCalibComment->text()), fm.height());

    mCalibGraph->setFont(mFont);
    mCalibGraph->setGeometry(0, mCalibComment->y() + mCalibComment->height() + mVerticalSpacer, width(), calibH);
    mCalibGraph->setMarginRight(marginRight);
    mCalibGraph->setMarginBottom(fm.ascent() * 2.2);
    mCalibGraph->setYAxisMode(GraphView::eMinMaxHidden);
   // mCalibGraph->autoAdjustYScale(true); //ici

    if (mMouseOverCurve) {
        showMarker();
        mMarkerX->resize( mMarkerX->thickness(), refH + calibH + 3*mVerticalSpacer + fmTitle.height() + fm.height());
        mMarkerY->resize( width() - 2*marginRight, mMarkerY->thickness());
    } else
        hideMarker();

}

void CalibrationDrawing::setMouseTracking(bool enable)
{
    QWidget::setMouseTracking(enable);
    if (mCalibGraph)
        mCalibGraph->setMouseTracking(enable);

    if (mRefGraphView)
        mRefGraphView->setMouseTracking(enable);

}

void CalibrationDrawing::mouseMoveEvent(QMouseEvent* e)
{
    const int x = std::clamp(e->pos().x(), 0, width());

    const int y = std::clamp(e->pos().y(), 0, height());

    // draw the red cross lines
    if (( mRefGraphView && mRefGraphView->geometry().contains(x, y))
            || (mCalibGraph && mCalibGraph->geometry().contains(x, y))) {

        mMouseOverCurve = true;

    } else
        mMouseOverCurve = false;


    mMarkerX->move(x, mRefComment->y() + mRefComment->height() + mVerticalSpacer);
    mMarkerY->resize(width(), mMarkerY->thickness());
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
    if (mRefGraphView && mRefGraphView->mGraph)
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
