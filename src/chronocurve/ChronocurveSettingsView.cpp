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

#include "ChronocurveSettingsView.h"
#include "Project.h"
#include "Painting.h"
#include <QtWidgets>
#include <QVariant>


ChronocurveSettingsView::ChronocurveSettingsView(QWidget* parent):QWidget(parent)
{
    mTitleLabel = new QLabel(tr("Curve Parameters"), this);
    mTitleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(20);
    mTitleLabel->setFont(titleFont);
    QPalette palette = mTitleLabel->palette();
    palette.setColor(QPalette::WindowText, CHRONOCURVE_COLOR_TEXT);
    mTitleLabel->setPalette(palette);
    
    mDescriptionLabel = new QLabel(tr("These parameters give you controls over the way curves are built. MCMC (Bayesian) can be activated for event time, VG or global alpha smoothing factor, etc blablabla"), this);
    mDescriptionLabel->setAlignment(Qt::AlignCenter);
    mDescriptionLabel->setWordWrap(true);
    //QFont descFont;
    //mDescriptionLabel->setFont(titleFont);
    palette = mDescriptionLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::gray);
    mDescriptionLabel->setPalette(palette);
    
    mProcessTypeLabel = new QLabel(tr("Process Type") + " :", this);
    mProcessTypeInput = new QComboBox(this);
    mProcessTypeInput->addItem(tr("Univariate"));
    mProcessTypeInput->addItem(tr("Spherical"));
    mProcessTypeInput->addItem(tr("Vectorial"));
    mProcessTypeInput->addItem(tr("3D"));
    
    mVariableTypeLabel = new QLabel(tr("Variable Type") + " :", this);
    mVariableTypeInput = new QComboBox(this);
    mVariableTypeInput->addItem(tr("Inclination"));
    mVariableTypeInput->addItem(tr("Declination"));
    mVariableTypeInput->addItem(tr("Field"));
    mVariableTypeInput->addItem(tr("Depth"));
    mVariableTypeInput->addItem(tr("Other Measure"));
    
    mUseErrMesureLabel = new QLabel(tr("Use err measure") + " :", this);
    mUseErrMesureInput = new QCheckBox(this);
    
    mTimeTypeLabel = new QLabel(tr("Time type") + " :", this);
    mTimeTypeInput = new QComboBox(this);
    mTimeTypeInput->addItem(tr("Fixed"));
    mTimeTypeInput->addItem(tr("Bayesian"));
    
    /*mUseTimeBayesianEventLabel = new QLabel(tr("Time bayesian event"), this);
    mUseTimeBayesianEventInput = new QCheckBox(this);
    
    mUseTimeBayesianConstraintLabel = new QLabel(tr("Time bayesian constraint"), this);
    mUseTimeBayesianConstraintInput = new QCheckBox(this);*/
    
    mVarianceTypeLabel = new QLabel(tr("Variance type") + " :", this);
    mVarianceTypeInput = new QComboBox(this);
    mVarianceTypeInput->addItem(tr("Fixed"));
    mVarianceTypeInput->addItem(tr("Bayesian"));
    
    mUseVarianceIndividualLabel = new QLabel(tr("Variance individual") + " :", this);
    mUseVarianceIndividualInput = new QCheckBox(this);
    
    mVarianceFixedLabel = new QLabel(tr("Variance fixed") + " :", this);
    mVarianceFixedInput = new QLineEdit(this);
    
    mCoeffLissageTypeLabel = new QLabel(tr("Coeff lissage type") + " :", this);
    mCoeffLissageTypeInput = new QComboBox(this);
    mCoeffLissageTypeInput->addItem(tr("Fixed"));
    mCoeffLissageTypeInput->addItem(tr("Bayesian"));
    
    mAlphaLissageLabel = new QLabel(tr("Alpha Smoothing") + " :", this);
    mAlphaLissageInput = new QLineEdit(this);
    
    
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(5);
    int row = -1;
    
    grid->addWidget(mProcessTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mProcessTypeInput, row, 1);

    grid->addWidget(mVariableTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mVariableTypeInput, row, 1);
     
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
    
    grid->addWidget(mCoeffLissageTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mCoeffLissageTypeInput, row, 1);
    
    grid->addWidget(mAlphaLissageLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAlphaLissageInput, row, 1);
    
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
    
    connect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::updateVisibilities);
    
    connect(mVariableTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::updateVisibilities);
    
    connect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::updateVisibilities);
    
    connect(mCoeffLissageTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::updateVisibilities);
    
    
    
    connect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::save);
    
    connect(mVariableTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::save);
    
    connect(mUseErrMesureInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &ChronocurveSettingsView::save);
    
    connect(mTimeTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::save);
    
    connect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::save);
    
    connect(mUseVarianceIndividualInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &ChronocurveSettingsView::save);
    
    connect(mVarianceFixedInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &ChronocurveSettingsView::save);
    
    connect(mCoeffLissageTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::save);
    
    connect(mAlphaLissageInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &ChronocurveSettingsView::save);
}

