/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#include "EventPropertiesView.h"

#include "LineEdit.h"
#include "ColorPicker.h"
#include "Event.h"
#include "DatesList.h"
#include "Button.h"
#include "GraphView.h"
#include "Painting.h"
#include "MainWindow.h"
#include "Project.h"
#include "PluginAbstract.h"
#include "PluginManager.h"

#include "CurveSettings.h"
#include "CurveWidget.h"

#include <QtWidgets>

EventPropertiesView::EventPropertiesView(QWidget* parent, Qt::WindowFlags flags):
    QWidget(parent, flags),
    mEventObj(nullptr),
    mButtonWidth(50),
    mButtonHeigth(50),
    mLineEditHeight(25),
    mComboBoxHeight(25),
    mCurveEnabled(false)
{
    minimumHeight = 0;

    QPalette palette_BW;
    palette_BW.setColor(QPalette::Base, Qt::white);
    palette_BW.setColor(QPalette::Window, Qt::white);
    palette_BW.setColor(QPalette::Text, Qt::black);
    palette_BW.setColor(QPalette::WindowText, Qt::black);

    // ------------- commun with default Event and Bound ----------
    mTopView = new QWidget(this);

    mNameLab = new QLabel(tr("Name"), mTopView);
    mNameLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mNameEdit = new LineEdit(mTopView);


    mColorLab = new QLabel(tr("Color"), mTopView);
    mColorLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mColorPicker = new ColorPicker(Qt::black, mTopView);

    mMethodLab = new QLabel(tr("MCMC"), mTopView);
    mMethodLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mMethodCombo = new QComboBox(mTopView);
    mMethodInfo = new QLabel(MHVariable::getSamplerProposalText(MHVariable::eMHAdaptGauss), mTopView);

    mMethodCombo->addItem(MHVariable::getSamplerProposalText(MHVariable::eDoubleExp));
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(MHVariable::eBoxMuller));
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(MHVariable::eMHAdaptGauss));
    
    connect(mNameEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventName);
    connect(mColorPicker, &ColorPicker::colorChanged, this, &EventPropertiesView::updateEventColor);
    connect(mMethodCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &EventPropertiesView::updateEventSampler);
    
    // Field curve parameter

    QPalette palette_lab;
    palette_lab.setColor(QPalette::WindowText, Qt::black);

    mCurveWidget = new CurveWidget(mTopView);

    mCurveNodeCB = new QCheckBox(tr("Node"), mCurveWidget);
    mCurveNodeCB->setPalette(palette_lab);
    mCurveNodeCB->setFixedWidth(15);
    connect(mCurveNodeCB, &QCheckBox::toggled, this, &EventPropertiesView::updateCurveNode);
    
    mX_IncLab = new QLabel(tr("Inclination"), mCurveWidget);
    mX_IncLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mX_IncLab->setPalette(palette_lab);

    mY_DecLab = new QLabel(tr("Declination"), mCurveWidget);
    mY_DecLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mY_DecLab->setPalette(palette_lab);

    mZ_IntLab = new QLabel(tr("Field"), mCurveWidget);
    mZ_IntLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mZ_IntLab->setPalette(palette_lab);
    
    mX_IncEdit = new LineEdit(mCurveWidget);
    mY_DecEdit = new LineEdit(mCurveWidget);
    mZ_IntEdit = new LineEdit(mCurveWidget);

    connect(mX_IncEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventXInc);
    connect(mY_DecEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventYDec);
    connect(mZ_IntEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventZF);
    
    QDoubleValidator* positiveValidator = new QDoubleValidator(this);
    positiveValidator->setBottom(0.);

    mS_X_IncLab = new QLabel("Error", mCurveWidget);
    mS_X_IncLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mS_X_IncLab->setPalette(palette_lab);

    mS_Y_Lab = new QLabel(tr("Error"), mCurveWidget);
    mS_Y_Lab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mS_Y_Lab->setPalette(palette_lab);

    mS_Z_IntLab = new QLabel(tr("Error"), mCurveWidget);
    mS_Z_IntLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mS_Z_IntLab->setPalette(palette_lab);
    
    mS_X_IncEdit = new LineEdit(mCurveWidget);
    mS_X_IncEdit->setValidator(positiveValidator);

    mS_Y_Edit = new LineEdit(mCurveWidget);
    mS_Y_Edit->setValidator(positiveValidator);

    mS_Z_IntEdit = new LineEdit(mCurveWidget);
    mS_Z_IntEdit->setValidator(positiveValidator);

    connect(mS_X_IncEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventSXInc);
    connect(mS_Y_Edit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventSYDec);
    connect(mS_Z_IntEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventSZF);

    setTabOrder(mX_IncEdit, mS_X_IncEdit);
    setTabOrder(mS_X_IncEdit, mY_DecEdit);
    setTabOrder(mY_DecEdit, mS_Y_Edit);
    setTabOrder(mS_Y_Edit, mZ_IntEdit);
    setTabOrder(mZ_IntEdit, mS_Z_IntEdit);

    // Event default propreties Window mEventView
    mEventView = new QWidget(this);

    minimumHeight += mEventView->height();
    // -------------
    mDatesList = new DatesList(mEventView);
    mDatesList->setPalette(palette_BW);
    connect(mDatesList, &DatesList::indexChange, this, &EventPropertiesView::updateIndex);
    connect(mDatesList, &DatesList::calibRequested, this, &EventPropertiesView::updateCalibRequested);

    // -------------

    const QList<PluginAbstract*> &plugins = PluginManager::getPlugins();

    for (auto p : plugins) {
        Button* button = new Button(p->getName(), mEventView);

        button->setIcon(p->getIcon());
        button->setFlatVertical();
        button->setIconOnly(false);
        button->setToolTip(tr("Insert %1 Data").arg(p->getName()) );

        connect(button, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &EventPropertiesView::createDate);

        minimumHeight += button->height();

        mPluginButs.append(button);

    }

    mDeleteBut = new Button(tr("Delete"), mEventView);
    mDeleteBut->setIcon(QIcon(":delete.png"));
    mDeleteBut->setFlatVertical();
    mDeleteBut->setToolTip(tr("Delete Data"));

    connect(mDeleteBut, static_cast<void (QPushButton::*)(bool)>(&Button::clicked), this, &EventPropertiesView::deleteSelectedDates);
    minimumHeight += mDeleteBut->height();

    mRecycleBut = new Button(tr("Restore"), mEventView);
    mRecycleBut->setIcon(QIcon(":restore.png"));
    mRecycleBut->setFlatVertical();
    mRecycleBut->setToolTip(tr("Restore Deleted Data"));

    connect(mRecycleBut, static_cast<void (QPushButton::*)(bool)>(&Button::clicked), this, &EventPropertiesView::recycleDates);

    // ---------------

    mCalibBut = new Button(tr("Calibrate"), mEventView);
    mCalibBut->setIcon(QIcon(":calib.png"));
    mCalibBut->setFlatVertical();
    mCalibBut->setCheckable(true);
    mCalibBut->setToolTip(tr("Calibrate"));

    mCombineBut = new Button(tr("Combine"), mEventView);
    mCombineBut->setFlatVertical();
    mCombineBut->setIconOnly(false);
    mCombineBut->setEnabled(false);
    mCombineBut->setToolTip(tr("Combine data from the same plugin"));

    mSplitBut = new Button(tr("Split"), mEventView);
    mSplitBut->setFlatVertical();
    mSplitBut->setIconOnly(false);
    mSplitBut->setEnabled(false);
    mSplitBut->setToolTip(tr("Split Combined Data"));

    connect(mCalibBut, &Button::clicked, this, &EventPropertiesView::showCalibRequested);
    connect(mCalibBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &EventPropertiesView::updateButton);
    connect(mCombineBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &EventPropertiesView::sendCombineSelectedDates);
    connect(mSplitBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &EventPropertiesView::sendSplitDate);

    // --------------- Case of Event is a Bound -> Bound properties windows---------------------------

    mBoundView = new QWidget(this);

    mKnownFixedEdit = new LineEdit(mBoundView);

    mKnownGraph = new GraphView(mBoundView);
    mKnownGraph->setMinimumHeight(250);

    mKnownGraph->showXAxisArrow(true);
    mKnownGraph->showXAxisTicks(true);
    mKnownGraph->showXAxisSubTicks(true);
    mKnownGraph->showXAxisValues(true);

    mKnownGraph->showYAxisArrow(true);
    mKnownGraph->showYAxisTicks(false);
    mKnownGraph->showYAxisSubTicks(false);
    mKnownGraph->showYAxisValues(false);

    mKnownGraph->setXAxisSupport(AxisTool::AxisSupport::eMin_Max);
    mKnownGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

    mKnownGraph->setXAxisMode(GraphView::eMinMax);
    mKnownGraph->setYAxisMode(GraphView::eMinMax);

    connect(mDatesList, &DatesList::itemSelectionChanged, this, &EventPropertiesView::updateCombineAvailability);
    connect(mKnownFixedEdit, &QLineEdit::textEdited, this, &EventPropertiesView::updateKnownFixed);

    QFormLayout* fixedLayout = new QFormLayout();
    fixedLayout->addRow(QString(tr("Value") + " " + "(BC/AD)"), mKnownFixedEdit);
    mFixedGroup = new QGroupBox();
    mFixedGroup->setLayout(fixedLayout);

    QVBoxLayout* boundLayout = new QVBoxLayout();
    boundLayout->setContentsMargins(10, 6, 15, 6);
    boundLayout->setSpacing(10);
    boundLayout->addWidget(mFixedGroup);

    boundLayout->addWidget(mKnownGraph);
    boundLayout->addStretch();
    mBoundView->setLayout(boundLayout);

    mTopView->setVisible(false);
    mEventView->setVisible(false);
    mBoundView->setVisible(false);

    applyAppSettings();
}

