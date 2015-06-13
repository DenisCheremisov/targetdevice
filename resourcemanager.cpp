#include "resourcemanager.hpp"


using namespace std;


ResourceIsBusy::ResourceIsBusy(const string &msg): string(msg) {}

ResourceIsBusy::~ResourceIsBusy() throw() {}

const char * ResourceIsBusy::what() const throw() {
    return this->c_str();
}


void Resources::take_resource(const string &resource) {
    auto &&it = resources.find(resource);
    if(it == resources.end()) {
        return;
    }
    bool rsc = it->second;
    if(!rsc) {
        throw ResourceIsBusy(resource);
    }
    auto &&git = binds.find(resource);
    if(git == binds.end()) {
        return;
    }
    auto &group = git->second;
    if(groups[group]) {
        it->second = false;
    } else {
        throw ResourceIsBusy(resource);
    }
}


void Resources::take_group(const string &resource) {
    auto &&git = binds.find(resource);
    if(git == binds.end()) {
        return;
    }
    groups[git->second] = false;
}


void Resources::add_resource(const string &resource, const string &group) {
    resources[resource] = true;
    binds[resource] = group;
    groups[group] = true;
    group_binds[group].push_back(resource);
}


void Resources::add_resource(const string &resource) {
    resources[resource] = true;
}


void Resources::release(const string &resource) {
    auto &&it = resources.find(resource);
    if(it == resources.end()) {
        return;
    }
    it->second = true;
    auto &&bit = binds.find(resource);
    if(bit == binds.end()) {
        return;
    }
    auto &group = bit->second;
    auto &ress = group_binds[group];
    bool all_free = true;
    for(auto &rit: ress) {
        if(!resources[rit]) {
            all_free = false;
            break;
        }
    }
    if(all_free) {
        groups[group] = true;
    }
}


ResourcesTaker *Resources::taker() {
    return new ResourcesTaker(*this);
}


ResourcesTaker::ResourcesTaker(Resources &resources): res(resources), success(false) {}


ResourcesTaker::~ResourcesTaker() throw() {
    if(success) {
        for(auto &it: groups_to_take) {
            res.take_group(it);
        }
    } else {
        for(auto &it: resources_taken) {
            res.release(it);
        }
    }
}


void ResourcesTaker::take(const string &resource) {
    try {
        res.take_resource(resource);
    } catch(...) {
        for(auto &it: resources_taken) {
            res.release(it);
        }
        throw;
    }
    resources_taken.push_back(resource);
    groups_to_take.push_back(resource);
}


list<string> ResourcesTaker::captured() {
    return resources_taken;
}


void ResourcesTaker::approve() {
    success = true;
}
