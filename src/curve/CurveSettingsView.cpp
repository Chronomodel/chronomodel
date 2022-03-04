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

#include "CurveSettingsView.h"
#include "Project.h"
#include "Painting.h"
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
    //QFont descFont;
    //mDescriptionLabel->setFont(titleFont);
    palette = mDescriptionLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::gray);
    mDescriptionLabel->setPalette(palette);
    
    mProcessTypeLabel = new QLabel(tr("Process") , this);
    mProcessTypeInput = new QComboBox(this);
    mProcessTypeInput->addItem(tr("None"));
    mProcessTypeInput->addItem(tr("Univariate"));
    mProcessTypeInput->addItem(tr("2D"));
    mProcessTypeInput->addItem(tr("Spherical"));
    mProcessTypeInput->addItem(tr("Vector"));
    mProcessTypeInput->addItem(tr("3D"));
    
    mVariableTypeLabel = new QLabel(tr("Variable"), this);
    mVariableTypeInput = new QComboBox(this);
    mVariableTypeInput->addItem(tr("Inclination"));
    mVariableTypeInput->addItem(tr("Declination"));
    mVariableTypeInput->addItem(tr("Field"));
    mVariableTypeInput->addItem(tr("Depth"));
    mVariableTypeInput->addItem(tr("Any Measure"));
    
    mThresholdLabel = new QLabel(tr("Speed threshold"), this);
    mThresholdInput = new QLineEdit(this);

    mUseErrMesureLabel = new QLabel(tr("Use Measurement Err."), this);
    mUseErrMesureInput = new QCheckBox(this);
    
    mTimeTypeLabel = new QLabel(tr("Time Type"), this);
    mTimeTypeInput = new QComboBox(this);
    mTimeTypeInput->addItem(tr("Fixed"));
    mTimeTypeInput->addItem(tr("Bayesian"));
    
    mVarianceTypeLabel = new QLabel(tr("Variance"), this);
    mVarianceTypeInput = new QComboBox(this);
    mVarianceTypeInput->addItem(tr("Fixed"));
    mVarianceTypeInput->addItem(tr("Bayesian"));
    
    mUseVarianceIndividualLabel = new QLabel(tr("Variance Individual"), this);
    mUseVarianceIndividualInput = new QCheckBox(this);
    
    mVarianceFixedLabel = new QLabel(tr("Variance Value"), this);
    mVarianceFixedInput = new QLineEdit(this);
    
    mLambdaSplineTypeLabel = new QLabel(tr("Smoothing"), this);
    mLambdaSplineTypeInput = new QComboBox(this);
    mLambdaSplineTypeInput->addItem(tr("Fixed"));
    mLambdaSplineTypeInput->addItem(tr("Bayesian"));
    
    mLambdaSplineLabel = new QLabel(tr("Smoothing Value"), this);
    mLambdaSplineInput = new QLineEdit(this);
    
    
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(5);
    int row = -1;
    
    grid->addWidget(mProcessTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mProcessTypeInput, row, 1);

    grid->addWidget(mVariableTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mVariableTypeInput, row, 1);

    grid->addWidget(mThresholdLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mThresholdInput, row, 1);

    grid->setVerticalSpacing(15);
    grid->addWidget(mUseErrMesureLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mUseErrMesureInput, row, 1);
    
    grid->addWidget(mTimeTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mTimeTypeInput, row, 1);
    
    /*grid->addWidget(mUseTimeBayesianEventLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mUseTimeBayesianEventInput, row, 1);
    
    grid->addWidget(mUseTimeBayesianConstraintLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mUseTimeBayesianConstraintInput, row, 1);*/
    
    grid->addWidget(mVarianceTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mVarianceTypeInput, row, 1);
    
    grid->addWidget(mUseVarianceIndividualLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mUseVarianceIndividualInput, row, 1);
    
    grid->addWidget(mVarianceFixedLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mVarianceFixedInput, row, 1);
    
    grid->addWidget(mLambdaSplineTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLambdaSplineTypeInput, row, 1);
    
    grid->addWidget(mLambdaSplineLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLambdaSplineInput, row, 1);
    
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
    
    updateVisibilities();
    
    connect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);
    
    connect(mVariableTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);
    
    connect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);
    
    connect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);
    
    
    
    connect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
    
    connect(mVariableTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);

    connect(mThresholdInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);
    
    connect(mUseErrMesureInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::save);
    
    connect(mTimeTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
    
    connect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
    
    connect(mUseVarianceIndividualInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::save);
    
    connect(mVarianceFixedInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);
    
    connect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
    
    connect(mLambdaSplineInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);
}

CurveSettingsView::~CurveSettingsView()
{
    
}

