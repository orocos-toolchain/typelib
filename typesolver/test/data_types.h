#ifndef MODEL_DATA_TYPES_H
#define MODEL_DATA_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * DATA STRUCTURES
 */

typedef struct p6d
{
    double pos[3];
    double rot[3];
} p6d;

/************************************************************/

#ifdef __cplusplus
};
#endif

#ifdef __cplusplus
#include <string>

/************************************************************
 * TYPE NAMES
 */
namespace models
{
    template<typename T> std::string getDataTypename();
    template<> inline std::string getDataTypename<p6d>() { return "p6d"; }
};

/************************************************************/

#endif


#endif

