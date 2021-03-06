/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "MorkParser.hxx"
#include <iostream>
#include <sal/log.hxx>

bool openAddressBook(const std::string& path)
{
    MorkParser mork;
    // Open and parse mork file
    if (!mork.open(path))
    {
        return false;
    }
    const int defaultScope = 0x80;
    MorkTableMap::Map::const_iterator tableIter;
    MorkTableMap *Tables = mork.getTables( defaultScope );
    if ( Tables )
    {
        // Iterate all tables
        for (auto const& table : Tables->map)
        {
            if ( 0 == table.first ) continue;
            SAL_INFO("connectivity.mork", "table->first : " << table.first);
            std::string column = mork.getColumn( table.first );
            std::string value = mork.getValue( table.first );
            SAL_INFO("connectivity.mork", "table.column : " << column);
            SAL_INFO("connectivity.mork", "table.value : " << value);
        }
    }

    mork.dump();

    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <path-to>/abook.mab" << std::endl;
        std::cerr << "Example: " << argv[0] << " /home/johndoe/.thunderbird/m0tpqlky.default/abook.mab" << std::endl;

        return 1;
    }

    OString aOString(argv[1]);
    SAL_INFO("connectivity.mork", "abook.mab: " << aOString);
    openAddressBook(aOString.getStr());

    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
