#include "EventDialog.h"
#include "ColorPicker.h"
#include "LineEdit.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include <QtWidgets>


EventDialog::EventDialog(QWidget* parent, const QString& title, Qt::WindowFlags flags):QDialog(parent, flags)
{
    setWindowTitle(title);
    
    // -----------
    
    QLabel* nameLab = new QLabel(tr("Name") + " :");
    nameLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    
    QLabel* colorLab = new QLabel(tr("Color") + " :");
    colorLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    
    mNameEdit = new LineEdit();
    mNameEdit->setText(tr("Untitled"));
    mNameEdit->selectAll();
    mNameEdit->setFocus();
    
    mColorPicker = new ColorPicker();
    mColorPicker->setColor(randomColor());
    
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addWidget(nameLab, 0, 0);
    gridLayout->addWidget(mNameEdit, 0, 1);
    gridLayout->addWidget(colorLab, 1, 0);
    gridLayout->addWidget(mColorPicker, 1, 1);
    
    // ----------
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox();
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
    
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    
    // ----------
    
    QFont font;
    font.setWeight(QFont::Bold);
    
    QLabel* titleLab = new QLabel(title);
    titleLab->setFont(font);
    titleLab->setAlignment(Qt::AlignCenter);
    
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    
    // ----------
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(titleLab);
    layout->addWidget(separator);
    layout->addLayout(gridLayout);
    layout->addWidget(buttonBox);
    setLayout(layout);
    
    setFixedWidth(300);
}

EventDialog::~EventDialog()
{
    
}

QString EventDialog::getName() const
{
    return mNameEdit->text();
}

QColor EventDialog::getColor() const
{
    return mColorPicker->getColor();
}

