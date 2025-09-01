#include <sdk.h> // Code::Blocks SDK
#include <configurationpanel.h>
//#include "cbcolourmanager.h"
#include "CBBkgdColor.h"
//#include "cbeditor.h" //not used
#include "cbstyledtextctrl.h"
#include <wx/colour.h>
#include <wx/stc/stc.h>  // for wxSTC_STYLE_DEFAULT and related constants
#include "loggers.h"


// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
// ----------------------------------------------------------------------------
namespace
// ----------------------------------------------------------------------------
{
    PluginRegistrant<CBBkgdColor> reg(_T("CBBkgdColor"));
    // File to hold data from .conf file
    wxTextFile confData;

}


    // events handling
    BEGIN_EVENT_TABLE(CBBkgdColor, cbPlugin)
    // add any events you want to handle here
    END_EVENT_TABLE()

// constructor
// ----------------------------------------------------------------------------
CBBkgdColor::CBBkgdColor()
// ----------------------------------------------------------------------------
{
    // Make sure our resources are available.
    // In the generated boilerplate code we have no resources but when
    // we add some, it will be nice that this code is in place already ;)
    //if(!Manager::LoadResource(_T("CBBkgdColor.zip")))
    //{
    //    NotifyMissingFile(_T("CBBkgdColor.zip"));
    //}
}

