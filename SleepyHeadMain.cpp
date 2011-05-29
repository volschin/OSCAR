/***************************************************************
 * Name:      SleepyHeadMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    Mark Watkins (jedimark64@users.sourceforge.net)
 * Created:   2011-05-20
 * Copyright: Mark Watkins (http://sourceforge.net/projects/sleepyhead/)
 * License:   GPL
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include "version.h"
#include <wx/app.h>
#include <wx/msgdlg.h>
#include <wx/dirdlg.h>
#include <wx/progdlg.h>
#include <wx/bitmap.h>
#include <wx/log.h>
#include <wx/dcscreen.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include "SleepyHeadMain.h"
#include "sleeplib/profiles.h"
//#include "graphs/sleepflagsgraph.h"
//#include "graphs/cpap_wavegraph.h"

#if defined(__WXMSW__)
extern "C" void *_GdipStringFormatCachedGenericTypographic = NULL;
#endif
wxProgressDialog *loader_progress;
//helper functions
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__WXMAC__)
        wxbuild << _T("-Mac");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}


SleepyHeadFrame::SleepyHeadFrame(wxFrame *frame)
    : GUIFrame(frame)
{
    wxInitAllImageHandlers();
    loader_progress=new wxProgressDialog(wxT("SleepyHead"),wxT("Please Wait..."),100,this, wxPD_APP_MODAL|wxPD_AUTO_HIDE|wxPD_SMOOTH);
    loader_progress->Hide();
    wxString title=wxTheApp->GetAppName()+wxT(" v")+wxString(AutoVersion::FULLVERSION_STRING,wxConvUTF8);
    SetTitle(title);

    profile=Profiles::Get();
    if (!profile) {
        wxLogError(wxT("Couldn't get active profile"));
        abort();
    }

    UpdateMachineMenu();

    //wxDisableAsserts();
    // Create AUINotebook Tabs
    wxCommandEvent dummy;
    OnViewMenuDaily(dummy);     // Daily Page
    OnViewMenuSummary(dummy);   // Summary Page

    int id=0;
    if (pref.Exists("DefaultMachine")) {
        id=pref[wxT("DefaultMachine")].GetInteger();
    }

    this->Connect(wxID_ANY, wxEVT_DO_SCREENSHOT, wxCommandEventHandler(SleepyHeadFrame::DoScreenshot));
    //this->Connect(wxID_ANY, wxEVT_MACHINE_SELECTED, wxCommandEventHandler(SleepyHeadFrame::OnMachineSelected));
    //this->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SleepyHeadFrame::DoScreenshot));

#if wxUSE_STATUSBAR
    //statusBar->SetStatusText(_("Hello!"), 0);
    statusBar->SetStatusText(wxbuildinfo(long_f), 1);
#endif
}

SleepyHeadFrame::~SleepyHeadFrame()
{
    if (loader_progress) {
        loader_progress->Hide();
        loader_progress->Destroy();
        delete loader_progress;
    }
}
void SleepyHeadFrame::UpdateMachineMenu()
{
    cpap_machines=profile->GetMachines(MT_CPAP);

    wxMenuItemList &z=MachineMenu->GetMenuItems();
    //wxMenuItemList::iterator q;
    int i=MachineMenuID;
    for (auto q=z.begin();q!=z.end();q++) {
        this->Disconnect(i,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(SleepyHeadFrame::OnMachineSelected));
        MachineMenu->Remove(*q);
        i++;
    }

    i=MachineMenuID;

    for (auto m=cpap_machines.begin();m!=cpap_machines.end();m++) {
        wxMenuItem *item=MachineMenu->AppendRadioItem(i,(*m)->properties[wxT("Serial")],wxEmptyString);
        this->Connect(i,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(SleepyHeadFrame::OnMachineSelected));
        i++;
    }

    long l=pref[wxT("DefaultMachine")];
    MachineMenu->Check(l+MachineMenuID,true);

    /*if (vm.size()>=1) {
        machine=vm[0];
    } else machine=NULL; */

}

void SleepyHeadFrame::OnClose(wxCloseEvent &event)
{
    Destroy();
}

