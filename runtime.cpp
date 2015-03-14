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


Schedule::~Schedule() throw() {
    for(list<BaseTask*>::iterator it = items.begin();
        it != items.end(); it++) {
        delete *it;
    }
}


Commands *Schedule::get_commands() {
    auto_ptr<Commands> result(new Commands);

    for(list<BaseTask*>::iterator it = items.begin();
        it != items.end(); it++) {
        if((*it)->ready()) {
            result->push_back((*it)->get_command());
        }
    }

    return result.release();
}


void Schedule::add_item(BaseTask *item) {
    // for(list<BaseTask*>::iterator it = items.begin();
    //     it != items.end(); it++) {
    //     if(item->get_command()->expired()) {
    //         delete *it;
    //         items.erase(it);
    //         items.insert(it, item);
    //         return;
    //     }
    // }
    items.push_back(item);
}


void Schedule::remove_expired() {
    list<BaseTask*>::iterator it = items.begin();
    while(it != items.end()) {
        if((*it)->expired()) {
            delete *it;
            items.erase(it++);
        } else {
            it++;
        }
    }
}