EventPropertiesView::~EventPropertiesView()
{
    mPluginButs.clear();
    mDatesList->clear();
    mEventObj = nullptr;
}

# pragma mark Event Managment
void EventPropertiesView::setEvent(QJsonObject *eventObj)
{
    // if set Event come because we use Project::updateDate(), we are on the same Event
    // so we are on the EventPropertiesView not on the EventScene
    try {
        if (eventObj == nullptr)
            return;

        if (eventObj->empty())
            return;

        if (mEventObj != nullptr) {
            if (!mEventObj->empty() && mEventObj->value(STATE_ID) != eventObj->value(STATE_ID)) {
                mCalibBut->setChecked(false);
            }

        } else {
            mCalibBut->setChecked(false);
        }
        // Assign the local event
        mEventObj = eventObj;

    } catch (...) {
        qDebug()<<"[EventPropertiesView::setEvent]" << " Hummm ....";
        return;
    }

    // Select the first date if the list is not empty
    if (mEventObj->value(STATE_EVENT_DATES).toArray().size() > 0) {
        mCurrentDateIdx = 0;
    }
    
    if (rect().width() > 0)
        updateEvent();
}

void EventPropertiesView::initEvent(QJsonObject *eventObj)
{
    // Assign the local event
    mCalibBut->setChecked(false);
    if (eventObj == nullptr || eventObj->empty())
        return;

    // Assign the local event

    mEventObj = eventObj;


    // Select the first date if the list is not empty
    if (eventObj == nullptr && mEventObj->value(STATE_EVENT_DATES).toArray().size() > 0) {
        mCurrentDateIdx = 0;
    }

}