void SleepyHeadFrame::OnQuit(wxCommandEvent &event)
{
    Destroy();
}
void SleepyHeadFrame::OnFullscreen(wxCommandEvent& event)
{
    if (!IsFullScreen()) {
        ShowFullScreen(true,wxFULLSCREEN_NOBORDER|wxFULLSCREEN_NOCAPTION|wxFULLSCREEN_NOSTATUSBAR);
    } else {
        ShowFullScreen(false);
    }
}
void SleepyHeadFrame::OnMachineSelected(wxCommandEvent& event)
{
    int id=event.GetId()-MachineMenuID;
    wxLogMessage(wxT("Machine Selected:")+wxString::Format(wxT("%i"),id));
    event.Skip();
    Machine *m=cpap_machines[id];

    if (m) {
        pref[wxT("DefaultMachine")]=(long)id;
    }
    int idx=main_auinotebook->GetPageIndex(daily);
    if (idx!=wxNOT_FOUND) {
        daily->RefreshData(m);
        daily->Refresh();
    }
    idx=main_auinotebook->GetPageIndex(summary);
    if (idx!=wxNOT_FOUND) {
        summary->RefreshData(m);
        summary->Refresh();
    }
    //Refresh();
}
void SleepyHeadFrame::OnScreenshot(wxCommandEvent& event)
{
    ToolsMenu->UpdateUI();
    //wxWindow::DoUpdateWindowUI();
    wxWindow::UpdateWindowUI();
    //Refresh(true); // Make sure the menu is closed.. (It pushes the Update event in front of the manual event we push next)
   // Update(true);

    wxCommandEvent MyEvent( wxEVT_DO_SCREENSHOT);
    wxPostEvent(this, MyEvent);
}

void SleepyHeadFrame::DoScreenshot( wxCommandEvent &event )
{
    wxRect r=GetRect();

#if defined(__UNIX__)
    int cx=r.x, cy=r.y;
    ClientToScreen(&cx,&cy);
    int border_width = cx - r.x;
    int title_bar_height = cy - r.y;
    r.width += (border_width * 2);
    r.height += title_bar_height + border_width;
#endif
    int x=r.x;
    int y=r.y;
    int w=r.width;
    int h=r.height;

    wxScreenDC sdc;
    wxMemoryDC mdc;

    wxBitmap bmp(r.width, r.height,-1);
    //wxBitMap *bmp=wxEmptyImage(r.width,r.height);
    mdc.SelectObject(bmp);

    mdc.Blit((wxCoord)0, (wxCoord)0, (wxCoord)r.width, (wxCoord)r.height, &sdc, (wxCoord)r.x, (wxCoord)r.y);

    mdc.SelectObject(wxNullBitmap);

    wxDateTime d=wxDateTime::Now();

//    wxDirDialog sfs(this,_("Choose a Directory")); //,wxT(""),wxT(""),style=wxFD_OPEN);
    wxString filename=wxSaveFileSelector(_("Please give a filename for the screenshot"),wxT("png"),wxT("Sleepyhead-")+d.Format(wxT("%Y%m%d-%H%M%S")),this);
    if (!filename.IsEmpty()) {
        if (!filename.Lower().EndsWith(wxT(".png"))) filename+=wxT(".png");
        wxImage img=bmp.ConvertToImage();
        if (!img.SaveFile(filename, wxBITMAP_TYPE_PNG)) {
            wxLogError(wxT("Couldn't save screenshot ")+filename);
        }
    }
}

