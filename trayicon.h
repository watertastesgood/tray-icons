#pragma once
#undef UNICODE

#include <functional>
#include <vector>
#include <windows.h>
#include <shellapi.h>
#include <string>

class TrayMenu;
class SystemTray;

using TrayCallback = std::function<void(TrayMenu*)>;

class MenuParent {
public:
	TrayMenu* AddMenuItem(const char* text, bool disabled, bool checked, TrayCallback callback, void* context = nullptr);

	const std::vector<TrayMenu>& GetChildren() const { return m_children; }
protected:
	std::vector<TrayMenu> m_children;
};

class TrayMenu : public MenuParent {
public:
	TrayMenu(const char* label, bool disabled, bool checked, TrayCallback callback, void* context)
		: m_label(label), m_disabled(disabled), m_checked(checked), m_callback(callback), m_context(context) {}

	const char* GetLabel() const { return m_label; }
	void SetLabel(const char* label) { m_label = label; }

	bool IsDisabled() const { return m_disabled; }
	void SetDisabled(bool disabled) { m_disabled = disabled; }
	bool IsChecked() const { return m_checked; }
	void SetChecked(bool checked) { m_checked = checked; }
	bool ToggleChecked() { m_checked = !m_checked; return m_checked; }

	void* GetContext() { return m_context; }
	void SetContext(void* context) { m_context = context; }

	const TrayCallback& GetCallback() const { return m_callback; }
	void SetCallback(TrayCallback callback) { m_callback = callback; }

	SystemTray* GetParent() { return m_parent; }
	void SetParent(SystemTray* parent);
private:
	const char* m_label{ "" };
	bool m_disabled{ false };
	bool m_checked{ false };

	void* m_context;
	TrayCallback m_callback;

	SystemTray* m_parent{ nullptr };
};

class SystemTray : public MenuParent {
public:
	SystemTray() = default;
	~SystemTray();

	void Update();

	void Init();
	int Loop(int blocking);

	static HMENU Menu(const std::vector<TrayMenu>& menuList, UINT* id);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:
	static HWND s_hwnd;
	static HMENU s_hMenu;

	WNDCLASSEX m_wc{};
	NOTIFYICONDATA m_nid{};

	const char* icon{ "Settings.ico" };
};