/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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
#include <QtWidgets>
#include <QVariant>


ChronocurveSettingsView::ChronocurveSettingsView(QWidget* parent):QWidget(parent)
{
    mProcessTypeLabel = new QLabel(tr("Process Type"), this);
    mProcessTypeInput = new QComboBox(this);
    mProcessTypeInput->addItem("Univarié");
    mProcessTypeInput->addItem("Sphérique");
    mProcessTypeInput->addItem("Vectoriel");
    
    mVariableTypeLabel = new QLabel(tr("Variable Type"), this);
    mVariableTypeInput = new QComboBox(this);
    mVariableTypeInput->addItem("Inclinaison");
    mVariableTypeInput->addItem("Déclinaison");
    mVariableTypeInput->addItem("Intensité");
    mVariableTypeInput->addItem("Profondeur");
    
    mSelectOuvLabel = new QLabel(tr("Select Ouv"), this);
    mSelectOuvInput = new QCheckBox(this);
    
    mOuvMaxLabel = new QLabel(tr("Ouv max"), this);
    mOuvMaxInput = new QLineEdit(this);
    
    mUseCorrLatLabel = new QLabel(tr("Use correction latitude"), this);
    mUseCorrLatInput = new QCheckBox(this);
    
    mLatLabel = new QLabel(tr("Latitude"), this);
    mLatInput = new QLineEdit(this);
    
    mLngLabel = new QLabel(tr("Longitude"), this);
    mLngInput = new QLineEdit(this);
    
    mUseErrMesureLabel = new QLabel(tr("Use err mesure"), this);
    mUseErrMesureInput = new QCheckBox(this);
    
    mTimeTypeLabel = new QLabel(tr("Time type"), this);
    mTimeTypeInput = new QComboBox(this);
    mTimeTypeInput->addItem("Fixed");
    mTimeTypeInput->addItem("Bayesian");
    
    /*mUseTimeBayesianEventLabel = new QLabel(tr("Time bayesian event"), this);
    mUseTimeBayesianEventInput = new QCheckBox(this);
    
    mUseTimeBayesianConstraintLabel = new QLabel(tr("Time bayesian constraint"), this);
    mUseTimeBayesianConstraintInput = new QCheckBox(this);*/
    
    mVarianceTypeLabel = new QLabel(tr("Variance type"), this);
    mVarianceTypeInput = new QComboBox(this);
    mVarianceTypeInput->addItem("Fixed");
    mVarianceTypeInput->addItem("Bayesian");
    
    mUseVarianceIndividualLabel = new QLabel(tr("Variance individual"), this);
    mUseVarianceIndividualInput = new QCheckBox(this);
    
    mVarianceFixedLabel = new QLabel(tr("Variance fixed"), this);
    mVarianceFixedInput = new QLineEdit(this);
    
    mCoeffLissageTypeLabel = new QLabel(tr("Coeff lissage type"), this);
    mCoeffLissageTypeInput = new QComboBox(this);
    mCoeffLissageTypeInput->addItem("Fixed");
    mCoeffLissageTypeInput->addItem("Bayesian");
    
    mAlphaLabel = new QLabel(tr("Alpha"), this);
    mAlphaInput = new QLineEdit(this);
    
    
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    int row = -1;
    
    grid->addWidget(mProcessTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mProcessTypeInput, row, 1);

    grid->addWidget(mVariableTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mVariableTypeInput, row, 1);
    
    grid->addWidget(mSelectOuvLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mSelectOuvInput, row, 1);
    
    grid->addWidget(mOuvMaxLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mOuvMaxInput, row, 1);
    
    grid->addWidget(mUseCorrLatLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mUseCorrLatInput, row, 1);
    
    grid->addWidget(mLatLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLatInput, row, 1);
    
    grid->addWidget(mLngLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLngInput, row, 1);
    
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
    
    grid->addWidget(mAlphaLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mAlphaInput, row, 1);
    
   
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(grid);
    setLayout(layout);
    
    updateVisibilities();
    
    connect(mProcessTypeInput, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::updateVisibilities);
    
    connect(mSelectOuvInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &ChronocurveSettingsView::updateVisibilities);
    
    connect(mUseCorrLatInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &ChronocurveSettingsView::updateVisibilities);
    
    connect(mVarianceTypeInput, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::updateVisibilities);
    
    connect(mCoeffLissageTypeInput, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::updateVisibilities);
    
    
    
    connect(mProcessTypeInput, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, &ChronocurveSettingsView::save);
    
    // TODO : connect all inputs to save
}

