/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

/*!
  MIT License
  Copyright (c) 2019 Mike Loomis
*/

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>
#include <msgpack/msgpack.hpp>

TEST_CASE("scenario: packing types")
{
  SUBCASE("test packing nil") {
    auto null_obj = std::nullptr_t{};
    auto packer = msgpack::Packer{};
    packer.process(null_obj);
    CHECK(packer.vector() == std::vector<uint8_t>{0xc0});
  }

  SUBCASE("test packing boolean") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};
    auto bool_obj = false;
    packer.process(bool_obj);
    unpacker.set_data(packer.vector().data(), packer.vector().size());
    bool_obj = true;
    unpacker.process(bool_obj);
    CHECK(packer.vector() == std::vector<uint8_t>{0xc2});
    CHECK(!bool_obj);

    bool_obj = true;
    packer.clear();
    packer.process(bool_obj);
    unpacker.set_data(packer.vector().data(), packer.vector().size());
    bool_obj = false;
    unpacker.process(bool_obj);
    CHECK(packer.vector() == std::vector<uint8_t>{0xc3});
    CHECK(bool_obj);
  }

  SUBCASE("test packing integer") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};

    for (auto i = 0U; i < 10; ++i) {
      uint8_t test_num = i * (std::numeric_limits<uint8_t>::max() / 10);
      packer.clear();
      packer.process(test_num);
      uint8_t x = 0U;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }

    for (auto i = 0U; i < 10; ++i) {
      uint16_t test_num = i * (std::numeric_limits<uint16_t>::max() / 10);
      packer.clear();
      packer.process(test_num);
      uint16_t x = 0U;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }

    for (auto i = 0U; i < 10; ++i) {
      uint32_t test_num = i * (std::numeric_limits<uint32_t>::max() / 10);
      packer.clear();
      packer.process(test_num);
      uint32_t x = 0U;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }

    for (auto i = 0U; i < 10; ++i) {
      uint64_t test_num = i * (std::numeric_limits<uint64_t>::max() / 10);
      packer.clear();
      packer.process(test_num);
      uint64_t x = 0U;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }

    for (auto i = -5; i < 5; ++i) {
      int8_t test_num = i * (std::numeric_limits<int8_t>::max() / 5);
      packer.clear();
      packer.process(test_num);
      int8_t x = 0;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }

    for (auto i = -5; i < 5; ++i) {
      int16_t test_num = i * (std::numeric_limits<int16_t>::max() / 5);
      packer.clear();
      packer.process(test_num);
      int16_t x = 0;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }

    for (auto i = -5; i < 5; ++i) {
      int32_t test_num = i * (std::numeric_limits<int32_t>::max() / 5);
      packer.clear();
      packer.process(test_num);
      int32_t x = 0;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }

    for (auto i = -5; i < 5; ++i) {
      int64_t test_num = i * (std::numeric_limits<int64_t>::max() / 5);
      packer.clear();
      packer.process(test_num);
      int64_t x = 0;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }
  }

  SUBCASE("test packing chrono") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};

    auto test_time_point = std::chrono::steady_clock::now();
    auto test_time_point_copy = test_time_point;

    packer.process(test_time_point);
    test_time_point = std::chrono::steady_clock::time_point{};
    CHECK(test_time_point != test_time_point_copy);
    unpacker.set_data(packer.vector().data(), packer.vector().size());
    unpacker.process(test_time_point);
    CHECK(test_time_point == test_time_point_copy);
  }

  SUBCASE("test packing float") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};

    for (auto i = -5; i < 5; ++i) {
      float test_num = 5.0f + float(i) * 12345.67f / 4.56f;
      packer.clear();
      packer.process(test_num);
      float x = 0.0f;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }

    for (auto i = -5; i < 5; ++i) {
      double test_num = 5.0 + double(i) * 12345.67 / 4.56;
      packer.clear();
      packer.process(test_num);
      double x = 0.0;
      unpacker.set_data(packer.vector().data(), packer.vector().size());
      unpacker.process(x);
      CHECK(x == test_num);
    }
  }

  SUBCASE("test packing string") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};

    auto str1 = std::string("test");
    packer.process(str1);
    str1 = "";
    unpacker.set_data(packer.vector().data(), packer.vector().size());
    unpacker.process(str1);
    CHECK(packer.vector() == std::vector<uint8_t>{0b10100000 | 4, 't', 'e', 's', 't'});
    CHECK(str1 == "test");
  }

  SUBCASE("test packing byte array") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};

    auto vec1 = std::vector<uint8_t>{1, 2, 3, 4};
    packer.process(vec1);
    vec1.clear();
    unpacker.set_data(packer.vector().data(), packer.vector().size());
    unpacker.process(vec1);
    CHECK(packer.vector() == std::vector<uint8_t>{0xc4, 4, 1, 2, 3, 4});
    CHECK(vec1 == std::vector<uint8_t>{1, 2, 3, 4});
  }

  SUBCASE("test packing array type") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};

    auto list1 = std::list<std::string>{"one", "two", "three"};
    packer.process(list1);
    list1.clear();
    unpacker.set_data(packer.vector().data(), packer.vector().size());
    unpacker.process(list1);
    CHECK(packer.vector() == std::vector<uint8_t>{0b10010000 | 3, 0b10100000 | 3, 'o', 'n', 'e',
                                                  0b10100000 | 3, 't', 'w', 'o',
                                                  0b10100000 | 5, 't', 'h', 'r', 'e', 'e'});
    CHECK(list1 == std::list<std::string>{"one", "two", "three"});
  }

  SUBCASE("test packing std::array") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};

    auto arr = std::array<std::string, 3>{"one", "two", "three"};
    packer.process(arr);
    arr.fill("");
    unpacker.set_data(packer.vector().data(), packer.vector().size());
    unpacker.process(arr);
    CHECK(packer.vector() == std::vector<uint8_t>{0b10010000 | 3, 0b10100000 | 3, 'o', 'n', 'e',
                                                  0b10100000 | 3, 't', 'w', 'o',
                                                  0b10100000 | 5, 't', 'h', 'r', 'e', 'e'});
    CHECK(arr == std::array<std::string, 3>{"one", "two", "three"});
  }

  SUBCASE("test packing map") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};

    auto map1 = std::map<uint8_t, std::string>{std::make_pair(0, "zero"), std::make_pair(1, "one")};
    packer.process(map1);
    map1.clear();
    unpacker.set_data(packer.vector().data(), packer.vector().size());
    unpacker.process(map1);
    CHECK(packer.vector() == std::vector<uint8_t>{0b10000000 | 2, 0, 0b10100000 | 4, 'z', 'e', 'r', 'o',
                                                  1, 0b10100000 | 3, 'o', 'n', 'e'});
    CHECK(map1 == std::map<uint8_t, std::string>{std::make_pair(0, "zero"), std::make_pair(1, "one")});
  }

  SUBCASE("test packing unordered map") {
    auto packer = msgpack::Packer{};
    auto unpacker = msgpack::Unpacker{};

    auto map1 = std::unordered_map<uint8_t, std::string>{std::make_pair(0, "zero"), std::make_pair(1, "one")};
    auto map_copy = map1;
    packer.process(map1);
    map1.clear();
    unpacker.set_data(packer.vector().data(), packer.vector().size());
    unpacker.process(map1);
    CHECK(map1[0] == map_copy[0]);
    CHECK(map1[1] == map_copy[1]);
  }
}

