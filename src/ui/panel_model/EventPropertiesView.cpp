/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2021

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
#include "Label.h"
#include "LineEdit.h"
#include "ColorPicker.h"
#include "Event.h"
#include "EventKnown.h"
#include "DatesList.h"
#include "Button.h"
#include "GraphView.h"
#include "Painting.h"
#include "MainWindow.h"
#include "Project.h"
#include "../PluginAbstract.h"
#include "PluginManager.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include "PluginOptionsDialog.h"
#include "CurveSettings.h"
#include "CurveWidget.h"
#include <QtWidgets>



EventPropertiesView::EventPropertiesView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),

    mButtonWidth  (int (1.3 * AppSettings::widthUnit()) * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE),
    mButtonHeigth  (int (1.3 * AppSettings::heigthUnit()) * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE),
    mLineEditHeight  (int (0.5 * AppSettings::heigthUnit())),
    mComboBoxHeight (int (0.7 * AppSettings::heigthUnit())),
    mCurveEnabled(false)

{
    minimumHeight = 0;
    qDebug() << "EventPropertiesView::EventPropertiesView mButtonWidth="<< mButtonWidth;
    // ------------- commun with defautlt Event and Bound ----------
    mTopView = new QWidget(this);

    mNameLab = new QLabel(tr("Name"), mTopView);
    mNameLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mNameEdit = new LineEdit(mTopView);

    mColorLab = new QLabel(tr("Color"), mTopView);
    mColorLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mColorPicker = new ColorPicker(Qt::black, mTopView);

    mMethodLab = new QLabel(tr("Method"), mTopView);
    mMethodLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mMethodCombo = new QComboBox(mTopView);
    mMethodInfo = new QLabel(tr("MH : proposal = adapt. Gaussian random walk"), mTopView);

    mMethodCombo->addItem(MHVariable::getSamplerProposalText(MHVariable::eDoubleExp));
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(MHVariable::eBoxMuller));
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(MHVariable::eMHAdaptGauss));
    
    connect(mNameEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventName);
    connect(mColorPicker, &ColorPicker::colorChanged, this, &EventPropertiesView::updateEventColor);
    connect(mMethodCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &EventPropertiesView::updateEventMethod);
    
    // field curve parameter

    mCurveWidget = new CurveWidget(mTopView);
    
    mX_IncLab = new QLabel(tr("Inclination"), mCurveWidget);
    mX_IncLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mY_DecLab = new QLabel(tr("Declination"), mCurveWidget);
    mY_DecLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mZ_IntLab = new QLabel(tr("Field"), mCurveWidget);
    mZ_IntLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    mX_IncEdit = new LineEdit(mCurveWidget);
    mY_DecEdit = new LineEdit(mCurveWidget);
    mZ_IntEdit = new LineEdit(mCurveWidget);

    
    connect(mX_IncEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventYInc);
    connect(mY_DecEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventYDec);
    connect(mZ_IntEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventYInt);
    
    mS_X_IncLab = new QLabel(tr("Error"), mCurveWidget);
    mS_X_IncLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mS_Y_Lab = new QLabel(tr("Error"), mCurveWidget);
    mS_Y_Lab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mS_Z_IntLab = new QLabel(tr("Error"), mCurveWidget);
    mS_Z_IntLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    mS_X_IncEdit = new LineEdit(mCurveWidget);
    mS_Y_Edit = new LineEdit(mCurveWidget);
    mS_Z_IntEdit = new LineEdit(mCurveWidget);
    
    connect(mS_X_IncEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventSInc);
    connect(mS_Y_Edit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventSDec);
    connect(mS_Z_IntEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventSInt);


    // Event default propreties Window mEventView
    mEventView = new QWidget(this);

    minimumHeight += mEventView->height();
    // -------------
    mDatesList = new DatesList(mEventView);
    connect(mDatesList, &DatesList::indexChange, this, &EventPropertiesView::updateIndex);
    connect(mDatesList, &DatesList::calibRequested, this, &EventPropertiesView::updateCalibRequested);

    // -------------

    const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();

    for (int i = 0; i<plugins.size(); ++i) {

        Button* button = new Button(plugins.at(i)->getName(), mEventView);
        button->setIcon(plugins.at(i)->getIcon());
        button->setFlatVertical();
        button->setIconOnly(false);
        button->setToolTip(tr("Insert %1 Data").arg(plugins.at(i)->getName()) );

        connect(button, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &EventPropertiesView::createDate);

        minimumHeight += button->height();

        if (plugins.at(i)->doesCalibration())
            mPluginButs1.append(button);
        else
            mPluginButs2.append(button);
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
    mCalibBut->setIcon(QIcon(":results_w.png"));
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

    mKnownFixedEdit = new QLineEdit(mBoundView);

    QDoubleValidator* doubleValidator = new QDoubleValidator(this);
    doubleValidator->setDecimals(2);

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

    mEvent = QJsonObject();
    mTopView->setVisible(false);
    mEventView->setVisible(false);
    mBoundView->setVisible(false);

    applyAppSettings();
}

