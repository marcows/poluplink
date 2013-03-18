include_directories(${Poluplink_SOURCE_DIR}/lib)

add_executable(poluplink-cli main.c)
target_link_libraries(poluplink-cli poluplink)

install(TARGETS poluplink-cli
	RUNTIME DESTINATION bin)

add_test(poluplink-cli-runs
	poluplink-cli ${PROJECT_SOURCE_DIR}/test/poluplink.txt test.wav)

add_test(wav-correctly-generated-from-raw-bytes
	cmake -E compare_files ${PROJECT_SOURCE_DIR}/test/poluplink.wav test.wav)
