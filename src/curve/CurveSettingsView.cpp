/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2020 - 2023

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

#include "CurveSettingsView.h"

#include "Project.h"
#include "Painting.h"
#include "QtUtilities.h"

#include <QtWidgets>
#include <QVariant>


CurveSettingsView::CurveSettingsView(QWidget* parent):QWidget(parent)
{
    mTitleLabel = new QLabel(tr("Curve Parameters"), this);
    mTitleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(20);
    mTitleLabel->setFont(titleFont);
    QPalette palette = mTitleLabel->palette();
    palette.setColor(QPalette::WindowText, CURVE_COLOR_TEXT);
    mTitleLabel->setPalette(palette);
    
    mDescriptionLabel = new QLabel(tr("These parameters give you controls over the way curves are built. MCMC (Bayesian) can be activated for event time, VG or global lambda spline factor, etc. "), this);
    mDescriptionLabel->setAlignment(Qt::AlignCenter);
    mDescriptionLabel->setWordWrap(true);

    palette = mDescriptionLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::gray);
    mDescriptionLabel->setPalette(palette);
    
    mProcessTypeLabel = new QLabel(tr("Process") , this);
    mProcessTypeInput = new QComboBox(this);
    mProcessTypeInput->addItem(tr("None"));
    mProcessTypeInput->addItem(tr("Univariate (1D)"));
    mProcessTypeInput->addItem(tr("Bi-variate (2D)"));
    mProcessTypeInput->addItem(tr("Tri-variate (3D)"));

    mProcessTypeInput->insertSeparator(4);
    mProcessTypeInput->addItem(tr("Depth"));
    mProcessTypeInput->insertSeparator(6);

    mProcessTypeInput->addItem(tr("Inclination"));
    mProcessTypeInput->addItem(tr("Declination"));
    mProcessTypeInput->addItem(tr("Field Intensity"));
    mProcessTypeInput->addItem(tr("Spherical (I, D)"));
    mProcessTypeInput->addItem(tr("Unknown Dec (I, F)"));
    mProcessTypeInput->addItem(tr("Vector (I, D, F)"));

    mThresholdLabel = new QLabel(tr("Minimal Rate of Change"), this);
    mThresholdInput = new QLineEdit(this);

    mUseErrMesureLabel = new QLabel(tr("Use Measurement Err."), this);
    mUseErrMesureInput = new QCheckBox(this);
    
    mTimeTypeLabel = new QLabel(tr("Event Date"), this);
    mTimeTypeInput = new QComboBox(this);
    mTimeTypeInput->addItem(tr("Fixed"));
    mTimeTypeInput->addItem(tr("Bayesian"));
    
    mLambdaSplineTypeLabel = new QLabel(tr("Smoothing"), this);
    mLambdaSplineTypeInput = new QComboBox(this);
    mLambdaSplineTypeInput->addItem(tr("Fixed"));
    mLambdaSplineTypeInput->addItem(tr("Bayesian"));
    mLambdaSplineTypeInput->addItem(tr("Interpolation (λ=0)"));

    mLambdaSplineLabel = new QLabel(tr("log 10 Smoothing Value"), this);
    mLambdaSplineInput = new QLineEdit(this);
    mLambdaSplineInput->setText("-6");

    mVarianceTypeLabel = new QLabel(tr("Std gi"), this);
    mVarianceTypeInput = new QComboBox(this);
    //mVarianceTypeInput->addItem(tr("Fixed"));
   // mVarianceTypeInput->addItem(tr("Bayesian"));

    mVarianceTypeInput->addItem(tr("Ind. Bayesian"));
    mVarianceTypeInput->addItem(tr("Global Bayesian"));
    mVarianceTypeInput->addItem(tr("Global Fixed"));

    
    //mUseVarianceIndividualLabel = new QLabel(tr("Individual Std gi"), this);
    //mUseVarianceIndividualCB = new QCheckBox(this);
    
    mVarianceValueLabel = new QLabel(tr("Std gi = Global Value"), this);
    mVarianceValueInput = new QLineEdit(this);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(5);
    int row = -1;
    
    grid->addWidget(mProcessTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mProcessTypeInput, row, 1);

    grid->addWidget(mThresholdLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mThresholdInput, row, 1);

    grid->setVerticalSpacing(15);
    grid->addWidget(mUseErrMesureLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mUseErrMesureInput, row, 1);
    
    grid->addWidget(mTimeTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mTimeTypeInput, row, 1);
    
    grid->addWidget(mLambdaSplineTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLambdaSplineTypeInput, row, 1);

    grid->addWidget(mLambdaSplineLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLambdaSplineInput, row, 1);

    grid->addWidget(mVarianceTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mVarianceTypeInput, row, 1);
    
    //grid->addWidget(mUseVarianceIndividualLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    //grid->addWidget(mUseVarianceIndividualCB, row, 1);
    
    grid->addWidget(mVarianceValueLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mVarianceValueInput, row, 1);

    
    QVBoxLayout* vlayout = new QVBoxLayout();
    //vlayout->setMargin(20);
    vlayout->addSpacing(20);
    vlayout->addWidget(mTitleLabel);
    vlayout->addSpacing(20);
    vlayout->addWidget(mDescriptionLabel);
    vlayout->addSpacing(30);
    vlayout->addLayout(grid);
    vlayout->addStretch();
    
    QWidget* vlayoutWidget = new QWidget();
    vlayoutWidget->setFixedWidth(400);
    vlayoutWidget->setLayout(vlayout);
    
    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    hlayout->addStretch();
    hlayout->addWidget(vlayoutWidget);
    hlayout->addStretch();
    
    setLayout(hlayout);

}

