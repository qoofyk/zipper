adios_groupsize = 4 \
                + 4 \
                + 4 \
                + 4 \
                + 8 * (n) * (size_one);
adios_group_size (adios_handle, adios_groupsize, &adios_totalsize);
adios_write (adios_handle, "NX", &NX);
adios_write (adios_handle, "lb", &lb);
adios_write (adios_handle, "n", &n);
adios_write (adios_handle, "size_one", &size_one);
adios_write (adios_handle, "atom", t);
