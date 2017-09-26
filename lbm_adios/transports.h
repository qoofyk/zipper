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
 * 0xff will returned for unsuported method
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
#define DSPACES (0)
#define DIMES (1)
#define FLEXPATH (2)
#define MPIIO (3)
#define MINOR_NO_DEF (0xff)

/*
 * minor method
 */
typedef uint8_t transport_method_t;

/*
 * @brief construct method using major and minor methods
 */
#define construct_method(major, minor) \
    (major<<4 | minor)


#define get_major(method) \
    (method >> 4)

#define get_minor(method) \
    (method & 0x0f)


/*
 * @brief get current tranport method from environment variables
 *
 * @return  current tranport method
 */
transport_method_t get_current_transport();

