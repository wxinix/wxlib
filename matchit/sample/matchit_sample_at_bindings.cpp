#include "../../matchit/matchit.hpp"
#include <iostream>

using namespace matchit;

struct Hello
{
  int32_t id;
};

using Message = std::variant<Hello>;

int32_t main()
{
  Message const msg = Hello{5};

  auto const l_as_hello_ds = as_ds_via<Hello>(&Hello::id);
  Id<int32_t> id_variable;

  match(msg)(
      pattern | l_as_hello_ds(id_variable.at(3 <= _ && _ <= 7)) = [&] {
        std::cout << "Found an id in range: " << *id_variable << std::endl;
      },

      pattern | l_as_hello_ds(10 <= _ && _ <= 12) = [&] {
        std::cout << "Found an id in another range" << std::endl;
      },

      pattern | l_as_hello_ds(id_variable) = [&] {
        std::cout << "Found some other id: " << *id_variable << std::endl;
      });
}