ChronocurveSettingsView::~ChronocurveSettingsView()
{
    
}

void ChronocurveSettingsView::setSettings(const ChronocurveSettings& settings)
{
    const QLocale mLoc = QLocale();
    
    if(settings.mProcessType == ChronocurveSettings::eProcessTypeUnivarie){
        mProcessTypeInput->setCurrentIndex(0);
    }else if(settings.mProcessType == ChronocurveSettings::eProcessTypeSpherique){
        mProcessTypeInput->setCurrentIndex(1);
    }else if(settings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel){
        mProcessTypeInput->setCurrentIndex(2);
    }
    
    if(settings.mVariableType == ChronocurveSettings::eVariableTypeInclinaison){
        mVariableTypeInput->setCurrentIndex(0);
    }else if(settings.mVariableType == ChronocurveSettings::eVariableTypeDeclinaison){
        mVariableTypeInput->setCurrentIndex(1);
    }else if(settings.mVariableType == ChronocurveSettings::eVariableTypeIntensite){
        mVariableTypeInput->setCurrentIndex(2);
    }else if(settings.mVariableType == ChronocurveSettings::eVariableTypeProfondeur){
        mVariableTypeInput->setCurrentIndex(3);
    }
    
    mSelectOuvInput->setChecked(settings.mSelectOuv);
    mOuvMaxInput->setText(QString::number(settings.mOuvMax));
    mUseCorrLatInput->setChecked(settings.mUseCorrLat);
    mUseErrMesureInput->setChecked(settings.mUseErrMesure);
    
    if(settings.mTimeType == ChronocurveSettings::eModeFixed){
        mTimeTypeInput->setCurrentIndex(0);
    }else if(settings.mTimeType == ChronocurveSettings::eModeBayesian){
        mTimeTypeInput->setCurrentIndex(1);
    }
    
    /*mUseTimeBayesianEventInput->setChecked(settings.mUseTimeBayesianEvent);
    mUseTimeBayesianConstraintInput->setChecked(settings.mUseTimeBayesianConstraint);*/
    
    if(settings.mVarianceType == ChronocurveSettings::eModeFixed){
        mVarianceTypeInput->setCurrentIndex(0);
    }else if(settings.mVarianceType == ChronocurveSettings::eModeBayesian){
        mVarianceTypeInput->setCurrentIndex(1);
    }
    
    mUseVarianceIndividualInput->setChecked(settings.mUseVarianceIndividual);
    mVarianceFixedInput->setText(QString::number(settings.mVarianceFixed));
    
    if(settings.mCoeffLissageType == ChronocurveSettings::eModeFixed){
        mCoeffLissageTypeInput->setCurrentIndex(0);
    }else if(settings.mCoeffLissageType == ChronocurveSettings::eModeBayesian){
        mCoeffLissageTypeInput->setCurrentIndex(1);
    }
    
    mAlphaInput->setText(QString::number(settings.mAlpha));
    
    updateVisibilities();
}

