/***********************************************************************\
* fathook.c - Input queue monitor hook interface library
*
* This dynlink is needed because global input hooks must reside in a DLL.
\***********************************************************************/

#define INCL_WINHOOKS
#define INCL_WININPUT

#include <os2.h>

VOID PASCAL FAR _loadds QJournalRecordHook(HAB, PQMSG);
BOOL PASCAL FAR _loadds InstallFatHook(HAB, HWND);
VOID PASCAL FAR _loadds ReleaseFatHook(void);
VOID PASCAL near _loadds Initdll(HMODULE);

static HMODULE qhmod;              /* dynlink module handle            */
static HWND hwnd;                  /* window to invalidate             */
static HAB habt = NULL;            /* Anchor block                     */

VOID PASCAL FAR _loadds QJournalRecordHook(hab,lpqmsg)
HAB hab;
PQMSG lpqmsg;
{
	hab;
	if (lpqmsg->msg == WM_MOUSEMOVE) WinInvalidateRect(hwnd,NULL,TRUE);
}

BOOL PASCAL FAR _loadds InstallFatHook(hab,thehwnd)
HAB hab;
HWND thehwnd;
{
	if (habt) return FALSE;
	hwnd = thehwnd;
	habt = hab;
	WinSetHook(hab,NULL,HK_JOURNALRECORD,(PFN)QJournalRecordHook,qhmod);
	return TRUE;
}

VOID PASCAL FAR _loadds ReleaseFatHook()
{
	WinReleaseHook(habt,NULL,HK_JOURNALRECORD,(PFN)QJournalRecordHook,qhmod);
	habt = NULL;
}


VOID PASCAL near _loadds Initdll(hmod)
HMODULE hmod;
{
	qhmod = hmod;
}