void EventPropertiesView::resetEvent()
{
    // Assign the local event
    mCalibBut->setChecked(false);

    // Assign the local event

    mEventObj = nullptr;
     mCurrentDateIdx = 0;


}

void EventPropertiesView::updateIndex(int index)
{
    mCurrentDateIdx = index;
}

void EventPropertiesView::updateEvent()
{
    qDebug()<<"[EventPropertiesView::updateEvent]";

    if (mEventObj->isEmpty()) {
        mTopView->setVisible(false);
        mEventView->setVisible(false);
        mBoundView->setVisible(false);
        return;

    } else {
        Event::Type type = Event::Type (mEventObj->value(STATE_EVENT_TYPE).toInt());
        const QString name = mEventObj->value(STATE_NAME).toString();
        const QColor color(mEventObj->value(STATE_COLOR_RED).toInt(),
                           mEventObj->value(STATE_COLOR_GREEN).toInt(),
                           mEventObj->value(STATE_COLOR_BLUE).toInt());

        if (name != mNameEdit->text())
            mNameEdit->setText(name);

        mColorPicker->setColor(color);

        // Curve
        
        // The Curve settings may have changed since the last time the event property view has been opened
        const QJsonObject &state = MainWindow::getInstance()->getState();
        const CurveSettings &settings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());

        mCurveEnabled = (settings.mProcessType != CurveSettings::eProcess_None);
        
        mMethodLab->setVisible(type == Event::eDefault);
        mMethodCombo->setVisible( !mCurveEnabled && (type == Event::eDefault));
        mMethodInfo->setVisible( mCurveEnabled && (type == Event::eDefault));
        
        // Y1 contient l'inclinaison. Elle est toujours nécessaire en sphérique et vectoriel.
        // En univarié, elle n'est nécessaire que pour les variables d'étude : inclinaison ou déclinaison.
        bool showXEdit, showYEdit, showZEdit, showYErr;

        if (!mCurveEnabled) {
            showXEdit = false;
            showYEdit = false;
            showZEdit = false;
            showYErr = false;

        } else {
            showXEdit = settings.showX();
            showYEdit = settings.showY() || settings.mProcessType == CurveSettings::eProcess_Declination;
            showZEdit = settings.showZ();
            showYErr = settings.showYErr();
        }
        Event::PointType tyPts = Event::PointType (mEventObj->value(STATE_EVENT_POINT_TYPE).toInt());
        mCurveNodeCB->setVisible(mCurveEnabled);

        mCurveNodeCB->blockSignals(true);
        mCurveNodeCB->setChecked(tyPts == Event::eNode); //emit updateCurveNode
        mCurveNodeCB->blockSignals(true);

        mX_IncLab->setVisible(showXEdit);
        mX_IncEdit->setVisible(showXEdit);

        mS_X_IncLab->setVisible(showXEdit);
        mS_X_IncLab->setText(XerrorLabel());

        mS_X_IncEdit->setVisible(showXEdit);
        if (showXEdit) {

            if (settings.mProcessType == CurveSettings::eProcess_Declination) {
                mX_IncLab->setText(tr("Inclination"));
                mX_IncEdit->setText(locale().toString(mEventObj->value(STATE_EVENT_X_INC_DEPTH).toDouble()));
                mS_X_IncEdit->setText(locale().toString(mEventObj->value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble()));

            } else if (settings.mProcessType == CurveSettings::eProcess_Field) {
                mX_IncLab->setText(settings.XLabel());
                mX_IncEdit->setText(locale().toString(mEventObj->value(STATE_EVENT_Z_F).toDouble()));
                mS_X_IncEdit->setText(locale().toString(mEventObj->value(STATE_EVENT_SZ_SF).toDouble()));

            } else {
                mX_IncLab->setText(settings.XLabel());
                mX_IncEdit->setText(locale().toString(mEventObj->value(STATE_EVENT_X_INC_DEPTH).toDouble()));
                mS_X_IncEdit->setText(locale().toString(mEventObj->value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble()));
            }
        }
        

        mY_DecLab->setVisible(showYEdit);
        mY_DecEdit->setVisible(showYEdit);

        if (showYEdit) {
            if (settings.mProcessType == CurveSettings::eProcess_Declination) {
                mY_DecLab->setText(settings.XLabel());
            } else {
                mY_DecLab->setText(settings.YLabel());
            }
            mY_DecEdit->setText(locale().toString(mEventObj->value(STATE_EVENT_Y_DEC).toDouble()));
        }

        mS_Y_Lab->setVisible(showYErr);
        mS_Y_Edit->setVisible(showYErr);

        if (showYErr) {
            mS_Y_Lab->setText(tr("Error"));
            mS_Y_Edit->setText(locale().toString(mEventObj->value(STATE_EVENT_SY).toDouble()));
        }

        mZ_IntLab->setVisible(showZEdit);
        mZ_IntEdit->setVisible(showZEdit);
        
        mS_Z_IntLab->setVisible(showZEdit);
        mS_Z_IntEdit->setVisible(showZEdit);
        
        if (showZEdit) {
            mZ_IntLab->setText(settings.ZLabel());
            mZ_IntEdit->setText(locale().toString(mEventObj->value(STATE_EVENT_Z_F).toDouble()));
            mS_Z_IntEdit->setText(locale().toString(mEventObj->value(STATE_EVENT_SZ_SF).toDouble()));
        }

        
        mTopView->setVisible(true);

        mEventView->setVisible(type == Event::eDefault);
        mBoundView->setVisible(type == Event::eBound);

        if (type == Event::eDefault) {
            mMethodCombo->setCurrentIndex(mEventObj->value(STATE_EVENT_SAMPLER).toInt());
            
            if (mEventObj->value(STATE_EVENT_SAMPLER).toInt() == MHVariable::eDoubleExp)
                mMethodCombo->setCurrentIndex(0);

            else if (mEventObj->value(STATE_EVENT_SAMPLER).toInt() == MHVariable::eBoxMuller)
                    mMethodCombo->setCurrentIndex(1);

            else if (mEventObj->value(STATE_EVENT_SAMPLER).toInt() == MHVariable::eMHAdaptGauss)
                mMethodCombo->setCurrentIndex(2);


            //       qDebug() << "[EventPropertiesView::updateEvent] mEvent mOrigin"  << mEvent.value(STATE_EVENT_DATES).toArray().at(0).toObject().value(STATE_DATE_ORIGIN).toInt();
            mDatesList->setEvent(*mEventObj);
            if (mCurrentDateIdx >= 0)
                mDatesList->setCurrentRow(mCurrentDateIdx); //connect to DatesList::handleItemIsChanged() emit calibRequested(date);

            const QJsonArray &dates = mEventObj->value(STATE_EVENT_DATES).toArray();
           
            const bool hasDates = (dates.size() > 0);

            if (hasDates && mCurrentDateIdx >= 0) {
                emit updateCalibRequested(dates[mCurrentDateIdx].toObject());
                mCalibBut->setEnabled(true);
                mDeleteBut->setEnabled(true);
                mRecycleBut->setEnabled(true);

            } else {
                mCalibBut->setEnabled(false);
                mDeleteBut->setEnabled(false);
                mRecycleBut->setEnabled(true);
            }

        } else if (type == Event::eBound) {
            mKnownFixedEdit -> setText(locale().toString(mEventObj->value(STATE_EVENT_KNOWN_FIXED).toDouble()));
            updateKnownGraph();
        }
    }

    updateLayout();
    update();
}

