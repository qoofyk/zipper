/*
 * @brief type definition of different tranports
 *
 * @author Feng Li, IUPUI
 * @date   2017
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/*
 * totally three types of transport methods are used
 *  I. adios-staging(DataSpaces/DIMES/Flexpath with ADIOS wrapper), 
 *  II.adios-mpiio(mature disk-based IO method suggested by ADIOS)
 *  III.native staging(native methods for DataSpaces and DIMES)
 *
 * To select different transport method, set environment method, such as
 *  * export Tranport=NATIVE_STAGING_DSPACES
 *
 * ff will returned for unsuported method
 */


/*
 * major method
 */
#define ADIOS_DISK (0)
#define ADIOS_STAGING (1)
#define NATIVE_STAGING (2)
#define MAJOR_NO_DEF (0xff)

/*
 * minor method
 */
#define MPIIO (0)
#define DSPACES (1)
#define DIMES (2)
#define FLEXPATH (3)
#define MINOR_NO_DEF (0xff)

/*
 * minor method
 */
typedef uint8_t transport_method_t;

/*
 * @brief construct method using major and minor methods
 */
static transport_method_t construct_method(uint8_t major, uint8_t minor ){
    return (major<<4|minor);
}

/*
 * @brief get current tranport method from environment variables
 *
 * @return  current tranport method
 */
transport_method_t get_current_transport();