// destructor
// ----------------------------------------------------------------------------
CBBkgdColor::~CBBkgdColor()
// ----------------------------------------------------------------------------
{
}
// ----------------------------------------------------------------------------
void CBBkgdColor::OnAttach()
// ----------------------------------------------------------------------------
{
    // do whatever initialization you need for your plugin
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be TRUE...
    // You should check for it in other functions, because if it
    // is FALSE, it means that the application did *not* "load"
    // (see: does not need) this plugin...

    Manager* pMgr = Manager::Get();
    // Register for events
    pMgr->RegisterEventSink(cbEVT_APP_STARTUP_DONE, new cbEventFunctor<CBBkgdColor, CodeBlocksEvent>(this, &CBBkgdColor::OnAppStartupDone));
    pMgr->RegisterEventSink(cbEVT_SETTINGS_CHANGED, new cbEventFunctor<CBBkgdColor, CodeBlocksEvent>(this, &CBBkgdColor::OnSettingsChanged));
    // Set current plugin version
	PluginInfo* pInfo = (PluginInfo*)(Manager::Get()->GetPluginManager()->GetPluginInfo(this));
	pInfo->version = wxT(VERSION);

}
// ----------------------------------------------------------------------------
void CBBkgdColor::OnRelease(bool appShutDown)
// ----------------------------------------------------------------------------
{
    // do de-initialization for your plugin
    // if appShutDown is true, the plugin is unloaded because Code::Blocks is being shut down,
    // which means you must not use any of the SDK Managers
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be FALSE...

    Disconnect(wxEVT_IDLE, wxIdleEventHandler(CBBkgdColor::OnIdle) );

}
// ----------------------------------------------------------------------------
void CBBkgdColor::BuildMenu(wxMenuBar* menuBar)
// ----------------------------------------------------------------------------
{
    //The application is offering its menubar for your plugin,
    //to add any menu items you want...
    //Append any items you need in the menu...
    //NOTE: Be careful in here... The application's menubar is at your disposal.

    //NotImplemented(_T("CBBkgdColor::BuildMenu()"));
}
// ----------------------------------------------------------------------------
void CBBkgdColor::BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data)
// ----------------------------------------------------------------------------
{
    //Some library module is ready to display a pop-up menu.
    //Check the parameter \"type\" and see which module it is
    //and append any items you need in the menu...
    //TIP: for consistency, add a separator as the first item...

    //NotImplemented(_T("CBBkgdColor::BuildModuleMenu()"));
}
// ----------------------------------------------------------------------------
void CBBkgdColor::OnAppStartupDone(CodeBlocksEvent& event)
// ----------------------------------------------------------------------------
{
    // Get the users desired color from the Syntax Highlighting settings
    // Set all background color to match that of the Editor backgroun color

    if (not m_startupDone)
    {
        m_startupDone += 1;
        m_UsrBkgdColor = GetDefaultEditorBackground();
        if (m_UsrBkgdColor == wxColour(0,0,0))
            return; //Dont set uninitialized color

        Connect(wxEVT_IDLE, wxIdleEventHandler(CBBkgdColor::OnIdle) );
    }

    wxColour color = m_UsrBkgdColor;
    SetAllWindowsBackgroundColor(m_UsrBkgdColor); // my desired color 254,233,201

    return;
}
// ----------------------------------------------------------------------------
void CBBkgdColor::OnSettingsChanged(CodeBlocksEvent& event)
// ----------------------------------------------------------------------------
{
    // Get the users desired color from the Syntax Highlighting settings
    // Set all background color to match that of the Editor backgroun color

        const int value = event.GetInt();
        if (value < int(cbSettingsType::First) || value >= int(cbSettingsType::Last))
            return;
        const cbSettingsType settingType = cbSettingsType(value);
        if (not (settingType == cbSettingsType::Editor))
            return;

        // stop OnIdle() processing
        Disconnect(wxEVT_IDLE, wxIdleEventHandler(CBBkgdColor::OnIdle) );
        // Stop OnSetFocus() events processing
        for (const auto& pair : logCtrlMap)
        {
            wxWindow* key = pair.first;
            key->Unbind(wxEVT_SET_FOCUS, &CBBkgdColor::OnSetFocus, this);
        }
        // clear the log pointers and say Starup done is false
        logCtrlMap.clear();
        m_startupDone = 0;

       // Defer editor changeed logic until after all other plugin process changes
        CallAfter([this]() {
            this->OnSettingsChangedPostProcessing(); }) ;

    return;
}
// ----------------------------------------------------------------------------
void CBBkgdColor::OnSettingsChangedPostProcessing()
// ----------------------------------------------------------------------------
{
    // A restart is needed after Editor settings changes
    CodeBlocksEvent evt(cbEVT_APP_STARTUP_DONE);
    OnAppStartupDone(evt);
}
// ----------------------------------------------------------------------------
void CBBkgdColor::OnIdle(wxIdleEvent& event)
// ----------------------------------------------------------------------------
{
    event.Skip();

    // On Idle will refresh the background colors until wxWidgets stops changing them.
    // Keep trying until wxWidgets stops over-writing our color choice
    if (m_startupDone < m_Max_Tries)
    {
        m_startupDone += 1;
        CodeBlocksEvent evt;
        OnAppStartupDone(evt);
        return;
    }

    // After max tries are done, bind event SET_FOCUS to each log control
    // and register to get CodeBlocks EditorOpen and EditorClose events.    // (ph 25/08/12)
    if (m_startupDone == m_Max_Tries)
    {
        // This code is executed once only

        m_startupDone += 1; // Increment so we only run this code once
        MakeLogCtrlMap();
        for (const auto& pair : logCtrlMap)
        {
            wxWindow* key = pair.first;
            key->Bind(wxEVT_SET_FOCUS, &CBBkgdColor::OnSetFocus, this);
        }

        // deprecated
        //- Register Editor Open/Close events // (ph 25/08/12)
        //-Manager::Get()->RegisterEventSink(cbEVT_EDITOR_CLOSE, new cbEventFunctor<CBBkgdColor, CodeBlocksEvent>(this, &CBBkgdColor::OnEditorClosed));
        //-Manager::Get()->RegisterEventSink(cbEVT_EDITOR_OPEN, new cbEventFunctor<CBBkgdColor, CodeBlocksEvent>(this, &CBBkgdColor::OnEditorOpened));
    }
}

