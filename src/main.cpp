/////////////////////////////////////////////////////////////////////////////
// Name:        main.cpp
// Purpose:     Romper WxWidgets App
// Author:      Rob Shelby https://github.com/undeadindustries
// Created:     2023-02-10
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////
// In linux:
// install libcurl4-openssl-dev
// ../configure --enable-debug --with-opengl --with-gtk=3 --disable-shared --enable-webrequest

#ifdef _WIN32
#include <Windows.h>
#endif
#include <iostream>
#include <sys/stat.h>
#include <string>
//#include <regex>
#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <map>
#include <vector>
#include <filesystem>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/filefn.h>
#include <wx/textfile.h>
#include <wx/simplebook.h>
#include <wx/grid.h>
#include <wx/aui/aui.h>
#include <wx/event.h>
#include <wx/progdlg.h>
#include <wx/webrequest.h>
#include <wx/wfstream.h>

#include <SQLiteCpp/SQLiteCpp.h>

//Set version
const std::string VERSION = "2023-2-23";

//Set default new line char for each OS.
#ifdef _WIN32
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif
namespace fs = std::filesystem;

#ifdef SQLITECPP_ENABLE_ASSERT_HANDLER // Do we need all this? Just copied from SQLiteCPP example...
namespace SQLite
{
    /// definition of the assertion handler enabled when SQLITECPP_ENABLE_ASSERT_HANDLER is defined in the project (CMakeList.txt)
    void assertion_failed(const char *apFile, const long apLine, const char *apFunc, const char *apExpr, const char *apMsg)
    {
        // Print a message to the standard error output stream, and abort the program.
        std::cerr << apFile << ":" << apLine << ":"
                  << " error: assertion failed (" << apExpr << ") in " << apFunc << "() with message \"" << apMsg << "\"\n";
        std::abort();
    }
}
#endif

//Define book pages
#define romperBlankPage 0
#define romperGameGrid 1
#define romperEditProfile 2
#define romperNewProfile 3
#define romperSearchOptions 4

//Required for trim
const std::string WHITESPACE = " \n\r\t\f\v";
std::string rtrim(const std::string &s);
std::string ltrim(const std::string &s);
std::string trim(const std::string &s);
std::string GetExeDirectory();

/*Check if a string is in an vector*/
bool in_array(const std::string &needle, const std::vector<std::string> &haystack);

/*Check if a folder exists*/
bool dir_exists(std::string dir);

class MyApp : public wxApp
{
public:
    virtual bool OnInit(); //replaces main()
};

class MyFrame : public wxFrame
{
public:
    /*Construct the wx frame window*/
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size, std::string profileDBFile, std::string gameDBFile);
    
    /*
    *Populate the grid with games.
    *This also orders, and filters the grid.
    */
    void BuildGrid(const std::string &orderDirection, const std::string &orderBy, const std::string &searchField, const std::string &searchValue, int page, const std::string limit);
    
    /*A simple way to display string messages.*/
    void DisplayMessage(std::string Message);
    
    /*Change the book to a different page. Use the constants define above.*/
    void ChangeMainBookPage(int page);

private:
    struct profile //The profile data for each profile. profile_map is a map of these.
    {
        std::string name; //Name of the profile
        int online; //If 1, then download roms. Else, use local files.
        std::string romSource; //If local files, this is the folder with all the .zip files
        std::string chdSource; //If local files, this is the folder with all the chd folders.
        std::string romTarget; //Where the rom zips are copied/downloaded.
        std::string chdTarget; //Where the CHD folders are copied/downloaded.
    };

    std::map<std::string, profile> profile_map; //Populated everytime profiles are loaded. Updated when profiles are changed.
    
    struct romperBook //Sometimes we need to know the previous book page. So we use this struct for the simplebook.
    {
        wxSimplebook *book;
        int previous;
    };

    struct controlChoice //Sometimes we need to know the previous selected choice. So we use this struct for the profile choice select.
    {
        wxChoice *choice;
        int prevChoice;
    };

    struct rGrid //The struct for the grid of games.
    {
        int curPage; //The current page number set from SQL
        int lastPage; //The last page number set from SQL
        std::string prevChangeAll; //When clicking the X of the grid header to select all, if 1, then select all, else, de-select all.
        std::string orderBy; //SQL order by. name, description, etc.
        std::string orderDirection; //SQL asc or desc.
        wxGrid *Grid; //the actual grid
    };

    struct gameMap
    {
        std::string name; //name of the game
        std::string disk; //name of the game's disk. If blank, no disk
    };

    enum //required for downloading roms.
    {
        DOWNLOAD_STATE_IDLE = 0,
        DOWNLOAD_STATE_BUSY,
        DOWNLOAD_STATE_DONE_OK,
        DOWNLOAD_STATE_DONE_ERROR,
    };

    //std::string exePath; //DELETE!
    wxPanel *panel;     //The main panel/window
    wxPanel *gamePanel; //The Game Grid panel on the grid book page.
    wxPanel *editProfilePanel; //Edit profile panel on the edit page.
    wxPanel *newProfilePanel; //New profile panel on the new profile page. Can probably combine edit and new.
    wxPanel *blankPanel; //Just a blank panel. Used at load and when loading data to make loading look smoother.
    romperBook *mainBook; //The book for all the above panels.
    rGrid *gameGrid; //The wxGrid with a few other parameters. Rows are added and created at every search.
    controlChoice *profileChoice; //The choice box to select which profile. On change, the profile loads and populates the grid.
    wxChoice *searchBy; //The choice box to select which field to search by. Name, Description, Date, etc.
    wxChoice *perPage; //How many results per page? 50, 100(default), 500, 1000(not recommended)
    wxBoxSizer *vSizerEditProfile; //Edit Profile panel's sizer.
    wxBoxSizer *vSizerNewProfile; //New Profile panel's sizer.
    wxBoxSizer *hSizerRunButtons; //Sizer to hold the run button. There used to be more buttons there. Is hidden when run shouldn't be available.
    wxBoxSizer *mainSizer; //Panel's main sizer. 
    wxBoxSizer *vSizer; //The main vertical sizer.
    wxBoxSizer *hSizerLoad; //Sizer for profile choice and its buttons
    wxBoxSizer *vSizerMainArea; //Sizer holding the book? I designed all this months ago.
    wxBoxSizer *vSizerGameGrid; //Veritcal Sizer for the Grid and the search button
    wxBoxSizer *hSizerGameGrid; //In vSizerGameGrid
    wxBoxSizer *hSizerSearch;  //In vSizerGameGrid
    wxFlexGridSizer *gridSizerEditProfile; //Sizer for edit profile labels, text input, buttons
    wxMenuItem *menuScreenless; //Menu checkbox for deselect screenless games. Search must be clicked after it changes for the grid to update.
    wxStaticText *totalGamesLabel; //How many games were found in the search. updated each search
    wxButton *nextResults; //Click to go to update the grid with the next page results
    wxButton *prevResults; //Click to go to update the grid with the prev page results
    wxButton *saveProfileGameChanges; //Save button. I think this is not used anymore and can be deleted.
    wxButton *profileEditButton; //Click to change to the edit profile page.
    wxButton *newProfileButton; //Click to change to the new profile page.
    wxButton *searchButton; //Click to search and populate the grid.
    wxButton *resetSearch; //Reset the search to its default params.
    wxStatusBar *statusBar; //Status bar. Used to show how long each search took. I should add that back.
    //wxToolBar *toolbar; //Can be deleted?
    wxMenuBar *menubar; //Menu bar for file, select, about etc.
    wxMenu *menuFile; //File menu drop down
    wxMenu *menuSelect; //Select menu drop down
    wxTextCtrl *searchInput; //Text box for search.
    wxCheckBox *newProfileOnline; //Create new profile: Download Roms checkbox. If checked, download roms. If not, local files.
    wxStaticText *newProfileROMSourceFolder;  //Create new profile: Folders for local .zips
    wxStaticText *newProfileCHDSourceFolder; //Create new profile: Folders for CHD folders
    wxStaticText *newProfileROMTargetFolder; //Create new profile:  Where to download/copy .zips
    wxStaticText *newProfileCHDTargetFolder; //Create new profile: Where to download/copy CHD folders
    wxButton *newProfileROMSourceFolderButton; //Create new profile: Select the local zips folder 
    wxButton *newProfileCHDSourceFolderButton; //Create new profile:  Select the local CHD folder
    wxTextCtrl *newProfileName; //Create new profile: name of the new profile
    wxCheckBox *editProfileOnline; //Edit profile: download or local files?
    wxStaticText *editProfileROMSourceFolder; //Edit profile: Folders for local .zips
    wxStaticText *editProfileCHDSourceFolder; //Edit profile: Folder for local CHD folders
    wxStaticText *editProfileROMTargetFolder; //Edit profile: Where to download/copy zips
    wxStaticText *editProfileCHDTargetFolder; //Edit profile: Where to download/copy CHD folders
    wxButton *editProfileROMSourceFolderButton; //Edit profile: select the local zip folder
    wxButton *editProfileCHDSourceFolderButton; //Edit profile: select the CHD folder
    wxTextCtrl *editProfileName; //Edit profile: Change the profile's name
    wxAuiManager *auiManager; //We need this for Wx stuff.
    wxMenu *menuRank; //Menubar drop down to select to only search for certain ranks.
    wxMenuItemList *ranks; //Menubar list of ranks. Used to loop through.
    wxMenu *menuGenre; //Menubar drop down to select certain genre. 
    wxMenuItemList *genres; //Menubar list of genres. Used to loop through.

    void OnProfileChange(wxCommandEvent &event);
    void OnSearch(wxCommandEvent &event);
    void AfterSearch();
    void OnPrev(wxCommandEvent &event);
    void OnNext(wxCommandEvent &event);
    void OnReset(wxCommandEvent &event);
    void OnPerPage(wxCommandEvent &event);
    void OnNewProfile(wxCommandEvent &event);
    void OnEditProfile(wxCommandEvent &event);
    void OnExit(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnGridClick(wxGridEvent &event);
    void OnGridLabelClick(wxGridEvent &event);
    void OnNewProfileROMSourceFolderButton(wxCommandEvent &event);
    void OnEditProfileROMSourceFolderButton(wxCommandEvent &event);
    void OnNewProfileROMTargetFolderButton(wxCommandEvent &event);
    void OnEditProfileROMTargetFolderButton(wxCommandEvent &event);
    void OnNewProfileCHDSourceFolderButton(wxCommandEvent &event);
    void OnEditProfileCHDSourceFolderButton(wxCommandEvent &event);
    void OnNewProfileCHDTargetFolderButton(wxCommandEvent &event);
    void OnEditProfileCHDTargetFolderButton(wxCommandEvent &event);
    void OnNewProfileCancelButton(wxCommandEvent &event);
    void OnNewProfileSaveButton(wxCommandEvent &event);
    void OnNewProfileOnline(wxCommandEvent &event);
    void OnEditProfileOnline(wxCommandEvent &event);
    void OnEditProfileCancelButton(wxCommandEvent &event);
    void OnEditProfileDeleteButton(wxCommandEvent &event);
    void OnEditProfileSaveButton(wxCommandEvent &event);
    void PopulateProfileChoice(int selection = 0);
    void OnRunButton(wxCommandEvent &event);
    bool DownloadGame(gameMap, const char *type, const std::string url, wxString &runErrors, wxProgressDialog &progress);

    wxDECLARE_EVENT_TABLE();
    SQLite::Database gameDB; //The SQLite DB of games. Not written to by this app.
    SQLite::Database profileDB; //Where profile data is saved. Written to by this app. Should probably be written by this app.
};

