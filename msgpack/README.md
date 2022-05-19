# wxlib.msgpack
A modern (C++20 required) implementation of the [msgpack spec](https://github.com/msgpack/msgpack/blob/master/spec.md). 

Adapted from [cpppack](https://github.com/mikeloomisgg/cppack), originally developed by Mike Loomis. Check out [this blog of his](https://mikeloomisgg.github.io/2019-07-02-making-a-serialization-library/) for the motivation creating this library.
 
Msgpack is a binary serialization specification. It allows you to save and load application objects like classes and structs over networks, to files, and between programs and even different languages.

## Credits
Originally developed by Mike Loomis. Improved (actually, almost a complete re-write) and updated to C++20 by Wuping Xin.

## Features
- Fast and compact
- Full test coverage
- Easy to use
- Automatic type handling
- Easy error handling

### Single Header only template library
Want to use this library? Just #include the header, and you're good to go. It's less than 800 lines of code. 

Note - you would also need to include matchit.hpp, because this C++20 version of wxlib.msgpack extensively uses pattern matching.


### Cereal style packaging
Easily pack objects into byte arrays using a pack free function:

```c++
struct Person {
  std::string name;
  uint16_t age;
  std::vector<std::string> aliases;

  template<class T>
  void pack(T &pack) {
    pack(name, age, aliases);
  }
};

int main() {
    auto person = Person{"John", 22, {"Ripper", "Silver-hand"}};

    auto data = msgpack::pack(person); // Pack your object
    auto john = msgpack::unpack<Person>(data.data()); // Unpack it
}
```

### Roadmap
- **Performance enhancement**. The internal storage uses std::vector. This can be optimized for better performance of packing/unpacking large objects and dataset.
- **Support for extension types**. The msgpack spec allows for additional types to be enumerated as Extensions. If reasonable use cases come about for this feature then it may be added.
- **Name/value pairs**. The msgpack spec uses the 'map' type differently than this library. This library implements maps in which key/value pairs must all have the same value types.
- **Endian conversion shortcuts**. On platforms that already hold types in big endian, the serialization could be optimized using type traits.

---

Last edited by Wuping Xin (@wxinix)