#include "cbstyledtextctrl.h"
// ----------------------------------------------------------------------------
void CBBkgdColor::OnEditorOpened(CodeBlocksEvent& event)  // (ph 25/08/12)
// ----------------------------------------------------------------------------
{
    /// deprecated
    event.Skip();
    return;
}
// ----------------------------------------------------------------------------
void CBBkgdColor::OnEditorClosed(CodeBlocksEvent& event)
// ----------------------------------------------------------------------------
{
    /// deprecated
    event.Skip();
    return;
}
// ----------------------------------------------------------------------------
void CBBkgdColor::OnSetFocus(wxFocusEvent& event)
// ----------------------------------------------------------------------------
{
    event.Skip();

    wxObject* obj = event.GetEventObject();

    wxWindow* pWin = nullptr;
    wxString logName = wxString();

    if (obj && obj->IsKindOf(CLASSINFO(wxWindow)))
        pWin = (wxWindow*)obj;
    else return;

    auto it = logCtrlMap.find(pWin);

    if (it != logCtrlMap.end())
        logName = it->second;
    else
        return;

    if (obj && obj->IsKindOf(CLASSINFO(wxTextCtrl)))
    {
        SetTextLogStyleBackground(logName, m_UsrBkgdColor);
    }
    else if (obj && obj->IsKindOf(CLASSINFO(wxListCtrl)))
    {
        SetListLogStyleBackground(logName, m_UsrBkgdColor);

    }
}
// ----------------------------------------------------------------------------
void CBBkgdColor::SetAllWindowsBackgroundColor(const wxColour& color)
// ----------------------------------------------------------------------------
{
    // Call this function to enumerate all top-level windows and set background color

    const wxWindowList& topLevelWindows = wxTopLevelWindows;
    for (wxWindowList::const_iterator it = topLevelWindows.begin(); it != topLevelWindows.end(); ++it)
    {
        wxWindow* topWin = *it;
        SetBackgroundColorRecursively(topWin, color);
        // Optionally force refresh to apply the color immediately
        topWin->Refresh();
    }

    SetTextLogStyleBackground("Code::Blocks", color);
    SetTextLogStyleBackground("Code::Blocks Debug", color);
    SetTextLogStyleBackground("Build log", color);
    SetListLogStyleBackground("Build messages", color); // Is not a textctrl
    SetTextLogStyleBackground("Debugger", color);
    SetListLogStyleBackground("LSP messages", color);   //Is not a textctrl
    SetListLogStyleBackground("Search results", color); //Is not a textctrl
}
// ----------------------------------------------------------------------------
void CBBkgdColor::SetBackgroundColorRecursively(wxWindow* window, const wxColour& color)
// ----------------------------------------------------------------------------
{
    // Recursive function that enumerates all children of a given window,
    // applies background color, and handles wxAuiNotebook pages explicitly.

    if (!window)
        return;

    // Set background color for the current window
    window->SetBackgroundColour(color);

    // If this window is a wxAuiNotebook, enumerate its pages
    wxAuiNotebook* notebook = wxDynamicCast(window, wxAuiNotebook);
    if (notebook)
    {
        int pageCount = notebook->GetPageCount();
        for (int i = 0; i < pageCount; ++i)
        {
            wxWindow* page = notebook->GetPage(i);
            if (page)
            {

                // Get page label/text
                wxString pageName = notebook->GetPageText(i);
                // Get page window runtime class name
                wxString pageType = page->GetClassInfo()->GetClassName();

                page->SetBackgroundColour(color);
                // Recursively process the children of each notebook page
                //SetBackgroundColorRecursively(page, color);
                page->Refresh();
                page->Update();
            }
            notebook->Refresh();
        }
    }

    // Recurse into all children of this window
    const wxWindowList& children = window->GetChildren();
    for (wxWindowList::const_iterator it = children.begin(); it != children.end(); ++it)
    {
        wxWindow* child = *it;
        SetBackgroundColorRecursively(child, color);
    }
}
// ----------------------------------------------------------------------------
int CBBkgdColor::GetLogIndex (const wxString& logRequest)
// ----------------------------------------------------------------------------
{
    // Usage:
    //    m_cbSearchResultsLogIndex = GetLogIndex (_T("Search results") );

    //    LogManager* pLogMgr = Manager::Get()->GetLogManager();
    //    LogSlot& logslot = pLogMgr->Slot(m_cbSearchResultsLogIndex);
    //    ListCtrlLogger* pLogger = (ListCtrlLogger*)logslot.GetLogger();
    //    pLogger->Clear();



    LogManager* pLogMgr = Manager::Get()->GetLogManager();
    int nNumLogs = 16; //just a guess

    int lm_CodeBlocksLogIndex = 0;                  // Code::Blocks
    int lm_CodeBlocksDebugLogIndex = 0;             // Code::Blocks Debug
    int lm_BuildLogIndex = 0;                       // Build log
    int lm_BuildMsgLogIndex = 0;                    // Build messages
    int lm_DebuggerLogIndex = 0;                    // Debugger
    int lm_CodeBlocksLSPMessagesLogIndex = 0;       // LSP messages
    int lm_CodeBlocksSearchResultsLogIndex = 0;     // Search results


    for (int i = 0; i < nNumLogs; ++i)
    {
        LogSlot& logSlot = pLogMgr->Slot (i);
        if (pLogMgr->FindIndex(logSlot.log) == pLogMgr->invalid_log) continue;
        {
            if ( logSlot.title.IsSameAs (wxT ("Code::Blocks") ) )
                lm_CodeBlocksLogIndex = i;
            else if ( logSlot.title.IsSameAs (wxT ("Code::Blocks Debug") ) )
                lm_CodeBlocksDebugLogIndex = i;
            else if ( logSlot.title.IsSameAs (wxT ("Build log") ) )
                lm_BuildLogIndex = i;
            else if ( logSlot.title.IsSameAs (wxT ("Build messages") ) )
                lm_BuildMsgLogIndex = i;
            else if ( logSlot.title.IsSameAs (wxT ("Debugger") ) )
                lm_DebuggerLogIndex = i;
            else if ( logSlot.title.IsSameAs (wxT ("Search results") ) )
                lm_CodeBlocksSearchResultsLogIndex = i;
            else if ( logSlot.title.IsSameAs (wxT ("LSP messages") ) )
                lm_CodeBlocksLSPMessagesLogIndex = i;
        }
    }//for

    if (logRequest == _ ("Code::Blocks") )
        return lm_CodeBlocksLogIndex;
    else if (logRequest == _ ("Code::Blocks Debug") )
        return lm_CodeBlocksDebugLogIndex;
    else if (logRequest == _ ("Build log") )
        return lm_BuildLogIndex;
    else if (logRequest == _ ("Build messages") )
        return lm_BuildMsgLogIndex;
    else if (logRequest == _ ("Debugger") )
        return lm_DebuggerLogIndex;
    else if (logRequest == _ ("Search results") )
        return lm_CodeBlocksSearchResultsLogIndex;
    else if (logRequest == _ ("LSP messages") )
        return lm_CodeBlocksLSPMessagesLogIndex;

    return 0;
}

