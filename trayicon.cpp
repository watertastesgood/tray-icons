#include "trayicon.h"
#include <iostream>

namespace {
	constexpr const char* CLASS_NAME = "TRAY";
	constexpr int MIN_ID = 1000;
}

HWND SystemTray::s_hwnd = NULL;
HMENU SystemTray::s_hMenu = NULL;

SystemTray::~SystemTray() {
	Shell_NotifyIcon(NIM_DELETE, &m_nid);
	if (m_nid.hIcon != 0)
		DestroyIcon(m_nid.hIcon);
	if (s_hMenu != 0)
		DestroyMenu(s_hMenu);
	PostQuitMessage(0);
	UnregisterClass(CLASS_NAME, GetModuleHandle(NULL));
}

void SystemTray::Update() {
	HICON icon;
	HMENU prevmenu = s_hMenu;
	UINT id = MIN_ID;

	if (GetChildren().empty())
		return;

	s_hMenu = SystemTray::Menu(GetChildren(), &id);
	SendMessage(s_hwnd, WM_INITMENUPOPUP, (WPARAM)s_hMenu, 0);
	ExtractIconEx((LPCSTR)this->icon, 0, NULL, &icon, 1);
	if (m_nid.hIcon)
		DestroyIcon(m_nid.hIcon);
	m_nid.hIcon = icon;
	Shell_NotifyIcon(NIM_MODIFY, &m_nid);

	if (prevmenu != NULL)
		DestroyMenu(prevmenu);
}

void SystemTray::Init() {
	memset(&m_wc, 0, sizeof(m_wc));
	m_wc.cbSize = sizeof(WNDCLASSEX);
	m_wc.lpfnWndProc = WndProc;
	m_wc.hInstance = GetModuleHandle(NULL);
	m_wc.lpszClassName = CLASS_NAME;
	if (!RegisterClassEx(&m_wc))
		return;

	s_hwnd = CreateWindowEx(0, CLASS_NAME, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	if (s_hwnd == NULL)
		return;
	UpdateWindow(s_hwnd);

	memset(&m_nid, 0, sizeof(m_nid));
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.hWnd = s_hwnd;
	m_nid.uID = 0;
	m_nid.uFlags = NIF_ICON | NIF_MESSAGE;
	m_nid.uCallbackMessage = WM_USER + 1;
	Shell_NotifyIcon(NIM_ADD, &m_nid);

	for (auto& child : m_children)
		child.SetParent(this);

	Update();
}

int SystemTray::Loop(int blocking) {
	MSG msg;
	if (blocking)
		GetMessage(&msg, NULL, 0, 0);
	else
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

	if (msg.message == WM_QUIT)
		return -1;

	TranslateMessage(&msg);
	DispatchMessage(&msg);
	return 0;
}

HMENU SystemTray::Menu(const std::vector<TrayMenu>& menuList, UINT* id) {
	HMENU hMenu = CreatePopupMenu();
	for (auto& menu : menuList) {
		if (strcmp(menu.GetLabel(), "-") == 0) {
			InsertMenu(hMenu, *id, MF_SEPARATOR, TRUE, (LPCSTR)"");
		} else {
			MENUITEMINFO item;
			memset(&item, 0, sizeof(item));
			item.cbSize = sizeof(MENUITEMINFO);
			item.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
			item.fType = 0;
			item.fState = 0;
			if (!menu.GetChildren().empty()) {
				item.fMask = item.fMask | MIIM_SUBMENU;
				item.hSubMenu = Menu(menu.GetChildren(), id);
			}

			if (menu.IsDisabled())
				item.fState |= MFS_DISABLED;
			if (menu.IsChecked())
				item.fState |= MFS_CHECKED;

			item.wID = *id;
			item.dwTypeData = (LPSTR)menu.GetLabel();
			item.dwItemData = (ULONG_PTR)&menu;

			InsertMenuItem(hMenu, *id, TRUE, &item);
		}
		(*id)++;
	}
	return hMenu;
}

LRESULT CALLBACK SystemTray::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_USER + 1:
		if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP) {
			POINT p;
			GetCursorPos(&p);
			SetForegroundWindow(hWnd);
			WORD cmd = TrackPopupMenu(s_hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON |
				TPM_RETURNCMD | TPM_NONOTIFY,
				p.x, p.y, 0, s_hwnd, NULL);
			SendMessage(s_hwnd, WM_COMMAND, cmd, 0);
			return 0;
		}
		break;
	case WM_COMMAND:
		if (wParam >= MIN_ID) {
			MENUITEMINFO item;
			item.cbSize = sizeof(MENUITEMINFO);
			item.fMask = MIIM_ID | MIIM_DATA;
			if (GetMenuItemInfo(s_hMenu, (UINT)wParam, FALSE, &item)) {
				TrayMenu* menu = (TrayMenu*)item.dwItemData;
				if (menu != NULL && menu->GetCallback()) {
					menu->GetCallback()(menu);
					if (menu->GetParent() != nullptr) {
						menu->GetParent()->Update();
					}
				}
			}
			return 0;
		}
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

TrayMenu* MenuParent::AddMenuItem(const char* text, bool disabled, bool checked, TrayCallback callback, void* context) {
	m_children.emplace_back(TrayMenu(text, disabled, checked, callback, context));
	return &(m_children.back());
}

void TrayMenu::SetParent(SystemTray* parent) {
	this->m_parent = parent;

	for (auto& child : m_children)
		child.SetParent(parent);
}