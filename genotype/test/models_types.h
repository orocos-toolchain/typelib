#ifndef SIMLIB_MODEL_TYPES_H
#define SIMLIB_MODEL_TYPES_H

#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_NOW       0
#define TIME_NORMAL    1
#define TIME_INFTY     2

typedef long model_sec_t;
typedef unsigned long model_usec_t;
    
typedef struct model_time
{
    char type;
    model_sec_t tv_sec;
    model_usec_t tv_usec;
//    long padding; // to have model_time aligned on 8 bytes
} model_time;

/** The module_model structure is defined using the same layout
 * than model_header into models.h
 *
 * DON'T FORGET TO UPDATE model_header WHEN YOU CHANGE module_model
 * (and vice-versa)
 */
#define MODEL(module, sample_count, data_type) \
    typedef struct module##_model_sample \
    { \
        model_time duration; \
        data_type data; \
    } module##_model_sample; \
    \
    typedef struct module##_model \
    { \
        model_time begin; \
        long count; \
        module##_model_sample samples[sample_count]; \
    } module##_model;
    

#ifdef __cplusplus
};
#endif

#endif

