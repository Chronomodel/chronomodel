#include "GraphViewResults.h"
#include "GraphView.h"
#include "ProjectManager.h"
#include "Project.h"
#include "Painting.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewResults::GraphViewResults(QWidget *parent):QWidget(parent),
mParentGraph(0),
mMainColor(QColor(50, 50, 50)),
mVisible(true),
mUnfoldToggled(false),
mShowUnfold(false),
mUnfoldText(tr("Show sub-graphs")),
mIsDataDown(false),
mIsImageDown(false),
mMargin(5),
mLineH(20),
mGraphLeft(150)
{
    setMouseTracking(true);
    
    mGraph = new GraphView(this);
    
    mGraph->showAxis(false);
    mGraph->showScrollBar(false);
    mGraph->showYValues(true);
    mGraph->setRangeY(0, 1);
    
    mChainColors.append(Qt::blue);
    mChainColors.append(Qt::red);
    mChainColors.append(Qt::green);
    mChainColors.append(Qt::yellow);
}

GraphViewResults::~GraphViewResults()
{
    
}

void GraphViewResults::setSettings(const ProjectSettings& settings)
{
    mSettings = settings;
}

void GraphViewResults::setMCMCSettings(const MCMCSettings& mcmc)
{
    mMCMCSettings = mcmc;
}

void GraphViewResults::setParentGraph(GraphViewResults* graph)
{
    mParentGraph = graph;
    connect(mParentGraph, SIGNAL(unfoldToggled(bool)), this, SLOT(setVisibility(bool)));
}

void GraphViewResults::setVisibility(bool visible)
{
    mVisible = visible;
    emit visibilityChanged(visible);
}

void GraphViewResults::setRange(float min, float max)
{
    mGraph->setRangeX(min, max);
}

void GraphViewResults::zoom(float min, float max)
{
    mGraph->zoomX(min, max);
}

bool GraphViewResults::visible() const
{
    return mVisible;
}

void GraphViewResults::showUnfold(bool show, const QString& text)
{
    mShowUnfold = show;
    mUnfoldText = text;
    update();
}

void GraphViewResults::setMainColor(const QColor& color)
{
    mMainColor = color;
    update();
}

void GraphViewResults::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, mMainColor.lighter());
    grad.setColorAt(1, mMainColor);
    p.fillRect(rect(), grad);
    
    // Unfold
    if(mShowUnfold && (height() >= 3*mMargin + 2*mLineH))
    {
        drawButton(p, mUnfoldRect, mUnfoldToggled, true, mUnfoldText);
    }
    
    // Export Data & Image
    if(height() >= 4*mMargin + 3*mLineH)
    {
        drawButton(p, mDataRect, mIsDataDown, true, tr("Export"));
        drawButton(p, mImageRect, mIsImageDown, true, tr("Image"));
    }
}

void GraphViewResults::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    mDataRect = QRectF(mMargin, 2*mMargin + mLineH, (mGraphLeft - 3*mMargin)/2, mLineH);
    mImageRect = QRectF(2*mMargin + mDataRect.width(), 2*mMargin + mLineH, mDataRect.width(), mLineH);
    mUnfoldRect = QRectF(mMargin, height() - mLineH - mMargin, mGraphLeft - 2*mMargin, mLineH);
    
    mGraph->setGeometry(mGraphLeft, 0, width() - mGraphLeft, height());
}

void GraphViewResults::mousePressEvent(QMouseEvent* e)
{
    if(mUnfoldRect.contains(e->pos()))
    {
        toggleUnfold(!mUnfoldToggled);
    }
    else if(mDataRect.contains(e->pos()))
    {
        mIsDataDown = true;
        
        /*QTextEdit* edit = new QTextEdit();
        const QMap<double, double>& data = mGraph->curveData("graph");
        QMap<double, double>::const_iterator it;
        for(it = data.begin(); it != data.end(); ++it)
        {
            edit->append(QString::number(it->first) + "|" + QString::number(it->second) + "\n");
        }
        edit->show();*/
        
    }
    else if(mImageRect.contains(e->pos()))
    {
        mIsImageDown = true;
    }
    update();
}

void GraphViewResults::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    mIsDataDown = false;
    mIsImageDown = false;
    update();
}

void GraphViewResults::toggleUnfold(bool toggle)
{
    mUnfoldToggled = toggle;
    emit unfoldToggled(mUnfoldToggled);
    update();
}
