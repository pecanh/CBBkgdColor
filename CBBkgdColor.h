/***************************************************************
 * Name:      CBBkgdColor
 * Purpose:   Code::Blocks plugin
 * Author:    Pecan ()
 * Created:   2022-12-27
 * Copyright: Pecan
 * License:   GPL
 **************************************************************/

#ifndef CBBkgdColor_H_INCLUDED
#define CBBkgdColor_H_INCLUDED

// For compilers that support precompilation, includes <wx/wx.h>
#include <wx/wxprec.h>
//#include <wx/aui/aui.h> // wxAuiManager

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include "wx/textfile.h"

#include <cbplugin.h> // for "class cbPlugin"
#include <logmanager.h>

class CBBkgdColor : public cbPlugin
{
    public:
        /** Constructor. */
        CBBkgdColor();
        /** Destructor. */
        virtual ~CBBkgdColor();

        /** Return the plugin's configuration priority.
          * This is a number (default is 50) that is used to sort plugins
          * in configuration dialogs. Lower numbers mean the plugin's
          * configuration is put higher in the list.
          */
        virtual int GetConfigurationPriority() const { return 50; }

        /** Return the configuration group for this plugin. Default is cgUnknown.
          * Notice that you can logically OR more than one configuration groups,
          * so you could set it, for example, as "cgCompiler | cgContribPlugin".
          */
        virtual int GetConfigurationGroup() const { return cgUnknown; }

        /** Return plugin's configuration panel.
          * @param parent The parent window.
          * @return A pointer to the plugin's cbConfigurationPanel. It is deleted by the caller.
          */
        virtual cbConfigurationPanel* GetConfigurationPanel(wxWindow* parent){ return 0; }

        /** Return plugin's configuration panel for projects.
          * The panel returned from this function will be added in the project's
          * configuration dialog.
          * @param parent The parent window.
          * @param project The project that is being edited.
          * @return A pointer to the plugin's cbConfigurationPanel. It is deleted by the caller.
          */
        virtual cbConfigurationPanel* GetProjectConfigurationPanel(wxWindow* parent, cbProject* project){ return 0; }

        /** This method is called by Code::Blocks and is used by the plugin
          * to add any menu items it needs on Code::Blocks's menu bar.\n
          * It is a pure virtual method that needs to be implemented by all
          * plugins. If the plugin does not need to add items on the menu,
          * just do nothing ;)
          * @param menuBar the wxMenuBar to create items in
          */
        virtual void BuildMenu(wxMenuBar* menuBar);

        /** This method is called by Code::Blocks core modules (EditorManager,
          * ProjectManager etc) and is used by the plugin to add any menu
          * items it needs in the module's popup menu. For example, when
          * the user right-clicks on a project file in the project tree,
          * ProjectManager prepares a popup menu to display with context
          * sensitive options for that file. Before it displays this popup
          * menu, it asks all attached plugins (by asking PluginManager to call
          * this method), if they need to add any entries
          * in that menu. This method is called.\n
          * If the plugin does not need to add items in the menu,
          * just do nothing ;)
          * @param type the module that's preparing a popup menu
          * @param menu pointer to the popup menu
          * @param data pointer to FileTreeData object (to access/modify the file tree)
          */
        virtual void BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data = 0);

        /** This method is called by Code::Blocks and is used by the plugin
          * to add any toolbar items it needs on Code::Blocks's toolbar.\n
          * It is a pure virtual method that needs to be implemented by all
          * plugins. If the plugin does not need to add items on the toolbar,
          * just do nothing ;)
          * @param toolBar the wxToolBar to create items on
          * @return The plugin should return true if it needed the toolbar, false if not
          */
        virtual bool BuildToolBar(wxToolBar* toolBar){ return false; }
    protected:
        /** Any descendent plugin should override this virtual method and
          * perform any necessary initialization. This method is called by
          * Code::Blocks (PluginManager actually) when the plugin has been
          * loaded and should attach in Code::Blocks. When Code::Blocks
          * starts up, it finds and <em>loads</em> all plugins but <em>does
          * not</em> activate (attaches) them. It then activates all plugins
          * that the user has selected to be activated on start-up.\n
          * This means that a plugin might be loaded but <b>not</b> activated...\n
          * Think of this method as the actual constructor...
          */
        virtual void OnAttach();

        /** Any descendent plugin should override this virtual method and
          * perform any necessary de-initialization. This method is called by
          * Code::Blocks (PluginManager actually) when the plugin has been
          * loaded, attached and should de-attach from Code::Blocks.\n
          * Think of this method as the actual destructor...
          * @param appShutDown If true, the application is shutting down. In this
          *         case *don't* use Manager::Get()->Get...() functions or the
          *         behaviour is undefined...
          */
        virtual void OnRelease(bool appShutDown);

    private:
        DECLARE_EVENT_TABLE();

        void OnAppStartupDone(CodeBlocksEvent& event);
        void OnSettingsChanged(CodeBlocksEvent& event);
        void OnSettingsChangedPostProcessing();

        void SetAllWindowsBackgroundColor(const wxColour& color);
        void SetBackgroundColorRecursively(wxWindow* window, const wxColour& color);
        void OnIdle(wxIdleEvent& event);
        int  GetLogIndex(const wxString& logRequest);
        bool SetTextLogStyleBackground(wxString logTitle, wxColour color);
        bool SetListLogStyleBackground(wxString logTitle, wxColour color);
        void OnSetFocus(wxFocusEvent& event);
        int  MakeLogCtrlMap();
        void OnEditorOpened(CodeBlocksEvent& event);    // (ph 25/08/12)
        void OnEditorClosed(CodeBlocksEvent& event);
        wxColour GetDefaultEditorBackground();
        wxColour ReadColourSetsConfig();
        size_t FindConfigItem(size_t linePosn, wxString key);

        #define MAX_TRIES 50;
        int             m_SearchResultLogIndex = 0;
        const int       m_Max_Tries = MAX_TRIES;

        std::map<wxWindow*,std::string> logCtrlMap;
        wxColour m_UsrBkgdColor = wxColour(); //color is reset in the code

        int m_startupDone = 0;

};

#endif // CBBkgdColor_H_INCLUDED
//----------------------------------------
#define VERSION "1.0.8 25/08/15"
//----------------------------------------
//versions
// ----------------------------------------------------------------------------
//  Modification History
// ----------------------------------------------------------------------------
// 1.0.8    2025/08/15  Abandon ConfigManager and use brute search of .conf file
// 1.0.7    2025/08/14  Treat Settings/Editor changes as a restart mechanism because
//                          the background color may have changed.
// 1.0.6    2025/08/13  Use the users Highlighting editor background color instead of the StartHere color
// 1.0.5    2025/08/02  Add support for wxTextAttr for future ListCtrlLogger patch
// 1.0.4    2025/07/30  Change MAX_TRIES for OnIdle to 50
// 1.0.3    2025/07/29  Initial version