#include "wx/colour.h"
#include "logger.h"
#include <type_traits>

// ----------------------------------------------------------------------------
class AccessTextCtrl : public TextCtrlLogger
// ----------------------------------------------------------------------------
{
    // Since only refs to protected members can be used by derived classes
    // and never from outside the class,
    // this code accesses the parent class protected vars by reference
    // and returns them so the caller can use the ref or cast to a pointer.
    // Note the 'using' statement in the 'Derived struct'.

    //  protected:
    //    // style and control vars are declared as:
    //    wxTextCtrl* control;
    //    bool        fixed;
    //    wxTextAttr  style[num_levels];
    //
    // num_levels is declared (in logger.h):
    //  enum level { caption, info, warning, success, error, critical, failure, pagetitle, spacer, asterisk };
    //  enum { num_levels = asterisk +1 };

  public:
    AccessTextCtrl() {}
    ~AccessTextCtrl() {}

    // Type alias for reference to the style array of size num_levels in logger.h
    typedef wxTextAttr (&StyleArrayRef)[Logger::num_levels];

    // Type alias for pointer to wxTextCtrl (control is a pointer)
    typedef wxTextCtrl* ControlPtr;

    // Accessor for the protected 'style' array
    // ----------------------------------------------------------------------------
    StyleArrayRef access_protected_style(TextCtrlLogger& base)
    // ----------------------------------------------------------------------------
    {
        struct Derived : TextCtrlLogger {
            using TextCtrlLogger::style;   // Promote access to style
        };
        return base.*(&Derived::style);
    }

    // Accessor for the protected 'control' pointer
    // ----------------------------------------------------------------------------
    ControlPtr& access_protected_control(TextCtrlLogger& base)
    // ----------------------------------------------------------------------------
    {
        struct Derived : TextCtrlLogger {
            using TextCtrlLogger::control;  // Promote access to control
        };
        return base.*(&Derived::control);
    }
};
// ----------------------------------------------------------------------------
class AccessListCtrl : public ListCtrlLogger
// ----------------------------------------------------------------------------
{
    // Since only refs to protected members can be used by derived classes
    // and never from outside the class,
    // this code accesses the parent class protected vars by reference
    // and returns them so the caller can use the ref or cast to a pointer.
    // Note the 'using' statement in the 'Derived' struct.

    // For ListCtrlLogger, the 'control' var is open to access, but the'style'
    // var is wrapped by a struct. So setting the 'style' var must be done
    // within this derived class but the 'control' var can be accessed directly
    // by ref or casting it, by outide caller, to ref or pointer.

    // Note also that the ListCtrlLogger style var is NOT a wxTextAttr, so its
    // text background color cannot be set, only the text foreground color. Rather
    //  than a wxTextAttr, it's declared as:
    //    struct ListStyles
    //    {
    //        wxFont font;
    //        wxColour colour;
    //    };
    //    ListStyles style[num_levels];
    //
    // num_levels is declared (in logger.h):
    //  enum level { caption, info, warning, success, error, critical, failure, pagetitle, spacer, asterisk };
    //  enum { num_levels = asterisk +1 };

    // The only way I've found to set a List logger background is by setting the ctrl background.
    // See OnSetFocus() and SetBackgroundColorRecursively() functions.
    // However, when the ListCtrlLogger patches ListStyle to wxTextAttr, the background color
    // can be set.

  public:
    typedef wxListCtrl* ControlPtr;

    // ----------------------------------------------------------------------------
    static ControlPtr& access_protected_control(ListCtrlLogger& base)
    // ----------------------------------------------------------------------------
    {
        struct Derived : ListCtrlLogger {
            using ListCtrlLogger::control;
        };
        return base.*(&Derived::control);
    }

    // Return reference to style array; type deduced by caller using 'auto'
    // ----------------------------------------------------------------------------
    static auto& access_protected_style(ListCtrlLogger& base)
    // ----------------------------------------------------------------------------
    {
        struct Derived : ListCtrlLogger {
            using ListCtrlLogger::style;
        };
        return base.*(&Derived::style);
    }

    // Helper function for wxTextAttr style type (new style)
    // ----------------------------------------------------------------------------
    template <typename StyleArray>
    static void SetStyleColourImpl(StyleArray& styles, int index, wxColour const& color, std::true_type /*is_wxTextAttr*/)
    // ----------------------------------------------------------------------------
    {
        styles[index].SetBackgroundColour(color);
    }

    // Helper function for non-wxTextAttr style type (old style with 'colour' member)
    // ----------------------------------------------------------------------------
    template <typename StyleArray>
    static void SetStyleColourImpl(StyleArray& styles, int index, wxColour const& color, std::false_type /*is_wxTextAttr*/)
    // ----------------------------------------------------------------------------
    {
        /// Don't do this. Both fore and background text color will be the same.
        //styles[index].colour = color;
    }

    // Main function, dispatches to the correct implementation based on StyleType
    // either the older ListStyles.colour or the newer wxTextAttr.
    // ----------------------------------------------------------------------------
    static void SetStyleColour(ListCtrlLogger& base, int index, wxColour const& color)
    // ----------------------------------------------------------------------------
    {
        //Debugging
        wxString hexValue = color.GetAsString(wxC2S_HTML_SYNTAX);

        auto& styles = access_protected_style(base);

        if (index < 0 || index >= Logger::num_levels)
            return;

        using StyleType = typename std::decay<decltype(styles[0])>::type;

        // Dispatch using std::is_same, creating true_type or false_type tag
        SetStyleColourImpl(styles, index, color, std::is_same<StyleType, wxTextAttr>{});
    }
};

