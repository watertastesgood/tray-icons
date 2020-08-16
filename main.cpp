#include <iostream>

#include "trayicon.h"

class ExampleClass {
public:
	ExampleClass(int id) : m_id(id) {}

	void SomeFunction(TrayMenu* menu) const { printf("example class ID %d!\n", m_id); }
private:
	int m_id{ -1 };
};

int main() {
	SystemTray systemTray;

	ExampleClass firstExample(1);
	ExampleClass secondExample(2);

	// adding standalone menu item
	// std::bind takes pointer to function (&ClassName::FunctionName), pointer to the object to perform the function on (&myObject) and some placeholder (idk just do it)
	systemTray.AddMenuItem("example text !", false, false, std::bind(&ExampleClass::SomeFunction, &firstExample, std::placeholders::_1));
	systemTray.AddMenuItem("more text !", false, false, std::bind(&ExampleClass::SomeFunction, &secondExample, std::placeholders::_1));

	// i want to use the subMenu item to add a sub-item, so keep pointer returned by AddMenuItem
	// since i want this to be an item with a sub-item, it can't have a callback happen when clicked !! so i give it nullptr as callback
	auto* subMenu = systemTray.AddMenuItem("sub menu !", false, false, nullptr);

	// can use lambdas for menu item callbacks!!
	auto coolCallback = [](TrayMenu* item) {
		if (item->ToggleChecked())
			printf("toggled on!\n");
		else
			printf("toggled off!\n");
	};

	// use pointer from AddMenuItem to add a menu items to the pets item
	subMenu->AddMenuItem("sub item !", false, false, coolCallback);
	subMenu->AddMenuItem("another sub item !", false, false, [](auto* a) { printf("inline lambda callback! \n"); });

	bool exited = false;
	systemTray.AddMenuItem("exit!", false, false, [&exited](auto* a) { exited = true; });

	systemTray.Init();

	// call .Loop in some kind of update loop to receive messages
	while (systemTray.Loop(0) == 0 && !exited) {}

	return 0;
}