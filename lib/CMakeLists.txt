add_library(poluplink SHARED poluplink.c)

install(TARGETS poluplink
	LIBRARY DESTINATION lib)
install(FILES poluplink.h
	DESTINATION include)