const QJsonObject& EventPropertiesView::getEvent()
{
    return *mEventObj;
}


void EventPropertiesView::updateEventName()
{
    if (!mEventObj->empty()) {
        QJsonObject ev (*mEventObj);
        ev[STATE_NAME] = mNameEdit->text();
        MainWindow::getInstance()->updateEvent(ev, tr("Event name updated"));
    }
}

void EventPropertiesView::updateEventColor(const QColor &color)
{
    QJsonObject ev (*mEventObj);
    ev[STATE_COLOR_RED] = color.red();
    ev[STATE_COLOR_GREEN] = color.green();
    ev[STATE_COLOR_BLUE] = color.blue();
    MainWindow::getInstance()->updateEvent(ev, tr("Event color updated"));
}

void EventPropertiesView::updateEventSampler(int index)
{
    if (mEventObj->value(STATE_EVENT_SAMPLER).toInt() == index)
        return;

    MHVariable::SamplerProposal sp = MHVariable::eDoubleExp;
    switch (index) {
    case 0 :
        sp = MHVariable::eDoubleExp;
        break;
    case 1 :
        sp = MHVariable::eBoxMuller;
        break;
    case 2 :
        sp = MHVariable::eMHAdaptGauss;
        break;
    }

    QJsonObject ev (*mEventObj);
    ev[STATE_EVENT_SAMPLER] = sp;
    MainWindow::getInstance()->updateEvent(ev, tr("Event method updated"));
}

// Event Known Properties

