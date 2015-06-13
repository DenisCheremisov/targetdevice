#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ResourceManagerCpp

#include <vector>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "../resourcemanager.hpp"


using namespace std;

typedef pair<string, string> ResourcePair;
vector<ResourcePair> RESOURCES;


class Initializer {
public:
    Initializer() {
        RESOURCES.push_back(ResourcePair("temperature.temperature", "temperature"));
        RESOURCES.push_back(ResourcePair("switcher.on", "switcher"));
        RESOURCES.push_back(ResourcePair("switcher.off", "switcher"));
        RESOURCES.push_back(ResourcePair("boiler.on", "boiler"));
        RESOURCES.push_back(ResourcePair("boiler.off", "boiler"));
    }
};

Initializer initializer;

Resources* get_resources() {
    auto resources = new Resources;
    for(auto it: RESOURCES) {
        resources->add_resource(it.first, it.second);
    }
    return resources;
}


BOOST_AUTO_TEST_CASE(test_resource_manager) {
    unique_ptr<Resources> rm(get_resources());
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        taker->take("temperature.temperature");
        taker->approve();
    }
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        BOOST_REQUIRE_THROW(taker->take("temperature.temperature"),
                            ResourceIsBusy);
    }
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        taker->take("switcher.on");
        taker->take("switcher.off");
        taker->approve();
    }
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        BOOST_REQUIRE_THROW(taker->take("switcher.off"),
                            ResourceIsBusy);
    }
    rm->release("switcher.off");
    rm->release("switcher.on");
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        taker->take("switcher.off");
        taker->take("boiler.on");
        taker->approve();
    }

    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        BOOST_REQUIRE_THROW(taker->take("boiler.off"),
                            ResourceIsBusy);
    }
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        BOOST_REQUIRE_THROW(taker->take("boiler.off"),
                            ResourceIsBusy);
    }
    rm->release("boiler.on");
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        taker->take("boiler.off");
        taker->approve();
    }
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        BOOST_REQUIRE_THROW(taker->take("boiler.on"),
                            ResourceIsBusy);
    }
}


BOOST_AUTO_TEST_CASE(test_error_rollback) {
    unique_ptr<Resources> rm(get_resources());
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        for(auto it: RESOURCES) {
            taker->take(it.first);
        }
        BOOST_REQUIRE_THROW(taker->take(RESOURCES[0].first),
                             ResourceIsBusy);
    }
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        for(auto it: RESOURCES) {
            taker->take(it.first);
        }
        taker->approve();
    }
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        for(auto it: RESOURCES) {
            BOOST_REQUIRE_THROW(taker->take(it.first),
                                ResourceIsBusy);
        }
    }
}


BOOST_AUTO_TEST_CASE(test_fair_rollback) {
    unique_ptr<Resources> rm(get_resources());
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        for(auto it: RESOURCES) {
            taker->take(it.first);
        }
        taker->approve();
    }
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        for(auto it: RESOURCES) {
            BOOST_REQUIRE_THROW(taker->take(it.first),
                                ResourceIsBusy);
        }
    }
    {
        unique_ptr<ResourcesTaker> taker(rm->taker());
        for(auto it: RESOURCES) {
            BOOST_REQUIRE_THROW(taker->take(it.first),
                                ResourceIsBusy);
        }
    }
}