// ----------------------------------------------------------------------------
bool CBBkgdColor::SetTextLogStyleBackground(wxString logTitle, wxColour color)
// ----------------------------------------------------------------------------
{
    int logIndex = GetLogIndex(logTitle);
    if (logIndex)
    {
        LogManager* pLogMgr = Manager::Get()->GetLogManager();
        LogSlot& logslot = pLogMgr->Slot(logIndex);
        TextCtrlLogger* pLogger = (TextCtrlLogger*)logslot.GetLogger();
        if (pLogger)
        {
            TextCtrlLogger* pTextLogger = pLogger;
            if (pTextLogger)
            {

                AccessTextCtrl logger;

                // Access the protected 'control' pointer of the existing logger via pointer
                wxTextCtrl*& control_ref = logger.access_protected_control(*pTextLogger);

                // test for wxTextCtrl or wxListCtrl
                wxWindow* pControl = control_ref;  // cast to wxWindow*
                // Use wxDynamicCast to test for wxTextCtrl
                bool isTextCtrl = false;
                wxTextCtrl* textCtrl = wxDynamicCast(pControl, wxTextCtrl);
                if (textCtrl) isTextCtrl = true;
                if ((not control_ref) or (not isTextCtrl))
                    return false;

                if (control_ref)
                {
                    // set window background colour on control
                    control_ref->SetBackgroundColour(color);
                }

                // Access the protected 'style' array of the existing logger via pointer
                wxTextAttr (&style_ref)[Logger::num_levels] = logger.access_protected_style(*pTextLogger);
                style_ref[Logger::info].SetBackgroundColour(color);  // set text background

                control_ref->Refresh();
                control_ref->Update();

                return true;
            }
        }
    }
    return false;
}
// ----------------------------------------------------------------------------
bool CBBkgdColor::SetListLogStyleBackground(wxString logTitle, wxColour color)
// ----------------------------------------------------------------------------
{
    int logIndex = GetLogIndex(logTitle);
    if (not logIndex)
        return false;

    LogManager* pLogMgr = Manager::Get()->GetLogManager();
    LogSlot& logslot = pLogMgr->Slot(logIndex);
    ListCtrlLogger* pLogger = static_cast<ListCtrlLogger*>(logslot.GetLogger());
    if (!pLogger)
        return false;

    wxListCtrl*& control_ref = AccessListCtrl::access_protected_control(*pLogger);
    if (!control_ref)
        return false;

    wxListCtrl* listCtrl = wxDynamicCast(control_ref, wxListCtrl);
    if (!listCtrl)
        return false;

    control_ref->SetBackgroundColour(color);

    /// Dont't do this; a list logger only supports foreground colors
    // Use the helper function that hides use of protected nested type
    //AccessListCtrl::SetStyleColour(*pLogger, Logger::info, color);
    AccessListCtrl::SetStyleColour(*pLogger, Logger::info, color);

    control_ref->Refresh();
    control_ref->Update();

    return true;
}
// ----------------------------------------------------------------------------
int CBBkgdColor::MakeLogCtrlMap()
// ----------------------------------------------------------------------------
{
    LogManager* pLogMgr = Manager::Get()->GetLogManager();
    for (int logIndex=0; logIndex < 16; ++logIndex)
    {
        LogSlot& logSlot = pLogMgr->Slot (logIndex);
        if (pLogMgr->FindIndex(logSlot.log) == pLogMgr->invalid_log)
            continue;
        if (logSlot.title.StartsWith("std"))
            continue;

        TextCtrlLogger* pLogger = (TextCtrlLogger*)logSlot.GetLogger();
        if (pLogger)
        {
            TextCtrlLogger* pTextLogger = pLogger;
            if (pTextLogger)
            {
                AccessTextCtrl logger;

                // Access the protected 'control' pointer of the existing logger via pointer
                wxTextCtrl*& control_ref = logger.access_protected_control(*pTextLogger);
                // test for wxTextCtrl or wxListCtrl
                wxWindow* pControl = control_ref;  // cast to wxWindow*

                // Use wxDynamicCast to test for window type
                wxTextCtrl* textCtrl = wxDynamicCast(pControl, wxTextCtrl);
                wxListCtrl* listCtrl = wxDynamicCast(pControl, wxListCtrl);
                if (textCtrl or listCtrl)
                    logCtrlMap[pControl] = logSlot.title;
            }
        }//endif
    }//endfor

    return logCtrlMap.size();
}
#include <cbcolourmanager.h>
//#include <editorcolours.h>
#include <manager.h>