EventPropertiesView::~EventPropertiesView()
{

}

// Event Managment
void EventPropertiesView::setEvent(const QJsonObject& event)
{
    // if set Event come becaus we use Project::updateDate(), we are on the same Event
    // so we are on the EventPropertiesView not on the EventScene
    if (mEvent[STATE_ID] != event[STATE_ID]) {
        mCalibBut->setChecked(false);
    }
    
    // Assign the local event
    mEvent = event;
    
    // Select the first date if the list is not empty
    const QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();
    bool hasDates = (dates.size() > 0);
    if (hasDates) {
        mCurrentDateIdx = 0;
    }
    
    if (rect().width() > 0)
        updateEvent();
}

void EventPropertiesView::updateIndex(int index)
{
    mCurrentDateIdx = index;
}

void EventPropertiesView::updateEvent()
{
    qDebug()<<"EventPropertiesView::updateEvent";

    if (mEvent.isEmpty()) {
        mTopView->setVisible(false);
        mEventView->setVisible(false);
        mBoundView->setVisible(false);

    } else {
        Event::Type type = Event::Type (mEvent.value(STATE_EVENT_TYPE).toInt());
        const QString name = mEvent.value(STATE_NAME).toString();
        const QColor color(mEvent.value(STATE_COLOR_RED).toInt(),
                     mEvent.value(STATE_COLOR_GREEN).toInt(),
                     mEvent.value(STATE_COLOR_BLUE).toInt());

        if (name != mNameEdit->text())
            mNameEdit->setText(name);

        mColorPicker->setColor(color);

        // Curve
        
        // The Curve settings may have changed since the last time the event property view has been opened
        const QJsonObject state = MainWindow::getInstance()->getProject()->mState;
        const CurveSettings settings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());
        mCurveEnabled = (settings.mProcessType != CurveSettings::eProcessTypeNone);

        //mCurveEnabled = settings.mEnabled;
        //mCurveProcessType = settings.mProcessType;
        
        mMethodLab->setVisible(type == Event::eDefault);
        mMethodCombo->setVisible( !mCurveEnabled && (type == Event::eDefault));
        mMethodInfo->setVisible( mCurveEnabled && (type == Event::eDefault));
        
        // Y1 contient l'inclinaison. Elle est toujours nécessaire en sphérique et vectoriel.
        // En univarié, elle n'est nécessaire que pour les variables d'étude : inclinaison ou déclinaison.
        bool showInc = mCurveEnabled && settings.showInclination();
        bool showDec = mCurveEnabled && settings.showDeclination();
        bool showInt = mCurveEnabled && settings.showIntensity();
        bool showYErr = mCurveEnabled && settings.showYErr();
        
        mX_IncLab->setVisible(showInc);
        mX_IncEdit->setVisible(showInc);

        mS_X_IncLab->setVisible(showInc);
        mS_X_IncEdit->setVisible(showInc);
        if (showInc) {
            mX_IncLab->setText(settings.inclinationLabel());
            mX_IncEdit->setText(locale().toString(mEvent.value(STATE_EVENT_X_INC_DEPTH).toDouble()));
            mS_X_IncEdit->setText(locale().toString(mEvent.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble()));
        }
        

        mY_DecLab->setVisible(showDec);
        mY_DecEdit->setVisible(showDec);

        if (showDec) {
            mY_DecLab->setText(settings.declinationLabel());
            mY_DecEdit->setText(locale().toString(mEvent.value(STATE_EVENT_Y_DEC).toDouble()));
        }

        mS_Y_Lab->setVisible(showYErr);
        mS_Y_Edit->setVisible(showYErr);

        if (showYErr) {
            mS_Y_Lab->setText(tr("Error"));
            mS_Y_Edit->setText(locale().toString(mEvent.value(STATE_EVENT_SY).toDouble()));
        }

        mZ_IntLab->setVisible(showInt);
        mZ_IntEdit->setVisible(showInt);
        
        mS_Z_IntLab->setVisible(showInt);
        mS_Z_IntEdit->setVisible(showInt);
        
        if (showInt) {
            mZ_IntLab->setText(settings.intensityLabel());
            mZ_IntEdit->setText(locale().toString(mEvent.value(STATE_EVENT_Z_F).toDouble()));
            mS_Z_IntEdit->setText(locale().toString(mEvent.value(STATE_EVENT_SZ_SF).toDouble()));
        }

        
        mTopView->setVisible(true);

        mEventView->setVisible(type == Event::eDefault);
        mBoundView->setVisible(type == Event::eKnown);

        if (type == Event::eDefault) {
            mMethodCombo->setCurrentIndex(mEvent.value(STATE_EVENT_SAMPLER).toInt());
            
            if (mEvent.value(STATE_EVENT_SAMPLER).toInt() == MHVariable::eDoubleExp)
                mMethodCombo->setCurrentIndex(0);

            else if (mEvent.value(STATE_EVENT_SAMPLER).toInt() == MHVariable::eBoxMuller)
                    mMethodCombo->setCurrentIndex(1);

            else if (mEvent.value(STATE_EVENT_SAMPLER).toInt() == MHVariable::eMHAdaptGauss)
                mMethodCombo->setCurrentIndex(2);




        //       qDebug() << "EventPropertiesView::updateEvent mEvent mOrigin"  << mEvent.value(STATE_EVENT_DATES).toArray().at(0).toObject().value(STATE_DATE_ORIGIN).toInt();
           mDatesList->setEvent(mEvent);
            if (mCurrentDateIdx >= 0)
                mDatesList->setCurrentRow(mCurrentDateIdx);

            const QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();
           
            const bool hasDates = (dates.size() > 0);
            if (hasDates && mCurrentDateIdx >= 0) {
                //emit updateCalibRequested(dates[mCurrentDateIdx].toObject());
                mCalibBut->setEnabled(true);
                mDeleteBut->setEnabled(true);
                mRecycleBut->setEnabled(true);

            } else {
               // mCalibBut->click();
                mCalibBut->setEnabled(false);
                mDeleteBut->setEnabled(false);
                mRecycleBut->setEnabled(true);
            }


        } else if (type == Event::eKnown) {
            mKnownFixedEdit -> setText(locale().toString(mEvent.value(STATE_EVENT_KNOWN_FIXED).toDouble()));
            updateKnownGraph();
        }
    }
    
    updateLayout();
    update();
}

