#include "ResultsControls.h"
#include "Painting.h"
#include <QtWidgets>


ResultsControls::ResultsControls(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mMargin(5),
mLineH(15),
mButH(25),
mIsZoomInDown(false),
mIsZoomDefaultDown(false),
mIsZoomOutDown(false),
mIsInfosChecked(false),
mIsDatesHistoSelected(true),
mIsDatesCalibChecked(true),
mIsDatesTraceSelected(false),
mIsDatesVarianceSelected(false),
mIsEventsHistoSelected(true),
mIsEventsTraceSelected(false),
mIsPhasesAlphaChecked(false),
mIsPhasesBetaChecked(false),
mIsPhasesPredictChecked(true)
{
    
}

ResultsControls::~ResultsControls()
{
    
}

void ResultsControls::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void ResultsControls::updateLayout()
{
    int w = width();
    int m = mMargin;
    int zoomW = (w-4*m)/3;
    
    mZoomRect = QRectF(m, m, w, mButH);
    mZoomInRect = QRectF(mZoomRect.x(), mZoomRect.y(), zoomW, mZoomRect.height());
    mZoomDefaultRect = QRectF(mZoomRect.x() + m + zoomW, mZoomRect.y(), zoomW, mZoomRect.height());
    mZoomOutRect = QRectF(mZoomRect.x() + 2 * (m + zoomW), mZoomRect.y(), zoomW, mZoomRect.height());
    
    mInfosRect = QRectF(m, 2*m + mZoomRect.height(), w - 2*m, mButH);
    
    mDatesRect = QRectF(m, mInfosRect.y() + mInfosRect.height() + m, w - 2*m, 105);
    mEventsRect = QRectF(m, mDatesRect.y() + mDatesRect.height() + m, w - 2*m, 65);
    mPhasesRect = QRectF(m, mEventsRect.y() + mEventsRect.height() + m, w - 2*m, 85);
    mChainsRect = QRectF(m, mPhasesRect.y() + mPhasesRect.height() + m, w - 2*m, 200);
    
    mDatesHistoRect = QRectF(mDatesRect.x() + m, mDatesRect.y() + 20 + m, mDatesRect.width() - 2*m, mLineH);
    mDatesCalibRect = QRectF(mDatesRect.x() + 20 + m, mDatesRect.y() + 20 + mLineH + 2*m, mDatesRect.width() - 2*m - 20, mLineH);
    mDatesTraceRect = QRectF(mDatesRect.x() + m, mDatesRect.y() + 20 + 2*mLineH + 3*m, mDatesRect.width() - 2*m, mLineH);
    mDatesVarianceRect = QRectF(mDatesRect.x() + m, mDatesRect.y() + 20 + 3*mLineH + 4*m, mDatesRect.width() - 2*m, mLineH);
    
    mEventsHistoRect = QRectF(mEventsRect.x() + m, mEventsRect.y() + 20 + m, mEventsRect.width() - 2*m, mLineH);
    mEventsTraceRect = QRectF(mEventsRect.x() + m, mEventsRect.y() + 20 + 2*m + mLineH, mEventsRect.width() - 2*m, mLineH);
    
    mPhasesAlphaRect = QRectF(mPhasesRect.x() + m, mPhasesRect.y() + 20 + m, mPhasesRect.width() - 2*m, mLineH);
    mPhasesBetaRect = QRectF(mPhasesRect.x() + m, mPhasesRect.y() + 20 + 2*m + mLineH, mPhasesRect.width() - 2*m, mLineH);
    mPhasesPredictRect = QRectF(mPhasesRect.x() + m, mPhasesRect.y() + 20 + 3*m + 2*mLineH, mPhasesRect.width() - 2*m, mLineH);
    
    update();
}

void ResultsControls::mousePressEvent(QMouseEvent* e)
{
    if(mZoomInRect.contains(e->pos()))
    {
        mIsZoomInDown = true;
    }
    else if(mZoomDefaultRect.contains(e->pos()))
    {
        mIsZoomDefaultDown = true;
    }
    else if(mZoomOutRect.contains(e->pos()))
    {
        mIsZoomOutDown = true;
    }
    else if(mInfosRect.contains(e->pos()))
    {
        mIsInfosChecked = !mIsInfosChecked;
    }
    else if(mDatesHistoRect.contains(e->pos()))
    {
        mIsDatesHistoSelected = true;
        mIsDatesTraceSelected = false;
        mIsDatesVarianceSelected = false;
    }
    else if(mDatesTraceRect.contains(e->pos()))
    {
        mIsDatesHistoSelected = false;
        mIsDatesTraceSelected = true;
        mIsDatesVarianceSelected = false;
    }
    else if(mDatesVarianceRect.contains(e->pos()))
    {
        mIsDatesHistoSelected = false;
        mIsDatesTraceSelected = false;
        mIsDatesVarianceSelected = true;
    }
    else if(mDatesCalibRect.contains(e->pos()))
    {
        mIsDatesCalibChecked = !mIsDatesCalibChecked;
    }
    else if(mEventsHistoRect.contains(e->pos()))
    {
        mIsEventsHistoSelected = true;
        mIsEventsTraceSelected = false;
    }
    else if(mEventsTraceRect.contains(e->pos()))
    {
        mIsEventsHistoSelected = false;
        mIsEventsTraceSelected = true;
    }
    else if(mPhasesAlphaRect.contains(e->pos()))
    {
        mIsPhasesAlphaChecked = !mIsPhasesAlphaChecked;
    }
    else if(mPhasesBetaRect.contains(e->pos()))
    {
        mIsPhasesBetaChecked = !mIsPhasesBetaChecked;
    }
    else if(mPhasesPredictRect.contains(e->pos()))
    {
        mIsPhasesPredictChecked = !mIsPhasesPredictChecked;
    }
    update();
}

void ResultsControls::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    
    mIsZoomInDown = false;
    mIsZoomDefaultDown = false;
    mIsZoomOutDown = false;
    
    update();
}

