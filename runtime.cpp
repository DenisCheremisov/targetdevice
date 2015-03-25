#include <memory>

#include "runtime.hpp"


using namespace std;


NamedResults* Executor::execute() throw() {
    for(list<Command*>::iterator it = commands.begin(); it != commands.end(); it++) {
        delete (**it).execute();
    }

    NamedResults *results = new NamedResults;
    for(map<string, Command*>::iterator it = named_commands.begin();
        it != named_commands.end(); it++) {
        Result *res = it->second->execute();
        (*results)[it->first] = res;
    }

    return results;
}


ListSchedule::~ListSchedule() throw() {
    for(ListSchedule::iterator it = this->begin();
        it != this->end(); it++) {
        delete *it;
    }
}


Commands *ListSchedule::get_commands() {
    auto_ptr<Commands> result(new Commands);

    for(ListSchedule::iterator it = this->begin();
        it != this->end(); it++) {
        if((*it)->ready()) {
            result->push_back((*it)->get_command());
        }
    }

    return result.release();
}


ListSchedule& ListSchedule::operator<<(BaseTask *item) {
    // for(list<BaseTask*>::iterator it = items.begin();
    //     it != items.end(); it++) {
    //     if(item->get_command()->expired()) {
    //         delete *it;
    //         items.erase(it);
    //         items.insert(it, item);
    //         return;
    //     }
    // }
    this->push_back(item);
    return *this;
}


void ListSchedule::remove_expired() {
    ListSchedule::iterator it = this->begin();
    while(it != this->end()) {
        if((*it)->expired()) {
            delete *it;
            this->erase(it++);
        } else {
            it++;
        }
    }
}


NamedSchedule::~NamedSchedule() throw() {
    for(NamedSchedule::iterator it = this->begin();
        it != this->end(); it++) {
        delete it->second;
    }
}


Commands *NamedSchedule::get_commands() {
    auto_ptr<Commands> result(new Commands);

    for(NamedSchedule::iterator it = this->begin();
        it != this->end(); it++) {
        auto_ptr<Commands> src(it->second->get_commands());
        result->splice(result->end(), *src);
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


void NamedSchedule::remove_expired() {
    NamedSchedule::iterator it = this->begin();
    for(NamedSchedule::iterator it = this->begin();
        it != this->end(); it++) {
        it->second->remove_expired();
    }
}