void SleepyHeadFrame::OnAbout(wxCommandEvent &event)
{
    wxString msg = wxbuildinfo(long_f);
    msg=wxTheApp->GetAppName()+wxT(" v")+wxString(AutoVersion::FULLVERSION_STRING,wxConvUTF8)+wxT("\nAuthors: Mark Watkins / Troy Schultz\nThis is alpha software is guaranteed to break regularly!\nUse at your own risk."); //,AutoVersion::DATE,AutoVersion::MONTH,AutoVersion::YEAR

    wxMessageBox(msg, _("Welcome to..."),0,this);
}
void SleepyHeadFrame::OnImportSD(wxCommandEvent &event)
{
    wxDirDialog dd(this,_("Choose a Directory")); //,wxT(""),wxT(""),style=wxFD_OPEN);
    if (dd.ShowModal()!=wxID_OK) return;

    wxString path=dd.GetPath();

    loader_progress->Update(0);
    loader_progress->Show();
    profile->Import(path);
    loader_progress->Update(100);
    loader_progress->Show(false);

    UpdateMachineMenu(); // Also updates cpap_machines list.
    auto q=MachineMenu->GetMenuItems().rbegin();
    int i=0;
    if (q!=MachineMenu->GetMenuItems().rend()) {
        (*q)->Check(true);
        i=(*q)->GetId()-MachineMenuID;
    }
    Machine *m=cpap_machines[i];

    int idx=main_auinotebook->GetPageIndex(daily);
    if (idx!=wxNOT_FOUND) {
        daily->RefreshData(m);
        daily->Refresh();
    }
    idx=main_auinotebook->GetPageIndex(summary);
    if (idx!=wxNOT_FOUND) {
        summary->RefreshData(m);
        summary->Refresh();
    }

}
void SleepyHeadFrame::OnViewMenuDaily( wxCommandEvent& event )
{
    int idx=main_auinotebook->GetPageIndex(daily);
    int id;
    if (idx==wxNOT_FOUND) {
        daily=new Daily(this);
        main_auinotebook->AddPage(daily,_("Daily"),true);
        if (pref.Exists(wxT("DefaultMachine")))
            id=pref["DefaultMachine"].GetInteger();
        else id=0;
        if (id<cpap_machines.size()) {
            Machine *m=cpap_machines[id];
            if (m) daily->RefreshData(m);
        }
        daily->Refresh();

    } else {
        main_auinotebook->SetSelection(idx);
    }


}
void SleepyHeadFrame::OnViewMenuSummary( wxCommandEvent& event )
{

    int id,idx=main_auinotebook->GetPageIndex(summary);
    if (idx==wxNOT_FOUND) {
        summary=new Summary(this);
        main_auinotebook->AddPage(summary,_("Summary"),true);
        if (pref.Exists(wxT("DefaultMachine")))
            id=pref["DefaultMachine"].GetInteger();
        else id=0;
        if (id<cpap_machines.size()) {
            Machine *m=cpap_machines[id];
            if (m) summary->RefreshData(m);
        }
        summary->Refresh();
    } else {
        main_auinotebook->SetSelection(idx);
    }
}

Summary::Summary(wxWindow *win)
:SummaryPanel(win)
{
    machine=NULL;

    const int days_shown=60;

    AddData(ahidata=new HistoryData(machine,days_shown));
    AddData(pressure=new HistoryCodeData(machine,CPAP_PressureAverage,days_shown));
    AddData(pressure_eap=new HistoryCodeData(machine,BIPAP_EAPAverage,days_shown));
    AddData(pressure_iap=new HistoryCodeData(machine,BIPAP_IAPAverage,days_shown));
    AddData(leak=new HistoryCodeData(machine,CPAP_LeakMedian,days_shown));
    AddData(usage=new UsageHistoryData(machine,days_shown,UHD_Hours));
    AddData(waketime=new UsageHistoryData(machine,days_shown,UHD_Waketime));
    AddData(bedtime=new UsageHistoryData(machine,days_shown,UHD_Bedtime));

    AHI=new gGraphWindow(ScrolledWindow,-1,wxT("AHI"),wxPoint(0,0), wxSize(400,200), wxNO_BORDER);
    AHI->SetMargins(10,15,60,80);
    AHI->AddLayer(new gBarChart(ahidata,wxRED));
    //AHI->AddLayer(new gLineChart(ahidata,wxRED));
    fgSizer->Add(AHI,1,wxEXPAND);

    PRESSURE=new gGraphWindow(ScrolledWindow,-1,wxT("Pressure"),wxPoint(0,0), wxSize(400,200), wxNO_BORDER);
    PRESSURE->SetMargins(10,15,60,80);
    //PRESSURE->AddLayer(new gBarChart(pressure,wxBLUE));
    PRESSURE->AddLayer(new gLineChart(pressure,wxBLUE));
    PRESSURE->AddLayer(new gLineChart(pressure_eap,wxRED));
    PRESSURE->AddLayer(new gLineChart(pressure_iap,wxGREEN));
    fgSizer->Add(PRESSURE,1,wxEXPAND);

    LEAK=new gGraphWindow(ScrolledWindow,-1,wxT("Mask Leak"),wxPoint(0,0), wxSize(400,200), wxNO_BORDER);
    LEAK->SetMargins(10,15,60,80);
    //LEAK->AddLayer(new gBarChart(leak,wxYELLOW));
    LEAK->AddLayer(new gLineChart(leak,wxYELLOW));
    fgSizer->Add(LEAK,1,wxEXPAND);


    USAGE=new gGraphWindow(ScrolledWindow,-1,wxT("Usage (Hours)"),wxPoint(0,0), wxSize(400,200), wxNO_BORDER);
    USAGE->SetMargins(10,15,60,80);
    USAGE->AddLayer(new gBarChart(usage,wxGREEN));
    //USAGE->AddLayer(new gLineChart(usage,wxGREEN));
    fgSizer->Add(USAGE,1,wxEXPAND);


//    RefreshData();

}
Summary::~Summary()
{
}

