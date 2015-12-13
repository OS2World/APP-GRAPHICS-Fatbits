/***************************************************************************\
* Module Name: fatbits.c
\***************************************************************************/

#define INCL_WINFRAMEMGR
#define INCL_WININPUT
#define INCL_WINMENUS
#define INCL_WINPOINTERS
#define INCL_WINSHELLDATA
#define INCL_WINSYS
#define INCL_WINTRACKRECT
#define INCL_WINWINDOWMGR

#include <os2.h>

typedef struct {
	BOOL icon,slow;
	SHORT cx,cy;
	SHORT magnify;
	SHORT x,y;
} PROFILE;

BOOL PASCAL FAR InstallFatHook(HAB, HWND);
VOID PASCAL FAR ReleaseFatHook(void);
void cdecl main(void);
MRESULT EXPENTRY _export fatclientproc(HWND, USHORT, MPARAM, MPARAM);
MRESULT EXPENTRY _export fatframeproc(HWND, USHORT, MPARAM, MPARAM);
void cdecl _setenvp(void);

static HAB hab;
static HWND framehndl;
static HWND clienthndl;
static HWND hwndtitlebar;
static HWND hwndsysmenu;
static PFNWP pfnwp;
static SHORT wsizex;
static SHORT wsizey;
static SHORT tempx;
static SHORT tempy;
static SHORT borderx;
static SHORT bordery;
static USHORT move = 0;
static PROFILE profile = { FALSE, TRUE, 7, 7, 7, 16, 16 };
static char fatbits[] = "Fatbits";
static char pos[] = "Position";

void cdecl main()