ChronocurveSettings ChronocurveSettingsView::getSettings()
{
    const QLocale mLoc = QLocale();
    ChronocurveSettings settings;
    
    if(mProcessTypeInput->currentIndex() == 0){
        settings.mProcessType = ChronocurveSettings::eProcessTypeUnivarie;
    }else if(mProcessTypeInput->currentIndex() == 1){
        settings.mProcessType = ChronocurveSettings::eProcessTypeSpherique;
    }else if(mProcessTypeInput->currentIndex() == 2){
        settings.mProcessType = ChronocurveSettings::eProcessTypeVectoriel;
    }
    
    if(mVariableTypeInput->currentIndex() == 0){
        settings.mVariableType = ChronocurveSettings::eVariableTypeInclinaison;
    }else if(mVariableTypeInput->currentIndex() == 1){
        settings.mVariableType = ChronocurveSettings::eVariableTypeDeclinaison;
    }else if(mVariableTypeInput->currentIndex() == 2){
        settings.mVariableType = ChronocurveSettings::eVariableTypeIntensite;
    }else if(mVariableTypeInput->currentIndex() == 3){
        settings.mVariableType = ChronocurveSettings::eVariableTypeProfondeur;
    }
    
    settings.mSelectOuv = mSelectOuvInput->isChecked();
    settings.mOuvMax = mOuvMaxInput->text().toDouble();
    settings.mUseCorrLat = mUseCorrLatInput->isChecked();
    settings.mUseErrMesure = mUseErrMesureInput->isChecked();

    if(mTimeTypeInput->currentIndex() == 0){
        settings.mTimeType = ChronocurveSettings::eModeFixed;
    }else if(mTimeTypeInput->currentIndex() == 1){
        settings.mTimeType = ChronocurveSettings::eModeBayesian;
    }
    
    /*settings.mUseTimeBayesianEvent = mUseTimeBayesianEventInput->isChecked();
    settings.mUseTimeBayesianConstraint = mUseTimeBayesianConstraintInput->isChecked();*/
    
    if(mVarianceTypeInput->currentIndex() == 0){
        settings.mVarianceType = ChronocurveSettings::eModeFixed;
    }else if(mVarianceTypeInput->currentIndex() == 1){
        settings.mVarianceType = ChronocurveSettings::eModeBayesian;
    }
        
    settings.mUseVarianceIndividual = mUseVarianceIndividualInput->isChecked();
    settings.mVarianceFixed = mVarianceFixedInput->text().toDouble();
    
    if(mCoeffLissageTypeInput->currentIndex() == 0){
        settings.mCoeffLissageType = ChronocurveSettings::eModeFixed;
    }else if(mCoeffLissageTypeInput->currentIndex() == 1){
        settings.mCoeffLissageType = ChronocurveSettings::eModeBayesian;
    }
    
    settings.mAlpha = mAlphaInput->text().toDouble();
    
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
    bool variableTypeRequired = (mProcessTypeInput->currentText() == "Univarié");
    mVariableTypeLabel->setVisible(variableTypeRequired);
    mVariableTypeInput->setVisible(variableTypeRequired);
    
    bool selectOuv = mSelectOuvInput->isChecked();
    mOuvMaxLabel->setVisible(selectOuv);
    mOuvMaxInput->setVisible(selectOuv);
    
    bool latLngRequired = selectOuv || mUseCorrLatInput->isChecked();
    mLatLabel->setVisible(latLngRequired);
    mLatInput->setVisible(latLngRequired);
    mLngLabel->setVisible(latLngRequired);
    mLngInput->setVisible(latLngRequired);
    
    bool varianceFixed = (mVarianceTypeInput->currentText() == "Fixed");
    mVarianceFixedLabel->setVisible(varianceFixed);
    mVarianceFixedInput->setVisible(varianceFixed);
    mUseVarianceIndividualLabel->setVisible(!varianceFixed);
    mUseVarianceIndividualInput->setVisible(!varianceFixed);
    
    bool coeffLissageFixed = (mCoeffLissageTypeInput->currentText() == "Fixed");
    mAlphaLabel->setVisible(coeffLissageFixed);
    mAlphaInput->setVisible(coeffLissageFixed);
}