void Summary::RefreshData(Machine *m)
{
    if (!m) return;
    machine=m;

    for (auto h=Data.begin();h!=Data.end();h++) {
        (*h)->SetMachine(m);
        (*h)->Update();
    }

    wxString submodel=_("Unknown Model");
    double ahi=ahidata->GetAverage();
    double avp=pressure->GetAverage();
    double bt=fmod(bedtime->GetAverage(),12.0);
    double ua=usage->GetAverage();
    double wt=waketime->GetAverage(); //fmod(bt+ua,12.0);

    wxString html=wxT("<html><body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0><table cellspacing=2 cellpadding=0>\n");

    if (machine) {
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Machine Information")+wxT("</i></td></tr>\n");
        if (machine->properties.find(wxT("SubModel"))!=machine->properties.end())
            submodel=wxT(" <br>\n ")+machine->properties[wxT("SubModel")];
        html=html+wxT("<tr><td colspan=2 align=center><b>")+machine->properties[wxT("Brand")]+wxT("</b> <br/>")+machine->properties[wxT("Model")]+wxT("&nbsp;")+machine->properties[wxT("ModelNumber")]+submodel+wxT("</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center>")+_("Firmware")+wxT(" ")+machine->properties[wxT("SoftwareVersion")]+wxT("</td></tr>");
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=left><i>")+_("Indice Averages")+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("AHI")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),ahi)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2fcmH2O"),avp)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Mask Leaks")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),leak->GetAverage())+wxT("</td></tr>\n");
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");

        html=html+wxT("<tr><td><b>")+_("Bedtime")+wxT("</b></td><td>")+wxString::Format(wxT("%02.0f:%02i"),bt,int(bt*60) % 60)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Waketime")+wxT("</b></td><td>")+wxString::Format(wxT("%02.0f:%02i"),wt,int(wt*60) % 60)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Hours/Night")+wxT("</b></td><td>")+wxString::Format(wxT("%02.0f:%02i"),ua,int(ua*60)%60)+wxT("</td></tr>\n");
        html=html+wxT("</table>");
    } else {
        html=html+_("Please import some data.");
    }
    html+=wxT("</body></html>");
    HTMLInfo->SetPage(html);


}