{
	HMQ hmsgq;
	QMSG qmsg;
	HWND sub;
	HSWITCH hsw;
	USHORT i;
	MENUITEM submenu;
	ULONG fcfval = FCF_SIZEBORDER|FCF_ICON|FCF_TITLEBAR|FCF_NOBYTEALIGN|FCF_SYSMENU;
	static char str[] = "  x";
	static MENUITEM mi = { MIT_END, MIS_SEPARATOR, 0, 256, NULL, 0 };
	static SWCNTRL swctl = { NULL, NULL, 0, 0, 0, SWL_VISIBLE,
		SWL_NOTJUMPABLE, "Fatbits Magnifier", 0 };

	hab = WinInitialize(0);
	hmsgq = WinCreateMsgQueue(hab,DEFAULT_QUEUE_SIZE);

	WinRegisterClass(hab,fatbits,fatclientproc,CS_SIZEREDRAW,0);
	framehndl = WinCreateStdWindow(HWND_DESKTOP,0L,&fcfval,fatbits,
		fatbits,0L,0,1,&clienthndl);
	if (InstallFatHook(hab,clienthndl)) {

		pfnwp = WinSubclassWindow(framehndl,fatframeproc);
		i = sizeof(PROFILE);
		WinQueryProfileData(hab,fatbits,pos,&profile,&i);
		tempx = profile.magnify*profile.x;
		tempy = profile.magnify*profile.y;
		wsizex = tempx-profile.slow;
		wsizey = tempy-profile.slow;
		borderx = (SHORT)WinQuerySysValue(HWND_DESKTOP,SV_CXSIZEBORDER)<<1;
		bordery = (SHORT)WinQuerySysValue(HWND_DESKTOP,SV_CYSIZEBORDER)<<1;
		WinSetWindowPos(framehndl,NULL,profile.cx,profile.cy,
			wsizex+borderx,wsizey+bordery,SWP_MOVE|SWP_SIZE);
		hwndtitlebar = WinWindowFromID(framehndl,FID_TITLEBAR);
		hwndsysmenu = WinWindowFromID(framehndl,FID_SYSMENU);
		i = SHORT1FROMMR(WinSendMsg(hwndsysmenu,MM_ITEMIDFROMPOSITION,0,0));
		WinSendMsg(hwndsysmenu,MM_QUERYITEM,MPFROM2SHORT(i,FALSE),MPFROMP(&submenu));
		WinSendMsg(submenu.hwndSubMenu,MM_DELETEITEM,MPFROM2SHORT(SC_MAXIMIZE,FALSE),0);
		WinSendMsg(submenu.hwndSubMenu,MM_DELETEITEM,MPFROM2SHORT(SC_SIZE,FALSE),0);
		sub = WinCreateWindow(HWND_OBJECT,WC_MENU,"",WS_VISIBLE,0,0,0,0,
			submenu.hwndSubMenu,HWND_TOP,0,NULL,NULL);
		WinSendMsg(submenu.hwndSubMenu,MM_INSERTITEM,MPFROMP(&mi),MPFROMP(""));
		mi.afStyle = MIS_SYSCOMMAND|MIS_TEXT|MIS_SUBMENU;
		mi.id = 0;
		mi.hwndSubMenu = sub;
		WinSendMsg(submenu.hwndSubMenu,MM_INSERTITEM,MPFROMP(&mi),MPFROMP("Magnification"));
		mi.afStyle = MIS_SYSCOMMAND|MIS_TEXT;
		if (profile.slow) mi.afAttribute = MIA_CHECKED;
		mi.id = 1;
		mi.hwndSubMenu = NULL;
		WinSendMsg(submenu.hwndSubMenu,MM_INSERTITEM,MPFROMP(&mi),MPFROMP("Crosshatch"));
		mi.afStyle = MIS_SEPARATOR;
		mi.afAttribute = 0;
		mi.id = 257;
		WinSendMsg(submenu.hwndSubMenu,MM_INSERTITEM,MPFROMP(&mi),MPFROMP(""));
		mi.afStyle = MIS_SYSCOMMAND|MIS_TEXT;
		mi.id = 2;
		WinSendMsg(submenu.hwndSubMenu,MM_INSERTITEM,MPFROMP(&mi),MPFROMP("~About Fatbits..."));
		for (mi.id = 3; mi.id < 21; mi.id++) {
			if (mi.id > 9) str[0] = (char)('0'+mi.id/10);
			str[1] = (char)('0'+mi.id%10);
			if ((SHORT)mi.id == profile.magnify) mi.afAttribute = MIA_CHECKED;
			else mi.afAttribute = 0;
			WinSendMsg(sub,MM_INSERTITEM,MPFROMP(&mi),MPFROMP(str));
		}

		if (profile.icon) WinSetWindowPos(framehndl,NULL,0,0,0,0,SWP_MINIMIZE);
		else {
			WinSetParent(hwndtitlebar,HWND_OBJECT,FALSE);
			WinSetParent(hwndsysmenu,HWND_OBJECT,FALSE);
			WinSendMsg(framehndl,WM_UPDATEFRAME,
				MPFROMSHORT(FCF_TITLEBAR|FCF_SYSMENU),0);
		}
		WinSetWindowPos(framehndl,NULL,0,0,0,0,SWP_SHOW);

		swctl.hwnd = framehndl;
		WinQueryWindowProcess(framehndl,&swctl.idProcess,NULL);
		hsw = WinAddSwitchEntry(&swctl);

		while(WinGetMsg(hab,&qmsg,NULL,0,0)) WinDispatchMsg(hab,&qmsg);

		WinRemoveSwitchEntry(hsw);
		if (!profile.icon) {
			WinDestroyWindow(hwndtitlebar);
			WinDestroyWindow(hwndsysmenu);
		}
		ReleaseFatHook();
	}
	else WinMessageBox(HWND_DESKTOP,framehndl,
		"FATBITS is already running.",
		"Whoops!",0,MB_CANCEL|MB_ICONHAND);
	WinDestroyWindow(framehndl);
	WinDestroyMsgQueue(hmsgq);
	WinTerminate(hab);
}

MRESULT EXPENTRY _export fatclientproc(hwnd,message,mp1,mp2)
HWND hwnd;
USHORT message;
MPARAM mp1,mp2;