ChronocurveSettingsView::~ChronocurveSettingsView()
{
    
}

void ChronocurveSettingsView::setSettings(const ChronocurveSettings& settings)
{
    mEnabled = settings.mEnabled;
    
    if (settings.mProcessType == ChronocurveSettings::eProcessTypeUnivarie) {
        mProcessTypeInput->setCurrentIndex(0);

    } else if (settings.mProcessType == ChronocurveSettings::eProcessTypeSpherique) {
        mProcessTypeInput->setCurrentIndex(1);

    } else if (settings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel ||
               settings.mProcessType == ChronocurveSettings::eProcessType3D) {
        mProcessTypeInput->setCurrentIndex(2);
    }
    
    if (settings.mVariableType == ChronocurveSettings::eVariableTypeInclinaison) {
        mVariableTypeInput->setCurrentIndex(0);

    } else if (settings.mVariableType == ChronocurveSettings::eVariableTypeDeclinaison) {
        mVariableTypeInput->setCurrentIndex(1);

    } else if (settings.mVariableType == ChronocurveSettings::eVariableTypeIntensite) {
        mVariableTypeInput->setCurrentIndex(2);

    } else if (settings.mVariableType == ChronocurveSettings::eVariableTypeProfondeur) {
        mVariableTypeInput->setCurrentIndex(3);

    } else if (settings.mVariableType == ChronocurveSettings::eVariableTypeAutre) {
        mVariableTypeInput->setCurrentIndex(4);
    }
    
    mUseErrMesureInput->setChecked(settings.mUseErrMesure);
    
    if (settings.mTimeType == ChronocurveSettings::eModeFixed) {
        mTimeTypeInput->setCurrentIndex(0);

    } else if (settings.mTimeType == ChronocurveSettings::eModeBayesian) {
        mTimeTypeInput->setCurrentIndex(1);
    }
    
    if (settings.mVarianceType == ChronocurveSettings::eModeFixed) {
        mVarianceTypeInput->setCurrentIndex(0);

    } else if (settings.mVarianceType == ChronocurveSettings::eModeBayesian) {
        mVarianceTypeInput->setCurrentIndex(1);
    }
    
    mUseVarianceIndividualInput->setChecked(settings.mUseVarianceIndividual);
    mVarianceFixedInput->setText(QString::number(settings.mVarianceFixed));
    
    if (settings.mCoeffLissageType == ChronocurveSettings::eModeFixed) {
        mCoeffLissageTypeInput->setCurrentIndex(0);

    } else if (settings.mCoeffLissageType == ChronocurveSettings::eModeBayesian) {
        mCoeffLissageTypeInput->setCurrentIndex(1);
    }
    
    mAlphaLissageInput->setText(QString::number(settings.mAlphaLissage));
    
    updateVisibilities();
}

