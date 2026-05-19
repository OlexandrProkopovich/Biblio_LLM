add_executable(tests test_main.cpp)

target_link_libraries(tests
	PRIVATE
	GTest::gtest_main
	core_math
	core_tokenizer
	core_model
	biblio
)

include(GoogleTest)
gtest_discover_tests(tests)