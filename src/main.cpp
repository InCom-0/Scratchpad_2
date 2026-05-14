
#include <filesystem>
#include <iostream>


#include <sqlpp23/sqlite3/sqlite3.h>
#include <sqlpp23/sqlpp23.h>

#include <settings.hpp>
#include <tableTest.hpp>


int main() {

    using namespace sqlpp;

    std::string exeDir = getExecutableDir().generic_string();
    std::string dbFile = exeDir;
    dbFile.append("/testDB.sqlite");


    auto configOnDisk              = std::make_shared<sqlpp::sqlite3::connection_config>();
    configOnDisk->path_to_database = dbFile;
    configOnDisk->flags            = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    sqlpp::sqlite3::connection dbOnDisk;
    dbOnDisk.connect_using(configOnDisk); // This can throw an exception.


    // dbOnDisk(R"sql(CREATE TABLE IF NOT EXISTS schemes (
    // scheme_id     INTEGER PRIMARY KEY,
    // name          TEXT NOT NULL UNIQUE,
    // fg_color      INTEGER NOT NULL CHECK(fg_color BETWEEN 0 AND 16777215),
    // bg_color      INTEGER NOT NULL CHECK(bg_color BETWEEN 0 AND 16777215),
    // cursor_color  INTEGER NOT NULL CHECK(cursor_color BETWEEN 0 AND 16777215),
    // sel_color     INTEGER NOT NULL CHECK(sel_color BETWEEN 0 AND 16777215)
    // );)sql");

    // dbOnDisk(R"sql(CREATE TABLE IF NOT EXISTS scheme_palette (
    // scheme_id        INTEGER NOT NULL REFERENCES schemes(scheme_id) ON DELETE CASCADE,
    // index_in_palette INTEGER NOT NULL CHECK(index_in_palette BETWEEN 0 AND 255),
    // color            INTEGER NOT NULL CHECK(color BETWEEN 0 AND 16777215),
    // PRIMARY KEY (scheme_id, index_in_palette)
    // );)sql");

    // dbOnDisk(R"sql(CREATE TABLE IF NOT EXISTS default_scheme (
    // id INTEGER PRIMARY KEY CHECK(id = 1),
    // scheme_id INTEGER UNIQUE
    //     REFERENCES schemes(scheme_id)
    //     ON DELETE SET NULL
    // );)sql");

    // dbOnDisk(R"sql(INSERT INTO default_scheme (id, scheme_id) VALUES (1, NULL);)sql");


    auto SQL_schemes  = my_project::Schemes();
    auto SQL_palettes = my_project::SchemePalette();

    dbOnDisk(insert_into(SQL_schemes)
                 .set(SQL_schemes.name = "testScheme3", SQL_schemes.fgColor = 5, SQL_schemes.bgColor = 12,
                      SQL_schemes.cursorColor = 24, SQL_schemes.selColor = 38));


    return 0;
}