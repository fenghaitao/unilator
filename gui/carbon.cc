/////////////////////////////////////////////////////////////////////////
// $Id: carbon.cc,v 1.1.1.1 2003/09/25 03:12:54 fht Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA


// carbon.cc -- bochs GUI file for MacOS X with Carbon API
// written by David Batterham <drbatter@progsoc.uts.edu.au>
// with contributions from Tim Senecal
// port to Carbon API by Emmanuel Maillard <e.rsz@libertysurf.fr>
// Carbon polishing by Jeremy Parsons (Br'fin) <brefin@mac.com>
// slight overhaul of Carbon key event, graphics and window handling
//    and SIM->notify alert support by Chris Thomas <cjack@cjack.com>

// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE 
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE


// BOCHS INCLUDES
#include "bochs.h"

#if BX_WITH_CARBON

#include "icon_bochs.h"
#include "font/vga.bitmap.h"
#include "bxversion.h"

// MAC OS INCLUDES
#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>

// NOTE about use of Boolean versus bx_bool:
// Boolean is defined as unsigned char in the Carbon headers, so when
// you are talking to the Carbon API, it expects to find Boolean variables
// and pointers to Booleans.  The rest of Bochs uses bx_bool to represent
// booleans, which are defined to be 32 bit unsigned, so member function
// definitions and booleans outside this file will be bx_bools.  "Boolean"
// should only be used in Carbon specific code such as in this file.

// CONSTANTS

#define rMBarID    128
#define mApple     128
#define iAbout       1
#define mFile      129
#define iQuit        1
#define mEdit      130
#define iUndo        1
#define iCut         3
#define iCopy        4
#define iPaste       5
#define iClear       6
#define mBochs     131
#define iFloppy      1
#define iCursor      3
#define iTool        4
#define iMenuBar     5
#define iFullScreen  6
#define iConsole     7
#define iSnapshot    9
#define iReset      10

enum {
  RESET_TOOL_BUTTON = 5,
  CONFIGURE_TOOL_BUTTON,
  SNAPSHOT_TOOL_BUTTON,
  PASTE_TOOL_BUTTON,
  COPY_TOOL_BUTTON,
  USER_TOOL_BUTTON
};

const MenuCommand kCommandFloppy = FOUR_CHAR_CODE ('FLPY');
const MenuCommand kCommandCursor = FOUR_CHAR_CODE ('CRSR');
const MenuCommand kCommandTool = FOUR_CHAR_CODE ('TOOL');
const MenuCommand kCommandMenuBar = FOUR_CHAR_CODE ('MENU');
const MenuCommand kCommandFullScreen = FOUR_CHAR_CODE ('SCRN');
const MenuCommand kCommandSnapshot = FOUR_CHAR_CODE ('SNAP');
const MenuCommand kCommandReset = FOUR_CHAR_CODE ('RSET');

#define SLEEP_TIME  0 // Number of ticks to surrender the processor during a WaitNextEvent()
// Change this to 15 or higher if you don't want Bochs to hog the processor!

#define FONT_WIDTH    8
#define FONT_HEIGHT  16

#define WINBITMAP(w)  GetPortBitMapForCopyBits(GetWindowPort(w))  //  (((GrafPtr)(w))->portBits)

#define ASCII_1_MASK  0x00FF0000
#define ASCII_2_MASK  0x000000FF

const RGBColor  black   = {0, 0, 0};
const RGBColor  white   = {0xFFFF, 0xFFFF, 0xFFFF};
const RGBColor  medGrey = {0xCCCC, 0xCCCC, 0xCCCC};
const RGBColor  ltGrey  = {0xEEEE, 0xEEEE, 0xEEEE};

// GLOBALS

WindowPtr            win, toolwin, fullwin, backdrop, hidden, SouixWin;
WindowGroupRef       fullwinGroup;
SInt16               gOldMBarHeight;
Boolean              menubarVisible = true, cursorVisible = true;
Boolean              windowUpdatesPending = true, mouseMoved = false;
RgnHandle            mBarRgn, cnrRgn;
unsigned             mouse_button_state = 0;
CTabHandle           gCTable;
PixMapHandle         gTile;
BitMap               *vgafont[256];
Rect                 srcTextRect, srcTileRect;
Point                scrCenter = {300, 240};
Ptr                  KCHR;
short                gheaderbar_y;
Point                prevPt;
unsigned             width, height, gMinTop, gMaxTop, gLeft;
GWorldPtr            gOffWorld;
ProcessSerialNumber  gProcessSerNum;

enum {
  TEXT_MODE,
  GRAPHIC_MODE
} last_screen_state = TEXT_MODE, screen_state = TEXT_MODE;

// HEADERBAR STUFF
#define TOOL_SPACING 10
#define TOOL_MARGIN_SPACE 4
int             numPixMaps = 0, toolPixMaps = 0;
unsigned        bx_bitmap_left_xorigin  = 2+TOOL_SPACING; // pixels from left
unsigned        bx_bitmap_right_xorigin = 2+TOOL_SPACING; // pixels from right
//PixMapHandle  bx_pixmap[BX_MAX_PIXMAPS];
CIconHandle     bx_cicn[BX_MAX_PIXMAPS];

struct {
//  CIconHandle cicn;
//  PixMapHandle pm;
  ControlRef control;
  unsigned xdim;
  unsigned ydim;
  unsigned xorigin;
  unsigned yorigin;
  unsigned alignment;
  void (*f)(void);
} bx_tool_pixmap[BX_MAX_PIXMAPS];

