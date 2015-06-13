#include <iostream>

#include "background.hpp"
#include "runtime.hpp"


void* background_worker(void *args) {
    NamedSchedule *sched = reinterpret_cast<NamedSchedule*>(args);

    while(true) {
        Commands *commands;
        {
            time_t now = time(NULL);
            UnifiedLocker<NamedSchedule> safe(sched);
            commands = safe->get_commands(now);
        }

        Executor exec(commands);
        delete exec.execute(); // Exception handling should be done within commands

        sleep(RUNTIME_WAKE_PAUSE);
    }

    return NULL;
}
