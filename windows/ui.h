#define ID_STATIC -1
#define ID_SETTINGS 107
#define ID_CHOOSEDRIVE 106
#define ID_DRIVE 105
#define ID_ADD_CONTEXTMENU 114
#define ID_REMOVE_CONTEXTMENU 115
#define ID_TRAYICON 110
#define ID_MENU 107
#define ID_MENU_MOUNT 108
#define ID_MENU_UNMOUNT 109
#define ID_MENU_SETTINGS 112
#define ID_MENU_EXIT 111
#define ID_MENU_FIRST 200

struct mount
{
	BOOL mounted;
	LPTSTR path;
	TCHAR  letter;
	HANDLE thread;
	UINT   menu;
};

void Mount(void *);

extern HMENU menu;