{
	int i;
	SWP swp;
	HPS hps,dhps;
	PROFILE temp;
	POINTL mouse,apoint[4];

	switch(message) {
	case WM_PAINT:
		if (move && !WinQueryCapture(HWND_DESKTOP,FALSE)) {
			temp = profile;
			WinQueryWindowPos(framehndl,&swp);
			wsizex = swp.cx-borderx;
			wsizey = swp.cy-bordery;
			profile.x = (wsizex+profile.slow)/profile.magnify;
			profile.y = (wsizey+profile.slow)/profile.magnify;
			profile.cx = swp.x;
			profile.cy = swp.y;
			if (temp.x != profile.x || temp.y != profile.y || temp.cx != profile.cx ||
				temp.cy != profile.cy) WinWriteProfileData(hab,fatbits,pos,
				&profile,sizeof(PROFILE));
			move = 0;
		}
	
		hps = WinBeginPaint(hwnd,NULL,NULL);

		if (profile.slow) GpiSetColor(hps,CLR_WHITE);

		WinQueryPointerPos(HWND_DESKTOP,&mouse);
		apoint[0].x = apoint[0].y = 0;
		apoint[1].x = wsizex;
		apoint[1].y = wsizey;
		apoint[2].x = mouse.x;
		apoint[2].y = mouse.y+1-profile.y;
		apoint[3].x = mouse.x+profile.x;
		apoint[3].y = mouse.y+1;
		dhps = WinGetScreenPS(HWND_DESKTOP);
		GpiBitBlt(hps,dhps,4L,apoint,ROP_SRCCOPY,BBO_IGNORE);

		if (profile.slow) {
			for (i = profile.magnify-1; i < wsizey-1; i += profile.magnify) {
				mouse.x = 0;
				mouse.y = i;
				GpiMove(hps,&mouse);
				mouse.x = wsizex;
				GpiLine(hps,&mouse);
			}
			for (i = profile.magnify-1; i < wsizex-1; i += profile.magnify) {
				mouse.x = i;
				mouse.y = 0;
				GpiMove(hps,&mouse);
				mouse.y = wsizey;
				GpiLine(hps,&mouse);
			}
		}

		WinReleasePS(dhps);
		WinEndPaint(hps);
		break;
	case WM_BUTTON1DOWN:
		return WinSendMsg(framehndl,WM_TRACKFRAME,(MPARAM)TF_MOVE,0);
	case WM_BUTTON1DBLCLK:
		if (!profile.icon) {
			profile.icon = TRUE;
			tempx = wsizex+profile.slow;
			tempy = wsizey+profile.slow;
			WinSetWindowPos(framehndl,NULL,0,0,0,0,SWP_HIDE);
			WinSetParent(hwndtitlebar,framehndl,FALSE);
			WinSetParent(hwndsysmenu,framehndl,FALSE);
			WinSendMsg(framehndl,WM_UPDATEFRAME,
				MPFROMSHORT(FCF_TITLEBAR|FCF_SYSMENU),0);
			WinSetWindowPos(framehndl,NULL,0,0,0,0,SWP_MINIMIZE|SWP_SHOW);
			WinWriteProfileData(hab,fatbits,pos,&profile,sizeof(PROFILE));
		}
		return MRFROMSHORT(TRUE);
	default:
		return WinDefWindowProc(hwnd,message,mp1,mp2);
	}
	return FALSE;
}

MRESULT EXPENTRY _export fatframeproc(hwnd,message,mp1,mp2)
HWND hwnd;
USHORT message;
MPARAM mp1,mp2;