void EventPropertiesView::updateKnownFixed(const QString &text)
{
    bool ok;
    double fixedValue = locale().toDouble(text, &ok);
    if ( ok == false || fixedValue == mEventObj->value(STATE_EVENT_KNOWN_FIXED).toDouble())
        return;

    QJsonObject ev (*mEventObj);
    ev[STATE_EVENT_KNOWN_FIXED] = fixedValue;
    MainWindow::getInstance()->updateEvent(ev, tr("Bound fixed value updated"));

}

// Curve
void EventPropertiesView::updateCurveNode(bool isNode)
{
    QJsonObject ev (*mEventObj);
    ev[STATE_EVENT_POINT_TYPE] = isNode ? Event::PointType::eNode : Event::PointType::ePoint;

    MainWindow::getInstance()->updateEvent(ev, tr("Event Node updated"));
}

void EventPropertiesView::updateEventXInc()
{
    const QJsonObject &state = MainWindow::getInstance()->getState();
    const auto processType  = CurveSettings::processType_fromJson(state.value(STATE_CURVE).toObject());

    QJsonObject ev (*mEventObj);
    if (processType == CurveSettings::eProcess_Field) {
        ev[STATE_EVENT_Z_F] = locale().toDouble(mX_IncEdit->text());

    } else
        ev[STATE_EVENT_X_INC_DEPTH] = locale().toDouble(mX_IncEdit->text());

    MainWindow::getInstance()->updateEvent(ev, tr("Event X-Inc updated"));
}

void EventPropertiesView::updateEventYDec()
{
    QJsonObject ev (*mEventObj);
    ev[STATE_EVENT_Y_DEC] = locale().toDouble(mY_DecEdit->text());
    MainWindow::getInstance()->updateEvent(ev, tr("Event Y-Dec updated"));
}

void EventPropertiesView::updateEventZF()
{
    QJsonObject ev (*mEventObj);
    ev[STATE_EVENT_Z_F] = locale().toDouble(mZ_IntEdit->text());
    MainWindow::getInstance()->updateEvent(ev, tr("Event Z-F updated"));
}

void EventPropertiesView::updateEventSXInc()
{
    const QJsonObject &state = MainWindow::getInstance()->getState();
    const auto processType  = CurveSettings::processType_fromJson(state.value(STATE_CURVE).toObject());

    QJsonObject ev (*mEventObj);
    if (processType ==  CurveSettings::eProcess_Field) {
        ev[STATE_EVENT_SZ_SF] = locale().toDouble(mS_X_IncEdit->text());

    } else
       ev[STATE_EVENT_SX_ALPHA95_SDEPTH] = locale().toDouble(mS_X_IncEdit->text());

    MainWindow::getInstance()->updateEvent(ev, tr("Event S X-Inc updated"));
}

void EventPropertiesView::updateEventSYDec()
{
    QJsonObject ev (*mEventObj);
    ev[STATE_EVENT_SY] = locale().toDouble(mS_Y_Edit->text());
    MainWindow::getInstance()->updateEvent(ev, tr("Event S Y updated"));
}

void EventPropertiesView::updateEventSZF()
{
    QJsonObject ev (*mEventObj);
    ev[STATE_EVENT_SZ_SF] = locale().toDouble(mS_Z_IntEdit->text());
    MainWindow::getInstance()->updateEvent(ev, tr("Event S Z-F updated"));
}

void EventPropertiesView::updateKnownGraph()
{
    mKnownGraph->removeAllCurves();

    const QJsonObject &state = MainWindow::getInstance()->getState();
    const QJsonObject &settings = state.value(STATE_SETTINGS).toObject();
    const double tmin = settings.value(STATE_SETTINGS_TMIN).toDouble();
    const double tmax = settings.value(STATE_SETTINGS_TMAX).toDouble();

    const double fixedValue = mEventObj->value(STATE_EVENT_KNOWN_FIXED).toDouble();

    if ( (tmin > fixedValue) || (fixedValue > tmax) )
        return;

    mKnownGraph->setRangeX(tmin, tmax);

    mKnownGraph->setCurrentX(tmin, tmax);

    mKnownGraph->setRangeY(0., 1.);
    mKnownGraph->showYAxisValues(false);

    //---------------------

    GraphCurve curve;
    curve.mVisible = true;
    curve.mName = "Bound";
    curve.mBrush = Painting::mainColorLight;

    curve.mPen = QPen(Painting::mainColorLight, 2.);

    curve.mType = GraphCurve::eHorizontalSections;
    qreal tLower = fixedValue;
    qreal tUpper = tLower;

    curve.mSections.push_back(qMakePair(tLower, tUpper));
    mKnownGraph->add_curve(curve);
    QFontMetrics fm (mKnownGraph->font());
    mKnownGraph->setMarginBottom(fm.ascent() + 10. );

    // Adjust scale
    const int xScale = int (log10(tmax-tmin)) -1;
    mKnownGraph->setXScaleDivision(std::pow(10, xScale), 4);
    //---------------------

}


void EventPropertiesView::createDate()
{
    if (!mEventObj->isEmpty()) {
        Button* but = dynamic_cast<Button*>(sender());
        if (but) {
            const auto &project = MainWindow::getInstance()->getProject();
            const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();

            for (int i=0; i<plugins.size(); ++i) {
                if (plugins.at(i)->getName() == but->text()) {
                    Date date = project->createDateFromPlugin(plugins.at(i));

                    if (!date.isNull())
                        project->addDate(mEventObj->value(STATE_ID).toInt(), date.toJson());
                }
            }
        }

    }
}