const QJsonObject& EventPropertiesView::getEvent() const
{
    return mEvent;
}


void EventPropertiesView::updateEventName()
{
    QJsonObject event = mEvent;
    event[STATE_NAME] = mNameEdit->text();
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event name updated"));
}

void EventPropertiesView::updateEventColor(const QColor &color)
{
    QJsonObject event = mEvent;
    event[STATE_COLOR_RED] = color.red();
    event[STATE_COLOR_GREEN] = color.green();
    event[STATE_COLOR_BLUE] = color.blue();
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event color updated"));
}

void EventPropertiesView::updateEventMethod(int index)
{
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
    QJsonObject event = mEvent;
    event[STATE_EVENT_SAMPLER] = sp;
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event method updated"));
}

// Event Known Properties

void EventPropertiesView::updateKnownFixed(const QString& text)
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_FIXED] = round(text.toDouble());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Bound fixed value updated"));
}

// Curve

/*
void EventPropertiesView::setCurveSettings(const CurveSettings::ProcessType processType)
{
    mCurveEnabled = (processType != CurveSettings::eProcessTypeNone);
    //mCurveProcessType = processType;
    updateEvent();
}
*/


// Curve
void EventPropertiesView::updateEventYInc()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_X_INC_DEPTH] = locale().toDouble(mX_IncEdit->text());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event X-Inc updated"));
}

