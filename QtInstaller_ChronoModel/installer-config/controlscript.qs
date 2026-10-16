/**************************************************************************
**
** ChronoModel - controlscript.qs
** version du 2026-10-16
**
** Script de controle QtIFW, embarque dans l'installeur ET dans le
** desinstalleur (Chronomodel_uninstaller), qui est une copie de l'installeur.
**
** Correctif : bug de la traduction francaise de QtIFW 4.11 (ifw_fr.qm).
** Le message de la page "Resume de l'installation" (ReadyForInstallationPage)
** y est stocke avec le HTML deja echappe :
**   "...ordinateur.&lt;br&gt;&lt;font color=&quot;red&quot;&gt;..."
** Le QLabel "MessageLabel" affiche alors les balises en clair.
** On decode les entites et on force l'affichage en texte riche.
**
**************************************************************************/

function Controller()
{
}

function decodeHtmlEntities(s)
{
    return s.replace(/&lt;/g, "<")
            .replace(/&gt;/g, ">")
            .replace(/&quot;/g, "\"")
            .replace(/&#39;/g, "'")
            .replace(/&amp;/g, "&");   // en dernier
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    var page = gui.currentPageWidget();
    if (page === null || page.MessageLabel === undefined)
        return;

    var label = page.MessageLabel;
    var txt = label.text;
    if (txt.indexOf("&lt;") >= 0)
        txt = decodeHtmlEntities(txt);

    label.textFormat = 1;   // Qt::RichText
    label.text = txt;
}