Daily::Daily(wxWindow *win)
:DailyPanel(win)
{
    machine=NULL;

    AddData(tap_eap=new TAPData(CPAP_EAP));
    AddData(tap_iap=new TAPData(CPAP_IAP));
    AddData(tap=new TAPData(CPAP_Pressure));

    TAP=new gGraphWindow(ScrolledWindow,-1,wxT("Time@Pressure"),wxPoint(0,0), wxSize(600,50), wxNO_BORDER);
    TAP->SetMargins(20,15,5,50);
    TAP->AddLayer(new gCandleStick(tap));

    TAP_IAP=new gGraphWindow(ScrolledWindow,-1,wxT("Time@IAP"),wxPoint(0,0), wxSize(600,50), wxNO_BORDER);
    TAP_IAP->SetMargins(20,15,5,50);
    TAP_IAP->AddLayer(new gCandleStick(tap_iap));

    TAP_EAP=new gGraphWindow(ScrolledWindow,-1,wxT("Time@EAP"),wxPoint(0,0), wxSize(600,50), wxNO_BORDER);
    TAP_EAP->SetMargins(20,15,5,50);
    TAP_EAP->AddLayer(new gCandleStick(tap_eap));

    G_AHI=new gGraphWindow(ScrolledWindow,-1,wxT("Event Breakdown"),wxPoint(0,0), wxSize(600,50), wxNO_BORDER);
    G_AHI->SetMargins(20,15,5,50);
    AddData(g_ahi=new AHIData());
    gCandleStick *l=new gCandleStick(g_ahi);
    l->AddName(wxT("H"));
    l->AddName(wxT("OA"));
    l->AddName(wxT("CA"));
    l->AddName(wxT("RE"));
    l->AddName(wxT("FL"));
    l->AddName(wxT("CSR"));
    l->color.clear();
    l->color.push_back(wxBLUE);
    l->color.push_back(wxAQUA);
    l->color.push_back(wxPURPLE);
    l->color.push_back(wxYELLOW);
    l->color.push_back(wxBLACK);
    l->color.push_back(wxGREEN2);
    G_AHI->AddLayer(l);

    AddData(leakdata=new PressureData(CPAP_Leak,0));
    LEAK=new gGraphWindow(ScrolledWindow,-1,wxT("Mask Leaks"),wxPoint(0,0), wxSize(600,150), wxNO_BORDER);
    LEAK->AddLayer(new gLineChart(leakdata,wxPURPLE,4096,false));

    AddData(pressure_iap=new PressureData(CPAP_IAP));
    AddData(pressure_eap=new PressureData(CPAP_EAP));
    AddData(prd=new PressureData(CPAP_Pressure));
    PRD=new gGraphWindow(ScrolledWindow,-1,wxT("Pressure"),wxPoint(0,0), wxSize(600,150), wxNO_BORDER);
    PRD->AddLayer(new gLineChart(prd,wxDARK_GREEN,4096,false));
    PRD->AddLayer(new gLineChart(pressure_iap,wxBLUE,4096,false,true));
    PRD->AddLayer(new gLineChart(pressure_eap,wxRED,4096,false,true));

    AddData(frw=new FlowData());
    FRW=new gGraphWindow(ScrolledWindow,-1,wxT("Flow Rate"),wxPoint(0,0), wxSize(600,150), wxNO_BORDER);

    AddData(flags[0]=new FlagData(CPAP_CSR,7,1,0));
    AddData(flags[1]=new FlagData(CPAP_ClearAirway,6));
    AddData(flags[2]=new FlagData(CPAP_Obstructive,5));
    AddData(flags[3]=new FlagData(CPAP_Hypopnea,4));
    AddData(flags[4]=new FlagData(CPAP_FlowLimit,3));
    AddData(flags[5]=new FlagData(CPAP_VSnore,2));
    AddData(flags[6]=new FlagData(CPAP_RERA,1));
    AddData(flags[7]=new FlagData(PRS1_PressurePulse,1));
    AddData(flags[8]=new FlagData(PRS1_VSnore2,1));
    AddData(flags[9]=new FlagData(PRS1_Unknown0E,1));

    FRW->AddLayer(new gLineOverlayBar(flags[0],wxGREEN2,wxT("CSR")));
    FRW->AddLayer(new gLineChart(frw,wxBLACK,200000,true));
    FRW->AddLayer(new gLineOverlayBar(flags[7],wxRED,wxT("PR"),LOT_Dot));
    FRW->AddLayer(new gLineOverlayBar(flags[6],wxYELLOW,wxT("RE")));
    FRW->AddLayer(new gLineOverlayBar(flags[9],wxDARK_GREEN,wxT("U0E")));
    FRW->AddLayer(new gLineOverlayBar(flags[5],wxRED,wxT("VS")));
    FRW->AddLayer(new gLineOverlayBar(flags[4],wxBLACK,wxT("FL")));
    FRW->AddLayer(new gLineOverlayBar(flags[3],wxBLUE,wxT("H")));
    FRW->AddLayer(new gLineOverlayBar(flags[2],wxAQUA,wxT("OA")));
    FRW->AddLayer(new gLineOverlayBar(flags[1],wxPURPLE,wxT("CA")));

    SF=new gGraphWindow(ScrolledWindow,-1,wxT("Sleep Flags"),wxPoint(0,0), wxSize(600,150), wxNO_BORDER);
    SF->SetMargins(10,15,20,80);

    SF->LinkZoom(FRW);
    #if defined(__UNIX__)
 //   SF->LinkZoom(PRD); // Uncomment to link in more graphs.. Too slow on windows.
 //   SF->LinkZoom(LEAK);
    #endif

    const int sfc=9;

    SF->AddLayer(new gFlagsLine(flags[9],wxDARK_GREEN,wxT("U0E"),8,sfc));
    SF->AddLayer(new gFlagsLine(flags[8],wxRED,wxT("VS2"),6,sfc));
    SF->AddLayer(new gFlagsLine(flags[6],wxYELLOW,wxT("RE"),7,sfc));
    SF->AddLayer(new gFlagsLine(flags[5],wxRED,wxT("VS"),5,sfc));
    SF->AddLayer(new gFlagsLine(flags[4],wxBLACK,wxT("FL"),4,sfc));
    SF->AddLayer(new gFlagsLine(flags[3],wxBLUE,wxT("H"),3,sfc));
    SF->AddLayer(new gFlagsLine(flags[2],wxAQUA,wxT("OA"),2,sfc));
    SF->AddLayer(new gFlagsLine(flags[1],wxPURPLE,wxT("CA"),1,sfc));
    SF->AddLayer(new gFlagsLine(flags[0],wxGREEN2,wxT("CSR"),0,sfc));

    fgSizer->Add(SF,1,wxEXPAND);
    fgSizer->Add(G_AHI,1,wxEXPAND);
    fgSizer->Add(FRW,1,wxEXPAND);
    fgSizer->Add(PRD,1,wxEXPAND);
    fgSizer->Add(LEAK,1,wxEXPAND);
    fgSizer->Add(TAP,1,wxEXPAND);
    fgSizer->Add(TAP_IAP,1,wxEXPAND);
    fgSizer->Add(TAP_EAP,1,wxEXPAND);
    //fgSizer->Layout();

    foobar_datehack=false; // this exists due to a wxGTK bug.
  //  RefreshData();

}
Daily::~Daily()
{

}
void Daily::RefreshData(Machine *m)
{
    if (!m) return;
    machine=m;


    wxDateTime day=m->LastDay()+wxTimeSpan::Days(1);
    Calendar->SetDate(day);
    day.ResetTime();
    day.SetHour(0);
    //et-=wxTimeSpan::Days(1);
    UpdateGraphs(day);
    wxCalendarEvent ev;
    ev.SetDate(day);
    OnCalendarMonth(ev);
    OnCalendarDay(ev);
}
///usr/local/bin/upx ./bin/Windows/SleepyHead

