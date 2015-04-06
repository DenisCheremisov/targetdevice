#ifndef _CONTROLER_HPP_INCLUDED_
#define _CONTROLER_HPP_INCLUDED_


#include "confbind.hpp"


class InteruptionHandling: public std::string {
public:
    InteruptionHandling(std::string message): std::string(message) {};
};


#endif
