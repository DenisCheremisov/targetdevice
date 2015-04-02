#include <memory>

#include "runtime.hpp"


using namespace std;

Results* Executor::execute() throw() {
    Results *results = new Results;
    for(Commands::iterator it = commands.begin();
        it != commands.end(); it++) {
        Result *res = (**it).execute();
        results->push_back(res);
    }

    return results;
}


ListSchedule::~ListSchedule() throw() {
    for(ListSchedule::iterator it = this->begin();
        it != this->end(); it++) {
        delete *it;
    }
}


Commands *ListSchedule::get_commands(time_t tm) {
    auto_ptr<Commands> result(new Commands);

    for(ListSchedule::iterator it = this->begin();
        it != this->end(); it++) {
        if(!(*it)->is_expired()) {
            auto_ptr<Commands> src((*it)->get_commands(tm));
            result->splice(result->end(), *src);
        }
    }

    return result.release();
}


ListSchedule& ListSchedule::operator<<(BaseSchedule *item) {
    this->push_back(item);
    return *this;
}


bool ListSchedule::is_expired() {
    ListSchedule::iterator it = this->begin();
    int not_expired_counter = 0;

    while(it != this->end()) {
        if((*it)->is_expired()) {
            delete *it;
            this->erase(it++);
        } else {
            not_expired_counter++;
            ++it;
        }
    }

    return not_expired_counter < 1;
}


NamedSchedule::~NamedSchedule() throw() {
    for(NamedSchedule::iterator it = this->begin();
        it != this->end(); it++) {
        delete it->second;
    }
}


Commands *NamedSchedule::get_commands(time_t tm) {
    auto_ptr<Commands> result(new Commands);

    for(NamedSchedule::iterator it = this->begin();
        it != this->end(); it++) {
        if(!it->second->is_expired()) {
            auto_ptr<Commands> src(it->second->get_commands(tm));
            result->splice(result->end(), *src);
        }
    }

    return result.release();
}


NamedSchedule& NamedSchedule::set_schedule(string name, BaseSchedule *sched) {
    NamedSchedule::iterator it = this->find(name);
    if(it != this->end()) {
        delete it->second;
    }
    (*this)[name] = sched;
    return *this;
}


bool NamedSchedule::is_expired() {
    NamedSchedule::iterator it = this->begin();
    int not_expired_counter = 0;

    for(NamedSchedule::iterator it = this->begin();
        it != this->end(); ) {
        if(it->second->is_expired()) {
            delete it->second;
            this->erase(it++);
        } else {
            not_expired_counter++;
            ++it;
        }
    }

    return not_expired_counter < 1;
}


SingleCommandSchedule::SingleCommandSchedule(Command *cmd,
                                             time_t start,
                                             int restart) {
    if(restart >= 0 && restart < RUNTIME_WAKE_PAUSE*2) {
        stringstream buf;
        buf << "Restart period must be longer than " <<
            RUNTIME_WAKE_PAUSE*2 << " seconds, now it is only " << restart;
        throw ScheduleSetupError(buf.str());
    }
    if(start > 0 && start <= time(NULL)) {
        throw ScheduleSetupError("Attempt to set a task to the past");
    }
    command = cmd;
    start_point = start;
    restart_period = restart;
    expired = false;
}


Commands *SingleCommandSchedule::get_commands(time_t tm) {
    auto_ptr<Commands> result(new Commands);
    if(tm >= start_point && !expired) {
        result->push_back(command);
        if(restart_period >= 0) {
            start_point += restart_period;
        } else {
            expired = true;
        }
    }
    return result.release();
}


bool SingleCommandSchedule::is_expired() {
    return expired;
}


CoupledCommandSchedule::~CoupledCommandSchedule() throw() {
    if(on_coupling) {
        /* Coupling means something important, thus we should
           execute coupled command before the schedule removal
        */
        try {
            delete coupled_command->execute();
        } catch(...) {
            ;// No luck
        }
    }
    delete coupled_command;
}


CoupledCommandSchedule::CoupledCommandSchedule(Command *cmd,
                                               time_t start,
                                               Command *coupled_cmd,
                                               int coupled,
                                               int restart):
    SingleCommandSchedule(cmd, start, restart) {
    if(coupled <= 0) {
        throw ScheduleSetupError("Coupling command interval must"
                                 " be greater than 0");
    }
    if(restart > 0 && (restart - coupled) <= RUNTIME_WAKE_PAUSE*2) {
        stringstream buf;
        buf << "Coupling must happen at least " << RUNTIME_WAKE_PAUSE*2 <<
            " seconds before than a restart";
        throw ScheduleSetupError(buf.str());
    }
    coupled_command = coupled_cmd;
    coupled_interval = coupled;
    on_coupling = false;
}


Commands *CoupledCommandSchedule::get_commands(time_t tm) {
    if(!on_coupling) {
        time_t start = get_start_point();
        Commands *result = SingleCommandSchedule::get_commands(tm);
        if(result->size() > 0) {
            on_coupling = true;
            coupled_point = start + coupled_interval;
        }
        return result;
    }
    auto_ptr<Commands> result(new Commands);
    if(tm >= coupled_point) {
        result->push_back(coupled_command);
        on_coupling = false;
    }
    return result.release();
}


bool CoupledCommandSchedule::is_expired() {
    if(!on_coupling) {
        return SingleCommandSchedule::is_expired();
    }
    return false;
}
