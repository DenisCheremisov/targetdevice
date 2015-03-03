#ifndef _GETTERS_H_INCLUDED_
#define _GETTERS_H_INCLUDED_

#include "binder.h"
#include "maplib.h"

work_result_t *is_connected(map_t *params, int serial);
work_result_t *adc_get(map_t *params, int serial);
work_result_t *read_all(map_t *params, int serial);
work_result_t *read_line(map_t *params, int serial);
work_result_t *write_line(map_t *params, int serial);

#endif