void Daily::OnCalendarDay( wxCalendarEvent& event )
{
    if (foobar_datehack) {
        OnCalendarMonth(event);
        foobar_datehack=false;
    }
	if (!machine) return;

    wxDateTime day=event.GetDate();
    day.ResetTime();
    day.SetHour(0);
    day-=wxTimeSpan::Days(1);
    Day *d;
    if (machine && (machine->day.find(day)!=machine->day.end()) && (d=machine->day[day]) && (d->size()>0)) { // && ((d->last()-d->first())>wxTimeSpan::Minutes(15))) {
    //HTMLInfo->SetPage(wxT(""));
        CPAPMode mode=(CPAPMode)d->summary_max(CPAP_Mode);
        UpdateGraphs(day);
        if (mode!=MODE_BIPAP) {

            TAP_EAP->Show(false);
            TAP_IAP->Show(false);
            TAP->Show(true);
        } else {
            TAP->Show(false);
            TAP_IAP->Show(true);
            TAP_EAP->Show(true);
        }

        TAP_EAP->Refresh();
        TAP_IAP->Refresh();
        TAP->Refresh();
        fgSizer->Layout();
//        Update();
        ScrolledWindow->FitInside();
        //ScrolledWindow->Refresh();
//        wxWindow::UpdateWindowUI();

//        Session *s=(*machine->day[day])[0];
        PRTypes pr=(PRTypes)d->summary_max(CPAP_PressureReliefType);
        wxString epr=PressureReliefNames[pr]+wxString::Format(wxT(" x%i"),(int)d->summary_max(CPAP_PressureReliefSetting));
        wxString modestr=CPAPModeNames[mode];

        float ahi=(d->count(CPAP_Obstructive)+d->count(CPAP_Hypopnea)+d->count(CPAP_ClearAirway))/d->hours();
        float csr=(100.0/d->hours())*(d->sum(CPAP_CSR)/3600.0);
        float oai=d->count(CPAP_Obstructive)/d->hours();
        float hi=d->count(CPAP_Hypopnea)/d->hours();
        float cai=d->count(CPAP_ClearAirway)/d->hours();
        float rei=d->count(CPAP_RERA)/d->hours();
        float vsi=d->count(CPAP_VSnore)/d->hours();
        float fli=d->count(CPAP_FlowLimit)/d->hours();
        wxString submodel=_("Unknown Model");


        wxString html=wxT("<html><body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0><table cellspacing=2 cellpadding=0>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Machine Information")+wxT("</i></td></tr>\n");
        if (machine->properties.find(wxT("SubModel"))!=machine->properties.end())
            submodel=wxT(" <br>")+machine->properties[wxT("SubModel")];
        html=html+wxT("<tr><td colspan=2 align=center><b>")+machine->properties[wxT("Brand")]+wxT("</b> <br>")+machine->properties[wxT("Model")]+wxT(" ")+machine->properties[wxT("ModelNumber")]+submodel+wxT("</td></tr>\n");
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Sleep Times")+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Date")+wxT("</b></td><td>")+machine->day[day]->first().Format(wxT("%x"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Sleep")+wxT("</b></td><td>")+machine->day[day]->first().Format(wxT("%H:%M"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Wake")+wxT("</b></td><td>")+machine->day[day]->last().Format(wxT("%H:%M"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Total Time")+wxT("</b></td><td><i>")+machine->day[day]->total_time().Format(wxT("%H:%M&nbsp;hours"))+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Indices")+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("AHI")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),ahi)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Obstructive")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),oai)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Hypopnea")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),hi)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("ClearAirway")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),cai)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("RERA")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),rei)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("FlowLimit")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),fli)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Vsnore")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),vsi)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("CSR")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f%%"),csr)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Other Information")+wxT("</i></td></tr>\n");