void CurveSettingsView::setSettings(const CurveSettings& settings)
{
    mProcessTypeInput->blockSignals(true);
    mVariableTypeInput->blockSignals(true);
    mThresholdInput->blockSignals(true);
    mUseErrMesureInput->blockSignals(true);
    mTimeTypeInput->blockSignals(true);
    mVarianceTypeInput->blockSignals(true);
    mUseVarianceIndividualInput->blockSignals(true);
    mVarianceFixedInput->blockSignals(true);
    mLambdaSplineTypeInput->blockSignals(true);
    mLambdaSplineInput->blockSignals(true);

    switch (settings.mProcessType) {
    case CurveSettings::eProcessTypeNone:
        mProcessTypeInput->setCurrentIndex(0);
        break;
    case CurveSettings::eProcessTypeUnivarie:
        mProcessTypeInput->setCurrentIndex(1);
        break;
    case CurveSettings::eProcessType2D:
        mProcessTypeInput->setCurrentIndex(2);
        break;
    case CurveSettings::eProcessTypeSpherical:
        mProcessTypeInput->setCurrentIndex(3);
        break;
    case CurveSettings::eProcessTypeVector:
        mProcessTypeInput->setCurrentIndex(4);
        break;
    case CurveSettings::eProcessType3D:
        mProcessTypeInput->setCurrentIndex(5);
        break;

    }

    switch (settings.mVariableType) {
    case CurveSettings::eVariableTypeInclination:
        mVariableTypeInput->setCurrentIndex(0);
        break;
    case CurveSettings::eVariableTypeDeclination:
        mVariableTypeInput->setCurrentIndex(1);
        break;
    case CurveSettings::eVariableTypeField:
        mVariableTypeInput->setCurrentIndex(2);
        break;
    case CurveSettings::eVariableTypeDepth:
        mVariableTypeInput->setCurrentIndex(3);
        break;
    case CurveSettings::eVariableTypeOther:
        mVariableTypeInput->setCurrentIndex(4);
        break;
    }
    
    
    mUseErrMesureInput->setChecked(settings.mUseErrMesure);
    
    if (settings.mTimeType == CurveSettings::eModeFixed) {
        mTimeTypeInput->setCurrentIndex(0);

    } else if (settings.mTimeType == CurveSettings::eModeBayesian) {
        mTimeTypeInput->setCurrentIndex(1);
    }
    
    if (settings.mVarianceType == CurveSettings::eModeFixed) {
        mVarianceTypeInput->setCurrentIndex(0);

    } else if (settings.mVarianceType == CurveSettings::eModeBayesian) {
        mVarianceTypeInput->setCurrentIndex(1);
    }
    mThresholdInput->setText(QString::number(settings.mThreshold));


    mUseVarianceIndividualInput->setChecked(settings.mUseVarianceIndividual);
    mVarianceFixedInput->setText(QString::number(settings.mVarianceFixed));
    
    if (settings.mLambdaSplineType == CurveSettings::eModeFixed) {
        mLambdaSplineTypeInput->setCurrentIndex(0);

    } else if (settings.mLambdaSplineType == CurveSettings::eModeBayesian) {
        mLambdaSplineTypeInput->setCurrentIndex(1);
    }
    
    mLambdaSplineInput->setText(QString::number(settings.mLambdaSpline));
    
    mProcessTypeInput->blockSignals(false);
    mVariableTypeInput->blockSignals(false);
    mThresholdInput->blockSignals(false);
    mUseErrMesureInput->blockSignals(false);
    mTimeTypeInput->blockSignals(false);
    mVarianceTypeInput->blockSignals(false);
    mUseVarianceIndividualInput->blockSignals(false);
    mVarianceFixedInput->blockSignals(false);
    mLambdaSplineTypeInput->blockSignals(false);
    mLambdaSplineInput->blockSignals(false);

    updateVisibilities();
}

