#include "PhaseDialog.h"
#include "Phase.h"
#include "ColorPicker.h"
#include "LineEdit.h"
#include <QtWidgets>


PhaseDialog::PhaseDialog(Phase* phase, QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mPhase(phase),
mModifying(false)
{
    setWindowTitle(tr("Create / Modify phase"));
    
    // -----------
    
    QLabel* nameLab = new QLabel(tr("Name") + " :");
    QLabel* colorLab = new QLabel(tr("Color") + " :");
    QLabel* tauTypeLab = new QLabel(tr("Duration") + " :");
    mTauFixedLab = new QLabel(tr("Value") + " :");
    mTauMinLab = new QLabel(tr("Value min") + " :");
    mTauMaxLab = new QLabel(tr("Value max") + " :");
    
    nameLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    colorLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    tauTypeLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mTauFixedLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mTauMinLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mTauMaxLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    
    mNameEdit = new LineEdit();
    mColorPicker = new ColorPicker();
    mColorPicker->setColor(QColor(150 + rand() % 100, 150 + rand() % 100, 150 + rand() % 100));
    
    mTauTypeCombo = new QComboBox();
    mTauTypeCombo->addItem(tr("Unknown"));
    //mTauTypeCombo->addItem(tr("Known"));
    mTauTypeCombo->addItem(tr("Range (uniform)"));
    
    mTauFixedEdit = new LineEdit();
    mTauMinEdit = new LineEdit();
    mTauMaxEdit = new LineEdit();
    
    
    QGridLayout* formLayout = new QGridLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    
    formLayout->addWidget(nameLab, 0, 0);
    formLayout->addWidget(mNameEdit, 0, 1);
    formLayout->addWidget(colorLab, 1, 0);
    formLayout->addWidget(mColorPicker, 1, 1);
    formLayout->addWidget(tauTypeLab, 2, 0);
    formLayout->addWidget(mTauTypeCombo, 2, 1);
    formLayout->addWidget(mTauFixedLab, 3, 0);
    formLayout->addWidget(mTauFixedEdit, 3, 1);
    formLayout->addWidget(mTauMinLab, 4, 0);
    formLayout->addWidget(mTauMinEdit, 4, 1);
    formLayout->addWidget(mTauMaxLab, 5, 0);
    formLayout->addWidget(mTauMaxEdit, 5, 1);
    
    // ----------
    
    QPushButton* okBut = new QPushButton(tr("OK"));
    okBut->setDefault(true);
    QPushButton* cancelBut = new QPushButton(tr("Cancel"));
    
    QHBoxLayout* butLayout = new QHBoxLayout();
    butLayout->setContentsMargins(0, 0, 0, 0);
    butLayout->addStretch();
    butLayout->addWidget(okBut);
    butLayout->addWidget(cancelBut);
    
    // ----------
    
    QFont font;
    font.setWeight(QFont::Bold);
    
    QLabel* titleLab = new QLabel(tr("Create / Modify Phase"));
    titleLab->setFont(font);
    titleLab->setAlignment(Qt::AlignCenter);
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(titleLab);
    layout->addLayout(formLayout);
    layout->addLayout(butLayout);
    setLayout(layout);
    
    connect(mTauTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showAppropriateTauOptions(int)));
    connect(okBut, SIGNAL(clicked()), this, SLOT(acceptPhase()));
    connect(cancelBut, SIGNAL(clicked()), this, SLOT(rejectPhase()));
    
    if(!mPhase)
    {
        mPhase = new Phase();
    }
    else
    {
        mModifying = true;
        
        mNameEdit->setText(mPhase->mName);
        mColorPicker->setColor(mPhase->mColor);
        mTauTypeCombo->setCurrentIndex((int)mPhase->mTauType);
        mTauFixedEdit->setText(QString::number(mPhase->mTauFixed));
        mTauMinEdit->setText(QString::number(mPhase->mTauMin));
        mTauMaxEdit->setText(QString::number(mPhase->mTauMax));
    }
    
    showAppropriateTauOptions((int)mPhase->mTauType);
}

PhaseDialog::~PhaseDialog()
{
    
}

void PhaseDialog::acceptPhase()
{
    Phase::TauType tauType = (Phase::TauType) mTauTypeCombo->currentIndex();
    double tauFixed = mTauFixedEdit->text().toDouble();
    double tauMin = mTauMinEdit->text().toDouble();
    double tauMax = mTauMaxEdit->text().toDouble();
    
    if(tauType == Phase::eTauFixed && tauFixed <= 0)
    {
        QMessageBox message(QMessageBox::Critical, tr("Inconsistent values"), tr("The duration must be a positive value !"), QMessageBox::Ok, qApp->activeWindow(), Qt::Sheet);
        message.exec();
    }
    else if(tauType == Phase::eTauRange && (tauMin >= tauMax || tauMin <= 0 || tauMax <= 0))
    {
        QMessageBox message(QMessageBox::Critical, tr("Inconsistent values"), tr("The min must be lower than the max, and both values must be positives !"), QMessageBox::Ok, qApp->activeWindow(), Qt::Sheet);
        message.exec();
    }
    else
    {
        mPhase->mName = mNameEdit->text();
        QColor color = mColorPicker->getColor();
        mPhase->mColor = color;
        
        mPhase->mTauType = tauType;
        mPhase->mTauFixed = tauFixed;
        mPhase->mTauMin = tauMin;
        mPhase->mTauMax = tauMax;
        
        accept();
    }
}

void PhaseDialog::rejectPhase()
{
    if(!mModifying)
    {
        delete mPhase;
    }
    reject();
}

void PhaseDialog::showAppropriateTauOptions(int typeIndex)
{
    Q_UNUSED(typeIndex)
    Phase::TauType type = (Phase::TauType) mTauTypeCombo->currentIndex();
    switch(type)
    {
        case Phase::eTauUnknown:
        {
            mTauFixedLab->setVisible(false);
            mTauMinLab->setVisible(false);
            mTauMaxLab->setVisible(false);
            
            mTauFixedEdit->setVisible(false);
            mTauMinEdit->setVisible(false);
            mTauMaxEdit->setVisible(false);
            break;
        }
        case Phase::eTauFixed:
        {
            mTauFixedLab->setVisible(true);
            mTauMinLab->setVisible(false);
            mTauMaxLab->setVisible(false);
            
            mTauFixedEdit->setVisible(true);
            mTauMinEdit->setVisible(false);
            mTauMaxEdit->setVisible(false);
            break;
        }
        case Phase::eTauRange:
        {
            mTauFixedLab->setVisible(false);
            mTauMinLab->setVisible(true);
            mTauMaxLab->setVisible(true);
            
            mTauFixedEdit->setVisible(false);
            mTauMinEdit->setVisible(true);
            mTauMaxEdit->setVisible(true);
            break;
        }
        default:
            break;
    }
}

