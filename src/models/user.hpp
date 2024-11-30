#include <string>

namespace split_bill {

struct TUser {
    int id;
    std::string username;
    std::optional<std::string> full_name;
    std::optional<std::string> photo_url;
    std::string password;
};

}