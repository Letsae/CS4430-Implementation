#include "../src/security/BcryptPasswordHasher.h"
#include "../src/domain/LmsException.h"
#include <iostream>

using namespace Lms;

int main() {
    try {
        BcryptPasswordHasher hasher(12);
        std::string password = "password";
        std::string hash = hasher.hashPassword(password);
        std::cout << hash << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