CurveSettings CurveSettingsView::getSettings()
{
    CurveSettings settings;

    if (mProcessTypeInput->currentIndex() == 0) {
        settings.mProcessType = CurveSettings::eProcessTypeNone;

    } else if (mProcessTypeInput->currentIndex() == 1) {
        settings.mProcessType = CurveSettings::eProcessTypeUnivarie;

        switch (mVariableTypeInput->currentIndex()) {
        case 0:
            settings.mVariableType = CurveSettings::eVariableTypeInclination;
            break;
        case 1:
            settings.mVariableType = CurveSettings::eVariableTypeDeclination;
            break;
        case 2:
            settings.mVariableType = CurveSettings::eVariableTypeField;
            break;
        case 3:
            settings.mVariableType = CurveSettings::eVariableTypeDepth;
            settings.mThreshold = mThresholdInput->text().toDouble();
            break;
        case 4:
            settings.mVariableType = CurveSettings::eVariableTypeOther;
            break;

        }

    } else if (mProcessTypeInput->currentIndex() == 2) {
        settings.mProcessType = CurveSettings::eProcessType2D;
        settings.mVariableType = CurveSettings::eVariableTypeInclination;

    } else if (mProcessTypeInput->currentIndex() == 3) {
        settings.mProcessType = CurveSettings::eProcessTypeSpherical;
        settings.mVariableType = CurveSettings::eVariableTypeInclination;

    } else if (mProcessTypeInput->currentIndex() == 4) {
        settings.mProcessType = CurveSettings::eProcessTypeVector;
        settings.mVariableType = CurveSettings::eVariableTypeInclination;

    } else if (mProcessTypeInput->currentIndex() == 5) {
        settings.mProcessType = CurveSettings::eProcessType3D;
        settings.mVariableType = CurveSettings::eVariableTypeOther;
    }
    

    
    settings.mUseErrMesure = mUseErrMesureInput->isChecked();
    
    if (mTimeTypeInput->currentIndex() == 0) {
        settings.mTimeType = CurveSettings::eModeFixed;

    } else if (mTimeTypeInput->currentIndex() == 1) {
        settings.mTimeType = CurveSettings::eModeBayesian;
    }
    
    if (mVarianceTypeInput->currentIndex() == 0) {
        settings.mVarianceType = CurveSettings::eModeFixed;

    } else if (mVarianceTypeInput->currentIndex() == 1) {
        settings.mVarianceType = CurveSettings::eModeBayesian;
    }
        
    settings.mUseVarianceIndividual = mUseVarianceIndividualInput->isChecked();
    settings.mVarianceFixed = mVarianceFixedInput->text().toDouble();
    
    if (mLambdaSplineTypeInput->currentIndex() == 0) {
        settings.mLambdaSplineType = CurveSettings::eModeFixed;

    } else if (mLambdaSplineTypeInput->currentIndex() == 1) {
        settings.mLambdaSplineType = CurveSettings::eModeBayesian;
    }
    
    settings.mLambdaSpline = mLambdaSplineInput->text().toDouble();
    
    return settings;
}

void CurveSettingsView::setProject(Project *project)
{
    mProject = project;
    blockSignals(true);


    CurveSettings curveSettings;
    curveSettings.fromJson(mProject->mState.value(STATE_CURVE).toObject());

    setSettings(curveSettings);
    blockSignals(false);
}

void CurveSettingsView::reset()
{
    CurveSettings settings = CurveSettings::getDefault();
    setSettings(settings);
}

void CurveSettingsView::save()
{
    CurveSettings curveSettings = getSettings();
    QJsonObject stateNext = mProject->mState;
    stateNext[STATE_CURVE] = curveSettings.toJson();
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

        mVariableTypeLabel->setVisible(false);
        mVariableTypeInput->setVisible(false);

        mThresholdLabel->setVisible(false);
        mThresholdInput->setVisible(false);

        mVarianceTypeLabel->setVisible(false);
        mVarianceTypeInput->setVisible(false);

        mVarianceFixedLabel->setVisible(false);
        mVarianceFixedInput->setVisible(false);

        mUseVarianceIndividualLabel->setVisible(false);
        mUseVarianceIndividualInput->setVisible(false);

        mLambdaSplineTypeLabel->setVisible(false);
        mLambdaSplineTypeInput->setVisible(false);

        mLambdaSplineLabel->setVisible(false);
        mLambdaSplineInput->setVisible(false);

    } else {

        mUseErrMesureLabel->setVisible(true);
        mUseErrMesureInput->setVisible(true);
        mTimeTypeLabel->setVisible(true);
        mTimeTypeInput->setVisible(true);

        const bool variableTypeRequired = (mProcessTypeInput->currentText() == "Univariate");
        mVariableTypeLabel->setVisible(variableTypeRequired);
        mVariableTypeInput->setVisible(variableTypeRequired);

        const bool showThreshold = (mVariableTypeInput->currentText() == "Depth");
        mThresholdLabel->setVisible(showThreshold);
        mThresholdInput->setVisible(showThreshold);

        mVarianceTypeLabel->setVisible(true);
        mVarianceTypeInput->setVisible(true);
        const bool varianceFixed = (mVarianceTypeInput->currentText() == "Fixed");
        mVarianceFixedLabel->setVisible(varianceFixed);
        mVarianceFixedInput->setVisible(varianceFixed);
        mUseVarianceIndividualLabel->setVisible(!varianceFixed);
        mUseVarianceIndividualInput->setVisible(!varianceFixed);

        mLambdaSplineTypeLabel->setVisible(true);
        mLambdaSplineTypeInput->setVisible(true);
        const bool lambdaSplineFixed = (mLambdaSplineTypeInput->currentText() == "Fixed");
        mLambdaSplineLabel->setVisible(lambdaSplineFixed);
        mLambdaSplineInput->setVisible(lambdaSplineFixed);
    }
    
}

