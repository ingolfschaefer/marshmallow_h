add_executable(engine_demo0 "demo0.cpp")

if (MARSHMALLOW_DEMO_CWD_OVERRIDE)
	if (APPLE)
		# Xcode workaround
		set_target_properties(engine_demo0 PROPERTIES
		    COMPILE_FLAGS -DDEMO_CWD=\\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\\")
	else ()
		set_target_properties(engine_demo0 PROPERTIES
		    COMPILE_FLAGS -DDEMO_CWD=\\"${CMAKE_CURRENT_SOURCE_DIR}\\")
	endif()
endif()

if(WIN32 AND NOT MARSHMALLOW_DEBUG)
	set_target_properties(engine_demo0 PROPERTIES WIN32_EXECUTABLE ON)
endif()

target_link_libraries(engine_demo0 marshmallow_entrypoint
                                   marshmallow_game)

