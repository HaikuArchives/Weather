/*
	Licensed under the MIT license.
	Made for Haiku.
*/


#include "App.h"
#include "MainWindow.h"

App::App(void) : BApplication("application/x-vnd.gw-Weather") {
	BRect frame(256, 256, 256, 256);
	MainWindow *mw = new MainWindow();
	mw->Show();
}

int main() {
	App *mApp = new App();
	mApp->Run();
	delete mApp;
	return 0;
}