CurveSettingsView::~CurveSettingsView()
{
    
}

void CurveSettingsView::setConnections(const bool doConnections)
{
    if (doConnections) {
        // updateVisibilities
        connect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);

        connect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);

        connect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);

        //connect(mUseVarianceIndividualCB, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::updateVisibilities);

        // Save
        connect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);

        connect(mThresholdInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);

        connect(mUseErrMesureInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::save);

        connect(mTimeTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);

        connect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);

        //connect(mUseVarianceIndividualCB, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::save);

        connect(mVarianceValueInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);

        connect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);

        connect(mLambdaSplineInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);

    } else {
        // updateVisibilities
        disconnect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);

        disconnect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);

        disconnect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);

        //disconnect(mUseVarianceIndividualCB, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::updateVisibilities);

        // Save
        disconnect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);

        disconnect(mThresholdInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);

        disconnect(mUseErrMesureInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::save);

        disconnect(mTimeTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);

        disconnect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);

        //disconnect(mUseVarianceIndividualCB, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::save);

        disconnect(mVarianceValueInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);

        disconnect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);

        disconnect(mLambdaSplineInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);
    }

}

void CurveSettingsView::setSettings(const CurveSettings &settings)
{
    // Separators must be counted :index 4 and 6
    switch (settings.mProcessType) {
        case CurveSettings::eProcess_None:
            mProcessTypeInput->setCurrentIndex(0);
            break;
        case CurveSettings::eProcess_Univariate:
            mProcessTypeInput->setCurrentIndex(1);
            break;
        case CurveSettings::eProcess_2D:
            mProcessTypeInput->setCurrentIndex(2);
            break;
        case CurveSettings::eProcess_3D:
            mProcessTypeInput->setCurrentIndex(3);
            break;
        case CurveSettings::eProcess_Depth:
            mProcessTypeInput->setCurrentIndex(5);
            break;
        case CurveSettings::eProcess_Inclination:
            mProcessTypeInput->setCurrentIndex(7);
            break;
        case CurveSettings::eProcess_Declination:
            mProcessTypeInput->setCurrentIndex(8);
            break;
        case CurveSettings::eProcess_Field:
            mProcessTypeInput->setCurrentIndex(9);
            break;
        case CurveSettings::eProcess_Spherical:
            mProcessTypeInput->setCurrentIndex(10);
            break;
        case CurveSettings::eProcess_Unknwon_Dec:
            mProcessTypeInput->setCurrentIndex(11);
            break;
        case CurveSettings::eProcess_Vector:
            mProcessTypeInput->setCurrentIndex(12);
            break;
    }

    
    mUseErrMesureInput->setChecked(settings.mUseErrMesure);
    
    if (settings.mTimeType == CurveSettings::eModeFixed) {
        mTimeTypeInput->setCurrentIndex(0);

    } else if (settings.mTimeType == CurveSettings::eModeBayesian) {
        mTimeTypeInput->setCurrentIndex(1);
    }
    
    if (settings.mVarianceType == CurveSettings::eModeBayesian) {
        mVarianceTypeInput->setCurrentIndex(0);

    } else if (settings.mVarianceType == CurveSettings::eModeGlobal) {
        mVarianceTypeInput->setCurrentIndex(1);

    } else if (settings.mVarianceType == CurveSettings::eModeFixed) {
        mVarianceTypeInput->setCurrentIndex(2);
    }
    mThresholdInput->setText(stringForLocal(settings.mThreshold));


    //mUseVarianceIndividualCB->setChecked(settings.mUseVarianceIndividual);
    mVarianceValueInput->setText(stringForLocal(sqrt(settings.mVarianceFixed)));
    
    if (settings.mLambdaSplineType == CurveSettings::eModeFixed) {
        mLambdaSplineTypeInput->setCurrentIndex(0);
        mLambdaSplineInput->setText(stringForLocal(log10(settings.mLambdaSpline)));

    } else if (settings.mLambdaSplineType == CurveSettings::eModeBayesian) {
        mLambdaSplineTypeInput->setCurrentIndex(1);

    } else if (settings.mLambdaSplineType == CurveSettings::eInterpolation) {
        mLambdaSplineTypeInput->setCurrentIndex(2);
        mLambdaSplineInput->setText("0");
    }
    
    updateVisibilities();
}

