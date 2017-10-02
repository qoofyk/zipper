/*
 * @author Feng Li, IUPUI
 * @date   2017
 */
#include "transports.h"
#include "utility.h"

transport_method_t get_current_transport(){
    uint8_t major, minor; // method id
    char * transport_string; //input from user

    /*
     * must define tranport method!
     */
    minor = MAJOR_NO_DEF ;
    minor = MINOR_NO_DEF;

    /*
     * get the configuration from environment variable
     */
    if((transport_string = getenv("MyTransport")) != NULL){
        clog_debug(CLOG(MY_LOGGER),"get string %s\n", transport_string);

        /*
         * adios-mpiio
         */ 
        if(strcmp(transport_string, "ADIOS_DISK_MPIIO") == 0){
            major = ADIOS_DISK;
            minor = MPIIO;
        }

        /*
         * adios staging
         */
        else if (strcmp(transport_string, "ADIOS_STAGING_DSPACES") == 0){
            major = ADIOS_STAGING;
            minor = DSPACES;
        }
        else if (strcmp(transport_string, "ADIOS_STAGING_DIMES") == 0){
            major = ADIOS_STAGING;
            minor = DIMES;
        }

        else if (strcmp(transport_string, "ADIOS_STAGING_FLEXPATH") == 0){
            major = ADIOS_STAGING;
            minor = FLEXPATH;
        }

        /*
         * native staging
         */

        else if (strcmp(transport_string, "NATIVE_STAGING_DSPACES") == 0){
            major = NATIVE_STAGING;
            minor = DSPACES;
        }
        else if (strcmp(transport_string, "NATIVE_STAGING_DIMES") == 0){
            major = NATIVE_STAGING;
            minor = DIMES;
        }

        else{
            minor = MAJOR_NO_DEF ;
            minor = MINOR_NO_DEF;
        }
    }


    transport_method_t transport = construct_method(major, minor);
    clog_debug(CLOG(MY_LOGGER), "tranport code %x\n", transport);
    return transport;
}
