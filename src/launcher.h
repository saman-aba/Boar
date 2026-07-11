#pragma once

#include <memory>
#include <assert.h>
#include <QString>
#include <QStringList>

namespace Core {
class Launcher {
    public:
        Launcher(int argc, char *argv[]);

        static std::unique_ptr<Launcher> Create(int argc, char *argv[]);
        static Launcher &instance() {
            assert(InstanceSetter::Instance);
            return *InstanceSetter::Instance;
        }

        static Launcher &Instance() {
            assert(InstanceSetter::Instance != nullptr);
            return *InstanceSetter::Instance;
        }

        virtual int exec();

        virtual ~Launcher();

    protected:
        enum class UpdaterLaunch {
            PerformUpdate,
            JustRelaunch,
        };

    private:
        void init();

        virtual std::optional<QStringList> readArgumentHook(
                int argc,
                char *argv[]) const {
                    return std::nullopt;
        }

	virtual bool launchUpdater(UpdaterLaunch action) = 0;
	int executeApplication();

        struct InstanceSetter {
            InstanceSetter(Launcher *instance) {
                Instance = instance;
            }
            static Launcher *Instance;
        };

        InstanceSetter _instanceSetter = {this};

        int _argc;
        char **_argv;
        QStringList _arguments;

        QString _initialWorkingDir;
        QString _customWorkingDir;

};
};