void EventPropertiesView::deleteSelectedDates()
{
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    QList<int> indexes;
    for (int i = 0; i<items.size(); ++i)
        indexes.push_back(mDatesList->row(items[i]));

    MainWindow::getInstance()->getProject()->deleteDates(mEventObj->value(STATE_ID).toInt(), indexes);

}

void EventPropertiesView::recycleDates()
{
    MainWindow::getInstance()->getProject()->recycleDates(mEventObj->value(STATE_ID).toInt());

}

// Merge / Split
void EventPropertiesView::updateCombineAvailability()
{
    bool mergeable (false);
    bool splittable (false);

    QJsonArray dates = mEventObj->value(STATE_EVENT_DATES).toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();

    if (items.size() == 1) {
        // Split?
        int idx = mDatesList->row(items[0]);
        if (dates.size()>idx ) {
            QJsonObject date = dates.at(idx).toObject();
            if (date.value(STATE_DATE_SUB_DATES).toArray().size() > 0)
                splittable = true;

         }
    } else if (items.size() > 1 && dates.size() > 1) {
        // Combine?
        mergeable = true;
        PluginAbstract* plugin = nullptr;

        for (int i= 0; i<items.size(); ++i) {
            int idx = mDatesList->row(items.at(i));
            if (idx < dates.size()) {
                QJsonObject date = dates.at(idx).toObject();
                // If selected date already has subdates, it cannot be combined :
                if (date.value(STATE_DATE_SUB_DATES).toArray().size() > 0) {
                    mergeable = false;
                    break;
                }
                // If selected dates have different plugins, they cannot be combined :
                PluginAbstract* plg = PluginManager::getPluginFromId(date.value(STATE_DATE_PLUGIN_ID).toString());
                if (plugin == nullptr)
                    plugin = plg;
                else if (plg != plugin) {
                    mergeable = false;
                    break;
                }
            }
        }
        // Dates are not combined yet and are from the same plugin.
        // We should now ask the plugin if they are combinable (they use the same ref curve for example...)
        if (mergeable && plugin != nullptr) {
            // This could be used instead to disable the "combine" button if dates cannot be combined.
            // We prefer letting the user combine them and get an error message explaining why they cannot be combined!
            // Check Plugin14C::mergeDates as example
            //mergeable = plugin->areDatesMergeable(dates);

            mergeable = true;
        }
    }
    mCombineBut->setEnabled(mergeable);
    mSplitBut->setEnabled(splittable);
}

void EventPropertiesView::sendCombineSelectedDates()
{
    QJsonArray dates = mEventObj->value(STATE_EVENT_DATES).toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    QList<int> dateIds;

    for (int i = 0; i<items.size(); ++i) {
        const int idx = mDatesList->row(items.at(i));
        if (idx < dates.size()) {
            const QJsonObject &date = dates.at(idx).toObject();
            dateIds.push_back(date.value(STATE_ID).toInt());
        }
    }
    emit combineDatesRequested(mEventObj->value(STATE_ID).toInt(), dateIds);
    updateEvent();
}

void EventPropertiesView::sendSplitDate()
{
    const QJsonArray &dates = mEventObj->value(STATE_EVENT_DATES).toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    if (items.size() > 0) {
        int idx = mDatesList->row(items[0]);
        if (idx < dates.size()) {
            const QJsonObject &date = dates.at(idx).toObject();
            int dateId = date.value(STATE_ID).toInt();
            emit splitDateRequested(mEventObj->value(STATE_ID).toInt(), dateId);
        }
        updateEvent();
    }
}

/**
 * @brief EventPropertiesView::keyPressEvent Append only when the windows has the focus
 * @param event
 */
void EventPropertiesView::keyPressEvent(QKeyEvent* e)
{
    if ((e->key() == Qt::Key_Escape) && mCalibBut->isChecked())
        mCalibBut->click();

    else if ((e->key() == Qt::Key_C) && !mCalibBut->isChecked())
        mCalibBut->click();

    QWidget::keyPressEvent(e);

}

// Layout
void EventPropertiesView::paintEvent(QPaintEvent* e)
{
    (void) e;
    QWidget::paintEvent(e);
    QPainter p(this);

    if (mEventObj == nullptr || mEventObj->isEmpty()) {
        QFont font = p.font();
        font.setBold(true);
        font.setPointSize(20);
        p.setFont(font);
        p.setPen(QColor(150, 150, 150));
        p.drawText(rect(), Qt::AlignCenter, tr("No event selected !"));
        return;
    }


    Event::Type type = Event::Type (mEventObj->value(STATE_EVENT_TYPE).toInt());
    if (type == Event::eBound)
       p.fillRect(rect(), palette().color(QPalette::Window));
    else
        p.fillRect(0, mTopView->height(), width(), height() - mTopView->y(), Painting::borderDark);
    // behind plugin button



}

void EventPropertiesView::resizeEvent(QResizeEvent* e)
{
    (void) e;
    updateLayout();
}

