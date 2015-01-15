#include "AboutDialog.h"
#include "Button.h"
#include "Painting.h"
#include <QtWidgets>


AboutDialog::AboutDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags)
{
    setWindowTitle(tr("About Chronomodel"));
    
    // -----------
    
    mLabel = new QLabel();
    mLabel->setTextFormat(Qt::RichText);
    mLabel->setWordWrap(true);
 
    QString text = "<b>" + qApp->applicationName() + " " + qApp->applicationVersion() + "</b><br><br>";
    text += "<a href=\"http://www.chronomodel.com\">http://www.chronomodel.com</a><br><br>";
    text += "Copyright © CNRS<br>";
    text += "Published in 2015<br><br>";
    text += "<b>Project director</b> : Philippe LANOS<br>";
    text += "<a href=\"mailto:philippe.lanos@univ-rennes1.fr\">philippe.lanos@univ-rennes1.fr</a><br><br>";
    text += "<b>Project co-director</b> : Anne PHILIPPE<br>";
    text += "<a href=\"mailto:anne.philippe@univ-nantes.fr\">anne.philippe@univ-nantes.fr</a><br><br>";
    text += "<b>Authors</b> :<br>";
    text += "Helori LANOS<br>";
    text += "<a href=\"mailto:helori.lanos@gmail.com\">helori.lanos@gmail.com</a><br><br>";
    text += "Philippe DUFRESNE<br>";
    text += "<a href=\"mailto:philippe.dufresne@univ-rennes1.fr\">philippe.dufresne@univ-rennes1.fr</a><br><br>";
    
    mLabel->setText(text);
    
    mLicenseBut = new Button(tr("Open Licence File"));
    QFont font = mLicenseBut->font();
    font.setPointSizeF(pointSize(12));
    mLicenseBut->setFont(font);
    connect(mLicenseBut, SIGNAL(clicked()), this, SLOT(showLicense()));
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(mLabel);
    layout->addWidget(mLicenseBut);
    setLayout(layout);
    
    setMinimumWidth(600);
}

AboutDialog::~AboutDialog()
{
    
}

void AboutDialog::showLicense()
{
    QString path = qApp->applicationDirPath();
#ifdef Q_OS_MAC
    QDir dir(path);
    dir.cdUp();
    path = dir.absolutePath() + "/Resources";
#endif
    path += "/License.txt";
    QDesktopServices::openUrl(QUrl("file:///" + path, QUrl::TolerantMode));
}

void AboutDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    QPixmap icon(":chronomodel.png");
    int s = 200;
    int m = 50;
    p.drawPixmap(QRect(width() - m - s, m, s, s), icon, icon.rect());
    
}
