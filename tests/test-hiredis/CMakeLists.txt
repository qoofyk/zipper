INCLUDE(FindPkgConfig)
# Check for GLib

#PKG_CHECK_MODULES(GLIB2 glib-2.0)
#if (GLIB2_FOUND)
    #INCLUDE_DIRECTORIES(${GLIB2_INCLUDE_DIRS})
    #LINK_DIRECTORIES(${GLIB2_LIBRARY_DIRS})
    #ADD_EXECUTABLE(example-glib example-glib.c)
    #TARGET_LINK_LIBRARIES(example-glib hiredis ${GLIB2_LIBRARIES})
#ENDIF(GLIB2_FOUND)

FIND_PATH(LIBEV ev.h
    HINTS /usr/local /usr/opt/local
    ENV LIBEV_INCLUDE_DIR)

if (LIBEV)
    # Just compile and link with libev
    ADD_EXECUTABLE(libev-hiredis libev-hiredis.c)
    TARGET_LINK_LIBRARIES(libev-hiredis hiredis ev)

    ADD_EXECUTABLE(libev-example libev-example.cc)
    TARGET_LINK_LIBRARIES(libev-example pthread ev)

ENDIF()
