/*

SleepLib Preferences Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#include <QString>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QVariant>
#include <QDateTime>
#include <QDir>
#include <QDesktopServices>
#include <QDebug>
#ifdef Q_WS_WIN32
#include "windows.h"
#include "lmcons.h"
#endif

#include "preferences.h"

const QString & getUserName()
{
    static QString userName;
    userName=getenv("USER");

    if (userName.isEmpty()) {
        userName="Windows User";

#if defined (Q_WS_WIN32)
    #if defined(UNICODE)
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_NT) {
        TCHAR winUserName[UNLEN + 1]; // UNLEN is defined in LMCONS.H
        DWORD winUserNameSize = sizeof(winUserName);
        GetUserNameW( winUserName, &winUserNameSize );
        userName = QString::fromStdWString( winUserName );
    } else
    #endif
    {
        char winUserName[UNLEN + 1]; // UNLEN is defined in LMCONS.H
        DWORD winUserNameSize = sizeof(winUserName);
        GetUserNameA( winUserName, &winUserNameSize );
        userName = QString::fromLocal8Bit( winUserName );
    }
#endif
    }

    return userName;
}

const QString & GetAppRoot()
{
    // Should it go here: QDesktopServices::DataLocation ???

    static QString HomeAppRoot=QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)+"/"+AppRoot;

    return HomeAppRoot;
}

Preferences::Preferences()
{
    p_name="Preferences";
    p_path=GetAppRoot();
}

Preferences::Preferences(QString name,QString filename)
{

    const QString xmlext=".xml";

    if (name.endsWith(xmlext)) {
        p_name=name.section(".",0,0);
    } else {
        p_name=name;
    }

    if (filename.isEmpty()) {
        p_filename=GetAppRoot()+"/"+p_name+xmlext;
    } else {
        if (!filename.contains("/")) {
            p_filename=GetAppRoot()+"/";
        } else p_filename="";

        p_filename+=filename;

        if (!p_filename.endsWith(xmlext)) p_filename+=xmlext;
    }
}

Preferences::~Preferences()
{
    //Save(); // Don't..Save calls a virtual function.
}

int Preferences::GetCode(QString s)
{
    int prefcode=0;
    for (QHash<int,QString>::iterator i=p_codes.begin(); i!=p_codes.end(); i++) {
        if (i.value()==s) return i.key();
        prefcode++;
    }
    p_codes[prefcode]=s;
    return prefcode;
}

const QString Preferences::Get(QString name)
{
    QString temp;
    QChar obr=QChar('{');
    QChar cbr=QChar('}');
    QString t,a,ref; // How I miss Regular Expressions here..
    if (p_preferences.find(name)!=p_preferences.end()) {
        temp="";
        t=p_preferences[name].toString();
        if (p_preferences[name].type()!=QVariant::String) {
            return t;
        }
    } else {
        t=name; // parse the string..
    }
    while (t.contains(obr)) {
        temp+=t.section(obr,0,0);
        a=t.section(obr,1);
        if (a.startsWith("{")) {
            temp+=obr;
            t=a.section(obr,1);
            continue;
        }
        ref=a.section(cbr,0,0);

        if (ref.toLower()=="home") {
            temp+=GetAppRoot();
        } else if (ref.toLower()=="user") {
            temp+=getUserName();
        } else if (ref.toLower()=="sep") { // redundant in QT
            temp+="/";
        } else {
            temp+=Get(ref);
        }
        t=a.section(cbr,1);
    }
    temp+=t;
    temp.replace("}}","}"); // Make things look a bit better when escaping braces.

    return temp;
}


bool Preferences::Open(QString filename)
{
    if (!filename.isEmpty())
        p_filename=filename;

    QDomDocument doc(p_name);
    QFile file(p_filename);
    qDebug() << "Opening " << p_filename;
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open" << p_filename;
        return false;
    }
    if (!doc.setContent(&file)) {
        qWarning() << "Invalid XML Content in" << p_filename;
        return false;
    }
    file.close();


    QDomElement root=doc.documentElement();
    if (root.tagName() != AppName) {
        return false;
    }

    root=root.firstChildElement();
    if (root.tagName() != p_name) {
        return false;
    }

    bool ok;
    p_preferences.clear();
    QDomNode n=root.firstChild();
    while (!n.isNull()) {
        QDomElement e=n.toElement();
        if (!e.isNull()) {
            QString name=e.tagName();
            QString type=e.attribute("type").toLower();
            QString value=e.text();;
            if (type=="double") {
                double d;
                d=value.toDouble(&ok);
                if (ok) {
                    p_preferences[name]=d;
                } else {
                    qDebug() << "XML Error:" << name << "=" << value << "??";
                }
            } else
            if (type=="qlonglong") {
                qint64 d;
                d=value.toLongLong(&ok);
                if (ok) {
                    p_preferences[name]=d;
                } else {
                    qDebug() << "XML Error:" << name << "=" << value << "??";
                }
            } else
            if (type=="bool") {
                QString v=value.toLower();
                if ((v=="true") || (v=="on") || (v=="yes")) {
                    p_preferences[name]=true;
                } else
                if ((v=="false") || (v=="off") || (v=="no")) {
                    p_preferences[name]=false;
                } else {
                    int d;
                    d=value.toInt(&ok);
                    if (ok) {
                        p_preferences[name]=d!=0;
                    } else {
                        qDebug() << "XML Error:" << name << "=" << value << "??";
                    }
                }
            } else if (type=="qdatetime") {
                QDateTime d;
                d.fromString(value,"yyyy-MM-dd HH:mm:ss");
                if (d.isValid())
                    p_preferences[name]=d;
                else
                    qWarning() << "XML Error: Invalid DateTime record" << name << value;

            } else  {
                p_preferences[name]=value;
            }

        }
        n=n.nextSibling();
    }
    root=root.nextSiblingElement();
    ExtraLoad(root);
    return true;
}

bool Preferences::Save(QString filename)
{
    if (!filename.isEmpty())
        p_filename=filename;

    QDomDocument doc(p_name);

    QDomElement droot = doc.createElement(AppName);
    doc.appendChild( droot );

    QDomElement root=doc.createElement(p_name);
    droot.appendChild(root);

    for (QHash<QString,QVariant>::iterator i=p_preferences.begin(); i!=p_preferences.end(); i++) {
        QVariant::Type type=i.value().type();
        if (type==QVariant::Invalid) continue;

        QDomElement cn=doc.createElement(i.key());
        cn.setAttribute("type",i.value().typeName());
        if (type==QVariant::DateTime) {
            cn.appendChild(doc.createTextNode(i.value().toDateTime().toString("yyyy-MM-dd HH:mm:ss")));
        } else {
            cn.appendChild(doc.createTextNode(i.value().toString()));
        }

        root.appendChild(cn);
    }

    droot.appendChild(ExtraSave(doc));

    QFile file(p_filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QTextStream ts(&file);
    ts << doc.toString();
    file.close();

    return true;
}


