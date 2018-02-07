#include "EventDialog.h"
#include "ColorPicker.h"
#include "LineEdit.h"
#include "Label.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include <QtWidgets>


EventDialog::EventDialog(QWidget* parent, const QString& title, Qt::WindowFlags flags):QDialog(parent, flags)
{
    setWindowTitle(title);
    
    // -----------
    
    Label* nameLab = new Label(tr("Name"), this);
    nameLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    
    Label* colorLab = new Label(tr("Color"), this);
    colorLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    
    mNameEdit = new LineEdit(this);
    mNameEdit->setText(tr("No name"));
   // mNameEdit->setStyleSheet("QLineEdit { border-radius: 5px; }");
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
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EventDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EventDialog::reject);
    
    // ----------
    
    QFont font;
    font.setWeight(QFont::Bold);
    
    Label* titleLab = new Label(title, this);
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