struct NestedObject
{
  std::string nested_value{};

  template<class T>
  void pack(T &pack)
  {
    pack(nested_value);
  }
};

struct BaseObject
{
  int first_member{};
  NestedObject second_member{};

  template<class T>
  void pack(T &pack)
  {
    pack(first_member, second_member);
  }
};

TEST_CASE("scenario: packing object")
{
  SUBCASE("test user objects serialization") {
    auto object = BaseObject{12345, {"NestedObject"}};
    auto data = msgpack::pack(object);
    auto unpacked_object = msgpack::unpack<BaseObject>(data);

    CHECK(object.first_member == unpacked_object.first_member);
    CHECK(object.second_member.nested_value == unpacked_object.second_member.nested_value);
  }
}

struct ExampleError
{
  std::map<std::string, bool> map;

  template<class T>
  void pack(T &pack)
  {
    pack(map);
  }
};

TEST_CASE("scenario: error-handling")
{
  SUBCASE("test unpacking a short dataset safely fails") {
    ExampleError example{{{"compact", true}, {"schema", false}}};
    auto data = std::vector<uint8_t>{0x82, 0xa7, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x63, 0x74, 0xc3, 0xa6, 0x73, 0x63};
    std::error_code ec{};
    CHECK(!ec);
    CHECK(example.map != msgpack::unpack<ExampleError>(data, ec).map);
    CHECK(ec);
    CHECK(ec == msgpack::UnpackerError::OutOfRange);
  }
}

struct Example
{
  std::map<std::string, bool> map;

  template<class T>
  void pack(T &pack)
  {
    pack(map);
  }
};

TEST_CASE("scenario: example")
{

  SUBCASE("test website example") {
    Example example{{{"compact", true}, {"schema", false}}};
    auto data = msgpack::pack(example);

    CHECK(data.size() == 18);
    CHECK(data == std::vector<uint8_t>{0x82, 0xa7, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x63, 0x74,
                                       0xc3, 0xa6, 0x73, 0x63, 0x68, 0x65, 0x6d, 0x61, 0xc2});

    CHECK(example.map == msgpack::unpack<Example>(data).map);
  }

  SUBCASE("test unpack with error handling") {
    Example example{{{"compact", true}, {"schema", false}}};
    auto data = std::vector<uint8_t>{0x82, 0xa7, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x63, 0x74, 0xc3, 0xa6, 0x73, 0x63};
    std::error_code ec{};
    auto unpacked_object = msgpack::unpack<Example>(data, ec);

    if (ec && ec == msgpack::UnpackerError::OutOfRange)
      CHECK(true);
    else
      CHECK(false);
  }
}