ChronocurveSettings ChronocurveSettingsView::getSettings()
{
   // const QLocale mLoc = QLocale();
    ChronocurveSettings settings;
    
    settings.mEnabled = mEnabled;
    
    if (mProcessTypeInput->currentIndex() == 0) {
        settings.mProcessType = ChronocurveSettings::eProcessTypeUnivarie;

        if (mVariableTypeInput->currentIndex() == 0) {
            settings.mVariableType = ChronocurveSettings::eVariableTypeInclinaison;

        } else if (mVariableTypeInput->currentIndex() == 1) {
            settings.mVariableType = ChronocurveSettings::eVariableTypeDeclinaison;

        } else if (mVariableTypeInput->currentIndex() == 2) {
            settings.mVariableType = ChronocurveSettings::eVariableTypeIntensite;

        } else if (mVariableTypeInput->currentIndex() == 3) {
            settings.mVariableType = ChronocurveSettings::eVariableTypeProfondeur;

        } else if (mVariableTypeInput->currentIndex() == 4) {
            settings.mVariableType = ChronocurveSettings::eVariableTypeAutre;
        }


    } else if (mProcessTypeInput->currentIndex() == 1) {
        settings.mProcessType = ChronocurveSettings::eProcessTypeSpherique;
        settings.mVariableType = ChronocurveSettings::eVariableTypeInclinaison;

    } else if (mProcessTypeInput->currentIndex() == 2) {
        settings.mProcessType = ChronocurveSettings::eProcessTypeVectoriel;
        settings.mVariableType = ChronocurveSettings::eVariableTypeInclinaison;

    } else if (mProcessTypeInput->currentIndex() == 3) {
        settings.mProcessType = ChronocurveSettings::eProcessType3D;
        settings.mVariableType = ChronocurveSettings::eVariableTypeAutre;
    }
    

    
    settings.mUseErrMesure = mUseErrMesureInput->isChecked();
    
    if (mTimeTypeInput->currentIndex() == 0) {
        settings.mTimeType = ChronocurveSettings::eModeFixed;

    } else if (mTimeTypeInput->currentIndex() == 1) {
        settings.mTimeType = ChronocurveSettings::eModeBayesian;
    }
    
    if (mVarianceTypeInput->currentIndex() == 0) {
        settings.mVarianceType = ChronocurveSettings::eModeFixed;

    } else if (mVarianceTypeInput->currentIndex() == 1) {
        settings.mVarianceType = ChronocurveSettings::eModeBayesian;
    }
        
    settings.mUseVarianceIndividual = mUseVarianceIndividualInput->isChecked();
    settings.mVarianceFixed = mVarianceFixedInput->text().toDouble();
    
    if (mCoeffLissageTypeInput->currentIndex() == 0) {
        settings.mCoeffLissageType = ChronocurveSettings::eModeFixed;

    } else if (mCoeffLissageTypeInput->currentIndex() == 1) {
        settings.mCoeffLissageType = ChronocurveSettings::eModeBayesian;
    }
    
    settings.mAlphaLissage = mAlphaLissageInput->text().toDouble();
    
    return settings;
}

void ChronocurveSettingsView::setProject(Project *project)
{
    mProject = project;
}

void ChronocurveSettingsView::reset()
{
    ChronocurveSettings settings = ChronocurveSettings::getDefault();
    setSettings(settings);
}

void ChronocurveSettingsView::save()
{
    ChronocurveSettings settings = getSettings();
    QJsonObject stateNext = mProject->mState;
    stateNext[STATE_CHRONOCURVE] = settings.toJson();
    mProject->pushProjectState(stateNext, CHRONOCURVE_SETTINGS_UPDATED_REASON, true);
}

void ChronocurveSettingsView::updateVisibilities()
{
    const bool variableTypeRequired = (mProcessTypeInput->currentText() == "Univariate");
    mVariableTypeLabel->setVisible(variableTypeRequired);
    mVariableTypeInput->setVisible(variableTypeRequired);
    
    const bool varianceFixed = (mVarianceTypeInput->currentText() == "Fixed");
    mVarianceFixedLabel->setVisible(varianceFixed);
    mVarianceFixedInput->setVisible(varianceFixed);
    mUseVarianceIndividualLabel->setVisible(!varianceFixed);
    mUseVarianceIndividualInput->setVisible(!varianceFixed);
    
    const bool coeffLissageFixed = (mCoeffLissageTypeInput->currentText() == "Fixed");
    mAlphaLissageLabel->setVisible(coeffLissageFixed);
    mAlphaLissageInput->setVisible(coeffLissageFixed);
}

