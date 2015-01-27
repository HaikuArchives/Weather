/*
	Licensed under the MIT license.
	Made for Haiku.
*/


#include "App.h"
#include "MainWindow.h"

App::App(void) : BApplication("application/x-vnd.przemub.HaikuWeather") {
	MainWindow *mw = new MainWindow();
	mw->Show();
}

int main() {
	App *mApp = new App();
	mApp->Run();
	delete mApp;
	return 0;
}