void EventPropertiesView::applyAppSettings()
{
    minimumHeight += mEventView->height();

    minimumHeight = 0;
    for (auto &but : mPluginButs) {
        but->resize(mButtonWidth, mButtonHeigth);
        minimumHeight += but->height();
    }

    minimumHeight += mDeleteBut->height();

    if (isVisible())
        updateLayout();
}

QString EventPropertiesView::XerrorLabel() const
{
    const QJsonObject &state = MainWindow::getInstance()->getState();
    const CurveSettings curveSettings (state.value(STATE_CURVE).toObject());
    switch (curveSettings.mProcessType) {
    case CurveSettings::eProcess_Inclination:
    case CurveSettings::eProcess_Declination:
    case CurveSettings::eProcess_Spherical:
    case CurveSettings::eProcess_Unknwon_Dec:
    case CurveSettings::eProcess_Vector:
        return tr("Alpha95");
        break;
    default:
        return tr("Error");
        break;
    }
}

void EventPropertiesView::updateLayout()
{
    if (mEventObj == nullptr)
        return;
    if (mEventObj->empty() || width()<1)
        return;

    const QJsonObject &state = MainWindow::getInstance()->getState();
    const CurveSettings curveSettings (state.value(STATE_CURVE).toObject());
    const bool withCurve = curveSettings.mProcessType != CurveSettings::eProcess_None;
    const bool withNode = (curveSettings.mLambdaSplineType != CurveSettings::eInterpolation) && (curveSettings.mVarianceType != CurveSettings::eModeFixed)
                          && (curveSettings.mUseVarianceIndividual == true);


    QFontMetrics fm (font());
    const int margin = 10;

    int shiftMax (qMax(fm.horizontalAdvance(mNameLab->text()), qMax(fm.horizontalAdvance(mColorLab->text()), fm.horizontalAdvance(mMethodLab->text()) )) );
    shiftMax = shiftMax + 2*margin;
    const int editWidth  = width() - shiftMax;
    const int labeltWidth = width() - editWidth - 2*margin;

    mNameLab->setGeometry(margin, margin, labeltWidth, mLineEditHeight);
    mNameEdit->setGeometry(shiftMax , mNameLab->y(), editWidth - margin, mLineEditHeight);

    mColorLab->setGeometry(margin, mNameEdit->y() + mNameEdit->height() + margin , labeltWidth, mLineEditHeight);
    mColorPicker->setGeometry(shiftMax, mColorLab->y(), editWidth - margin, mLineEditHeight);

    int topViewHeight = mColorPicker->y() + mColorPicker->height();

    int CurveHeight = 0;

    if (withCurve) {
        auto curveWidgetWidth = mCurveWidget->width()<0 ? width() - 2*margin : mCurveWidget->width();

        int lines = 1;
        if (curveSettings.showYErr()) {
            ++lines;
        }
        if (curveSettings.showZ()) {
            ++lines;
        }
       // const int lines = (curveSettings.showInclination() && curveSettings.showIntensity()) ? 2 : 1;
        CurveHeight = margin + (margin + mLineEditHeight) * lines;

        int dx = margin;
        int dy = margin;
        const int labW = 80;
        const int YshiftLabel = (mLineEditHeight - mX_IncLab->height())/2;
        if (withNode) {
            mCurveNodeCB->setVisible(true);
            mCurveNodeCB->setFixedWidth(labW);

        } else {
            mCurveNodeCB->setVisible(false);
            mCurveNodeCB->setFixedWidth(0);
        }
        const int curveNodeCBWidth = mCurveNodeCB->width();

        int editW;
        if ( curveSettings.showX()) {
            if ((curveSettings.showY() || curveSettings.mProcessType == CurveSettings::eProcess_Declination) && !curveSettings.showYErr() ) {
                editW = std::max(0, (curveWidgetWidth - 8*margin - 3*labW - curveNodeCBWidth) / 3);

            } else {
                editW = std::max(0, (curveWidgetWidth - 6*margin - 2*labW - curveNodeCBWidth) / 2);
            }

            mX_IncLab->setGeometry(dx, dy - YshiftLabel, labW, mLineEditHeight);
            mX_IncEdit->setGeometry(dx += labW + margin, dy, editW, mLineEditHeight);
            mS_X_IncLab->setGeometry(dx += editW + margin, dy - YshiftLabel, labW, mLineEditHeight);
            mS_X_IncEdit->setGeometry(dx += labW + margin, dy, editW, mLineEditHeight);

            if ((curveSettings.showY() || curveSettings.mProcessType == CurveSettings::eProcess_Declination) && !curveSettings.showYErr())  {
                mY_DecLab->setGeometry(dx += editW + margin, dy - YshiftLabel, labW, mLineEditHeight);
                mY_DecEdit->setGeometry(dx += labW + margin, dy, editW, mLineEditHeight);
            } 

            if (withNode)
                mCurveNodeCB->setGeometry(dx + editW + margin, dy, mCurveNodeCB->width(), mLineEditHeight);

            dy += mLineEditHeight + margin;
        }

        if (curveSettings.showYErr()) {
            dx = margin;
            editW = (mCurveWidget->width() - 6*margin - 2*labW - curveNodeCBWidth) / 2;

            mY_DecLab->setGeometry(dx, dy - YshiftLabel, labW, mLineEditHeight);
            mY_DecEdit->setGeometry(dx += labW + margin, dy, editW, mLineEditHeight);
            mS_Y_Lab->setGeometry(dx += editW + margin, dy - YshiftLabel, labW, mLineEditHeight);
            mS_Y_Edit->setGeometry(dx + labW + margin, dy, editW, mLineEditHeight);

            dy += mLineEditHeight + margin;
        }

        if (curveSettings.showZ()) {
            dx = margin;
            editW = (curveWidgetWidth - 6*margin - 2*labW - curveNodeCBWidth) / 2;

            mZ_IntLab->setGeometry(dx, dy - YshiftLabel, labW, mLineEditHeight);
            mZ_IntEdit->setGeometry(dx += labW + margin, dy, editW, mLineEditHeight);
            mS_Z_IntLab->setGeometry(dx += editW + margin, dy - YshiftLabel, labW, mLineEditHeight);
            mS_Z_IntEdit->setGeometry(dx + labW + margin, dy, editW, mLineEditHeight);
        }

    }

    if (hasEvent())  {

        int butPluginHeigth = mButtonHeigth;

        // in EventPropertiesView coordinates
        mBoundView->resize(0, 0);

        mMethodLab->setGeometry(margin, mColorPicker->y() + mColorPicker->height() + margin, labeltWidth, mLineEditHeight);
        if (withCurve) {
            mMethodCombo->setGeometry(0 , 0, 0, 0);
            mMethodInfo ->setGeometry(shiftMax , mMethodLab->y(), editWidth - margin, mComboBoxHeight);
        } else {
            mMethodCombo->setGeometry(shiftMax , mMethodLab->y(), editWidth - margin, mComboBoxHeight);
            mMethodInfo ->setGeometry(0 , 0, 0, 0);
        }

        
        // ----------------------------------
        //  Top View Height
        // ----------------------------------
        topViewHeight += mComboBoxHeight + 2*margin;

        mTopView->resize(width(), topViewHeight + ((CurveHeight > 0) ? CurveHeight + margin : 0));
        
        // ----------------------------------
        //  Curve event data
        // ----------------------------------
        mCurveWidget->setGeometry(margin, mMethodLab->y() + mMethodLab->height() + margin, width() - 2*margin, CurveHeight);
        
        mEventView->setGeometry(0, mTopView->height(), width(), height() - mTopView->height());

         //in mEventView coordinates
        QRect listRect(0, 0, mEventView->width() - mButtonWidth, mEventView->height() - butPluginHeigth);
        mDatesList->setGeometry(listRect);

        int x = listRect.width();
        int y = 0;

        // for (auto&& plugBut : mPluginButs) {
        for (auto& plugBut : mPluginButs) {
            plugBut->setGeometry(x, y, mButtonWidth, butPluginHeigth);
            y += butPluginHeigth;
        }

        y = listRect.y() + listRect.height();
        const int w  = mButtonWidth;
        const int h = mButtonHeigth;

        mCalibBut->setGeometry(0, y, w, butPluginHeigth);
        mDeleteBut ->setGeometry(mCalibBut->width(), y, w, h);
        mRecycleBut->setGeometry(mDeleteBut->x() + mDeleteBut->width(), y, w, butPluginHeigth);
        mCombineBut->setGeometry(mRecycleBut->x() + mRecycleBut->width(), y, w, butPluginHeigth);
        mSplitBut->setGeometry(mCombineBut->x() + mCombineBut->width(), y, w, butPluginHeigth);
    }
    else if (hasBound()) {
            //-----------
            mEventView->resize(0, 0);
            // mCurveWidget belongs to mTopView
            if (withCurve) {
                mCurveWidget->setGeometry(margin, topViewHeight + margin, mTopView->width() - 2*margin, CurveHeight);

            } else {
                mCurveWidget->resize(0, 0);
            }
            mTopView->resize(width(), topViewHeight + (withCurve ? CurveHeight + margin : 0));

            mBoundView->setGeometry(0, mTopView->height(), width(), height() - mTopView->height());

    }

}

void EventPropertiesView::updateButton() {
    mDeleteBut->setEnabled(!isCalibChecked());
    mDeleteBut->update();
}


bool EventPropertiesView::hasEvent() const
{
    if (!mEventObj->isEmpty()) {
            Event::Type type = Event::Type (mEventObj->value(STATE_EVENT_TYPE).toInt());
            return (type == Event::eDefault);
    }
    return false;
}

bool EventPropertiesView::hasBound() const
{
    if (!mEventObj->isEmpty()) {
            Event::Type type = Event::Type (mEventObj->value(STATE_EVENT_TYPE).toInt());
        return (type == Event::eBound);
    }
    return false;
}

bool EventPropertiesView::hasEventWithDates() const
{
    if (!mEventObj->isEmpty()) {
        Event::Type type = Event::Type (mEventObj->value(STATE_EVENT_TYPE).toInt());
        if (type == Event::eDefault) {
                const QJsonArray dates = mEventObj->value(STATE_EVENT_DATES).toArray();
                return (dates.size() > 0);
        }
    }
    return false;
}
