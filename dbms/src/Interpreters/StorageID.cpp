#include <Interpreters/StorageID.h>
#include <Parsers/ASTQueryWithTableAndOutput.h>
#include <Parsers/ASTIdentifier.h>
#include <Common/quoteString.h>
#include <IO/WriteHelpers.h>
#include <Interpreters/DatabaseAndTableWithAlias.h>

namespace DB
{

namespace ErrorCodes
{
    extern const int LOGICAL_ERROR;
    extern const int UNKNOWN_DATABASE;
}

StorageID::StorageID(const ASTQueryWithTableAndOutput & query)
{
    database_name = query.database;
    table_name = query.table;
    uuid = query.uuid;
    assertNotEmpty();
}

StorageID::StorageID(const ASTIdentifier & table_identifier_node)
{
    DatabaseAndTableWithAlias database_table(table_identifier_node);
    database_name = database_table.database;
    table_name = database_table.table;
    uuid = database_table.uuid;
    assertNotEmpty();
}

StorageID::StorageID(const ASTPtr & node)
{
    if (auto identifier = node->as<ASTIdentifier>())
        *this = StorageID(*identifier);
    else if (auto simple_query = node->as<ASTQueryWithTableAndOutput>())
        *this = StorageID(*simple_query);
    else
        throw Exception("Unexpected AST", ErrorCodes::LOGICAL_ERROR);
}

String StorageID::getTableName() const
{
    assertNotEmpty();
    return table_name;
}

String StorageID::getDatabaseName() const
{
    assertNotEmpty();
    if (database_name.empty())
        throw Exception("Database name is empty", ErrorCodes::UNKNOWN_DATABASE);
    return database_name;
}

String StorageID::getNameForLogs() const
{
    assertNotEmpty();
    return (database_name.empty() ? "" : backQuoteIfNeed(database_name) + ".") + backQuoteIfNeed(table_name)
           + (hasUUID() ? " (UUID " + toString(uuid) + ")" : "");
}

bool StorageID::operator<(const StorageID & rhs) const
{
    assertNotEmpty();
    /// It's needed for ViewDependencies
    if (!hasUUID() && !rhs.hasUUID())
        /// If both IDs don't have UUID, compare them like pair of strings
        return std::tie(database_name, table_name) < std::tie(rhs.database_name, rhs.table_name);
    else if (hasUUID() && rhs.hasUUID())
        /// If both IDs have UUID, compare UUIDs and ignore database and table name
        return uuid < rhs.uuid;
    else
        /// All IDs without UUID are less, then all IDs with UUID
        return !hasUUID();
}

String StorageID::getFullTableName() const
{
    return backQuoteIfNeed(getDatabaseName()) + "." + backQuoteIfNeed(table_name);
}

}