CurveSettings CurveSettingsView::getSettings() const
{
    CurveSettings settings;
    // Separators must be counted : index 4 and 6
    switch (mProcessTypeInput->currentIndex()) {
        case 1:
            settings.mProcessType = CurveSettings::eProcess_Univariate;
            break;
        case 2:
            settings.mProcessType = CurveSettings::eProcess_2D;
            break;
        case 3:
            settings.mProcessType = CurveSettings::eProcess_3D;
            break;
        case 5:
            settings.mProcessType = CurveSettings::eProcess_Depth;
            break;
        case 7:
            settings.mProcessType = CurveSettings::eProcess_Inclination;
            break;
        case 8:
            settings.mProcessType = CurveSettings::eProcess_Declination;
            break;
        case 9:
            settings.mProcessType = CurveSettings::eProcess_Field;
            break;
        case 10:
            settings.mProcessType = CurveSettings::eProcess_Spherical;
            break;
        case 11:
            settings.mProcessType = CurveSettings::eProcess_Unknwon_Dec;
            break;
        case 12:
            settings.mProcessType = CurveSettings::eProcess_Vector;
            break;

        case 0:
        default:
            settings.mProcessType = CurveSettings::eProcess_None;
            break;
    }
    
    settings.mUseErrMesure = mUseErrMesureInput->isChecked();
    
    switch (mTimeTypeInput->currentIndex()) {
    case 0:
        settings.mTimeType = CurveSettings::eModeFixed;
        break;

    case 1:
    default:
        settings.mTimeType = CurveSettings::eModeBayesian;
        break;
    }

    switch (mVarianceTypeInput->currentIndex()) {
    case 2:
        settings.mVarianceType = CurveSettings::eModeFixed;
        break;

    case 1:
        settings.mVarianceType = CurveSettings::eModeGlobal;
        break;

    case 0:
    default:
        settings.mVarianceType = CurveSettings::eModeBayesian;
        break;
    }

    //settings.mUseVarianceIndividual = mUseVarianceIndividualCB->isChecked() && (mVarianceTypeInput->currentIndex() == 1) ;
    settings.mVarianceFixed = pow(locale().toDouble(mVarianceValueInput->text()), 2.);


    switch (mLambdaSplineTypeInput->currentIndex()) {
    case 0:
        settings.mLambdaSplineType = CurveSettings::eModeFixed;
        if (locale().toDouble(mLambdaSplineInput->text())>10)
            settings.mLambdaSpline = pow(10., 10.);
        else
            settings.mLambdaSpline = pow(10., locale().toDouble(mLambdaSplineInput->text()));
        break;

    case 2:
        settings.mLambdaSplineType = CurveSettings::eInterpolation;
        settings.mLambdaSpline = 0;
        break;

    case 1:
    default:
        settings.mLambdaSplineType = CurveSettings::eModeBayesian;
        break;

    }
    
    return settings;
}