#include <cbcolourmanager.h>
//-#include <cbeditorcolours.h>
#include <manager.h>
#include "personalitymanager.h"
// ----------------------------------------------------------------------------
wxColour CBBkgdColor::GetDefaultEditorBackground()
// ----------------------------------------------------------------------------
{
    ConfigManager* pCfg = Manager::Get()->GetConfigManager(_T("editor"));
    LogManager* pLogMgr = Manager::Get()->GetLogManager();

    wxColour bgColour = wxColour(0,0,0);
    //  Need the config personality
    wxString personality = Manager::Get()->GetPersonalityManager()->GetPersonality();

    // Guarantee .conf is closed on any return
    struct CloseConf
    {
        CloseConf(){}
        ~CloseConf(){if (confData.IsOpened()) confData.Close();}
    } closeConf;

    wxString confFile = pCfg->GetConfigFolder();
    wxString confFilePath = confFile + "\\" + personality + ".conf";
    if (platform::Linux) confFilePath.Replace("\\", "/");
    if (not confData.Open(confFilePath))
    {
        cbMessageBox("CBBkgdColor plugin could not open "+confFilePath);
        return bgColour;
    }

    wxString activeLang;
    wxString defaultColour;

    int ii = 0;
    if ( wxNOT_FOUND == (ii = FindConfigItem(ii, "<editor>")))
        return bgColour;
    wxString line = confData.GetLine(ii);

    if (wxNOT_FOUND == (ii = FindConfigItem(ii, "<ACTIVE_LANG>")))
        return bgColour;
    activeLang = confData.GetLine(ii+2);
    // <![CDATA[C/C++]]>
    activeLang = activeLang.AfterFirst('[').BeforeFirst(']');
    activeLang = activeLang.AfterFirst('[');
    activeLang = '<' + activeLang + ">";
    if (activeLang.Contains("C/C++"))
        activeLang.Replace("C/C++","cc");

    if (wxNOT_FOUND == (ii = FindConfigItem(ii, activeLang)))
        return bgColour;
    line = confData.GetLine(ii);

    if (wxNOT_FOUND == (ii = FindConfigItem(ii, "<BACK>")))
        return bgColour;
    if (wxNOT_FOUND == (ii = FindConfigItem(ii, "<colour r=")))
        return bgColour;
    // <colour r="254" g="233" b="201" />
    defaultColour = confData.GetLine(ii).Trim(false);
    defaultColour = defaultColour.Mid(10);
    // transform defaultColour to a wxColour
    //  "254" g="233" b="201" />
    defaultColour.Replace("g=", "");
    defaultColour.Replace("b=", "");
    defaultColour.Replace(" />", "");
    defaultColour.Replace("\"", "");
    // 254 233 201
    wxString red = defaultColour.BeforeFirst(' ').ToStdString();
    wxString green = defaultColour.Mid(red.Length()+1).BeforeFirst(' ');
    wxString blue = defaultColour.Mid(red.Length()+1+green.Length()+1);

    bgColour = wxColour(std::stoi(red.ToStdString()),
                        std::stoi(green.ToStdString()),
                        std::stoi(blue.ToStdString()) );

    wxString logit; logit << "CBBkgdColor wxColour: " << (int)bgColour.Red() << "," << (int)bgColour.Green() << "," << (int)bgColour.Blue();
    pLogMgr->DebugLog(logit); logit.Clear();

    // Return the colour
    return bgColour;
}
// ----------------------------------------------------------------------------
size_t CBBkgdColor::FindConfigItem(size_t linePosn, wxString key)
// ----------------------------------------------------------------------------
{
    // Find an entry in the .conf file starting at linePosn

    size_t ii = linePosn;

    for (; ii < confData.GetLineCount(); ++ii)
    {
        wxString line = confData.GetLine(ii);
        if (line.Trim(false).StartsWith(key))
            return ii;
        if (ii == confData.GetLineCount()-1)
            return wxNOT_FOUND;
    }

    return wxNOT_FOUND;
}