void EventPropertiesView::updateEventYDec()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_Y_DEC] = locale().toDouble(mY_DecEdit->text());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event Y-Dec updated"));
}

void EventPropertiesView::updateEventYInt()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_Z_F] = locale().toDouble(mZ_IntEdit->text());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event Z-Int updated"));
}

void EventPropertiesView::updateEventSInc()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_SX_ALPHA95_SDEPTH] = locale().toDouble(mS_X_IncEdit->text());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event S X-Inc updated"));
}

void EventPropertiesView::updateEventSDec()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_SY] = locale().toDouble(mS_Y_Edit->text());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event S Y updated"));
}

void EventPropertiesView::updateEventSInt()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_SZ_SF] = locale().toDouble(mS_Z_IntEdit->text());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event S Z-Int updated"));
}

void EventPropertiesView::updateKnownGraph()
{
    mKnownGraph->removeAllCurves();

    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    const QJsonObject settings = state.value(STATE_SETTINGS).toObject();
    const double tmin = settings.value(STATE_SETTINGS_TMIN).toDouble();
    const double tmax = settings.value(STATE_SETTINGS_TMAX).toDouble();
    const double step = settings.value(STATE_SETTINGS_STEP).toDouble();
    EventKnown event;
    event = EventKnown::fromJson(mEvent);

    if ( (tmin>event.mFixed) || (event.mFixed>tmax) )
        return;

    event.updateValues(tmin, tmax,step );

    mKnownGraph->setRangeX(tmin,tmax);

    mKnownGraph->setCurrentX(tmin,tmax);

    double max = map_max_value(event.mValues);
    max = (max == 0.) ? 1. : max;
    mKnownGraph->setRangeY(0, max);
    mKnownGraph->showYAxisValues(false);

    //---------------------

    GraphCurve curve;
    curve.mName = "Bound";
    curve.mBrush = Painting::mainColorLight;

    curve.mPen = QPen(Painting::mainColorLight, 2.);

    curve.mIsHorizontalSections = true;
    qreal tLower;
    qreal tUpper;
    tLower = event.fixedValue();
    tUpper = tLower;


    curve.mSections.append(qMakePair(tLower,tUpper));
    mKnownGraph->addCurve(curve);
    QFontMetrics fm (mKnownGraph->font());
    mKnownGraph->setMarginBottom(fm.ascent() + 10. );

    // Adjust scale
    const int xScale = int (log10(tmax-tmin)) -1;
    mKnownGraph->setXScaleDivision(std::pow(10, xScale), 4);
    //---------------------

}