{
	HWND sysmenu;
	SHORT i;
	MRESULT mr;

	switch (message) {
	case WM_QUERYTRACKINFO:
		if (!profile.icon && SHORT1FROMMP(mp1)&TF_MOVE) {
			move = SHORT1FROMMP(mp1)&TF_MOVE;
			if (move != TF_MOVE) {
				mr = (pfnwp)(hwnd,message,mp1,mp2);
				((PTRACKINFO)mp2)->cxGrid = ((PTRACKINFO)mp2)->cyGrid = profile.magnify;
				i = ((SHORT)(((PTRACKINFO)mp2)->ptlMinTrackSize.x)-borderx+
					profile.magnify)/profile.magnify*profile.magnify-profile.slow;
				((PTRACKINFO)mp2)->ptlMinTrackSize.x = i+borderx;
				((PTRACKINFO)mp2)->ptlMinTrackSize.y = i+bordery;
				((PTRACKINFO)mp2)->fs |= TF_GRID;
				return mr;
			}
		}
		break;
	case WM_SYSCOMMAND:
		switch (SHORT1FROMMP(mp1)) {
		case 1:
			profile.slow = !profile.slow;
			WinSendMsg(WinWindowFromID(hwnd,FID_SYSMENU),MM_SETITEMATTR,
				MPFROM2SHORT(1,TRUE),MPFROM2SHORT(MIA_CHECKED,
				profile.slow ? MIA_CHECKED : 0));
			wsizex = profile.magnify*profile.x-profile.slow;
			wsizey = profile.magnify*profile.y-profile.slow;
			WinSetWindowUShort(hwnd,QWS_CXRESTORE,wsizex+borderx);
			WinSetWindowUShort(hwnd,QWS_CYRESTORE,wsizey+bordery);
			WinWriteProfileData(hab,fatbits,pos,&profile,sizeof(PROFILE));
			return FALSE;
		case 2:
			WinMessageBox(HWND_DESKTOP,clienthndl,"A screen pixel magnifier.\n\n"
				"By John Ridges","FATBITS",0,MB_OK|MB_NOICON|MB_DEFBUTTON1|MB_APPLMODAL);
			return FALSE;
		case SC_RESTORE:
			WinSetWindowPos(hwnd,HWND_TOP,0,0,0,0,SWP_RESTORE|SWP_ZORDER);
			return FALSE;
		default:
			if (SHORT1FROMMP(mp1) >= 3 && SHORT1FROMMP(mp1) <= 20) {
				sysmenu = WinWindowFromID(hwnd,FID_SYSMENU);
				WinSendMsg(sysmenu,MM_SETITEMATTR,MPFROM2SHORT(profile.magnify,TRUE),
					MPFROM2SHORT(MIA_CHECKED,0));
				profile.magnify = SHORT1FROMMP(mp1);
				WinSendMsg(sysmenu,MM_SETITEMATTR,MPFROM2SHORT(profile.magnify,TRUE),
					MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
				profile.x = (tempx+profile.magnify-1)/profile.magnify;
				profile.y = (tempy+profile.magnify-1)/profile.magnify;
				wsizex = profile.magnify*profile.x-profile.slow;
				wsizey = profile.magnify*profile.y-profile.slow;
				WinSetWindowUShort(hwnd,QWS_CXRESTORE,wsizex+borderx);
				WinSetWindowUShort(hwnd,QWS_CYRESTORE,wsizey+bordery);
				WinWriteProfileData(hab,fatbits,pos,&profile,sizeof(PROFILE));
				return FALSE;
			}
		}
		break;
	case WM_ADJUSTWINDOWPOS:
		if (profile.icon && ((PSWP)mp1)->fs&SWP_RESTORE) {
			WinSetParent(hwndtitlebar,HWND_OBJECT,FALSE);
			WinSetParent(hwndsysmenu,HWND_OBJECT,FALSE);
			WinSendMsg(hwnd,WM_UPDATEFRAME,MPFROMSHORT(FCF_TITLEBAR|FCF_SYSMENU),0);
			profile.icon = FALSE;
			WinWriteProfileData(hab,fatbits,pos,&profile,sizeof(PROFILE));
		}
	}
	return (pfnwp)(hwnd,message,mp1,mp2);
}

void cdecl _setenvp()
{
}
