
#include <filesystem>
#include <iostream>


#include <sqlpp23/sqlite3/sqlite3.h>
#include <sqlpp23/sqlpp23.h>

#include <settings.hpp>
#include <tableTest.hpp>
#include <reflect.hpp>


int main() {

    auto ar = incom::reflect::get_enum_values<incom::reflect::Color>();


    return 0;
}