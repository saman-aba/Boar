
#include "launcher_linux.h"

namespace Platform {
Launcher::Launcher(int argc, char *argv[]) :
Core::Launcher(argc, argv) {

}

int Launcher::exec() {
	return Core::Launcher::exec();
}

bool Launcher::launchUpdater(UpdaterLaunch action) {
	return false;
}

};
