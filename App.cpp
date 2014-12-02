/*
	Licensed under the MIT license.
	Made for Haiku.
*/


#include "App.h"
#include "Menu.h"

App::App(void) : BApplication("application/x-vnd.gw-Weather") {
	BRect frame(256, 256, 256, 256);
	window = new BWindow(frame, "Weather", B_TITLED_WINDOW,
					B_QUIT_ON_WINDOW_CLOSE);
	window->Show();
}

int main() {
	App *mApp = new App();
	mApp->Run();
	delete mApp;
	return 0;
}
