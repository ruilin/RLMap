project(coding_tests)

add_definitions(-DOMIM_UNIT_TEST_DISABLE_PLATFORM_INIT)

set(
  SRC
  base64_test.cpp
  bit_streams_test.cpp
  bwt_coder_tests.cpp
  coder_test.hpp
  coder_util_test.cpp
  compressed_bit_vector_test.cpp
  dd_vector_test.cpp
  diff_test.cpp
  elias_coder_test.cpp
  endianness_test.cpp
  file_container_test.cpp
  file_data_test.cpp
  file_sort_test.cpp
  file_utils_test.cpp
  fixed_bits_ddvector_test.cpp
  hex_test.cpp
  huffman_test.cpp
  mem_file_reader_test.cpp
  mem_file_writer_test.cpp
  multilang_utf8_string_test.cpp
  png_decoder_test.cpp
  point_to_integer_test.cpp
  reader_cache_test.cpp
  reader_test.cpp
  reader_test.hpp
  reader_writer_ops_test.cpp
  simple_dense_coding_test.cpp
  succinct_mapper_test.cpp
  text_storage_tests.cpp
  traffic_test.cpp
  uri_test.cpp
  url_encode_test.cpp
  value_opt_string_test.cpp
  var_record_reader_test.cpp
  var_serial_vector_test.cpp
  varint_test.cpp
  #varint_vector_test.cpp
  writer_test.cpp
  zip_creator_test.cpp
  zip_reader_test.cpp
  zlib_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(${PROJECT_NAME} coding base geometry minizip succinct ${LIBZ})