void CurveSettingsView::setProject(Project *project)
{
    if (mProject != nullptr)
        setConnections(false);

    mProject = project;

    const CurveSettings curveSettings (mProject->mState.value(STATE_CURVE).toObject());

    setSettings(curveSettings);

    setConnections(true);
}

void CurveSettingsView::reset()
{
    CurveSettings settings = CurveSettings::getDefault();
    setSettings(settings);
}

void CurveSettingsView::save()
{
    //const CurveSettings &curveSettings = getSettings();
    QJsonObject stateNext = mProject->mState;
    stateNext[STATE_CURVE] = getSettings().toJson();
    mProject->pushProjectState(stateNext, CURVE_SETTINGS_UPDATED_REASON, true);
    emit newProcess(mProcessTypeInput->currentText());
}

void CurveSettingsView::updateVisibilities()
{
    if (mProcessTypeInput->currentText() == "None") {

        mUseErrMesureLabel->setVisible(false);
        mUseErrMesureInput->setVisible(false);

        mTimeTypeLabel->setVisible(false);
        mTimeTypeInput->setVisible(false);

        mThresholdLabel->setVisible(false);
        mThresholdInput->setVisible(false);

        mVarianceTypeLabel->setVisible(false);
        mVarianceTypeInput->setVisible(false);

        mVarianceValueLabel->setVisible(false);
        mVarianceValueInput->setVisible(false);

        mLambdaSplineTypeLabel->setVisible(false);
        mLambdaSplineTypeInput->setVisible(false);

        mLambdaSplineLabel->setVisible(false);
        mLambdaSplineInput->setVisible(false);

    } else {

        mUseErrMesureLabel->setVisible(true);
        mUseErrMesureInput->setVisible(true);
        mTimeTypeLabel->setVisible(true);
        mTimeTypeInput->setVisible(true);

        const bool showThreshold = (mProcessTypeInput->currentIndex() ==  5);//"Depth");
        mThresholdLabel->setVisible(showThreshold);
        mThresholdInput->setVisible(showThreshold);

        mLambdaSplineTypeLabel->setVisible(true);
        mLambdaSplineTypeInput->setVisible(true);
        const bool lambdaSplineFixed = (mLambdaSplineTypeInput->currentText() == "Fixed");
        const bool lambdaSplineInterpol = (mLambdaSplineTypeInput->currentIndex() == 2);
        mLambdaSplineLabel->setVisible(lambdaSplineFixed);
        mLambdaSplineInput->setVisible(lambdaSplineFixed);

        mVarianceTypeLabel->setVisible(!lambdaSplineInterpol);
        mVarianceTypeInput->setVisible(!lambdaSplineInterpol);
        const bool varianceFixed = (mVarianceTypeInput->currentIndex() == 2);// "Global fixed"

        mVarianceValueLabel->setVisible(!lambdaSplineInterpol && varianceFixed );
        mVarianceValueInput->setVisible(!lambdaSplineInterpol && varianceFixed );


    }
    
}

