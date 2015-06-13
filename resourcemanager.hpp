#ifndef _RESOURCESMANAGER_HPP_INCLUDED_
#define _RESOURCESMANAGER_HPP_INCLUDED_

#include <string>
#include <map>
#include <list>


class ResourcesTaker;


class ResourceIsBusy: public std::exception, public std::string {
public:
    ResourceIsBusy(const std::string &);
    ~ResourceIsBusy() throw();
    const char *what() const throw();
};


typedef std::map<std::string, bool> MapSB;

class Resources {
private:
    MapSB resources;
    MapSB groups;
    std::map<std::string, std::string> binds;
    std::map<std::string, std::list<std::string> > group_binds;

    void take_resource(const std::string &);
    void take_group(const std::string &);

public:
    void add_resource(const std::string &, const std::string &);
    void add_resource(const std::string &);
    void release(const std::string &);

    friend class ResourcesTaker;
    ResourcesTaker *taker();
};


class ResourcesTaker {
private:
    Resources &res;
    std::list<std::string> groups_to_take;
    std::list<std::string> resources_taken;
    bool success;

public:
    ResourcesTaker(Resources &);
    virtual ~ResourcesTaker() throw();
    void take(const std::string &);
    std::list<std::string> captured();
    void approve();
};

#endif
