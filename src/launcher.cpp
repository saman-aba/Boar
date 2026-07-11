#include "launcher.h"
#include "sandbox.h"
#include "platform/platform_launcher.h"
#include <QDir>

namespace Core {

Launcher::Launcher(int argc, char *argv[]): _argc(argc), _argv(argv)
, _initialWorkingDir(QDir::currentPath() + '/') 
{

}

Launcher *Launcher::InstanceSetter::Instance = nullptr;

std::unique_ptr<Launcher> Launcher::Create(int argc, char *argv[]) {
    return std::make_unique<Platform::Launcher>(argc, argv);
}


Launcher::~Launcher() {
	InstanceSetter::Instance = nullptr;
}

int Launcher::exec() {
	auto result = executeApplication();
	return result;	
}

int Launcher::executeApplication() {
	Sandbox sandbox(_argc, _argv);
	return sandbox.start();
}

};
