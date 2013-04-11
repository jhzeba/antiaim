project (cstrike_train)

set (src
	cstrike.cpp
	../ann.cpp
)

include_directories (..)
add_executable (cstrike ${src})
#target_link_libraries (cstrike efence)