#include <manager.h>
#include <configmanager.h>
#include <wx/colour.h>
#include <wx/string.h>
#include <wx/arrstr.h>
// ----------------------------------------------------------------------------
wxColour CBBkgdColor::ReadColourSetsConfig()
// ----------------------------------------------------------------------------
{

    /// Deprecated. This works on Windows, but not on Linux.
    // 48 hours of trying cannot make tinyxml work on linux
    // See brutforce GetDefaultEditorBackground() method.

    // Get ConfigManager for <edit> namespace
    ConfigManager* cfg = Manager::Get()->GetConfigManager(_T("editor"));
    LogManager* pLogMgr = Manager::Get()->GetLogManager();
    wxString logit;

    // Set XML path to colour_sets
    cfg->SetPath(_T("/colour_sets"));
    wxString curPath = cfg->GetPath();
    logit << "CurPath:" << curPath;
    pLogMgr->DebugLog(logit); logit.Clear();

    // Example: read the value of ACTIVE_COLOUR_SET/str
    wxString activeSet = cfg->Read("/colour_sets/active_colour_set", "");
    logit << "Active colour set: " << activeSet.mb_str();
    pLogMgr->DebugLog(logit); logit.Clear();

    wxString activeLang = cfg->Read("/colour_sets/active_lang", "");
    logit << "Active Language: " << activeLang.mb_str();
    pLogMgr->DebugLog(logit); logit.Clear();

    //// You can enumerate keys inside colour_sets:
    //wxArrayString keys = cfg->EnumerateKeys("/colour_sets/"+activeSet);
    //for (size_t i = 0; i < keys.GetCount(); ++i)
    //{
    //    logit << "Found element: " << keys[i].mb_str()
    //              << " = " << cfg->Read(keys[i]).mb_str();
    //    pLogMgr->DebugLog(logit); logit.Clear();
    //
    //}

    if (activeLang == "C/C++") activeLang = "cc";
    //cfg->SetPath(_T("/colour_sets/default/cc/style0"));
    if (activeSet.Length() and activeLang.Length())
        cfg->SetPath(_T("/colour_sets/" + activeSet + "/" + activeLang + "/style0"));
    else return wxColour(0,0,0);
    wxColour col = cfg->ReadColour(_T("BACK"), *wxBLACK);
    logit << "BG Colour: " << (int)col.Red() << "," << (int)col.Green() << "," << (int)col.Blue();
    pLogMgr->DebugLog(logit); logit.Clear();

    return col;
}



