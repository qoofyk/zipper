if(LAMMPS_INCLUDE_DIR)
    #add_definitions("-DPRECISE")

    include_directories       (${LAMMPS_INCLUDE_DIR})

    add_executable            (lammps_adios_prod        lammps_adios_prod.cpp adios_helper.c)
    add_executable            (lammps_adios_con     lammps_adios_con.cpp)
    target_link_libraries     (lammps_adios_prod
                                ${LAMMPS_LIBRARY_STATIC}
                                msd_analysis
                                adaptor
                                ${common_library})

    target_link_libraries     (lammps_adios_con
                                ${LAMMPS_LIBRARY_STATIC}
                                msd_analysis
                                adaptor
                                ${common_library})
        # ${bredala_transports} bredala_datamodel manala decaf)



endif()