void ResultsControls::mouseMoveEvent(QMouseEvent* e)
{
    if(mZoomInRect.contains(e->pos()) ||
       mZoomDefaultRect.contains(e->pos()) ||
       mZoomDefaultRect.contains(e->pos()) ||
       mInfosRect.contains(e->pos()) ||
       mDatesHistoRect.contains(e->pos()) ||
       mDatesCalibRect.contains(e->pos()) ||
       mDatesTraceRect.contains(e->pos()) ||
       mDatesVarianceRect.contains(e->pos()) ||
       mEventsHistoRect.contains(e->pos()) ||
       mEventsTraceRect.contains(e->pos()))
    {
        setCursor(QCursor(Qt::PointingHandCursor));
    }
    else
    {
        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void ResultsControls::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.fillRect(rect(), QColor(100, 100, 100));
    
    drawButton(painter, mZoomInRect, mIsZoomInDown, true, QString(), QPixmap(":zoom_in_w.png"));
    drawButton(painter, mZoomDefaultRect, mIsZoomDefaultDown, true, QString(), QPixmap(":zoom_default_w.png"));
    drawButton(painter, mZoomOutRect, mIsZoomOutDown, true, QString(), QPixmap(":zoom_out_w.png"));
    drawButton(painter, mInfosRect, mIsInfosChecked, true, QString(tr("Show Infos")));
    
    drawBox(painter, mChainsRect, tr("Chains"));
    drawBox(painter, mPhasesRect, tr("Phases"));
    drawBox(painter, mEventsRect, tr("Events"));
    drawBox(painter, mDatesRect, tr("Dates"));
    
    // Dates
    drawRadio(painter, mDatesHistoRect, tr("Histogram"), mIsDatesHistoSelected);
    drawCheckbox(painter, mDatesCalibRect, tr("Calibration"), mIsDatesCalibChecked ? Qt::Checked : Qt::Unchecked);
    drawRadio(painter, mDatesTraceRect, tr("Trace"), mIsDatesTraceSelected);
    drawRadio(painter, mDatesVarianceRect, tr("Variance"), mIsDatesVarianceSelected);
    
    // Events
    drawRadio(painter, mEventsHistoRect, tr("Histogram"), mIsEventsHistoSelected);
    drawRadio(painter, mEventsTraceRect, tr("Trace"), mIsEventsTraceSelected);
    
    // Phases
    drawCheckbox(painter, mPhasesAlphaRect, tr("Start"), mIsPhasesAlphaChecked ? Qt::Checked : Qt::Unchecked);
    drawCheckbox(painter, mPhasesBetaRect, tr("End"), mIsPhasesBetaChecked ? Qt::Checked : Qt::Unchecked);
    drawCheckbox(painter, mPhasesPredictRect, tr("Predict"), mIsPhasesPredictChecked ? Qt::Checked : Qt::Unchecked);
}