// Carbon Event Handlers
pascal OSStatus CEvtHandleWindowToolCommand     (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleWindowToolUpdate      (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleWindowBackdropUpdate  (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleWindowEmulatorClick   (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleWindowEmulatorUpdate  (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleWindowEmulatorKeys    (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleApplicationAppleEvent (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleApplicationMouseMoved (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleApplicationMouseUp    (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleApplicationMenuClick  (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
pascal OSStatus CEvtHandleApplicationMenus      (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);

// Event handlers
OSStatus HandleKey(EventRef theEvent, Bit32u keyState);
static BxEvent * CarbonSiminterfaceCallback (void *theClass, BxEvent *event);

// Show/hide UI elements
void HidePointer(void);
void ShowPointer(void);
void HideTools(void);
void ShowTools(void);
void HideMenubar(void);
void ShowMenubar(void);
// void HideConsole(void);
// void ShowConsole(void);

void UpdateTools(void);

// Initialisation
void InitToolbox(void);
void CreateTile(void);
void CreateMenus(void);
void CreateWindows(void);
void CreateKeyMap(void);
void CreateVGAFont(void);
BitMap *CreateBitMap(unsigned width,  unsigned height);
PixMapHandle CreatePixMap(unsigned left, unsigned top, unsigned width,
                          unsigned height, unsigned depth, CTabHandle clut);
unsigned char reverse_bitorder(unsigned char);

static pascal OSErr QuitAppleEventHandler(const AppleEvent *appleEvt, AppleEvent* reply, SInt32 refcon);

class bx_carbon_gui_c : public bx_gui_c {
public:
  bx_carbon_gui_c (void) {}
  DECLARE_GUI_VIRTUAL_METHODS()
};

// declare one instance of the gui object and call macro to insert the
// plugin code
static bx_carbon_gui_c *theGui = NULL;
IMPLEMENT_GUI_PLUGIN_CODE(carbon)

#define LOG_THIS theGui->

// Carbon Event Handlers

pascal OSStatus CEvtHandleWindowToolCommand (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  HICommand commandStruct;
  UInt32 theCommandID;

  GetEventParameter (theEvent, kEventParamDirectObject,
                     typeHICommand, NULL, sizeof(HICommand), 
                     NULL, &commandStruct);

  theCommandID = commandStruct.commandID;
  
  if(theCommandID < toolPixMaps ) {
    bx_tool_pixmap[theCommandID].f();
  }
  
  return noErr; // Report success
}

pascal OSStatus CEvtHandleWindowToolUpdate (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  theGui->show_headerbar();

  return noErr; // Report success
}

pascal OSStatus CEvtHandleWindowBackdropUpdate (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  Rect  box;
        
  WindowRef myWindow;
  GetEventParameter (theEvent, kEventParamDirectObject, typeWindowRef,
                     NULL, sizeof(WindowRef), NULL, &myWindow);

  GetWindowPortBounds(myWindow, &box);
  BackColor(blackColor);
  EraseRect(&box);

  return noErr; // Report success
}

// Translate MouseDowns in a handled window into Bochs events
// Main ::HANDLE_EVENTS will feed all mouse updates to Bochs
pascal OSStatus CEvtHandleWindowEmulatorClick (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  UInt32 keyModifiers;
  GetEventParameter (theEvent, kEventParamKeyModifiers, typeUInt32,
                     NULL, sizeof(UInt32), NULL, &keyModifiers);

//  if (!IsWindowActive(win))
//  {
//    SelectWindow(win);
//  }
  if (keyModifiers & cmdKey)
    mouse_button_state |= 0x02;
  else
    mouse_button_state |= 0x01;

  return noErr; // Report success
}

pascal OSStatus CEvtHandleWindowEmulatorUpdate (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  Rect  box;
  Pattern qdBlackPattern;
  
  WindowRef myWindow;
  GetEventParameter (theEvent, kEventParamDirectObject, typeWindowRef,
                     NULL, sizeof(WindowRef), NULL, &myWindow);

  GetWindowPortBounds(myWindow, &box);
  DEV_vga_redraw_area(box.left, box.top, box.right, box.bottom);

  return noErr; // Report success
}

pascal OSStatus CEvtHandleWindowEmulatorKeys (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  UInt32  kind;
  OSStatus  outStatus = eventNotHandledErr;
  
  kind = GetEventKind(theEvent);
  switch(kind)
  {
    case kEventRawKeyDown:
    case kEventRawKeyRepeat:
      outStatus = HandleKey(theEvent, BX_KEY_PRESSED);
      break;
    case kEventRawKeyUp:
      outStatus = HandleKey(theEvent, BX_KEY_RELEASED);
      break;
    }
    
    return outStatus; 
}

#if 0
// This stuff does work... it gets called, but converting the record
// and then calling AEProcessAppleEvent consistently results in noOutstandingHLE(err result -608)
// And its going to take more work to get RunApplicationLoop to work...
pascal OSStatus CEvtHandleApplicationAppleEvent (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  EventRecord eventRec;

  fprintf(stderr, "# Carbon apple event handler called\n");
  if(ConvertEventRefToEventRecord(theEvent, &eventRec))
  {
    fprintf(stderr, "# Calling AEProcessAppleEvent\n");
    OSStatus result = AEProcessAppleEvent(&eventRec);
    fprintf(stderr, "# Received AE result: %i\n", result);
    returm result;
  }
  else
    BX_PANIC(("Can't convert apple event"));

  return noErr; // Report success
}
#endif

// Only have our application deal with mouseEvents when we catch the movement
pascal OSStatus CEvtHandleApplicationMouseMoved (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  mouseMoved = true;
        
  return eventNotHandledErr;
}

pascal OSStatus CEvtHandleApplicationMouseUp (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  UInt32 keyModifiers;
  GetEventParameter (theEvent, kEventParamKeyModifiers, typeUInt32,
                     NULL, sizeof(UInt32), NULL, &keyModifiers);

  if (keyModifiers & cmdKey)
    mouse_button_state &= ~0x02;
  else
    mouse_button_state &= ~0x01;

  return eventNotHandledErr; // Don't want to eat all the mouseups
}

// Catch MouseDown's in the menubar, trigger menu browsing
pascal OSStatus CEvtHandleApplicationMenuClick (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  short part;
  WindowPtr whichWindow;

  Point wheresMyMouse;
  GetEventParameter (theEvent, kEventParamMouseLocation, typeQDPoint,
                     NULL, sizeof(Point), NULL, &wheresMyMouse);

  part = FindWindow(wheresMyMouse, &whichWindow);

  if(part == inMenuBar)
  {
    // MenuSelect will actually trigger an event cascade, 
    // Triggering command events for any selected menu item
    MenuSelect(wheresMyMouse);
    return noErr;
  }

  return eventNotHandledErr; // Don't want to eat all the clicks
}

pascal OSStatus CEvtHandleApplicationMenus (EventHandlerCallRef nextHandler,
  EventRef theEvent,
  void* userData)
{
  HICommand commandStruct;

  OSErr  err = noErr;
  short  i;

  GetEventParameter (theEvent, kEventParamDirectObject,
                     typeHICommand, NULL, sizeof(HICommand),
                     NULL, &commandStruct);

  switch(commandStruct.commandID)
  {
    case kHICommandAbout:
    {
      DialogRef       aboutDialog;
      DialogItemIndex index;
      CFStringRef     cf_version;
      char            version[256];
      sprintf(version, "Bochs x86 Emulator version %s (MacOS X port)", VER_STRING);
      cf_version = CFStringCreateWithCString(NULL, version, kCFStringEncodingASCII);
      
      AlertStdCFStringAlertParamRec aboutParam = {0};
      aboutParam.version       = kStdCFStringAlertVersionOne;
      aboutParam.position      = kWindowDefaultPosition;
      aboutParam.defaultButton = kAlertStdAlertOKButton;

      CreateStandardAlert(
        kAlertNoteAlert,
        cf_version,
        NULL,            /* can be NULL */
        &aboutParam,     /* can be NULL */
        &aboutDialog);

      RunStandardAlert(
        aboutDialog,
        NULL,       /* can be NULL */
        &index);
      CFRelease( cf_version );
    }
    break;

    case kHICommandQuit:
      BX_EXIT(0);
      break;

    case kCommandFloppy:
      //DiskEject(1);
      break;

    case kCommandCursor:
      if (cursorVisible)
        HidePointer();
      else
        ShowPointer();
      break;

    case kCommandTool:
      if (IsWindowVisible(toolwin))
        HideTools();
      else
        ShowTools();
      break;

    case kCommandMenuBar:
      if (menubarVisible)
        HideMenubar();
      else
        ShowMenubar();
      break;

    case kCommandFullScreen:
      if (IsWindowVisible(toolwin) || menubarVisible)
      {
        if (menubarVisible)
          HideMenubar();
        if (IsWindowVisible(toolwin))
          HideTools();
      }
      else
      {
        if (!menubarVisible)
          ShowMenubar();
        if (!IsWindowVisible(toolwin))
          ShowTools();
      }
      break;
      
/*
    // Codewarrior programatic console that isn't available under Carbon without Codewarrior
    case iConsole:
      if (IsWindowVisible(SouixWin))
        HideConsole();
      else
        ShowConsole();
      break;
*/
    case kCommandSnapshot:
      bx_tool_pixmap[SNAPSHOT_TOOL_BUTTON].f();
      break;

    case kCommandReset:
      bx_tool_pixmap[RESET_TOOL_BUTTON].f();
      break;

    case kHICommandCopy:
      bx_tool_pixmap[COPY_TOOL_BUTTON].f();
      break;

    case kHICommandPaste:
      bx_tool_pixmap[PASTE_TOOL_BUTTON].f();
      break;
  }
  
  return noErr; // Report success
}

void MacPanic(void)
{
  StopAlert(200, NULL);
}

void InitToolbox(void)
{
  OSErr err;
  //  gQuitFlag = false;
  
  InitCursor();

#if 0
  // Our handler gets called... but I can't AEProcesAppleEvent successfully upon it?
  EventTypeSpec appleEvent = { kEventClassAppleEvent, kEventAppleEvent };
  InstallApplicationEventHandler(NewEventHandlerUPP(CEvtHandleApplicationAppleEvent),
    1, &appleEvent, 0, NULL);
#endif

  err = AEInstallEventHandler( kCoreEventClass, kAEQuitApplication,
    NewAEEventHandlerUPP(QuitAppleEventHandler), 0, false);
  if (err != noErr)
    ExitToShell();
}

static pascal OSErr QuitAppleEventHandler(const AppleEvent *appleEvt, AppleEvent* reply, SInt32 refcon)
{
  //gQuitFlag =  true;
  BX_PANIC(("User terminated"));
  return (noErr);
}

void CreateTile(void)
{
  GDHandle  saveDevice;
  CGrafPtr  savePort;
  OSErr     err;
  
  if (bx_options.Oprivate_colormap->get ())
  {
    GetGWorld(&savePort, &saveDevice);
  
    err = NewGWorld(&gOffWorld, 8, 
      &srcTileRect, gCTable, NULL, useTempMem);
    if (err != noErr)
      BX_PANIC(("mac: can't create gOffWorld"));

    SetGWorld(gOffWorld, NULL);
  
    gTile = GetGWorldPixMap(gOffWorld);
    
    if (gTile != NULL)
    {
      NoPurgePixels(gTile);
      LockPixels(gTile);
      (**gTile).rowBytes = 0x8000 | srcTileRect.right;
      (**gCTable).ctFlags = (**gCTable).ctFlags | 0x4000;   //use palette manager indexes
      (**gTile).pmTable = gCTable;
    }
    else
      BX_PANIC(("mac: can't create gTile"));
  
    SetGWorld(savePort, saveDevice);
  }
  else
  {
    gTile = CreatePixMap(0, 0, srcTileRect.right, srcTileRect.bottom, 8, gCTable);
    if (gTile == NULL)
      BX_PANIC(("mac: can't create gTile"));
  }
}

void CreateMenus(void)
{
  Handle menu;
  
  menu = GetNewMBar(rMBarID);   //  get our menus from resource
  if (menu != nil) 
  {
    SetMenuBar(menu);
    DrawMenuBar();
  }
  else
    BX_PANIC(("can't create menu"));
  
  SetMenuItemCommandID (GetMenuRef(mApple), iAbout,      kHICommandAbout);
  SetMenuItemCommandID (GetMenuRef(mFile),  iQuit,       kHICommandQuit);
  SetMenuItemCommandID (GetMenuRef(mEdit),  iCopy,       kHICommandCopy);
  SetMenuItemCommandID (GetMenuRef(mEdit),  iPaste,      kHICommandPaste);
  SetMenuItemCommandID (GetMenuRef(mBochs), iFloppy,     kCommandFloppy);
  SetMenuItemCommandID (GetMenuRef(mBochs), iCursor,     kCommandCursor);
  SetMenuItemCommandID (GetMenuRef(mBochs), iTool,       kCommandTool);
  SetMenuItemCommandID (GetMenuRef(mBochs), iMenuBar,    kCommandMenuBar);
  SetMenuItemCommandID (GetMenuRef(mBochs), iFullScreen, kCommandFullScreen);
  SetMenuItemCommandID (GetMenuRef(mBochs), iSnapshot,   kCommandSnapshot);
  SetMenuItemCommandID (GetMenuRef(mBochs), iReset,      kCommandReset);
  
  DisableMenuItem(GetMenuRef(mEdit), iUndo);
  DisableMenuItem(GetMenuRef(mEdit), iCut);
  DisableMenuItem(GetMenuRef(mEdit), iClear);
  DisableMenuItem(GetMenuRef(mBochs), iFloppy);

  EventTypeSpec commandEvents = {kEventClassCommand, kEventCommandProcess};
  EventTypeSpec menuEvents = {kEventClassMouse, kEventMouseDown};
  InstallApplicationEventHandler(NewEventHandlerUPP(CEvtHandleApplicationMenus),
    1, &commandEvents, 0, NULL);
  InstallApplicationEventHandler(NewEventHandlerUPP(CEvtHandleApplicationMenuClick),
    1, &menuEvents, 0, NULL);
}

void CreateWindows(void)
{
  Rect winRect;
  Rect screenBounds;
  Rect positioningBounds;

  EventTypeSpec eventClick =  { kEventClassWindow, kEventWindowHandleContentClick };
  EventTypeSpec eventUpdate = { kEventClassWindow, kEventWindowDrawContent };
  EventTypeSpec keyboardEvents[3] = {
    { kEventClassKeyboard, kEventRawKeyDown }, { kEventClassKeyboard, kEventRawKeyRepeat },
    { kEventClassKeyboard, kEventRawKeyUp }};
  EventTypeSpec eventCommand =  { kEventClassCommand, kEventCommandProcess };

  // Create a backdrop window for fullscreen mode
//  GetRegionBounds(GetGrayRgn(), &screenBounds);

  // Fullscreen mode only really wants to be on one screen
  screenBounds = (**GetMainDevice()).gdRect;
  GetAvailableWindowPositioningBounds(GetMainDevice(), &positioningBounds);
  
  SetRect(&winRect, 0, 0, screenBounds.right, screenBounds.bottom + GetMBarHeight());
  CreateNewWindow(kPlainWindowClass, (kWindowStandardHandlerAttribute ), &winRect, &backdrop);
  if (backdrop == NULL)
    {BX_PANIC(("mac: can't create backdrop window"));}
  InstallWindowEventHandler(backdrop, NewEventHandlerUPP(CEvtHandleWindowBackdropUpdate), 1, &eventUpdate, NULL, NULL);
  InstallWindowEventHandler(backdrop, NewEventHandlerUPP(CEvtHandleWindowEmulatorClick), 1, &eventClick, NULL, NULL);
  InstallWindowEventHandler(backdrop, NewEventHandlerUPP(CEvtHandleWindowEmulatorKeys), 3, keyboardEvents, 0, NULL);
  
  width = 640;
  height = 480;
  gLeft = positioningBounds.left;
  gMinTop = positioningBounds.top;
  gMaxTop = gMinTop + gheaderbar_y;

  // Create a moveable tool window for the "headerbar"
  winRect.top   = positioningBounds.top + 10;
  winRect.left  = positioningBounds.left;
  winRect.bottom  = winRect.top + gheaderbar_y;
  winRect.right = positioningBounds.right;

  CreateNewWindow(kFloatingWindowClass, kWindowStandardHandlerAttribute, &winRect, &toolwin);
  if (toolwin == NULL)
    {BX_PANIC(("mac: can't create tool window"));}

  // Use an Aqua-savvy window background
  SetThemeWindowBackground(toolwin, kThemeBrushUtilityWindowBackgroundActive, false);

  SetWindowTitleWithCFString (toolwin, CFSTR("MacBochs Hardware Controls")); // Set title
  //InstallWindowEventHandler(toolwin, NewEventHandlerUPP(CEvtHandleWindowToolClick),  1, &eventClick, NULL, NULL);
  InstallWindowEventHandler(toolwin, NewEventHandlerUPP(CEvtHandleWindowToolCommand),  1, &eventCommand, NULL, NULL);
  InstallWindowEventHandler(toolwin, NewEventHandlerUPP(CEvtHandleWindowToolUpdate), 1, &eventUpdate, NULL, NULL);

  // Create the emulator window for full screen mode
  winRect.left  = (screenBounds.right - width) /2;  //(qd.screenBits.bounds.right - width)/2;
  winRect.right = winRect.left + width;
  winRect.top   = (screenBounds.bottom - height)/2;
  winRect.bottom  = winRect.top + height;

  CreateNewWindow(kPlainWindowClass, (kWindowStandardHandlerAttribute), &winRect, &fullwin);
  if (fullwin == NULL)
    BX_PANIC(("mac: can't create fullscreen emulator window"));

  InstallWindowEventHandler(fullwin, NewEventHandlerUPP(CEvtHandleWindowEmulatorUpdate), 1, &eventUpdate, NULL, NULL);
  InstallWindowEventHandler(fullwin, NewEventHandlerUPP(CEvtHandleWindowEmulatorClick), 1, &eventClick, NULL, NULL);  InstallWindowEventHandler(fullwin, NewEventHandlerUPP(CEvtHandleWindowEmulatorKeys), 3, keyboardEvents, 0, NULL);

  // Create the regular emulator window
  winRect.left  = gLeft;
  winRect.top   = gMaxTop;
  winRect.right = winRect.left + width;
  winRect.bottom  = winRect.top + height;

  CreateNewWindow(kDocumentWindowClass,
    (kWindowStandardHandlerAttribute | kWindowCollapseBoxAttribute),
    &winRect, &win);
  if (win == NULL)
    BX_PANIC(("mac: can't create emulator window"));
  
  SetWindowTitleWithCFString (win, CFSTR("MacBochs x86 PC")); // Set title
  InstallWindowEventHandler(win, NewEventHandlerUPP(CEvtHandleWindowEmulatorUpdate), 1, &eventUpdate, NULL, NULL);
  InstallWindowEventHandler(win, NewEventHandlerUPP(CEvtHandleWindowEmulatorClick), 1, &eventClick, NULL, NULL);
  InstallWindowEventHandler(win, NewEventHandlerUPP(CEvtHandleWindowEmulatorKeys), 3, keyboardEvents, 0, NULL);

  // Group the fullscreen and backdrop windows together, since they also share the same click
  // event handler they will effectively act as a single window for layering and events

  CreateWindowGroup((kWindowGroupAttrLayerTogether | kWindowGroupAttrSharedActivation), &fullwinGroup);
  SetWindowGroupName(fullwinGroup, CFSTR("net.sourceforge.bochs.windowgroups.fullscreen"));

  // This *can't* be the right way, then again groups aren't yet the right way
  // For the life of me I couldn't find a right way of making sure my created group stayed
  // below the layer of Floating Windows. But with the windows we have there's no current
  // harm from making it part of the same group.
  SetWindowGroup(toolwin, fullwinGroup);
  SetWindowGroup(fullwin, fullwinGroup);
  SetWindowGroup(backdrop, fullwinGroup);

  RepositionWindow( win, NULL, kWindowCenterOnMainScreen );

  hidden = fullwin;
  
  ShowWindow(toolwin);
  ShowWindow(win);
  
  SetPortWindowPort(win);
}

// ::SPECIFIC_INIT()
//
// Called from gui.cc, once upon program startup, to allow for the
// specific GUI code (X11, BeOS, ...) to be initialized.
//
// argc, argv: not used right now, but the intention is to pass native GUI
//     specific options from the command line.  (X11 options, BeOS options,...)
//
// tilewidth, tileheight: for optimization, graphics_tile_update() passes
//     only updated regions of the screen to the gui code to be redrawn.
//     These define the dimensions of a region (tile).
// headerbar_y:  A headerbar (toolbar) is display on the top of the
//     VGA window, showing floppy status, and other information.  It
//     always assumes the width of the current VGA mode width, but
//     it's height is defined by this parameter.

void bx_carbon_gui_c::specific_init(int argc, char **argv, unsigned tilewidth, unsigned tileheight,
  unsigned headerbar_y)
{ 
  put("MGUI");
  InitToolbox();
  
  gheaderbar_y = headerbar_y + TOOL_MARGIN_SPACE + TOOL_MARGIN_SPACE;
  
  CreateKeyMap();

  gCTable = GetCTable(128);
  BX_ASSERT (gCTable != NULL);
  (*gCTable)->ctSeed = GetCTSeed(); 
  SetRect(&srcTextRect, 0, 0, FONT_WIDTH, FONT_HEIGHT);
  SetRect(&srcTileRect, 0, 0, tilewidth, tileheight);
  
  CreateMenus();
  CreateVGAFont();
  CreateTile();
  CreateWindows();
  
  EventTypeSpec mouseUpEvent = { kEventClassMouse, kEventMouseUp };
  EventTypeSpec mouseMoved[2] = { { kEventClassMouse, kEventMouseMoved },
    { kEventClassMouse, kEventMouseDragged } };
  InstallApplicationEventHandler(NewEventHandlerUPP(CEvtHandleApplicationMouseUp),
    1, &mouseUpEvent, 0, NULL);
  InstallApplicationEventHandler(NewEventHandlerUPP(CEvtHandleApplicationMouseMoved),
    2, mouseMoved, 0, NULL);

  GetCurrentProcess(&gProcessSerNum);
  
  GetMouse(&prevPt);

  SIM->set_notify_callback(CarbonSiminterfaceCallback, NULL);
  
  UNUSED(argc);
  UNUSED(argv);

  // loads keymap for x11
  if(bx_options.keyboard.OuseMapping->get()) {
    bx_keymap.loadKeymap(NULL); // I have no function to convert X windows symbols
  }
}

// HandleKey()
//
// Handles keyboard-related events.

OSStatus HandleKey(EventRef theEvent, Bit32u keyState)
{
  UInt32    key;
  UInt32    trans;
  OSStatus    status;
  UInt32    modifiers;
  
  static UInt32 transState = 0;
  
  status = GetEventParameter (theEvent,
                              kEventParamKeyModifiers,
                              typeUInt32, NULL,
                              sizeof(UInt32), NULL,
                              &modifiers);
  if( status == noErr )
  {
    status = GetEventParameter (theEvent,
                                kEventParamKeyCode,
                                typeUInt32, NULL,
                                sizeof(UInt32), NULL,
                                &key);
    if( status == noErr )
    {

//    key = event->message & charCodeMask;
      
      // Let our menus process command keys
      if( modifiers & cmdKey )
      {
        status = eventNotHandledErr;
      }
      else
      {
        if (modifiers & shiftKey)
          DEV_kbd_gen_scancode(BX_KEY_SHIFT_L | keyState);
        if (modifiers & controlKey)
          DEV_kbd_gen_scancode(BX_KEY_CTRL_L | keyState);
        if (modifiers & optionKey)
          DEV_kbd_gen_scancode(BX_KEY_ALT_L | keyState);
        
//        key = (event->message & keyCodeMask) >> 8;
        
        trans = KeyTranslate(KCHR, key, &transState);
        
        // KeyTranslate maps Mac virtual key codes to any type of character code
        // you like (in this case, Bochs key codes). Much nicer than a huge switch
        // statement!
        
        if (trans > 0)
          DEV_kbd_gen_scancode(trans | keyState);

        if (modifiers & shiftKey)
          DEV_kbd_gen_scancode(BX_KEY_SHIFT_L | BX_KEY_RELEASED);
        if (modifiers & controlKey)
          DEV_kbd_gen_scancode(BX_KEY_CTRL_L | BX_KEY_RELEASED);
        if (modifiers & optionKey)
          DEV_kbd_gen_scancode(BX_KEY_ALT_L | BX_KEY_RELEASED);
      }
    }
  }
  
  return status;
}

BX_CPP_INLINE void ResetPointer(void)
{
  // this appears to work well, especially when combined with
  // mouse processing on the MouseMoved events
  if(CGWarpMouseCursorPosition(CGPointMake(scrCenter.h, scrCenter.v)))
  {
    fprintf(stderr, "# Failed to warp cursor");
  }
}

// ::HANDLE_EVENTS()
//
// Called periodically (vga_update_interval in .bochsrc) so the
// the gui code can poll for keyboard, mouse, and other
// relevant events.

void bx_carbon_gui_c::handle_events(void)
{
  EventRecord event;
  Point mousePt;
  int dx, dy;
  int oldMods=0;
  unsigned curstate;
  GrafPtr oldport;

  curstate = mouse_button_state;      //so we can compare the old and the new mouse state

  if (WaitNextEvent(everyEvent, &event, SLEEP_TIME, NULL))
  {
    switch(event.what)
    {
/*
      // This event is just redundant
      case nullEvent:
                        
      // These events are all covered by installed carbon event handlers
      case mouseDown:
      case mouseUp:
      case keyDown:
      case autoKey:
      case keyUp:
      case updateEvt:
        break;
*/        
      case diskEvt:
      //  floppyA_handler();
        break;
      
      case kHighLevelEvent:
        // fprintf(stderr, "# Classic apple event handler called\n");
        AEProcessAppleEvent(&event);
        show_headerbar(); // Update if necessary (example, clipboard change)
        
      default:
        break;
    }
  }
  
  // Only update mouse if we're not in the dock
  // and we are the frontmost app.
  ProcessSerialNumber frontProcessSerNum;
  Boolean isSameProcess;
  
  GetFrontProcess(&frontProcessSerNum);
  SameProcess(&frontProcessSerNum, &gProcessSerNum, &isSameProcess);

  if(isSameProcess && !IsWindowCollapsed(win))
  {
    GetPort(&oldport);
    SetPortWindowPort(win);
    
    GetMouse(&mousePt);
    
    if(menubarVisible && cursorVisible)
    {
      // Don't track the mouse if we're working with the main window
      // and we're outside the window
      if(mouseMoved &&
        (mousePt.v < 0 || mousePt.v > height || mousePt.h < 0 || mousePt.h > width) &&
        (prevPt.v < 0 || prevPt.v > height || prevPt.h < 0 || prevPt.h > width))
      {
        mouseMoved = false;
      }
/*
      // Limit mouse action to window
      // Grr, any better ways to sync host and bochs cursor?
      if(mousePt.h < 0) { mousePt.h = 0; }
      else if(mousePt.h > width) { mousePt.h = width; }
      if(mousePt.v < 0) { mousePt.v = 0; }
      else if(mousePt.v > height) { mousePt.v = height; }
*/
    }
    
    //if mouse has moved, or button has changed state
    if (mouseMoved || (curstate != mouse_button_state))
    {
      if(mouseMoved)
      {
        CGMouseDelta CGdX, CGdY;
        CGGetLastMouseDelta( &CGdX, &CGdY );
        dx = CGdX;
        dy = - CGdY; // Windows has an opposing grid
      }
      else
      {
        dx = 0;
        dy = 0;
      }
      
      DEV_mouse_motion(dx, dy, mouse_button_state);
      
      if (!cursorVisible && mouseMoved)
      {
        SetPt(&scrCenter, 300, 240);
        LocalToGlobal(&scrCenter);
        ResetPointer();   //next getmouse should be 300, 240
        SetPt(&mousePt, 300, 240);
      }
      mouseMoved = false;
    }

    prevPt = mousePt;
    
    SetPort(oldport);
  }
}


// ::FLUSH()
//
// Called periodically, requesting that the gui code flush all pending
// screen update requests.

void bx_carbon_gui_c::flush(void)
{
  // an opportunity to make the Window Manager happy.
  // not needed on the macintosh....
        
  // Unless you don't want to needlessly update the dock icon 
  // umpteen zillion times a second for each tile.
  // A further note, UpdateCollapsedWindowDockTile is not
  // recommended for animation. Setup like this my performance
  // seems reasonable for little fuss.
  if(windowUpdatesPending)
  {
    if(IsWindowCollapsed(win))
    {
      UpdateCollapsedWindowDockTile(win);
    }
    if(last_screen_state != screen_state)
    {
      last_screen_state = screen_state;
      UpdateTools();
    }
  }
  windowUpdatesPending = false;
}


// ::CLEAR_SCREEN()
//
// Called to request that the VGA region is cleared.  Don't
// clear the area that defines the headerbar.

void bx_carbon_gui_c::clear_screen(void)
{
  Rect r;
  
  SetPortWindowPort(win);
  
  RGBForeColor(&black);
  RGBBackColor(&white);
  GetWindowPortBounds(win, &r);
  PaintRect (&r);
        
  windowUpdatesPending = true;
}



// ::TEXT_UPDATE()
//
// Called in a VGA text mode, to update the screen with
// new content.
//
// old_text: array of character/attributes making up the contents
//           of the screen from the last call.  See below
// new_text: array of character/attributes making up the current
//           contents, which should now be displayed.  See below
//
// format of old_text & new_text: each is 80*nrows*2 bytes long.
//     This represents 80 characters wide by 'nrows' high, with
//     each character being 2 bytes.  The first by is the
//     character value, the second is the attribute byte.
//     I currently don't handle the attribute byte.
//
// cursor_x: new x location of cursor
// cursor_y: new y location of cursor

void bx_carbon_gui_c::text_update(Bit8u *old_text, Bit8u *new_text,
  unsigned long cursor_x, unsigned long cursor_y,
  Bit16u cursor_state, unsigned nrows)
{
  int           i;
  unsigned char achar;
  int           x, y;
  static int    previ;
  int           cursori;
  Rect          destRect;
  RGBColor      fgColor, bgColor;
  GrafPtr       oldPort;
  GrafPtr       winGrafPtr = GetWindowPort(win);
  unsigned      nchars, ncols;
  
  screen_state = TEXT_MODE;
  
  GetPort(&oldPort);
  
  SetPortWindowPort(win);

  ncols = width/8;

  //current cursor position
  cursori = (cursor_y*ncols + cursor_x)*2;

  // Number of characters on screen, variable number of rows
  nchars = ncols*nrows;
  
  for (i=0; i<nchars*2; i+=2)
  {
    if ( i == cursori || i == previ || new_text[i] != old_text[i] || new_text[i+1] != old_text[i+1])
    {
      achar = new_text[i];
      
//      fgColor = (**gCTable).ctTable[new_text[i+1] & 0x0F].rgb;
//      bgColor = (**gCTable).ctTable[(new_text[i+1] & 0xF0) >> 4].rgb;
      
//      RGBForeColor(&fgColor);
//      RGBBackColor(&bgColor);
      
      if (bx_options.Oprivate_colormap->get ())
      {
        PmForeColor(new_text[i+1] & 0x0F);
        PmBackColor((new_text[i+1] & 0xF0) >> 4);
      }
      else
      {
        fgColor = (**gCTable).ctTable[new_text[i+1] & 0x0F].rgb;
        bgColor = (**gCTable).ctTable[(new_text[i+1] & 0xF0) >> 4].rgb;
      
        RGBForeColor(&fgColor);
        RGBBackColor(&bgColor);
      }
      
      x = ((i/2) % ncols)*FONT_WIDTH;
      y = ((i/2) / ncols)*FONT_HEIGHT;

      SetRect(&destRect, x, y,
        x+FONT_WIDTH, y+FONT_HEIGHT);
        
      CopyBits( vgafont[achar], WINBITMAP(win),
        &srcTextRect, &destRect, srcCopy, NULL);

      if (i == cursori)   //invert the current cursor block
      {
        InvertRect(&destRect);
      }
      
    }
  }
 
//previous cursor position
  previ = cursori;
  
  SetPort(oldPort);

  windowUpdatesPending = true;
}

  int
bx_carbon_gui_c::get_clipboard_text(Bit8u **bytes, Bit32s *nbytes)
{
  ScrapRef         theScrap;
  ScrapFlavorFlags theScrapFlags;
  Size             theScrapSize;
  OSStatus         err;

  GetCurrentScrap( &theScrap );
  
  // Make sure there is text to paste
  err= GetScrapFlavorFlags( theScrap, kScrapFlavorTypeText, &theScrapFlags);
  if(err == noErr)
  {
    GetScrapFlavorSize( theScrap, kScrapFlavorTypeText, &theScrapSize);
    *nbytes = theScrapSize;
    *bytes = new Bit8u[1 + *nbytes];
    BX_INFO (("found %d bytes on the clipboard", *nbytes));
    err= GetScrapFlavorData( theScrap, kScrapFlavorTypeText, &theScrapSize, *bytes);
    BX_INFO (("first byte is 0x%02x", *bytes[0]));
  }
  else
  {
    BX_INFO (("No text found on clipboard..."));
  }
  return (err == noErr);
}

  int
bx_carbon_gui_c::set_clipboard_text(char *text_snapshot, Bit32u len)
{
  ScrapRef theScrap;
  
  // Clear out the existing clipboard
  ClearCurrentScrap ();
  
  GetCurrentScrap( &theScrap );
  PutScrapFlavor ( theScrap, kScrapFlavorTypeText, kScrapFlavorMaskNone, len, text_snapshot);
  return 1;
}


// ::PALETTE_CHANGE()
//
// Allocate a color in the native GUI, for this color, and put
// it in the colormap location 'index'.
// returns: 0=no screen update needed (color map change has direct effect)
//          1=screen updated needed (redraw using current colormap)

bx_bool bx_carbon_gui_c::palette_change(unsigned index, unsigned red, unsigned green, unsigned blue)
{
  PaletteHandle thePal, oldpal;
  GDHandle  saveDevice;
  CGrafPtr  savePort;
  
  if (bx_options.Oprivate_colormap->get ())
  {
    GetGWorld(&savePort, &saveDevice);

    SetGWorld(gOffWorld, NULL);
  }

  (**gCTable).ctTable[index].value = index;
  (**gCTable).ctTable[index].rgb.red = (red << 8);
  (**gCTable).ctTable[index].rgb.green = (green << 8);
  (**gCTable).ctTable[index].rgb.blue = (blue << 8);

  SetEntries(index, index, (**gCTable).ctTable);
  
  CTabChanged(gCTable);
  
  if (bx_options.Oprivate_colormap->get ())
  {
    SetGWorld(savePort, saveDevice);
  
    thePal = NewPalette(index, gCTable, pmTolerant, 0x5000);
    oldpal = GetPalette(win);
  
    SetPalette(win, thePal, false);
    SetPalette(fullwin, thePal, false);
    SetPalette(hidden, thePal, false);
  }

  return(1);
}


// ::GRAPHICS_TILE_UPDATE()
//
// Called to request that a tile of graphics be drawn to the
// screen, since info in this region has changed.
//
// tile: array of 8bit values representing a block of pixels with
//       dimension equal to the 'tilewidth' & 'tileheight' parameters to
//       ::specific_init().  Each value specifies an index into the
//       array of colors you allocated for ::palette_change()
// x0: x origin of tile
// y0: y origin of tile
//
// note: origin of tile and of window based on (0,0) being in the upper
//       left of the window.

void bx_carbon_gui_c::graphics_tile_update(Bit8u *tile, unsigned x0, unsigned y0)
{
  Rect      destRect;
/*  GDHandle  saveDevice;
  CGrafPtr  savePort;
  
  GetGWorld(&savePort, &saveDevice);

  SetGWorld(gOffWorld, NULL); */
        
  screen_state = GRAPHIC_MODE;

  // SetPort - Otherwise an update happens to the headerbar and ooomph, we're drawing weirdly on the screen
  SetPortWindowPort(win);
  destRect = srcTileRect;
  OffsetRect(&destRect, x0, y0);
  
  (**gTile).baseAddr = (Ptr)tile; 
  
  CopyBits( & (** ((BitMapHandle)gTile) ), WINBITMAP(win),
            &srcTileRect, &destRect, srcCopy, NULL);

  windowUpdatesPending = true;
//  SetGWorld(savePort, saveDevice);
}



// ::DIMENSION_UPDATE()
//
// Called when the VGA mode changes it's X,Y dimensions.
// Resize the window to this size, but you need to add on
// the height of the headerbar to the Y value.
//
// x: new VGA x size
// y: new VGA y size (add headerbar_y parameter from ::specific_init().

void bx_carbon_gui_c::dimension_update(unsigned x, unsigned y, unsigned fheight)
{
  if (fheight > 0) {
    if (fheight != 16) {
      y = y * 16 / fheight;
    }
  }
  if (x != width || y != height)
  {
#if 1
    SizeWindow(win, x, y, false);
#endif

#if 0
    // Animates the resizing, cute, but gratuitous
    Rect box, frame;
    GetWindowBounds(win, kWindowStructureRgn, &frame);
    GetWindowPortBounds(win, &box);
    frame.right = frame.right - box.right + x;
    frame.bottom = frame.bottom - box.bottom + y;

    TransitionWindow(win, kWindowSlideTransitionEffect, kWindowResizeTransitionAction, &frame);
#endif
    SizeWindow(fullwin, x, y, false);
    SizeWindow(hidden, x, y, false);
    width = x;
    height = y;
  }
        
  windowUpdatesPending = true;
}


// ::CREATE_BITMAP()
//
// Create a monochrome bitmap of size 'xdim' by 'ydim', which will
// be drawn in the headerbar.  Return an integer ID to the bitmap,
// with which the bitmap can be referenced later.
//
// bmap: packed 8 pixels-per-byte bitmap.  The pixel order is:
//       bit0 is the left most pixel, bit7 is the right most pixel.
// xdim: x dimension of bitmap
// ydim: y dimension of bitmap

// rewritten by tim senecal to use the cicn (color icon) resources instead

// We need to have a cicn resource for each and every call to create_bitmap
// If this fails, it is probably because more icons were added to bochs and
// we need to create more cicns in bochs.r to match with create_bitmap calls in
// gui.cc
unsigned bx_carbon_gui_c::create_bitmap(const unsigned char *bmap, unsigned xdim, unsigned ydim)
{
  unsigned i;
  unsigned char *data;
  long row_bytes, bytecount;
  
  bx_cicn[numPixMaps] = GetCIcon(numPixMaps+128);
  BX_ASSERT(bx_cicn[numPixMaps]);
  
  numPixMaps++;

  return(numPixMaps-1);
}


// ::HEADERBAR_BITMAP()
//
// Called to install a bitmap in the bochs headerbar (toolbar).
//
// bmap_id: will correspond to an ID returned from
//     ::create_bitmap().  'alignment' is either BX_GRAVITY_LEFT
//     or BX_GRAVITY_RIGHT, meaning install the bitmap in the next
//     available leftmost or rightmost space.
// f: a 'C' function pointer to callback when the mouse is clicked in
//     the boundaries of this bitmap.

unsigned bx_carbon_gui_c::headerbar_bitmap(unsigned bmap_id, unsigned alignment, void (*f)(void))
{
  unsigned hb_index;
  Rect destRect, r;
  GetWindowPortBounds(toolwin, &r);
  int xorigin, yorigin = TOOL_MARGIN_SPACE;
  ControlButtonContentInfo info;
  
  toolPixMaps++;
  hb_index = toolPixMaps-1;
//  bx_tool_pixmap[hb_index].pm = bx_pixmap[bmap_id];
//  bx_tool_pixmap[hb_index].cicn = bx_cicn[bmap_id];
  bx_tool_pixmap[hb_index].alignment = alignment;
  bx_tool_pixmap[hb_index].f = f;

  if (alignment == BX_GRAVITY_LEFT)
  {
    bx_tool_pixmap[hb_index].xorigin = bx_bitmap_left_xorigin;
    bx_tool_pixmap[hb_index].yorigin = TOOL_MARGIN_SPACE;
//    bx_bitmap_left_xorigin += (**bx_pixmap[bmap_id]).bounds.right;
    bx_bitmap_left_xorigin += 32 + TOOL_SPACING;
    xorigin = bx_tool_pixmap[hb_index].xorigin;
  }
  else
  {
//    bx_bitmap_right_xorigin += (**bx_pixmap[bmap_id]).bounds.right;
    bx_bitmap_right_xorigin += 32;
    bx_tool_pixmap[hb_index].xorigin = bx_bitmap_right_xorigin;
    bx_tool_pixmap[hb_index].yorigin = TOOL_MARGIN_SPACE;
    xorigin = r.right - bx_tool_pixmap[hb_index].xorigin;
    bx_bitmap_right_xorigin += TOOL_SPACING;
  }

  SetRect(&destRect, xorigin, yorigin, xorigin+32, yorigin+32);
  
  info.contentType = kControlContentCIconHandle;
  info.u.cIconHandle = bx_cicn[bmap_id];
  
  CreateIconControl( toolwin, &destRect, &info, false, &(bx_tool_pixmap[hb_index].control) );
  SetControlCommandID(bx_tool_pixmap[hb_index].control, hb_index);
  return(hb_index);
}


// ::SHOW_HEADERBAR()
//
// Show (redraw) the current headerbar, which is composed of
// currently installed bitmaps.

void bx_carbon_gui_c::show_headerbar(void)
{
  UpdateTools();
  DrawControls(toolwin);
}


// ::REPLACE_BITMAP()
//
// Replace the bitmap installed in the headerbar ID slot 'hbar_id',
// with the one specified by 'bmap_id'.  'bmap_id' will have
// been generated by ::create_bitmap().  The old and new bitmap
// must be of the same size.  This allows the bitmap the user
// sees to change, when some action occurs.  For example when
// the user presses on the floppy icon, it then displays
// the ejected status.
//
// hbar_id: headerbar slot ID
// bmap_id: bitmap ID

void bx_carbon_gui_c::replace_bitmap(unsigned hbar_id, unsigned bmap_id)
{
//  bx_tool_pixmap[hbar_id].pm = bx_pixmap[bmap_id];
//  bx_tool_pixmap[hbar_id].cicn = bx_cicn[bmap_id];
  ControlButtonContentInfo info;

  info.contentType = kControlContentCIconHandle;
  info.u.cIconHandle = bx_cicn[bmap_id];

  SetControlData(bx_tool_pixmap[hbar_id].control,
    kControlEntireControl, kControlIconContentTag, sizeof(ControlButtonContentInfo), &info);
  show_headerbar();
}

 
// ::EXIT()
//
// Called before bochs terminates, to allow for a graceful
// exit from the native GUI mechanism.

void bx_carbon_gui_c::exit(void)
{
  if (!menubarVisible)
    ShowMenubar(); // Make the menubar visible again
  InitCursor();
  
  // Make the clipboard all happy before we go
  CallInScrapPromises();
}

#if 0
void bx_carbon_gui_c::snapshot_handler(void)
{
  PicHandle ScreenShot;
  long val;
  
  SetPortWindowPort(win);
  
  ScreenShot = OpenPicture(&win->portRect);
  
  CopyBits(&win->portBits, &win->portBits, &win->portRect, &win->portRect, srcCopy, NULL);
  
  ClosePicture();
  
  val = ZeroScrap();
  
  HLock((Handle)ScreenShot);
  PutScrap(GetHandleSize((Handle)ScreenShot), 'PICT', *ScreenShot);
  HUnlock((Handle)ScreenShot);
  
  KillPicture(ScreenShot);
}
#endif

// HidePointer()
//
// Hides the Mac mouse pointer

void HidePointer()
{
  HiliteMenu(0);
  HideCursor();
  SetPortWindowPort(win);
  SetPt(&scrCenter, 300, 240);
  LocalToGlobal(&scrCenter);
  ResetPointer();
  GetMouse(&prevPt);
  cursorVisible = false;
  CheckMenuItem(GetMenuHandle(mBochs), iCursor, false); 
}

// ShowPointer()
//
// Shows the Mac mouse pointer

void ShowPointer()
{
  InitCursor();
  cursorVisible = true;
  CheckMenuItem(GetMenuHandle(mBochs), iCursor, true);
  //CheckItem(GetMenuHandle(mBochs), iCursor, true);
}

// UpdateTools()
//
// Check the state of the emulation and use it to tell which tools are available or not.
void UpdateTools()
{
  ScrapRef         theScrap;
  ScrapFlavorFlags theScrapFlags;
  
  GetCurrentScrap( &theScrap );

  // If keyboard mapping is on AND there is text on the clipboard enable pasting
  if(bx_options.keyboard.OuseMapping->get() &&
    (GetScrapFlavorFlags( theScrap, kScrapFlavorTypeText, &theScrapFlags) == noErr))
  {
    EnableMenuItem(GetMenuRef(mEdit), iPaste);
    EnableControl(bx_tool_pixmap[PASTE_TOOL_BUTTON].control);
  }
  else
  {
    DisableMenuItem(GetMenuRef(mEdit), iPaste);
    DisableControl(bx_tool_pixmap[PASTE_TOOL_BUTTON].control);
  }

  // Currently copy and snapshot aren't available if we aren't in text mode
  if (screen_state == GRAPHIC_MODE) {
    DisableMenuItem(GetMenuRef(mEdit), iCopy);
    DisableMenuItem(GetMenuRef(mBochs), iSnapshot);
    DisableControl(bx_tool_pixmap[COPY_TOOL_BUTTON].control);
    DisableControl(bx_tool_pixmap[SNAPSHOT_TOOL_BUTTON].control);
  } else
  {
    EnableMenuItem(GetMenuRef(mEdit), iCopy);
    EnableMenuItem(GetMenuRef(mBochs), iSnapshot);
    EnableControl(bx_tool_pixmap[COPY_TOOL_BUTTON].control);
    EnableControl(bx_tool_pixmap[SNAPSHOT_TOOL_BUTTON].control);
  }
  
  // User control active if keys defined
  char *user_shortcut;
  user_shortcut = bx_options.Ouser_shortcut->getptr();
  if (user_shortcut[0] && (strcmp(user_shortcut, "none"))) {
    EnableControl(bx_tool_pixmap[USER_TOOL_BUTTON].control);
  }
  else
  {
    DisableControl(bx_tool_pixmap[USER_TOOL_BUTTON].control);
  }
  
  // Config panel only available if user has a terminal or equivalent
  if(isatty(STDIN_FILENO))
  {
    EnableControl(bx_tool_pixmap[CONFIGURE_TOOL_BUTTON].control);
  }
  else
  {
    DisableControl(bx_tool_pixmap[CONFIGURE_TOOL_BUTTON].control);
  }
}

// HideTools()
//
// Hides the Bochs toolbar

void HideTools()
{
  HideWindow(toolwin);
#if 0
  if (menubarVisible)
  {
    MoveWindow(win, gLeft, gMinTop, false);
  }
  else
  {
    MoveWindow(hidden, gLeft, gMinTop, false);
  }
#endif
  CheckMenuItem(GetMenuHandle(mBochs), iTool, false);
  HiliteWindow(win, true);
}

// ShowTools()
//
// Shows the Bochs toolbar

void ShowTools()
{
#if 0
  if (menubarVisible)
  {
    MoveWindow(win, gLeft, gMaxTop, false);
  }
  else
  {
    MoveWindow(hidden, gLeft, gMaxTop, false);
  }
#endif
  ShowWindow(toolwin);
//  theGui->show_headerbar();
  CheckMenuItem(GetMenuHandle(mBochs), iTool, true);
  HiliteWindow(win, true);
}

// HideMenubar()
//
// Hides the menubar (obviously)

void HideMenubar()
{
  HideMenuBar();
  
  HideWindow(win);
  ShowWindow(backdrop);
  hidden = win;
  win = fullwin;
  ShowWindow(win);

  SelectWindow(win);
  menubarVisible = false;
  CheckMenuItem(GetMenuHandle(mBochs), iMenuBar, false);
}

// ShowMenubar()
//
// Makes the menubar visible again so other programs will display correctly.

void ShowMenubar()
{
  HideWindow(backdrop);
  win = hidden;
  hidden = fullwin;
  HideWindow(hidden);
  ShowWindow(win);
  HiliteWindow(win, true);
  
  ShowMenuBar();
  
  menubarVisible = true;
  CheckMenuItem(GetMenuHandle(mBochs), iMenuBar, true);
}

void HideConsole()
{
//  HideWindow(SouixWin);
  CheckMenuItem(GetMenuHandle(mBochs), iConsole, false);
}

void ShowConsole()
{
//  ShowWindow(SouixWin);
//  SelectWindow(SouixWin);
  CheckMenuItem(GetMenuHandle(mBochs), iConsole, true);
}

// CreateKeyMap()
//
// Create a KCHR data structure to map Mac virtual key codes to Bochs key codes

void CreateKeyMap(void)
{
  const unsigned char KCHRHeader [258] = {
    0,
    1
  };

  const unsigned char KCHRTable [130] = {
    0,
    1,
    BX_KEY_A,
    BX_KEY_S,
    BX_KEY_D,
    BX_KEY_F,
    BX_KEY_H,
    BX_KEY_G,
    BX_KEY_Z,
    BX_KEY_X,
    BX_KEY_C,
    BX_KEY_V,
    0,
    BX_KEY_B,
    BX_KEY_Q,
    BX_KEY_W,
    BX_KEY_E,
    BX_KEY_R,
    BX_KEY_Y,
    BX_KEY_T,
    BX_KEY_1,
    BX_KEY_2,
    BX_KEY_3,
    BX_KEY_4,
    BX_KEY_6,
    BX_KEY_5,
    BX_KEY_EQUALS,
    BX_KEY_9,
    BX_KEY_7,
    BX_KEY_MINUS,
    BX_KEY_8,
    BX_KEY_0,
    BX_KEY_RIGHT_BRACKET,
    BX_KEY_O,
    BX_KEY_U,
    BX_KEY_LEFT_BRACKET,
    BX_KEY_I,
    BX_KEY_P,
    BX_KEY_ENTER,
    BX_KEY_L,
    BX_KEY_J,
    BX_KEY_SINGLE_QUOTE,
    BX_KEY_K,
    BX_KEY_SEMICOLON,
    BX_KEY_BACKSLASH,
    BX_KEY_COMMA,
    BX_KEY_SLASH,
    BX_KEY_N,
    BX_KEY_M,
    BX_KEY_PERIOD,
    BX_KEY_TAB,
    BX_KEY_SPACE,
    BX_KEY_GRAVE,
    BX_KEY_BACKSPACE,
    0,
    BX_KEY_ESC,
    0, // 0x36
    0, // 0x37
    0, // 0x38
    0, // 0x39
    0, // 0x3A
    0, // 0x3B
    0, // 0x3C
    0, // 0x3D
    0, // 0x3E
    0, // 0x3F
    0, // 0x40
    BX_KEY_PERIOD, // KP_PERIOD
    0, // 0x42
    BX_KEY_KP_MULTIPLY,
    0, // 0x44
    BX_KEY_KP_ADD,
    0, // 0x46
    BX_KEY_KP_DELETE,
    0, // 0x48
    0, // 0x49
    0, // 0x4A
    BX_KEY_KP_DIVIDE,
    BX_KEY_KP_ENTER,
    0, // 0x4D
    BX_KEY_KP_SUBTRACT,
    0, // 0x4F
    0, // 0x50
    0, // 0x51 (kp equals)
    0, // 0x52 (kp 0)
    0, // 0x53 (kp 1)
    BX_KEY_KP_DOWN, // 0x54 (kp 2)
    0, // 0x55 (kp 3)
    BX_KEY_KP_LEFT, // 0x56 (kp 4)
    BX_KEY_KP_5,
    BX_KEY_KP_RIGHT, // 0x58 (kp 6)
    0, // 0x59 (kp 7)
    0, // 0x5A
    BX_KEY_KP_UP, // 0x5B (kp 8)
    0, // 0x5C (kp 9)
    0, // 0x5D
    0, // 0x5E
    0, // 0x5F
    BX_KEY_F5,
    BX_KEY_F6,
    BX_KEY_F7,
    BX_KEY_F3,
    BX_KEY_F8,
    BX_KEY_F9,
    0, // 0x66
    BX_KEY_F11,
    0, // 0x68
    0, // 0x69 (print screen)
    0, // 0x6A
    0, // 0x6B (scroll lock)
    0, // 0x6C
    BX_KEY_F10,
    0, // 0x6E
    BX_KEY_F12,
    0, // 0x70
    0, // 0x71 (pause)
    BX_KEY_INSERT,
    BX_KEY_HOME,
    BX_KEY_PAGE_UP,
    BX_KEY_DELETE,
    BX_KEY_F4,
    BX_KEY_END,
    BX_KEY_F2,
    BX_KEY_PAGE_DOWN,
    BX_KEY_F1,
    BX_KEY_LEFT,
    BX_KEY_RIGHT,
    BX_KEY_DOWN,
    BX_KEY_UP
  };

  KCHR = NewPtrClear(390);
  if (KCHR == NULL)
    BX_PANIC(("mac: can't allocate memory for key map"));
  
  BlockMove(KCHRHeader, KCHR, sizeof(KCHRHeader));
  BlockMove(KCHRTable, Ptr(KCHR + sizeof(KCHRHeader)), sizeof(KCHRTable));
}

// CreateVGAFont()
//
// Create an array of PixMaps for the PC screen font

void CreateVGAFont(void)
{
  int i, x;
  unsigned char *fontData, curPixel;
  long row_bytes, bytecount;
  
  for (i=0; i<256; i++)
  {
    vgafont[i] = CreateBitMap(FONT_WIDTH, FONT_HEIGHT);
    row_bytes = (*(vgafont[i])).rowBytes;
    bytecount = row_bytes * FONT_HEIGHT;
    fontData = (unsigned char *)NewPtrClear(bytecount);

    for (x=0; x<16; x++)
    {
      //curPixel = ~(bx_vgafont[i].data[x]);
      curPixel = (bx_vgafont[i].data[x]);
      fontData[x*row_bytes] = reverse_bitorder(curPixel);
    }
    vgafont[i]->baseAddr = Ptr(fontData);
  }
}

// CreateBitMap()
// Allocate a new bitmap and fill in the fields with appropriate
// values.

BitMap *CreateBitMap(unsigned width,  unsigned height)
{
  BitMap  *bm;
  long    row_bytes;
  
  row_bytes = (( width + 31) >> 5) << 2;
  bm = (BitMap *)calloc(1, sizeof(BitMap));
  if (bm == NULL)
    BX_PANIC(("mac: can't allocate memory for pixmap"));
  SetRect(&bm->bounds, 0, 0, width, height);
  bm->rowBytes = row_bytes;
  // Quickdraw allocates a new color table by default, but we want to
  // use one we created earlier.
    
  return bm;
}

// CreatePixMap()
// Allocate a new pixmap handle and fill in the fields with appropriate
// values.

PixMapHandle CreatePixMap(unsigned left, unsigned top, unsigned width,
  unsigned height, unsigned depth, CTabHandle clut)
{
  PixMapHandle  pm;
  long          row_bytes;
  
  row_bytes = (((long) depth * ((long) width) + 31) >> 5) << 2;
  pm = NewPixMap();
  if (pm == NULL)
    BX_PANIC(("mac: can't allocate memory for pixmap"));
  (**pm).bounds.left = left;
  (**pm).bounds.top = top;
  (**pm).bounds.right = left+width;
  (**pm).bounds.bottom = top+height;
  (**pm).pixelSize = depth;
  (**pm).rowBytes = row_bytes | 0x8000;
  
  DisposeCTable((**pm).pmTable);
  (**pm).pmTable = clut;
  // Quickdraw allocates a new color table by default, but we want to
  // use one we created earlier.
    
  return pm;
}

unsigned char reverse_bitorder(unsigned char b)
{
  unsigned char ret=0;
  
  for (unsigned i=0; i<8; i++)
  {
    ret |= (b & 0x01) << (7-i);
    b >>= 1;
  }
  
  return(ret);
}

  void
bx_carbon_gui_c::mouse_enabled_changed_specific (bx_bool val)
{
}

// we need to handle "ask" events so that PANICs are properly reported
static BxEvent * CarbonSiminterfaceCallback (void *theClass, BxEvent *event)
{
  event->retcode = 0;  // default return code
  
  if( event->type == BX_ASYNC_EVT_LOG_MSG || event->type == BX_SYNC_EVT_LOG_ASK)
  {
  DialogRef                     alertDialog;
  CFStringRef                   title;
  CFStringRef                   exposition;
  DialogItemIndex               index;
  AlertStdCFStringAlertParamRec alertParam = {0};

  if( event->u.logmsg.prefix != NULL )
  {
    title      = CFStringCreateWithCString(NULL, event->u.logmsg.prefix, kCFStringEncodingASCII);
    exposition = CFStringCreateWithCString(NULL, event->u.logmsg.msg, kCFStringEncodingASCII);
  }
  else
  {
    title      = CFStringCreateWithCString(NULL, event->u.logmsg.msg, kCFStringEncodingASCII);
    exposition = NULL;
  }
  
  alertParam.version       = kStdCFStringAlertVersionOne;
  alertParam.defaultText   = CFSTR("Continue");
  alertParam.cancelText    = CFSTR("Quit");
  alertParam.position      = kWindowDefaultPosition;
  alertParam.defaultButton = kAlertStdAlertOKButton;
  alertParam.cancelButton  = kAlertStdAlertCancelButton;

  CreateStandardAlert(
    kAlertCautionAlert,
    title,
    exposition,       /* can be NULL */
    &alertParam,             /* can be NULL */
    &alertDialog);
  
  RunStandardAlert(
   alertDialog,
   NULL,       /* can be NULL */
   &index);

  CFRelease( title );

  if( exposition != NULL )
  {
    CFRelease( exposition );
  }
  
  // continue
  if( index == kAlertStdAlertOKButton )
  {
    event->retcode = 0;
  }
  // quit
  else if( index == kAlertStdAlertCancelButton )
  {
    event->retcode = 2;
  }
  }

#if 0
  // Track down the message that exiting leaves...
  switch(event->type)
  {
    case BX_SYNC_EVT_TICK:
    case BX_SYNC_EVT_LOG_ASK:
      break;
    default:
      BX_INFO(("Callback tracing: Evt: %d (%s)", event->type,
        ((BX_EVT_IS_ASYNC(event->type))?"async":"sync")));
      if(event->type == BX_ASYNC_EVT_LOG_MSG || event->type == BX_SYNC_EVT_LOG_ASK)
      {
        if( event->u.logmsg.prefix != NULL )
        {
        BX_INFO(("Callback log:     Prefix: %s", event->u.logmsg.prefix));
        }
        BX_INFO(("Callback log:     Message: %s", event->u.logmsg.msg));
      }
  }
#endif
  return event;
}
#endif /* if BX_WITH_CARBON */