// (ph 25/08/02)

// For ListCtrlLoggers to support SetBackgroundColour(), this patch
// must be made to ListCtrlLogger:
/*
Index: src/include/loggers.h
===================================================================
--- src/include/loggers.h	(revision 13680)
+++ src/include/loggers.h	(working copy)
@@ -126,13 +126,15 @@
     wxArrayString titles;
     wxArrayInt    widths;

-    struct ListStyles
-    {
-        wxFont font;
-        wxColour colour;
-    };
-    ListStyles style[num_levels];
+//    struct ListStyles
+//    {
+//        wxFont font;
+//        wxColour colour;
+//    };
+//    ListStyles style[num_levels];
+    wxTextAttr  style[num_levels];

+
     wxString GetItemAsText(long item) const;
 public:

Index: src/sdk/loggers.cpp
===================================================================
--- src/sdk/loggers.cpp	(revision 13680)
+++ src/sdk/loggers.cpp	(working copy)
@@ -329,25 +329,25 @@
     wxColour default_text_colour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
     for(unsigned int i = 0; i < num_levels; ++i)
     {
-        style[i].font = default_font;
-        style[i].colour = default_text_colour;
+        style[i].SetFont(default_font);
+        style[i].SetTextColour(default_text_colour);
     }

     ColourManager *colours = Manager::Get()->GetColourManager();

-    style[caption].font = bigger_font;
-    style[success].colour = colours->GetColour(wxT("logs_success_text"));
-    style[failure].colour = colours->GetColour(wxT("logs_failure_text"));
+    style[caption].SetFont(bigger_font);
+    style[success].SetTextColour(colours->GetColour(wxT("logs_success_text")));
+    style[failure].SetTextColour(colours->GetColour(wxT("logs_failure_text")));

-    style[warning].font = italic_font;
-    style[warning].colour = colours->GetColour(wxT("logs_warning_text"));
+    style[warning].SetFont(italic_font);
+    style[warning].SetTextColour(colours->GetColour(wxT("logs_warning_text")));

-    style[error].colour = colours->GetColour(wxT("logs_error_text"));
+    style[error].SetTextColour(colours->GetColour(wxT("logs_error_text")));

-    style[critical].font = bold_font;
-    style[critical].colour = colours->GetColour(wxT("logs_critical_text_listctrl"));
+    style[critical].SetFont(bold_font);
+    style[critical].SetTextColour(colours->GetColour(wxT("logs_critical_text_listctrl")));

-    style[spacer].font = small_font;
+    style[spacer].SetFont(small_font);
     style[pagetitle] = style[caption];

     // Tell control and items about the font change
@@ -374,8 +374,8 @@

     control->Freeze();
     control->InsertItem(idx, msg);
-    control->SetItemFont(idx, style[lv].font);
-    control->SetItemTextColour(idx, style[lv].colour);
+    control->SetItemFont(idx, style[lv].GetFont());
+    control->SetItemTextColour(idx, style[lv].GetTextColour());

     if (autoScroll)
         control->EnsureVisible(idx);

*/