void EventPropertiesView::createDate()
{
    if (!mEvent.isEmpty()) {
        Button* but = dynamic_cast<Button*>(sender());
        if (but) {
            Project* project = MainWindow::getInstance()->getProject();
            const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();

            for (int i=0; i<plugins.size(); ++i) {
                if (plugins.at(i)->getName() == but->text()) {
                    Date date = project->createDateFromPlugin(plugins.at(i));
                    //date.mUUID =QString::fromStdString(Generator::UUID()); // Add UUID since version 2.1.3
                    if (!date.isNull())
                        project->addDate(mEvent.value(STATE_ID).toInt(), date.toJson());
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

    MainWindow::getInstance()->getProject()->deleteDates(mEvent.value(STATE_ID).toInt(), indexes);
}

void EventPropertiesView::recycleDates()
{
    MainWindow::getInstance()->getProject()->recycleDates(mEvent.value(STATE_ID).toInt());
}

// Merge / Split
void EventPropertiesView::updateCombineAvailability()
{
    bool mergeable (false);
    bool splittable (false);

    QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();
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
    QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    QList<int> dateIds;

    for (int i = 0; i<items.size(); ++i) {
        const int idx = mDatesList->row(items.at(i));
        if (idx < dates.size()) {
            QJsonObject date = dates.at(idx).toObject();
            dateIds.push_back(date.value(STATE_ID).toInt());
        }
    }
    emit combineDatesRequested(mEvent.value(STATE_ID).toInt(), dateIds);
}

void EventPropertiesView::sendSplitDate()
{
    QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    if (items.size() > 0) {
        int idx = mDatesList->row(items[0]);
        if (idx < dates.size()) {
            QJsonObject date = dates.at(idx).toObject();
            int dateId = date.value(STATE_ID).toInt();
            emit splitDateRequested(mEvent.value(STATE_ID).toInt(), dateId);
        }
    }
}

/**
 * @brief EventPropertiesView::keyPressEvent Append only when the windows has the focus
 * @param event
 */
void EventPropertiesView::keyPressEvent(QKeyEvent* e)
{
    if ((e->key() == Qt::Key_Escape) && mCalibBut->isChecked())
        emit mCalibBut->click();

    else if ((e->key() == Qt::Key_C) && !mCalibBut->isChecked())
        emit mCalibBut->click();

    QWidget::keyPressEvent(e);

}

// Layout
void EventPropertiesView::paintEvent(QPaintEvent* e)
{
    (void) e;
    QWidget::paintEvent(e);
    QPainter p(this);
    //p.fillRect(rect(), palette().color(QPalette::Window));
    Event::Type type = Event::Type (mEvent.value(STATE_EVENT_TYPE).toInt());
    if (type == Event::eKnown)
       p.fillRect(rect(), palette().color(QPalette::Window));
    else
        p.fillRect(0, mTopView->height(), width(), height() - mTopView->y(), Painting::borderDark);
    // behind plugin button

    if (mEvent.isEmpty()) {
        QFont font = p.font();
        font.setBold(true);
        font.setPointSize(20);
        p.setFont(font);
        p.setPen(QColor(150, 150, 150));
        p.drawText(rect(), Qt::AlignCenter, tr("No event selected !"));
    }

}

void EventPropertiesView::resizeEvent(QResizeEvent* e)
{
    (void) e;
    updateLayout();
}

void EventPropertiesView::applyAppSettings()
{
    mButtonWidth = 50; //int (1.3 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = 50; //int (1.3 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mLineEditHeight = 25; //int (0.5 * AppSettings::heigthUnit());
    mComboBoxHeight = 25; //int(0.7 * AppSettings::heigthUnit());
    minimumHeight += mEventView->height();

    minimumHeight = 0;
    for (auto &&but : mPluginButs1) {
        but->resize(mButtonWidth, mButtonHeigth);
        minimumHeight += but->height();
    }

    for (auto &&but : mPluginButs2) {
        but->resize(mButtonWidth, mButtonHeigth);
        minimumHeight += but->height();
    }

    minimumHeight += mDeleteBut->height();

    if (isVisible())
        updateLayout();
}

void EventPropertiesView::updateLayout()
{
    if (mEvent.empty())
        return;

    QJsonObject state = MainWindow::getInstance()->getProject()->mState;
    const CurveSettings curveSettings = CurveSettings::fromJson(state.value(STATE_CURVE).toObject());
    const bool withCurve = (curveSettings.mProcessType != CurveSettings::eProcessTypeNone);

    mButtonWidth = 50; //int (1.3 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = 50; //int (1.3 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mLineEditHeight = 25; //int (0.5 * AppSettings::heigthUnit());
    mComboBoxHeight = 25; //int (0.7 * AppSettings::heigthUnit());

    QFontMetrics fm (font());
    const int margin = 10; //(int (0.2 * AppSettings::widthUnit()));

    int shiftMax (qMax(fm.boundingRect(mNameLab->text()).width(), qMax(fm.boundingRect(mColorLab->text()).width(), fm.boundingRect(mMethodLab->text()).width() )) );
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
        int lines = 1;
        if (curveSettings.showYErr()) {
            ++lines;
        }
        if (curveSettings.showInclination() && curveSettings.showIntensity()) {
            ++lines;
        }
       // const int lines = (curveSettings.showInclination() && curveSettings.showIntensity()) ? 2 : 1;
        CurveHeight = margin + (margin + mLineEditHeight) * lines;

        int dx = margin;
        int dy = margin;
        const int labW = 80;
        const  int YshiftLabel = (mLineEditHeight - mX_IncLab->height())/2;

        int editW;
        if (curveSettings.showInclination()) {
            if (curveSettings.showDeclination() && !curveSettings.showYErr()) {
                editW = (mCurveWidget->width() - 9*margin - 3*labW) / 3;

            } else {
                editW = (mCurveWidget->width() - 5*margin - 2*labW) / 2;
            }

           /* if (curveSettings.showDeclination()) {
                editW = (width() - 9*margin - 3*labW) / 3;
            }*/

            mX_IncLab->setGeometry(dx, dy - YshiftLabel, labW, mLineEditHeight);
            mX_IncEdit->setGeometry(dx += labW + margin, dy, editW, mLineEditHeight);
            mS_X_IncLab->setGeometry(dx += editW + margin, dy - YshiftLabel, labW, mLineEditHeight);
            mS_X_IncEdit->setGeometry(dx += labW + margin, dy, editW, mLineEditHeight);

            if (curveSettings.showDeclination() && !curveSettings.showYErr())  {
                mY_DecLab->setGeometry(dx += editW + margin, dy - YshiftLabel, labW, mLineEditHeight);
                mY_DecEdit->setGeometry(dx + labW + margin, dy, editW, mLineEditHeight);
            }

            dy += mLineEditHeight + margin;
        }

        if (curveSettings.showYErr()) {
            dx = margin;
            editW = (mCurveWidget->width() - 5*margin - 2*labW) / 2;

            mY_DecLab->setGeometry(dx, dy - YshiftLabel, labW, mLineEditHeight);
            mY_DecEdit->setGeometry(dx += labW + margin, dy, editW, mLineEditHeight);
            mS_Y_Lab->setGeometry(dx += editW + margin, dy - YshiftLabel, labW, mLineEditHeight);
            mS_Y_Edit->setGeometry(dx + labW + margin, dy, editW, mLineEditHeight);

            dy += mLineEditHeight + margin;
        }


        if (curveSettings.showIntensity()) {
            dx = margin;
            editW = (mCurveWidget->width() - 5*margin - 2*labW) / 2;

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

        // Plugin with calibration,
         for (auto&& plugBut : mPluginButs1) {
            plugBut->setGeometry(x, y, mButtonWidth, butPluginHeigth);
            y += butPluginHeigth;
        }

        // plugin without calibration

        for (auto&& plugBut : mPluginButs2) {
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
    else {
        if (hasBound()) {
            //-----------
            mEventView->resize(0, 0);
            // mCurveWidget belongs to mTopView
            if (withCurve) {
                mCurveWidget->setGeometry(margin, topViewHeight + margin, mTopView->width() - 2*margin, CurveHeight);

            } else {
                mCurveWidget->resize(0, 0);
            }
            mTopView->resize(width(), topViewHeight + (withCurve ? CurveHeight + margin : 0));

            mBoundView->setGeometry(0, mCurveWidget->y() + mCurveWidget->height() + margin, width(), height() - mCurveWidget->y() - mCurveWidget->height() - 2*margin);

        }
    }
}

void EventPropertiesView::updateButton() {
    mDeleteBut->setEnabled(!isCalibChecked());
    mDeleteBut->update();
}

void EventPropertiesView::setCalibChecked(bool checked)
{
    mCalibBut->setChecked(checked);
}

bool EventPropertiesView::isCalibChecked() const
{
    return mCalibBut->isChecked();
}

bool EventPropertiesView::hasEvent() const
{
    if (!mEvent.isEmpty()) {
        Event::Type type = Event::Type (mEvent.value(STATE_EVENT_TYPE).toInt());
        return (type == Event::eDefault);
    }
    return false;
}

bool EventPropertiesView::hasBound() const
{
    if (!mEvent.isEmpty()) {
        Event::Type type = Event::Type (mEvent.value(STATE_EVENT_TYPE).toInt());
        return (type == Event::eKnown);
    }
    return false;
}

bool EventPropertiesView::hasEventWithDates() const
{
    if (!mEvent.isEmpty()) {
        Event::Type type = Event::Type (mEvent.value(STATE_EVENT_TYPE).toInt());
        if (type == Event::eDefault) {
            const QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();
            return (dates.size() > 0);
        }
    }
    return false;
}
