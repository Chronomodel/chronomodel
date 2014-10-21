#include "AboutDialog.h"
#include <QtWidgets>


AboutDialog::AboutDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags)
{
    setWindowTitle(tr("About Chronomodel"));
    
    // -----------
    
    mLabel = new QLabel();
    mLabel->setTextFormat(Qt::RichText);
    mLabel->setWordWrap(true);
    
    QString about = "Copyright CNRS 2014<br>\
    Authors:<br>\
    H. LANOS (IEventory)<br>\
    Ph. DUFRESNE<br>\
    <br>\
    Contact:<br>\
    Email : philippe.lanos@univ-rennes1.fr (project manager)<br>\
    philippe.dufresne@u-bordeaux-montaigne.fr (software support)<br>\
    <br>\
    Français<br>\
    <br><br>\
    Ce logiciel est un logiciel gratuit et ne peut donc pas être vendu.<br>\
    <br>\
    Ce logiciel peut être redistribué du moment qu'il n'est pas modifié par rapport à la version disponible sur notre site officiel. Cette redistribution ne peut être effectuée qu'à titre gratuit et sans aucun frais.<br>\
    <br>\
    Il est interdit de décompiler, désassembler ou modifier les fichiers de ce logiciel (exécutable ou non). Toute tentative de modification ou récupération de code source par quelque moyen que ce soit est interdite.<br>\
    <br>\
    Ce logiciel est distribué sans aucune garantie. Nous ne pourrons être tenus responsable de tout dommage directs, induits ou consécutifs à un quelconque dysfonctionnement d'un de nos produits. Le fonctionnement de ce logiciel n'est pas garanti.<br>\
    <br>\
    Il est interdit d'utiliser ce logiciel dans des environnements à risque comme par exemple et sans s'y limiter installations nucléaires, système de contrôle aérien ou d'armement.<br>\
    <br><br>\
    English<br>\
    <br>\
    This software is freeware and can not be sold.<br>\
    <br>\
    This software can be redistributed as long it is strictly similar from the version available on our official website. Redistribution must be free with no hidden cost.<br>\
    <br>\
    It is prohibited to decompile, disassemble or modify any files of this software (executable or not). Any attempt to modify or recovering the source code by any means is prohibited.<br>\
    <br>\
    This software is provided without any warranty. We are not liable for any damage caused directly, induced or resulting by any malfunction of this software. The function of this software is not guaranteed.<br>\
    <br>\
    It is forbidden to use this software in dangerous environments such as but not limited to nuclear facilities, air traffic control systems or weapons.<br>\
    ";
    
    mLabel->setText(about);
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(mLabel);
    setLayout(layout);
    
    setMinimumWidth(600);
}

AboutDialog::~AboutDialog()
{
    
}
