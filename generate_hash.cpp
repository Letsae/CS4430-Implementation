#include "src/security/BcryptPasswordHasher.h"
#include <iostream>
#include <string>

using namespace Lms;

int main() {
    BcryptPasswordHasher hasher(12);
    std::string password = "password";
    std::string hash = hasher.hashPassword(password);
    std::cout << "Hash for '" << password << "': " << hash << std::endl;
    return 0;
}