//        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
//        html=html+wxT("<tr><td colspan=2 align=center><i>Session Informaton</i></td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Mode")+wxT("</b></td><td>")+modestr+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Relief")+wxT("</b></td><td>")+epr+wxT("</td></tr>\n");
        if (mode==MODE_CPAP) {
            html=html+wxT("<tr><td><b>")+_("Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMin))+wxT("</td></tr>\n");
        } else if (mode==MODE_APAP) {
            html=html+wxT("<tr><td><b>")+_("Avg&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.2fcmH2O"),d->summary_avg(CPAP_PressureAverage))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Min&nbsp;Reached")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMinAchieved))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Max&nbsp;Reached")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(CPAP_PressureMaxAchieved))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("90%&nbsp;Pressure?")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_avg(CPAP_PressurePercentValue))+wxT("</td></tr>\n");
        } else if (mode==MODE_BIPAP) {
            html=html+wxT("<tr><td><b>")+_("Avg IAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.2fcmH2O"),d->summary_avg(BIPAP_IAPAverage))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Avg EAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_avg(BIPAP_EAPAverage))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Min IAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.2fcmH2O"),d->summary_min(BIPAP_IAPMin))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Max IAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(BIPAP_IAPMax))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Min EAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(BIPAP_EAPMin))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Max EAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(BIPAP_EAPMax))+wxT("</td></tr>\n");

        }
        html=html+wxT("<tr><td><b>")+_("Avg Leak")+wxT("</b></td><td>")+wxString::Format(wxT("%.2f"),d->summary_avg(CPAP_LeakAverage))+wxT("</td></tr>\n");

        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("System Settings")+wxT("</i></td></tr>\n");

        if (mode==MODE_CPAP) {
            html=html+wxT("<tr><td><b>")+_("Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMin))+wxT("</td></tr>\n");
        } else if (mode==MODE_APAP) {
            html=html+wxT("<tr><td><b>")+_("Pressure&nbsp;Min")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMin))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Pressure&nbsp;Max")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(CPAP_PressureMax))+wxT("</td></tr>\n");
        } else if (mode==MODE_BIPAP) {
            html=html+wxT("<tr><td><b>")+_("Pressure&nbsp;IAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMin))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Pressure&nbsp;EAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(CPAP_PressureMax))+wxT("</td></tr>\n");
        }
        html=html+wxT("<tr><td><b>")+_("Ramp-Time")+wxT("</b></td><td>")+wxString::Format(wxT("%imin"),(int)d->summary_max(CPAP_RampTime))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Ramp-Prs.")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_RampStartingPressure))+wxT("</td></tr>\n");

        // check HumidiferStatus..
        wxString str;
        if (bool(d->summary_max(CPAP_HumidifierStatus))) {
            str=wxString::Format(wxT("x%i"),(int)d->summary_max(CPAP_HumidifierSetting));
        } else str=wxT("No");
        html=html+wxT("<tr><td><b>")+_("Humidifier")+wxT("</b></td><td>")+str+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("System-Lock")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_SystemLockStatus)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Auto-Off")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_AutoOff)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Mask-Alert")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_MaskAlert)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Show-AHI")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_ShowAHI)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Hose-Size")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_HoseDiameter)) ? _("22mm") : _("15mm"))+wxT("</td></tr>\n");
        if (bool(d->summary_max(PRS1_SystemResistanceStatus))) {
            str=wxString::Format(wxT("x%i"),(int)d->summary_max(PRS1_SystemResistanceSetting));
        } else str=wxT("No");
        html=html+wxT("<tr><td><b>")+_("Sys-Resist.")+wxT("</b></td><td>")+str+wxT("</td></tr>\n");

        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Session Files")+wxT("</i></td></tr>\n");
        for (auto i=d->begin();i!=d->end();i++) {
            html=html+wxT("<tr><td colspan=2 align=center>")+(*i)->first().Format(wxT("%d-%m-%Y %H:%M:%S"))+wxT(" ")+wxString::Format(wxT("%05i"),(*i)->session())+wxT("</td></tr>\n");
        }
        //PRS1_SystemLockStatus

        html=html+wxT("</table>");
      /*  for (auto i=s->summary.begin();i!=s->summary.end();i++) {
            MachineCode c=(*i).first;
            wxString name;
            if (DefaultMCShortNames.find(c)!=DefaultMCShortNames.end()) name=DefaultMCShortNames[c];
            else name=wxString::Format(wxT("%04i"),(int)c);
            html+=name+wxT(" = ")+(*i).second.GetString()+wxT("<br/>\n");
        } */
        html+=wxT("</body></html>");
        HTMLInfo->SetPage(html);
    } else {
        HTMLInfo->SetPage(_("No CPAP Machine Data Available"));
        UpdateGraphs(wxInvalidDateTime);
    }

}
void Daily::UpdateGraphs(wxDateTime d)
{
    Day *day=NULL;
    if (!machine) return;
    if (d!=wxInvalidDateTime) {
        if (machine->day.find(d)!=machine->day.end()) {
            day=machine->day[d];
        }
    }
    for (auto g=Data.begin();g!=Data.end();g++) {

        (*g)->Update(day);
    }
};

void Daily::OnCalendarMonth( wxCalendarEvent& event )
{
    wxDateTime et=event.GetDate();
    if (!et.IsValid()) {
        foobar_datehack=true;
        return;
    }

	wxDateTime::Month m=et.GetMonth();
	int y=et.GetYear();

    static wxFont f=*wxNORMAL_FONT; //wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    f.SetWeight(wxBOLD);

	if (!machine) return;
	for (int i=1;i<31;i++) {
	    int j=wxDateTime::GetNumberOfDays(m,y);
		if (i>j) break;
		wxDateTime d(i,m,y,0,0,0,0);
		d-=wxTimeSpan::Days(1);
		if ((machine->day.find(d)!=machine->day.end())) {
#if wxCHECK_VERSION(2,9,0)
			Calendar->Mark(i,true);
#else
            wxCalendarDateAttr *a=new wxCalendarDateAttr();
            a->SetFont(f);
            //wxNORM
			Calendar->SetAttr(i,a);
#endif
		} else {
#if wxCHECK_VERSION(2,9,0)
			Calendar->Mark(i,false);
#else
		    Calendar->ResetAttr(i);
#endif
//			Calendar->SetAttr(i,NULL);
		}
	}
}