//The event table. There are also events created within the frame construct.
wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT, MyFrame::OnExit)
        EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
            wxEND_EVENT_TABLE()
                wxIMPLEMENT_APP(MyApp);

//This acts at main(). Calls the class to create the Window
bool MyApp::OnInit()
{
    try
    {
        auto romperFolder = GetExeDirectory();
        std::string profileDBFile = romperFolder + "/data/profiles.romper";
        std::string gameDBFile = romperFolder + "/data/romper.romper";
        MyFrame *frame = new MyFrame("Romper", wxPoint(50, 50), wxSize(800, 600), profileDBFile, gameDBFile);
        frame->Refresh();
        frame->Show(true);
    }
    catch (std::exception &e)
    {
        std::string m("frame error: ");
        m.append(e.what());
        std::cout << m << std::endl;
        return false;
    }

    return true;
}

void MyFrame::BuildGrid(const std::string &orderDirection, const std::string &orderBy, const std::string &searchField, const std::string &searchValue, int page = 1, const std::string limit = "100")
{ // reset the grid
    SetStatusText("Searching");
    ChangeMainBookPage(romperBlankPage);
    gameGrid->Grid->BeginBatch();
    if (gameGrid->Grid->GetNumberRows() > 0)
    {
        gameGrid->Grid->DeleteRows(0, gameGrid->Grid->GetNumberRows());
    }
    if (gameGrid->Grid->GetNumberCols() > 0)
    {
        gameGrid->Grid->DeleteCols(0, gameGrid->Grid->GetNumberCols());
    }

    std::vector<std::string> checkedGames{};
    try
    {
        SQLite::Statement pq(profileDB, "SELECT game FROM games WHERE profile = ?;");
        pq.bind(1, profileChoice->choice->GetStringSelection().ToStdString());
        while (pq.executeStep())
        {
            checkedGames.push_back(pq.getColumn(0));
        }
    }
    catch (std::exception &e)
    {
        gameGrid->Grid->EndBatch();
        SetStatusText("Error while searching");
        std::string m("Build Grid Profile SQLite exception: ");
        m.append(e.what());
        DisplayMessage(m);
        return; // unexpected error : exit the example program
    }

    try
    {
        std::string querystr;
        // build the query

        bool needBind = false;
        std::string where = "";
        bool whereBool = false; // determine if the WHERE needs to be WHERE or AND. If TRUE, then AND. There's got to be a better way for this.
        // If the search value is not empty, set the WHERE var and we'll need to bind it.
        if (trim(searchValue) != "")
        {
            needBind = true; // We only bind on the searchValue because it's the only WHERE that's not programically assigned.
            whereBool = true;
            where = " WHERE games." + searchField + " LIKE ? ";
        }

        // Screenless: if NOT screenless, then don't show screenless. Else, show all.
        // Who cares about only screenless games?
        if (!menuScreenless->IsChecked())
        {
            if (whereBool)
            {
                where += " AND games.Screenless=0 ";
            }
            else
            {
                whereBool = true;
                where = " WHERE games.Screenless=0 ";
            }
        }

        // RANK
        bool notInRank = false;
        std::string rankTempSQL = "";
        if (!whereBool)
        {
            whereBool = true;
            rankTempSQL = " WHERE games.rank NOT IN (";
        }
        else
        {
            rankTempSQL = " AND games.rank NOT IN (";
        }
        wxMenuItemList mr = menuRank->GetMenuItems();
        for (wxMenuItemList::iterator i = mr.begin(); i != mr.end(); ++i)
        {
            if (!i.m_node->GetData()->IsChecked())
            {
                notInRank = true;
                if (i.m_node->GetData()->GetItemLabelText().ToStdString() == "Blank")
                {
                    rankTempSQL.append("\"\",");
                }
                else
                {
                    rankTempSQL.append("\"" + i.m_node->GetData()->GetItemLabelText().ToStdString() + "\",");
                }
            }
        }
        if (notInRank)
        {
            rankTempSQL.pop_back(); // remove last comma.
            where.append(rankTempSQL.append(") "));
        }
        // Genre
        bool notInGenre = false;
        std::string genreTempSQL = "";
        if (!whereBool)
        {
            whereBool = true;
            genreTempSQL = " WHERE games.genre NOT IN (";
        }
        else
        {
            genreTempSQL = " AND games.genre NOT IN (";
        }
        wxMenuItemList mg = menuGenre->GetMenuItems();
        for (wxMenuItemList::iterator i = mg.begin(); i != mg.end(); ++i)
        {
            if (!i.m_node->GetData()->IsChecked())
            {
                notInGenre = true;
                if (i.m_node->GetData()->GetItemLabelText().ToStdString() == "Blank")
                {
                    genreTempSQL.append("\"\",");
                }
                else
                {
                    genreTempSQL.append("\"" + i.m_node->GetData()->GetItemLabelText().ToStdString() + "\",");
                }
            }
        }
        if (notInGenre)
        {
            genreTempSQL.pop_back(); // remove last comma.
            where.append(genreTempSQL.append(") "));
        }

        //std::string queryStr = "WITH All_Games AS (SELECT * FROM games {WHERE}), Count_Games AS (SELECT COUNT(*) AS TotalGames FROM All_Games) SELECT c.TotalGames, g.Name, g.Genre, g.Cat, g.Developer, g.Publisher, g.Year, g.Series, g.Description, g.ROMof, g.Disk, g.Rank, g.Screenless FROM All_Games g CROSS JOIN Count_Games c ";
        std::string queryStr = "WITH All_Games AS (SELECT * FROM games "+where+"), Count_Games AS (SELECT COUNT(*) AS TotalGames FROM All_Games) SELECT c.TotalGames, g.Name, g.Genre, g.Cat, g.Developer, g.Publisher, g.Year, g.Series, g.Description, g.ROMof, g.Disk, g.Rank, g.Screenless FROM All_Games g CROSS JOIN Count_Games c ";
        //queryStr = std::regex_replace(queryStr, std::regex("\\{WHERE\\}"), where);

        std::string sqlOrderBy;
        if (orderBy == "")
        {
            sqlOrderBy = "g.Description";
        }
        else
        {
            sqlOrderBy = "g." + orderBy;
        }

        std::string sqlOrderDirection;
        if (orderDirection != "ASC" && orderDirection != "DESC")
        {
            sqlOrderDirection = "ASC";
        }
        else
        {
            sqlOrderDirection = orderDirection;
        }
        int limitInt = stoi(limit);
        int start = (limitInt * page) - limitInt;
        std::string temp = queryStr + " ORDER BY " + sqlOrderBy + " " + sqlOrderDirection + " LIMIT " + std::to_string(limitInt) + " OFFSET " + std::to_string(start) + ";";

        SQLite::Statement query(gameDB, temp);
        if (needBind)
        {
            query.bind(1, trim(searchValue + "%"));
        }
        // Get the total games;
        int totalGames;
        while (query.executeStep())
        {
            totalGames = query.getColumn(0);
            if (totalGames < 1)
            {
                DisplayMessage("No games in the result");
                SetStatusText("No games in search results");
                return;
            }

            break;
        }
        gameGrid->Grid->AppendRows(limitInt, false); // we'll fix this later if it's too big.
        int totalPages;
        if (totalGames >= limitInt)
        {
            totalPages = totalGames / limitInt;
        }
        else
        {
            totalPages = 1;
        }
        gameGrid->lastPage = totalPages;
        gameGrid->curPage = page;
        std::string tgl = std::to_string(page).append(" of ").append(std::to_string(totalPages));
        totalGamesLabel->SetLabelText(tgl);
        hSizerSearch->Layout();
        const char *headers[8] = {"X", "Name", "Description", "Developer", "Series", "Cat", "Genre", "Rank"};
        gameGrid->Grid->AppendCols(8);
        for (int i = 0; i < 8; i++)
        {
            gameGrid->Grid->SetColLabelValue(i, headers[i]);
        }

        query.reset();
        int row = 0;
        // wxGridCellBoolEditor *editor = new wxGridCellBoolEditor();
        // wxGridCellBoolRenderer *renderer = new wxGridCellBoolRenderer();
        while (query.executeStep())
        {
            int col = 0;
            // Bool col
            gameGrid->Grid->SetReadOnly(row, col);
            // editor->IncRef();
            gameGrid->Grid->SetCellEditor(row, col, new wxGridCellBoolEditor());
            // renderer->IncRef();
            gameGrid->Grid->SetCellRenderer(row, col, new wxGridCellBoolRenderer());
            // Name
            const char *gName = query.getColumn(1);
            std::string gNameString = query.getColumn(1).getString();
            if (in_array(gNameString, checkedGames))
            {
                gameGrid->Grid->SetCellValue(row, col, "1");
            }
            else
            {
                gameGrid->Grid->SetCellValue(row, col, "");
            }
            col++;

            gameGrid->Grid->SetReadOnly(row, col);
            gameGrid->Grid->SetCellValue(row, col, gName);

            // Description
            col++;

            const char *gDescription = query.getColumn(8);
            gameGrid->Grid->SetReadOnly(row, col);
            gameGrid->Grid->SetCellValue(row, col, gDescription);

            // Developer
            col++;
            const char *gDev = query.getColumn(4);
            gameGrid->Grid->SetReadOnly(row, col);
            gameGrid->Grid->SetCellValue(row, col, gDev);

            // Series
            col++;
            const char *gSeries = query.getColumn(7);
            gameGrid->Grid->SetReadOnly(row, col);
            gameGrid->Grid->SetCellValue(row, col, gSeries);

            // Cat
            col++;
            const char *gCat = query.getColumn(3);
            gameGrid->Grid->SetReadOnly(row, col);
            gameGrid->Grid->SetCellValue(row, col, gCat);

            // Genre
            col++;
            const char *gGenre = query.getColumn(2);
            gameGrid->Grid->SetReadOnly(row, col);
            gameGrid->Grid->SetCellValue(row, col, gGenre);

            // const int gYear = query.getColumn(5);

            // const char *gDescription = query.getColumn(7);
            // const char *gRomOf = query.getColumn(8);
            // const char *gDisk = query.getColumn(9);

            // Rank
            col++;
            const char *gRank = query.getColumn(11);
            gameGrid->Grid->SetReadOnly(row, col);
            gameGrid->Grid->SetCellValue(row, col, gRank);
            // const int gScreenless = query.getColumn(11);
            row++;
        }
        query.clearBindings();
        query.reset();
        // editor.Destroy();

        if (row < limitInt)
        {
            gameGrid->Grid->DeleteRows(row, limitInt - row);
        }
        gameGrid->Grid->AutoSizeColumns(false);
        gameGrid->prevChangeAll = "";
        vSizer->Show(hSizerRunButtons);
        hSizerRunButtons->Hide(saveProfileGameChanges);
        vSizer->Layout();
        ChangeMainBookPage(romperGameGrid);
        gameGrid->Grid->SetScrollPos(wxHORIZONTAL, 0, true);
        gameGrid->Grid->SetScrollPos(wxVERTICAL, 0, true);
        gameGrid->Grid->EndBatch();
        profileEditButton->Show();
        hSizerLoad->Layout();
        vSizerGameGrid->Layout();
        Refresh();
        SetStatusText("Total Games Found: " + std::to_string(totalGames));
    }
    catch (std::exception &e)
    {
        gameGrid->Grid->EndBatch();
        SetStatusText("Error while searching");
        std::string m("SQLite exception: ");
        m.append(e.what());
        DisplayMessage(m);
        return; // unexpected error : exit the example program
    }
}

MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size, std::string profileDBFile, std::string gameDBFile)
    : wxFrame(NULL, wxID_ANY, title, pos, size),
      gameDB(gameDBFile), profileDB(profileDBFile, SQLite::OPEN_READWRITE)
{

    menuFile = new wxMenu;
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    menuSelect = new wxMenu;
    menuScreenless = new wxMenuItem(menuSelect, wxID_ANY, "Screenless", "Select to include Screenless", wxITEM_CHECK);
    menuSelect->Append(menuScreenless);
    menuScreenless->Check(false);
    menuRank = new wxMenu;
    menuSelect->AppendSubMenu(menuRank, "Select Rank", "Choose which ranks are included.");
    try
    {
        SQLite::Statement query(gameDB, "SELECT rank FROM games GROUP BY rank ORDER BY rank;");
        while (query.executeStep())
        {
            std::string s = query.getColumn(0).getString();
            if (s == "")
            {
                s = "Blank";
            }
            menuRank->AppendCheckItem(wxID_ANY, s, s);
        }
        wxMenuItemList mr = menuRank->GetMenuItems();
        for (wxMenuItemList::iterator i = mr.begin(); i != mr.end(); ++i)
        {
            i.m_node->GetData()->Check(true);
        }
    }
    catch (std::exception &e)
    {
        std::string m("rank populate error: ");
        m.append(e.what());
        DisplayMessage(m);
        return;
    }

    // GENRE
    menuGenre = new wxMenu;
    menuSelect->AppendSubMenu(menuGenre, "Select Genre", "Select genre to include in search.");
    try
    {
        SQLite::Statement query(gameDB, "SELECT genre FROM games GROUP BY genre ORDER BY genre;");
        while (query.executeStep())
        {
            std::string s = query.getColumn(0).getString();
            if (s == "")
            {
                s = "Blank";
            }
            menuGenre->AppendCheckItem(wxID_ANY, s, s);
        }
        wxMenuItemList mg = menuGenre->GetMenuItems();
        for (wxMenuItemList::iterator i = mg.begin(); i != mg.end(); ++i)
        {
            i.m_node->GetData()->Check(true);
        }
    }
    catch (std::exception &e)
    {
        std::string m("Genre populate error: ");
        m.append(e.what());
        DisplayMessage(m);
        return;
    }

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    menubar = new wxMenuBar;
    menubar->Append(menuFile, "&File");
    menubar->Append(menuSelect, "&Select");
    menubar->Append(menuHelp, "&Help");
    SetMenuBar(menubar);
    CreateStatusBar();
    SetStatusText("Welcome to Romper");
    /*Panel and Sizer setup.*/
    panel = new wxPanel(this, wxID_ANY);
    mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(panel, 1, wxALL | wxEXPAND, 5);
    SetSizer(this->mainSizer);
    vSizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(this->vSizer);
    hSizerLoad = new wxBoxSizer(wxHORIZONTAL);
    profileChoice = new controlChoice;
    profileChoice->choice = new wxChoice(this->panel, wxID_ANY);
    hSizerLoad->Add(profileChoice->choice, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    profileChoice->choice->Bind(wxEVT_CHOICE, &MyFrame::OnProfileChange, this);
    profileChoice->choice->SetAutoLayout(true);
    newProfileButton = new wxButton(panel, wxID_ANY, "New Profile");
    newProfileButton->Bind(wxEVT_BUTTON, &MyFrame::OnNewProfile, this);
    hSizerLoad->Add(newProfileButton, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    profileEditButton = new wxButton(panel, wxID_ANY, "Edit Profile");
    profileEditButton->Bind(wxEVT_BUTTON, &MyFrame::OnEditProfile, this);
    hSizerLoad->Add(profileEditButton, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    vSizer->Add(hSizerLoad, 0, wxEXPAND | wxALL, 5);
    vSizerMainArea = new wxBoxSizer(wxHORIZONTAL);
    vSizer->Add(vSizerMainArea, 3, wxEXPAND | wxALL, 5);
    hSizerRunButtons = new wxBoxSizer(wxHORIZONTAL);
    wxButton *runButton = new wxButton(panel, wxID_ANY, "Run");
    runButton->Bind(wxEVT_BUTTON, &MyFrame::OnRunButton, this);
    saveProfileGameChanges = new wxButton(panel, wxID_ANY, "Save Changes");
    hSizerRunButtons->Add(saveProfileGameChanges, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    hSizerRunButtons->Add(runButton, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    vSizer->Hide(hSizerRunButtons, true);
    vSizer->Add(hSizerRunButtons, 0, wxEXPAND | wxALL, 5);

    profileEditButton->Hide();      // hidden until profile is selected
    vSizer->Hide(hSizerRunButtons); // hidden until profile is selected
    /*Simple Book Setup*/
    mainBook = new romperBook;
    mainBook->book = new wxSimplebook(panel, wxID_ANY);
    blankPanel = new wxPanel(mainBook->book, wxID_ANY);
    gamePanel = new wxPanel(mainBook->book, wxID_ANY);
    editProfilePanel = new wxPanel(mainBook->book, wxID_ANY);
    newProfilePanel = new wxPanel(mainBook->book, wxID_ANY);
    mainBook->book->AddPage(blankPanel, "Blank Panel", true);
    mainBook->book->AddPage(gamePanel, "Game Panel Text", false);
    mainBook->book->AddPage(editProfilePanel, "Edit Profile Panel Text", false);
    mainBook->book->AddPage(newProfilePanel, "New Profile Panel Text", false);
    vSizerMainArea->Add(mainBook->book, 1, wxEXPAND | wxALL, 5);

    mainBook->book->SetSelection(romperBlankPage);

    hSizerSearch = new wxBoxSizer(wxHORIZONTAL);
    perPage = new wxChoice(gamePanel, wxID_ANY);
    perPage->Append("50");
    perPage->Append("100");
    perPage->Append("500");
    perPage->Append("1000");
    perPage->SetSelection(1);
    perPage->Bind(wxEVT_CHOICE, &MyFrame::OnPerPage, this);
    totalGamesLabel = new wxStaticText(gamePanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    totalGamesLabel->SetAutoLayout(true);
    searchInput = new wxTextCtrl(gamePanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    searchInput->SetValue("");
    searchBy = new wxChoice(gamePanel, wxID_ANY);
    searchBy->Append("Name");
    searchBy->Append("Description");
    searchBy->Append("Developer");
    searchBy->Append("Series");
    searchBy->SetSelection(1);
    searchButton = new wxButton(gamePanel, wxID_ANY, "Search");
    searchButton->Bind(wxEVT_BUTTON, &MyFrame::OnSearch, this);
    searchInput->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent &)
                      {
        wxCommandEvent evt(wxEVT_BUTTON, searchButton->GetId());
        evt.SetEventObject(this);
        wxPostEvent(searchButton, evt);
        CallAfter(&MyFrame::AfterSearch); });
    resetSearch = new wxButton(gamePanel, wxID_ANY, "Reset Search");
    prevResults = new wxButton(gamePanel, wxID_ANY, "Previous");
    prevResults->Bind(wxEVT_BUTTON, &MyFrame::OnPrev, this);
    nextResults = new wxButton(gamePanel, wxID_ANY, "Next");
    nextResults->Bind(wxEVT_BUTTON, &MyFrame::OnNext, this);
    hSizerSearch->Add(searchInput, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    hSizerSearch->Add(searchBy, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    hSizerSearch->Add(searchButton, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    hSizerSearch->Add(resetSearch, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    hSizerSearch->Add(perPage, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    hSizerSearch->Add(prevResults, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    hSizerSearch->Add(totalGamesLabel, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    hSizerSearch->Add(nextResults, 0, wxALIGN_BOTTOM | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    hSizerSearch->Layout();
    vSizerGameGrid = new wxBoxSizer(wxVERTICAL);
    hSizerGameGrid = new wxBoxSizer(wxHORIZONTAL);
    gameGrid = new rGrid;
    gameGrid->Grid = new wxGrid(gamePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxEXPAND | wxALL);
    gameGrid->curPage = 0;
    gameGrid->lastPage = 0;
    gameGrid->orderBy = "Description";
    gameGrid->orderDirection = "ASC";
    gameGrid->Grid->HideRowLabels();
    gameGrid->Grid->EnableDragCell(false);
    gameGrid->Grid->EnableDragColMove(false);
    gameGrid->Grid->EnableDragRowSize(false);
    gameGrid->Grid->CreateGrid(0, 0);
    hSizerGameGrid->Add(gameGrid->Grid);
    vSizerGameGrid->Add(hSizerSearch);
    vSizerGameGrid->Add(hSizerGameGrid);
    gamePanel->SetSizer(vSizerGameGrid);
    gameGrid->Grid->Bind(wxEVT_GRID_CELL_LEFT_DCLICK, &MyFrame::OnGridClick, this);
    gameGrid->Grid->Bind(wxEVT_GRID_LABEL_LEFT_DCLICK, &MyFrame::OnGridLabelClick, this);

    /*New profile panel*/
    vSizerNewProfile = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *gridSizerNewProfile = new wxFlexGridSizer(3, 3, 7);
    wxStaticText *newProfileNameLabel = new wxStaticText(newProfilePanel, wxID_ANY, "Profile Name:");
    newProfileName = new wxTextCtrl(newProfilePanel, wxID_ANY, "",wxDefaultPosition,wxSize(350,wxDefaultSize.GetHeight()));
    auto newProfileSize = newProfileName->GetSize();
    newProfileName->SetSize(500,newProfileSize.GetHeight());
    newProfileName->SetInsertionPoint(0);
    newProfileOnline = new wxCheckBox(newProfilePanel, wxID_ANY, "Download files automatically");
    newProfileOnline->Bind(wxEVT_CHECKBOX, &MyFrame::OnNewProfileOnline, this);
    wxStaticText *newProfileROMSourceLabel = new wxStaticText(newProfilePanel, wxID_ANY, "Rom Source Dir:");
    newProfileROMSourceFolder = new wxStaticText(newProfilePanel, wxID_ANY, "");
    newProfileROMSourceFolderButton = new wxButton(newProfilePanel, wxID_ANY, "Select");

    wxStaticText *newProfileCHDSourceLabel = new wxStaticText(newProfilePanel, wxID_ANY, "CHD Source Dir:");
    newProfileCHDSourceFolder = new wxStaticText(newProfilePanel, wxID_ANY, "");
    newProfileCHDSourceFolderButton = new wxButton(newProfilePanel, wxID_ANY, "Select");

    wxStaticText *newProfileROMTargetLabel = new wxStaticText(newProfilePanel, wxID_ANY, "Rom Target Dir:");
    newProfileROMTargetFolder = new wxStaticText(newProfilePanel, wxID_ANY, "");
    wxButton *newProfileROMTargetFolderButton = new wxButton(newProfilePanel, wxID_ANY, "Select");

    wxStaticText *newProfileCHDTargetLabel = new wxStaticText(newProfilePanel, wxID_ANY, "CHD Target Dir:");
    newProfileCHDTargetFolder = new wxStaticText(newProfilePanel, wxID_ANY, "");
    wxButton *newProfileCHDTargetFolderButton = new wxButton(newProfilePanel, wxID_ANY, "Select");

    wxButton *newProfileCancelButton = new wxButton(newProfilePanel, wxID_ANY, "Cancel");
    wxButton *newProfileSaveButton = new wxButton(newProfilePanel, wxID_ANY, "Save");
    newProfileSaveButton->Bind(wxEVT_BUTTON, &MyFrame::OnNewProfileSaveButton, this);
    gridSizerNewProfile->Add(newProfileNameLabel);
    gridSizerNewProfile->AddSpacer(1);
    gridSizerNewProfile->Add(newProfileName,wxEXPAND | wxALL);
    gridSizerNewProfile->Add(newProfileOnline);
    gridSizerNewProfile->AddSpacer(1);
    gridSizerNewProfile->AddSpacer(1);
    gridSizerNewProfile->Add(newProfileROMSourceLabel);
    gridSizerNewProfile->Add(newProfileROMSourceFolderButton);
    gridSizerNewProfile->Add(newProfileROMSourceFolder);

    gridSizerNewProfile->Add(newProfileCHDSourceLabel);
    gridSizerNewProfile->Add(newProfileCHDSourceFolderButton);
    gridSizerNewProfile->Add(newProfileCHDSourceFolder);

    gridSizerNewProfile->Add(newProfileROMTargetLabel);
    gridSizerNewProfile->Add(newProfileROMTargetFolderButton);
    gridSizerNewProfile->Add(newProfileROMTargetFolder);

    gridSizerNewProfile->Add(newProfileCHDTargetLabel);
    gridSizerNewProfile->Add(newProfileCHDTargetFolderButton);
    gridSizerNewProfile->Add(newProfileCHDTargetFolder);

    gridSizerNewProfile->Add(newProfileCancelButton);
    gridSizerNewProfile->Add(newProfileSaveButton,wxEXPAND | wxALL);
    newProfilePanel->SetSizer(vSizerNewProfile);
    vSizerNewProfile->Add(gridSizerNewProfile,wxEXPAND | wxALL);
    newProfileROMSourceFolderButton->Bind(wxEVT_BUTTON, &MyFrame::OnNewProfileROMSourceFolderButton, this);
    newProfileCHDSourceFolderButton->Bind(wxEVT_BUTTON, &MyFrame::OnNewProfileCHDSourceFolderButton, this);
    newProfileROMTargetFolderButton->Bind(wxEVT_BUTTON, &MyFrame::OnNewProfileROMTargetFolderButton, this);
    newProfileCHDTargetFolderButton->Bind(wxEVT_BUTTON, &MyFrame::OnNewProfileCHDTargetFolderButton, this);
    newProfileCancelButton->Bind(wxEVT_BUTTON, &MyFrame::OnNewProfileCancelButton, this);

/*EDIT PROFILE*/
    vSizerEditProfile = new wxBoxSizer(wxVERTICAL);
    gridSizerEditProfile = new wxFlexGridSizer(3, 3, 7);
    wxStaticText *editProfileNameLabel = new wxStaticText(editProfilePanel, wxID_ANY, "Profile Name:");
    editProfileName = new wxTextCtrl(editProfilePanel, wxID_ANY, "",wxDefaultPosition,wxSize(350,wxDefaultSize.GetHeight()));
    editProfileName->SetInsertionPoint(0);
    editProfileOnline = new wxCheckBox(editProfilePanel, wxID_ANY, "Download files automatically");
    editProfileOnline->Bind(wxEVT_CHECKBOX, &MyFrame::OnEditProfileOnline, this);
    wxStaticText *editProfileROMSourceLabel = new wxStaticText(editProfilePanel, wxID_ANY, "Rom Source Dir:");
    editProfileROMSourceFolder = new wxStaticText(editProfilePanel, wxID_ANY, "");
    editProfileROMSourceFolderButton = new wxButton(editProfilePanel, wxID_ANY, "Select");

    wxStaticText *editProfileCHDSourceLabel = new wxStaticText(editProfilePanel, wxID_ANY, "CHD Source Dir:");
    editProfileCHDSourceFolder = new wxStaticText(editProfilePanel, wxID_ANY, "");
    editProfileCHDSourceFolderButton = new wxButton(editProfilePanel, wxID_ANY, "Select");
    wxStaticText *editProfileROMTargetLabel = new wxStaticText(editProfilePanel, wxID_ANY, "Rom Target Dir:");

    editProfileROMTargetFolder = new wxStaticText(editProfilePanel, wxID_ANY, "");
    wxButton *editProfileROMTargetFolderButton = new wxButton(editProfilePanel, wxID_ANY, "Select");
    wxStaticText *editProfileCHDTargetLabel = new wxStaticText(editProfilePanel, wxID_ANY, "CHD Target Dir:");
    editProfileCHDTargetFolder = new wxStaticText(editProfilePanel, wxID_ANY, "");

    wxButton *editProfileCHDTargetFolderButton = new wxButton(editProfilePanel, wxID_ANY, "Select");
    wxButton *editProfileSaveButton = new wxButton(editProfilePanel, wxID_ANY, "Save");
    editProfileSaveButton->Bind(wxEVT_BUTTON, &MyFrame::OnEditProfileSaveButton, this);
    wxButton *editProfileDeleteButton = new wxButton(editProfilePanel, wxID_ANY, "Delete");
    editProfileDeleteButton->Bind(wxEVT_BUTTON, &MyFrame::OnEditProfileDeleteButton, this);
    wxButton *editProfileCancelButton = new wxButton(editProfilePanel, wxID_ANY, "Cancel");
    editProfileCancelButton->Bind(wxEVT_BUTTON, &MyFrame::OnEditProfileCancelButton, this);
    editProfileROMSourceFolderButton->Bind(wxEVT_BUTTON, &MyFrame::OnEditProfileROMSourceFolderButton, this);
    editProfileCHDSourceFolderButton->Bind(wxEVT_BUTTON, &MyFrame::OnEditProfileCHDSourceFolderButton, this);
    editProfileROMTargetFolderButton->Bind(wxEVT_BUTTON, &MyFrame::OnEditProfileROMTargetFolderButton, this);
    editProfileCHDTargetFolderButton->Bind(wxEVT_BUTTON, &MyFrame::OnEditProfileCHDTargetFolderButton, this);

    gridSizerEditProfile->Add(editProfileNameLabel);
    gridSizerEditProfile->Add(editProfileName);
    gridSizerEditProfile->AddSpacer(1);
    gridSizerEditProfile->Add(editProfileOnline);
    gridSizerEditProfile->AddSpacer(1);
    gridSizerEditProfile->AddSpacer(1);
    gridSizerEditProfile->Add(editProfileROMSourceLabel);
    gridSizerEditProfile->Add(editProfileROMSourceFolderButton);
    gridSizerEditProfile->Add(editProfileROMSourceFolder);

    gridSizerEditProfile->Add(editProfileCHDSourceLabel);
    gridSizerEditProfile->Add(editProfileCHDSourceFolderButton);
    gridSizerEditProfile->Add(editProfileCHDSourceFolder);

    gridSizerEditProfile->Add(editProfileROMTargetLabel);
    gridSizerEditProfile->Add(editProfileROMTargetFolderButton);
    gridSizerEditProfile->Add(editProfileROMTargetFolder);

    gridSizerEditProfile->Add(editProfileCHDTargetLabel);
    gridSizerEditProfile->Add(editProfileCHDTargetFolderButton);
    gridSizerEditProfile->Add(editProfileCHDTargetFolder);

    gridSizerEditProfile->Add(editProfileSaveButton);
    gridSizerEditProfile->Add(editProfileCancelButton);
    gridSizerEditProfile->Add(editProfileDeleteButton);
    editProfilePanel->SetSizer(vSizerEditProfile);
    vSizerEditProfile->Add(gridSizerEditProfile);
    PopulateProfileChoice();
    SetInitialSize();
    SetSize(wxSize(1100, 700));
    Layout();
}

/*Events & Methods*/
void MyFrame::DisplayMessage(std::string Message = "no message")
{
    wxMessageBox(Message, "Romper Message", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnExit(wxCommandEvent &event)
{
    Hide();
    DeletePendingEvents();
    gameGrid->Grid->DestroyChildren();
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent &event)
{
    wxMessageBox("Romper is a tool to help sift through your MAME ROMs. More info on Github.",
                 "Romper Version: " + VERSION, wxOK | wxICON_INFORMATION);
}

void MyFrame::PopulateProfileChoice(int selection /*=0*/)
{
    profileChoice->choice->Clear();
    profileChoice->choice->Append("Choose a profile");
    profileChoice->prevChoice = 0;
    profile_map.clear();
    try
    {
        SQLite::Statement query(profileDB, "SELECT name, online, romSource, chdSource, romTarget, chdTarget FROM profiles ORDER BY name;");
        while (query.executeStep())
        {
            profile_map[query.getColumn(0).getString()] = profile{query.getColumn(0).getString(), query.getColumn(1), query.getColumn(2).getString(), query.getColumn(3).getString(), query.getColumn(4).getString(), query.getColumn(5).getString()};
            std::string str = query.getColumn(0);
            profileChoice->choice->Append(str);
        }
    }
    catch (std::exception &e)
    {
        std::string m("Profile populate error: ");
        m.append(e.what());
        DisplayMessage(m);
        profileChoice->choice->InvalidateBestSize();
        profileChoice->choice->SetSize(this->profileChoice->choice->GetBestSize());
        return;
    }
    profileChoice->choice->SetSelection(selection);
    profileChoice->choice->InvalidateBestSize();
    profileChoice->choice->SetSize(this->profileChoice->choice->GetBestSize());
    wxCommandEvent evt(wxEVT_CHOICE, profileChoice->choice->GetId());
    evt.SetEventObject(this);
    wxPostEvent(profileChoice->choice, evt);
    Layout();
    Refresh();
    
}

void MyFrame::OnProfileChange(wxCommandEvent &event)
{

    if (profile_map.empty()) {
        vSizer->Hide(hSizerLoad);
        vSizer->Hide(hSizerRunButtons);
        mainBook->book->ChangeSelection(romperNewProfile);
        profileEditButton->Hide();
        vSizer->Hide(hSizerRunButtons);
        Layout();
        Refresh();
        DisplayMessage("You must create a profile. Profiles differentiates each game set. Profile ideas would be: 'Best games' or 'Capcom games'.");
        return;
    }
    if (profileChoice->choice->GetSelection() == 0)
    {
        mainBook->book->ChangeSelection(romperBlankPage);
        profileEditButton->Hide();
        vSizer->Hide(hSizerRunButtons);
    }
    else
    {
        wxMenuItemList mr = menuRank->GetMenuItems();
        for (wxMenuItemList::iterator i = mr.begin(); i != mr.end(); ++i)
        {
            i.m_node->GetData()->Check(true);
        }
        wxMenuItemList mg = menuGenre->GetMenuItems();
        for (wxMenuItemList::iterator i = mg.begin(); i != mg.end(); ++i)
        {
            i.m_node->GetData()->Check(true);
        }
        BuildGrid("asc", "Description", searchBy->GetStringSelection().ToStdString(), "", 1, perPage->GetStringSelection().ToStdString());
    }
}

void MyFrame::OnNewProfile(wxCommandEvent &event)
{
    vSizer->Hide(hSizerLoad);
    ChangeMainBookPage(romperNewProfile);
}

void MyFrame::OnEditProfile(wxCommandEvent &event)
{
    vSizer->Hide(hSizerLoad);
    vSizer->Hide(hSizerRunButtons);
    std::string name = profileChoice->choice->GetStringSelection().ToStdString();
    editProfileName->SetValue(name);
    if (profile_map[name].online == 1)
    {
        editProfileOnline->SetValue(true);
    }
    else
    {
        editProfileOnline->SetValue(false);
    }
    editProfileROMSourceFolder->SetLabelText(profile_map[name].romSource);
    editProfileROMTargetFolder->SetLabelText(profile_map[name].romTarget);
    editProfileCHDSourceFolder->SetLabelText(profile_map[name].chdSource);
    editProfileCHDTargetFolder->SetLabelText(profile_map[name].chdTarget);
    vSizerEditProfile->Layout();
    ChangeMainBookPage(romperEditProfile);
}

void MyFrame::OnSearch(wxCommandEvent &event)
{
    BuildGrid("asc", "Description", searchBy->GetStringSelection().ToStdString(), searchInput->GetValue().ToStdString(), 1, perPage->GetStringSelection().ToStdString());
}

void MyFrame::AfterSearch()
{
    searchInput->SetFocus();
    searchInput->SetInsertionPointEnd();
}

void MyFrame::OnNext(wxCommandEvent &event)
{
    if (gameGrid->lastPage > gameGrid->curPage)
    {
        BuildGrid("asc", "Description", searchBy->GetStringSelection().ToStdString(), searchInput->GetValue().ToStdString(), gameGrid->curPage + 1, perPage->GetStringSelection().ToStdString());
    }
}

void MyFrame::OnPrev(wxCommandEvent &event)
{
    if (gameGrid->curPage > 1)
    {
        BuildGrid("asc", "Description", searchBy->GetStringSelection().ToStdString(), searchInput->GetValue().ToStdString(), gameGrid->curPage - 1, perPage->GetStringSelection().ToStdString());
    }
}

void MyFrame::OnPerPage(wxCommandEvent &event)
{
    BuildGrid(gameGrid->orderDirection, gameGrid->orderBy, searchBy->GetStringSelection().ToStdString(), searchInput->GetValue().ToStdString(), 1, perPage->GetStringSelection().ToStdString());
}

void MyFrame::ChangeMainBookPage(int page)
{
    if (romperBlankPage != mainBook->book->GetSelection())
    {
        mainBook->previous = mainBook->book->GetSelection();
    }
    mainBook->book->SetSelection(page);
}

void MyFrame::OnGridLabelClick(wxGridEvent &event)
{
    // if the check box, the select all.
    if (gameGrid->Grid->GetColLabelValue(event.GetCol()).ToStdString() == "X")
    {
        if (gameGrid->prevChangeAll == "")
        {
            gameGrid->prevChangeAll = "1";
        }
        else
        {
            gameGrid->prevChangeAll = "";
        }
        std::string gameNames[gameGrid->Grid->GetNumberRows()];
        std::string qmarks = "";
        for (int i = 0; i <= gameGrid->Grid->GetNumberRows()-1; i++)
        {
            
            wxGridCellCoords coords = wxGridCellCoords();
            coords.Set(i, 1);
            gameNames[i] = gameGrid->Grid->GetCellValue(coords).ToStdString();
            qmarks.append("?,");
        }
        qmarks.pop_back(); // remove last comma.
        try
        {
            if (gameGrid->prevChangeAll == "")
            {
                
                std::string qstr = "DELETE FROM games WHERE profile=? AND game IN(" + qmarks + ");";
                SQLite::Statement query(profileDB, qstr);
                query.bind(1, profileChoice->choice->GetStringSelection().ToStdString());
                for (int i = 1; i <= gameGrid->Grid->GetNumberRows(); i++)
                {
                    query.bind(i + 1, gameNames[i - 1]);
                }
                query.exec();
                //Set X after exec so we know it ran fine.
                
                gameGrid->Grid->BeginBatch();
                for (int i = 0; i < gameGrid->Grid->GetNumberRows(); i++)
                {
                    gameGrid->Grid->SetCellValue(i, 0, "");
                }
                gameGrid->Grid->EndBatch();
                
            }
            else
            {
                
                for (int i = 0; i < gameGrid->Grid->GetNumberRows(); i++)
                {
                    SQLite::Statement query(profileDB, "INSERT OR REPLACE INTO games (profile,game) VALUES (?,?);");
                    query.bind(1, profileChoice->choice->GetStringSelection().ToStdString());
                    query.bind(2, gameNames[i]);
                    //DisplayMessage(query.getExpandedSQL());
                    query.exec();
                    wxGridCellCoords coords = wxGridCellCoords();
                    coords.Set(i, 0);
                    gameGrid->Grid->SetCellValue(i, 0, "1");
                }
            }
        }
        catch (std::exception &e)
        {
            std::string m("SQL ERROR: ");
            m.append(e.what());
            DisplayMessage(m);
        }
        return;
    }

    // else, do order.
    //  Swap order direction
    if (gameGrid->orderDirection == "ASC")
    {
        gameGrid->orderDirection = "DESC";
    }
    else
    {
        gameGrid->orderDirection = "ASC";
    }
    gameGrid->orderBy = gameGrid->Grid->GetColLabelValue(event.GetCol()).ToStdString();
    BuildGrid(gameGrid->orderDirection, gameGrid->orderBy, searchBy->GetStringSelection().ToStdString(), searchInput->GetValue().ToStdString(), 1, perPage->GetStringSelection().ToStdString());
}

void MyFrame::OnGridClick(wxGridEvent &event)
{
    auto col = event.GetCol();
    std::string v = gameGrid->Grid->GetCellValue(event.GetRow(), 0).ToStdString(); //"1" means selected. "" means not selected.
    std::string g = gameGrid->Grid->GetCellValue(event.GetRow(), 1).ToStdString(); // the game's name.
    try
    {
        if (v == "1")
        {
            SQLite::Statement query(profileDB, "DELETE FROM games WHERE profile=? AND game=?;");
            query.bind(1, profileChoice->choice->GetStringSelection().ToStdString());
            query.bind(2, g);
            query.exec();
            gameGrid->Grid->SetCellValue(event.GetRow(), 0, "");
        }
        else
        {
            SQLite::Statement query(profileDB, "INSERT OR REPLACE INTO games (profile,game) VALUES (?,?);");
            query.bind(1, profileChoice->choice->GetStringSelection().ToStdString());
            query.bind(2, g);
            query.exec();
            gameGrid->Grid->SetCellValue(event.GetRow(), 0, "1");
        }
    }
    catch (std::exception &e)
    {
        SetStatusText("Grid Click Error");
        std::string m("Grid Click Error: ");
        m.append(e.what());
        DisplayMessage(m);
        return;
    }
}

void MyFrame::OnNewProfileSaveButton(wxCommandEvent &event)
{
    if (trim(newProfileName->GetValue().ToStdString()) == "" || newProfileROMTargetFolder->GetLabelText().ToStdString() == "" || newProfileCHDTargetFolder->GetLabelText().ToStdString() == "")
    {
        DisplayMessage("You must have a profile name and target folders for ROMS and CHDs. Name must be unique.");
        return;
    }

    // If we are using local files, make sure the folders are set.
    if (!newProfileOnline->IsChecked())
    {
        if (newProfileROMSourceFolder->GetLabelText().ToStdString() == "" || newProfileCHDSourceFolder->GetLabelText().ToStdString() == "")
        {
            DisplayMessage("If you are using local files, you must specify ROM and CHD source folders. Or you can pull files from online.");
            return;
        }
    }
    try
    {
        SQLite::Statement query(profileDB, "INSERT INTO profiles (name,online,romSource,chdSource,romTarget,chdTarget) VALUES (?,?,?,?,?,?);");
        int isOnline = 0;
        if (newProfileOnline->IsChecked())
        {
            isOnline = 1;
        }
        std::string name = trim(newProfileName->GetValue().ToStdString());
        query.bind(1, name);
        query.bind(2, isOnline);
        query.bind(3, newProfileROMSourceFolder->GetLabelText().ToStdString());
        query.bind(4, newProfileCHDSourceFolder->GetLabelText().ToStdString());
        query.bind(5, newProfileROMTargetFolder->GetLabelText().ToStdString());
        query.bind(6, newProfileCHDTargetFolder->GetLabelText().ToStdString());
        query.exec();
        PopulateProfileChoice();
        PopulateProfileChoice(profileChoice->choice->GetStrings().Index(name));
        vSizer->Show(hSizerLoad);
        vSizer->Layout();
        Refresh();
    }
    catch (std::exception &e)
    {
        std::string m("new profile error: ");
        m.append(e.what());
        DisplayMessage(m);
        vSizer->Show(hSizerLoad);
        vSizer->Layout();
        Refresh();
        return;
    }
}

void MyFrame::OnEditProfileSaveButton(wxCommandEvent &event)
{
    std::string prevName = profileChoice->choice->GetStringSelection().ToStdString(); // Get the name so we don't need to covert from wxString twice.
    if (trim(editProfileName->GetValue().ToStdString()) == "" || editProfileROMTargetFolder->GetLabelText().ToStdString() == "" || editProfileCHDTargetFolder->GetLabelText().ToStdString() == "")
    {
        DisplayMessage("You must have a profile name and target folders for ROMS and CHDs. Name must be unique.");
        return;
    }

    // If we are using local files, make sure the folders are set.
    if (!editProfileOnline->IsChecked())
    {
        if (editProfileROMSourceFolder->GetLabelText().ToStdString() == "" || editProfileCHDSourceFolder->GetLabelText().ToStdString() == "")
        {
            DisplayMessage("If you are using local files, you must specify ROM and CHD source folders. Or you can pull files from online.");
            return;
        }
    }
    try
    {
        SQLite::Statement query(profileDB, "UPDATE profiles SET name=?,online=?,romSource=?,chdSource=?,romTarget=?,chdTarget=? WHERE name=?;");
        int isOnline = 0;
        if (editProfileOnline->IsChecked())
        {
            isOnline = 1;
        }

        query.bind(1, trim(editProfileName->GetValue().ToStdString()));
        query.bind(2, isOnline);
        query.bind(3, editProfileROMSourceFolder->GetLabelText().ToStdString());
        query.bind(4, editProfileCHDSourceFolder->GetLabelText().ToStdString());
        query.bind(5, editProfileROMTargetFolder->GetLabelText().ToStdString());
        query.bind(6, editProfileCHDTargetFolder->GetLabelText().ToStdString());
        query.bind(7, prevName);
        query.exec();
        PopulateProfileChoice(profileChoice->choice->GetStrings().Index(prevName));
        vSizer->Show(hSizerLoad);
        vSizer->Layout();
        Refresh();
    }
    catch (std::exception &e)
    {
        std::string m("Edit profile error: ");
        m.append(e.what());
        DisplayMessage(m);
        vSizer->Show(hSizerLoad);
        vSizer->Layout();
        Refresh();
        return;
    }
}

void MyFrame::OnEditProfileDeleteButton(wxCommandEvent &event)
{
    int r = wxMessageBox(
        "Delete " + profileChoice->choice->GetStringSelection().ToStdString() + "?", "Delete Confirmation",
        wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    switch (r)
    {
    case wxYES:
        try
        {
            SQLite::Statement query(profileDB, "DELETE FROM profiles WHERE name=?;");
            query.bind(1, profileChoice->choice->GetStringSelection().ToStdString());
            query.exec();
            SQLite::Statement query2(profileDB, "DELETE FROM games WHERE profile=?;");
            query2.bind(1, profileChoice->choice->GetStringSelection().ToStdString());
            query2.exec();
            PopulateProfileChoice();
            vSizer->Show(hSizerLoad);
            vSizer->Layout();
            Refresh();
        }
        catch (std::exception &e)
        {
            std::string m("Edit profile error: ");
            m.append(e.what());
            DisplayMessage(m);
            vSizer->Show(hSizerLoad);
            vSizer->Layout();
            Refresh();
            return;
        }
        break;
    case wxNO:
        break;
    default:
        break;
    };
}

void MyFrame::OnNewProfileROMSourceFolderButton(wxCommandEvent &event)
{
    wxDirDialog dd = wxDirDialog(panel, "Choose ROM source directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    dd.ShowModal();
    newProfileROMSourceFolder->SetLabel(dd.GetPath());
}

void MyFrame::OnEditProfileROMSourceFolderButton(wxCommandEvent &event)
{
    wxDirDialog dd = wxDirDialog(panel, "Choose ROM source directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    dd.ShowModal();
    editProfileROMSourceFolder->SetLabel(dd.GetPath());
}

void MyFrame::OnNewProfileCHDSourceFolderButton(wxCommandEvent &event)
{
    wxDirDialog dd = wxDirDialog(panel, "Choose CHD source directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    dd.ShowModal();
    newProfileCHDSourceFolder->SetLabel(dd.GetPath());
}

void MyFrame::OnEditProfileCHDSourceFolderButton(wxCommandEvent &event)
{
    wxDirDialog dd = wxDirDialog(panel, "Choose CHD source directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    dd.ShowModal();
    editProfileCHDSourceFolder->SetLabel(dd.GetPath());
}

void MyFrame::OnNewProfileROMTargetFolderButton(wxCommandEvent &event)
{
    wxDirDialog dd = wxDirDialog(panel, "Choose ROM target directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    dd.ShowModal();
    newProfileROMTargetFolder->SetLabel(dd.GetPath());
}

void MyFrame::OnEditProfileROMTargetFolderButton(wxCommandEvent &event)
{
    wxDirDialog dd = wxDirDialog(panel, "Choose ROM target directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    dd.ShowModal();
    editProfileROMTargetFolder->SetLabel(dd.GetPath());
}

void MyFrame::OnNewProfileCHDTargetFolderButton(wxCommandEvent &event)
{
    wxDirDialog dd = wxDirDialog(panel, "Choose ROM target directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    dd.ShowModal();
    newProfileCHDTargetFolder->SetLabel(dd.GetPath());
}

void MyFrame::OnEditProfileCHDTargetFolderButton(wxCommandEvent &event)
{
    wxDirDialog dd = wxDirDialog(panel, "Choose ROM target directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    dd.ShowModal();
    editProfileCHDTargetFolder->SetLabel(dd.GetPath());
}

void MyFrame::OnNewProfileCancelButton(wxCommandEvent &event)
{
    newProfileROMSourceFolder->SetLabel("");
    newProfileCHDSourceFolder->SetLabel("");
    newProfileROMTargetFolder->SetLabel("");
    newProfileCHDTargetFolder->SetLabel("");
    newProfileName->SetValue("");
    newProfileOnline->SetValue(false);

    profileChoice->choice->SetSelection(0);
    wxCommandEvent evt(wxEVT_CHOICE, profileChoice->choice->GetId());
    evt.SetEventObject(this);
    wxPostEvent(profileChoice->choice, evt);
    vSizer->Show(hSizerLoad);
}

void MyFrame::OnEditProfileCancelButton(wxCommandEvent &event)
{
    editProfileROMSourceFolder->SetLabel("");
    editProfileCHDSourceFolder->SetLabel("");
    editProfileROMTargetFolder->SetLabel("");
    editProfileCHDTargetFolder->SetLabel("");
    editProfileName->SetValue("");
    editProfileOnline->SetValue(false);

    // profileChoice->choice->SetSelection(0);
    wxCommandEvent evt(wxEVT_CHOICE, profileChoice->choice->GetId());
    evt.SetEventObject(this);
    wxPostEvent(profileChoice->choice, evt);
    vSizer->Show(hSizerLoad);
    vSizer->Layout();
    Refresh();
}

void MyFrame::OnNewProfileOnline(wxCommandEvent &event)
{
    if (newProfileOnline->IsChecked())
    {
        newProfileROMSourceFolderButton->Hide();
        newProfileROMSourceFolder->SetLabelText("");
        newProfileROMSourceFolder->Hide();
        newProfileCHDSourceFolderButton->Hide();
        newProfileCHDSourceFolder->SetLabelText("");
        newProfileCHDSourceFolder->Hide();
    }
    else
    {
        newProfileROMSourceFolderButton->Show();
        newProfileROMSourceFolder->Show();
        newProfileCHDSourceFolderButton->Show();
        newProfileCHDSourceFolder->Show();
    }
}

void MyFrame::OnEditProfileOnline(wxCommandEvent &event)
{
    if (editProfileOnline->IsChecked())
    {
        editProfileROMSourceFolderButton->Hide();
        editProfileROMSourceFolder->SetLabelText("");
        editProfileROMSourceFolder->Hide();
        editProfileCHDSourceFolderButton->Hide();
        editProfileCHDSourceFolder->SetLabelText("");
        editProfileCHDSourceFolder->Hide();
    }
    else
    {
        editProfileROMSourceFolderButton->Show();
        editProfileROMSourceFolder->Show();
        editProfileCHDSourceFolderButton->Show();
        editProfileCHDSourceFolder->Show();
    }
}

void MyFrame::OnRunButton(wxCommandEvent &event)
{
    std::string profileName = profileChoice->choice->GetStringSelection().ToStdString();
    // make sure the target folders are real.
    struct stat sb;

    if (!dir_exists(profile_map[profileName].romTarget))
    {
        DisplayMessage("Your Rom Target path is invalid. Edit your profile and try again.");
        return;
    }
    std::string romTargetFolder = profile_map[profileName].romTarget;
    if (!dir_exists(profile_map[profileName].chdTarget))
    {
        DisplayMessage("Your CHD Target path is invalid. Edit your profile and try again.");
        return;
    }
    std::string chdTargetFolder = profile_map[profileName].chdTarget;
    // get current profile's selected games.
    try
    {
        SQLite::Statement query(profileDB, "SELECT game FROM games WHERE profile = ?;");
        query.bind(1, profileName);
        std::string whereIn = "(";

        std::vector<gameMap> checkedGames{};

        while (query.executeStep())
        {
            whereIn.append("\"" + query.getColumn(0).getString() + "\",");
        }
        whereIn.pop_back();
        whereIn.append(");");
        std::string strTmp = "SELECT name,disk FROM games WHERE name IN " + whereIn;
        SQLite::Statement query2(gameDB, strTmp);
        while (query2.executeStep())
        {
            checkedGames.push_back(gameMap{query2.getColumn(0).getString(), query2.getColumn(1).getString()});
        }
        int totalGames = std::size(checkedGames);

        std::string totalGamesString = std::to_string(totalGames); // So we don't have to keep calling to_string
        wxString runErrors = "";
        if (profile_map[profileName].online == 1)
        {
            int current_file = 0;
            bool downloading = false;
            bool abort = false;
            bool done = false;
            wxProgressDialog progress("DOWNLOAD GAMES", "Downloading Games", 100, this, wxPD_SMOOTH | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME | wxPD_APP_MODAL);
            progress.Show();
            std::string url = "https://archive.org/download/mame-chds-roms-extras-complete/";
            wxString runErrors;

            while (!abort && current_file < totalGames)
            {
                abort |= DownloadGame(checkedGames[current_file], "rom", url, runErrors, progress);
                if (checkedGames[current_file].disk.size() > 0)
                {
                    abort |= DownloadGame(checkedGames[current_file], "chd", url, runErrors, progress);
                }
                current_file++;
            }
            if (abort)
            {
                progress.Hide();
                DisplayMessage("Aborted");
                return; // If we aborted, don't download CHDs.
            }
            if (runErrors.size() < 1)
            {
                DisplayMessage("Finised with no errors!");
            }
            else
            {
                DisplayMessage("Finished with errors");
            }
        }
        else
        {
            // COPY instead of download
            if (!dir_exists(profile_map[profileName].romSource))
            {
                wxLogMessage("Your Rom Source path is invalid. Edit your profile or select to 'download' instead and try again.");
                return;
            }
            wxProgressDialog progress("COPY FILES", "Copying Files", 100, this, wxPD_SMOOTH | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME | wxPD_APP_MODAL);
            progress.Show();
            int current_file = 0;
            bool abort = false;
            while (current_file < totalGames && !abort)
            {
                // Copy the ROM
                abort |= !progress.Pulse(wxString::Format("Copying (%d/%d): %s", current_file + 1, totalGames, checkedGames[current_file].name));
                if (!std::filesystem::copy_file(profile_map[profileName].romSource + "/" + checkedGames[current_file].name + ".zip", profile_map[profileName].romTarget + "/" + checkedGames[current_file].name + ".zip",
                                                std::filesystem::copy_options::overwrite_existing))
                {
                    runErrors.Append(wxString::Format("Error copying file: %s%s", checkedGames[current_file].name + ".zip", NEWLINE));
                }

                if (checkedGames[current_file].disk.size() > 0)
                {
                    std::filesystem::remove_all(profile_map[profileName].chdTarget + "/" + checkedGames[current_file].name);
                    if (!std::filesystem::create_directory(profile_map[profileName].chdTarget + "/" + checkedGames[current_file].name))
                    {
                        wxLogMessage("error creating CHD folder. Be sure you have write permissions and that there is enough space. Aborting.");
                        return;
                    }
                    abort |= !progress.Pulse(wxString::Format("Copying (%d/%d): DISK %s", current_file + 1, totalGames, checkedGames[current_file].disk));
                    if (!std::filesystem::copy_file(profile_map[profileName].chdSource + "/" + checkedGames[current_file].name + "/" + checkedGames[current_file].disk + ".chd",
                                                    profile_map[profileName].chdTarget + "/" + checkedGames[current_file].name + "/" + checkedGames[current_file].disk + ".chd",
                                                    std::filesystem::copy_options::overwrite_existing))
                    {
                        runErrors.Append(wxString::Format("Error copying file: %s%s", checkedGames[current_file].disk + ".chd", NEWLINE));
                    }
                }

                current_file++;
                if (abort)
                {
                    return; // If we aborted, don't download CHDs.
                }
            }
            progress.Hide();

            if (runErrors.size() > 0)
            {
                int dialog_return_value = wxNO;
                wxMessageDialog *saveLog = new wxMessageDialog(this, "This finished with errors. Would you like to save the errors to a file?", "This finished with errors. Would you like to save the errors to a file?", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
                dialog_return_value = saveLog->ShowModal();
                if (dialog_return_value == wxYES)
                {
                    wxDirDialog dd = wxDirDialog(panel, "Choose location to save log", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
                    dd.ShowModal();
                    dd.GetPath();
                    wxTextFile file(dd.GetPath().Append("/romper_log.txt"));
                    file.Create();
                    file.Open();
                    file.AddLine(runErrors);
                    file.Write();
                    file.Close();
                }
            }
            else
            {
                DisplayMessage("Completed with no errors");
            }
        }
    }
    catch (std::exception &e)
    {
        std::string m("RUN Error: ");
        m.append(e.what());
        DisplayMessage(m);
        return;
    }
}
bool MyFrame::DownloadGame(gameMap game, const char *type, const std::string url, wxString &runErrors, wxProgressDialog &progress)
{
    bool abort = false;
    bool done = false;
    bool downloading = false;

    while (!done && !abort)
    {
        if (!downloading)
        {
            wxWebRequest request;
            if (type == "rom")
            {
                request = wxWebSession::GetDefault().CreateRequest(this, url + game.name + ".zip");
            }
            else
            {
                request = wxWebSession::GetDefault().CreateRequest(this, url + game.name + "/" + game.disk + ".chd");
                // std::cout << url + game.name + "/" + game.disk + ".chd" << std::endl;
            }
            Bind(wxEVT_WEBREQUEST_STATE, [&](wxWebRequestEvent &evt)
                 {
                        switch (evt.GetState())
                        {
                            //Done! mark download as false.
                            case wxWebRequest::State_Completed:
                            {   
                                downloading = false;
                                done = true;
                                wxInputStream *istream = evt.GetResponse().GetStream();
                                if( istream->IsOk() ) {
                                    if (type == "rom") {
                                        wxFileOutputStream ostream( profile_map[profileChoice->choice->GetStringSelection().ToStdString()].romTarget + "/" + game.name+".zip" );
                                        if( ostream.IsOk() ) {
                                        ostream.Write( *istream );
                                        ostream.Close();
                                        }
                                    } else {
                                        if( istream->IsOk() ) {
                                            std::filesystem::remove_all(profile_map[profileChoice->choice->GetStringSelection().ToStdString()].romTarget + "/" + game.name);
                                            if (!std::filesystem::create_directory(profile_map[profileChoice->choice->GetStringSelection().ToStdString()].romTarget + "/" + game.name)){
                                                runErrors.Append(wxString::Format("Could not create folder: %s%s", profile_map[profileChoice->choice->GetStringSelection().ToStdString()].romTarget + "/" + game.name,NEWLINE));
                                            }
                                            wxFileOutputStream ostream( profile_map[profileChoice->choice->GetStringSelection().ToStdString()].romTarget + "/" + game.name+"/"+ game.disk+".chd");
                                            if( ostream.IsOk() ) {
                                                ostream.Write( *istream );
                                                ostream.Close();
                                            }
                                }
                                    }
                                    
                                } else {
                                    switch (istream->GetLastError()) {
                                        case wxStreamError::wxSTREAM_NO_ERROR:
                                        {
                                            std::cout << "STREAM NO ERROR";
                                            break;
                                        }
                                        case wxStreamError::wxSTREAM_EOF:
                                        {
                                            std::cout << "STREAM EOF ERROR";
                                            break;
                                        }
                                        case wxStreamError::wxSTREAM_WRITE_ERROR:
                                        {
                                            std::cout << "STREAM WRITE ERROR";
                                            break;
                                        }
                                        case wxStreamError::wxSTREAM_READ_ERROR:
                                        {
                                            std::cout << "STREAM READ ERROR";
                                            break;
                                        }
                                        default:
                                        {
                                            break;
                                        }
                                    }
                                }
                        
                                done = true;
                                downloading = false;
                            }
                                break;
                                case wxWebRequest::State_Failed:
                                {
                                    std::cout << "State_Failed " << type << " " << game.name << std::endl;
                                    //wxLogError("Could not download: %s", checkedGames[current_file].name);
                                    //abort = true;
                                    done = true;
                                    downloading = false;
                                     break;
                                }
                                case wxWebRequest::State_Unauthorized:
                                {
                                    std::cout << "State_Unauthorized " << type << " " << game.name << std::endl;
                                    done = true;
                                    downloading = false;
                                    break;
                                }
                                default:
                                //std::cout << evt.GetState() << "Unknown " << type << " " << game.name << std::endl;
                                break;
                        } });
            done = false;
            downloading = true;
            request.Start();
        }
        abort |= !progress.Pulse(wxString::Format("Downloading %s %s", type, game.name));
        ::wxYield();
        ::wxMilliSleep(250);
    }
    if (abort)
    {
        return abort; // If we aborted, don't download CHDs.
    }
    return abort;
}

// Non-MyFrame helpers
std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s)
{
    return rtrim(ltrim(s));
}

bool in_array(const std::string &needle, const std::vector<std::string> &haystack)
{
    int max = haystack.size();

    if (max == 0)
        return false;

    for (int i = 0; i < max; i++)
        if (haystack[i] == needle)
            return true;
    return false;
}
// https://en.cppreference.com/w/cpp/filesystem/exists
bool dir_exists(std::string dir)
{
    std::filesystem::path filepath = dir;
    return std::filesystem::is_directory(filepath.parent_path());
}

std::string GetExeDirectory()
{
#ifdef _WIN32
    // Windows specific
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);
#else
    // Linux specific
    char szPath[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", szPath, PATH_MAX);
    if (count < 0 || count >= PATH_MAX)
        return {}; // some error
    szPath[count] = '\0';
#endif
    return std::filesystem::path{szPath}.parent_path().generic_string(); // to finish the folder path with (back)slash
}