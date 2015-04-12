#ifndef _BACKGROUND_HPP_INCLUDED_
#define _BACKGROUND_HPP_INCLUDED_

#include <ctime>

#include <unistd.h>
#include <pthread.h>

#include "runtime.hpp"
#include "locker.hpp"


void* background_worker(void *args);

#endif
