// #include "doctest.h"
//
// #include <algorithm>
// #include <deque>
// #include <sstream>
//
// #include <rz/idx.h>
//
// #include <pqxx/pqxx>
//
// using namespace rz;
// using namespace pqxx;
//
//
//
// // Example 1: Bulk insert
// void bulk_insert(connection& C) {
//     work W(C);
//
//     // Prepare bulk insert data
//     vector<pair<string, int>> users = {
//         {"Alice", 25},
//         {"Bob", 30},
//         {"Charlie", 35}
//     };
//
//     // Use pqxx::stream_to for efficient bulk insert
//     stream_to stream(W, "users", {"name", "age"});
//     for (const auto& user : users) {
//         stream << make_tuple(user.first, user.second);
//     }
//     stream.complete();
//
//     W.commit();
//     cout << "Bulk insert completed" << endl;
// }
//
// // Example 2: Using transactions and savepoints
// void transaction_with_savepoint(connection& C) {
//     work W(C);
//
//     try {
//         W.exec("INSERT INTO users (name, age) VALUES ('David', 40)");
//
//         // Create savepoint
//         W.exec("SAVEPOINT my_savepoint");
//
//         // This operation will fail (assuming age column has CHECK constraint)
//         W.exec("INSERT INTO users (name, age) VALUES ('Eve', -5)");
//     }
//     catch (const exception& e) {
//         cout << "Exception caught: " << e.what() << endl;
//
//         // Rollback to savepoint
//         W.exec("ROLLBACK TO SAVEPOINT my_savepoint");
//
//         // Continue with other operations
//         W.exec("INSERT INTO users (name, age) VALUES ('Frank', 45)");
//     }
//
//     W.commit();
//     cout << "Transaction completed" << endl;
// }
//
// // Example 3: Using prepared statements and parameterized queries
// void prepared_statement(connection& C) {
//     work W(C);
//
//     // Prepared statements
//     C.prepare("insert_user", "INSERT INTO users (name, age) VALUES ($1, $2)");
//     C.prepare("get_user", "SELECT * FROM users WHERE name = $1");
//
//     // Use prepared statement
//     W.exec_prepared("insert_user", "Grace", 50);
//
//     // Parameterized query
//     result R = W.exec_prepared("get_user", "Grace");
//
//     for (const auto& row : R) {
//         cout << "User: " << row["name"].as<string>()
//              << ", Age: " << row["age"].as<int>() << endl;
//     }
//
//     W.commit();
// }
//
// // Example 4: Handling large result sets
// void handle_large_result_set(connection& C) {
//     work W(C);
//
//     // Assume we have a very large table
//     result R = W.exec("SELECT * FROM large_table");
//
//     // Use cursor to process results row by row
//     for (auto row = R.begin(); row != R.end(); ++row) {
//         // Process each row...
//         cout << "Processing row " << row.rownumber() << endl;
//
//         // If we only need to process first 1000 rows
//         if (row.rownumber() >= 1000) break;
//     }
//
//     W.commit();
// }
//
// // Example 5: Handling JSON data
// void handle_json_data(connection& C) {
//     work W(C);
//
//     // Insert JSON data
//     W.exec("INSERT INTO json_table (data) VALUES ('{\"name\": \"John\", \"age\": 30}')");
//
//     // Query JSON data
//     result R = W.exec("SELECT data->>'name' as name, (data->>'age')::int as age FROM json_table");
//
//     for (const auto& row : R) {
//         cout << "Name: " << row["name"].as<string>()
//              << ", Age: " << row["age"].as<int>() << endl;
//     }
//
//     W.commit();
// }
//
//
//
// void create_tables(connection& C) {
//     work W(C);
//
//     // Create a normal table
//     W.exec("CREATE TABLE IF NOT EXISTS users ("
//            "id SERIAL PRIMARY KEY,"
//            "name VARCHAR(100) NOT NULL,"
//            "email VARCHAR(100) UNIQUE NOT NULL"
//            ")");
//
//     // Create a table with JSON column
//     W.exec("CREATE TABLE IF NOT EXISTS user_profiles ("
//            "user_id INTEGER PRIMARY KEY REFERENCES users(id),"
//            "profile JSONB NOT NULL"
//            ")");
//
//     W.commit();
//     cout << "Tables created" << endl;
// }
//
// void insert_and_query_json(connection& C) {
//     work W(C);
//
//     // Insert user data
//     W.exec("INSERT INTO users (name, email) VALUES ('John Doe', 'john@example.com') "
//            "ON CONFLICT (email) DO NOTHING");
//
//     // Get user ID
//     result R = W.exec("SELECT id FROM users WHERE email = 'john@example.com'");
//     int user_id = R[0][0].as<int>();
//
//     // Insert JSON data
//     string json_profile = R"({
//         "age": 30,
//         "city": "New York",
//         "interests": ["reading", "traveling", "photography"],
//         "is_active": true
//     })";
//
//     W.exec_params("INSERT INTO user_profiles (user_id, profile) VALUES ($1, $2::jsonb) "
//                   "ON CONFLICT (user_id) DO UPDATE SET profile = EXCLUDED.profile",
//                   user_id, json_profile);
//
//     // Query JSON data
//     cout << "Query JSON data:" << endl;
//     R = W.exec("SELECT u.name, p.profile->>'city' as city, p.profile->'interests' as interests "
//                "FROM users u JOIN user_profiles p ON u.id = p.user_id");
//
//     for (const auto& row : R) {
//         cout << "Name: " << row["name"].as<string>() << endl;
//         cout << "City: " << row["city"].as<string>() << endl;
//         cout << "Interests: " << row["interests"].as<string>() << endl;
//     }
//
//     W.commit();
// }
//
// void update_json(connection& C) {
//     work W(C);
//
//     // Update JSON data
//     W.exec_params("UPDATE user_profiles "
//                   "SET profile = jsonb_set(profile, '{age}', $1::jsonb) "
//                   "WHERE user_id = (SELECT id FROM users WHERE email = $2)",
//                   31, "john@example.com");
//
//     // Add element to JSON array
//     W.exec_params("UPDATE user_profiles "
//                   "SET profile = jsonb_set(profile, '{interests}', "
//                   "    (profile->'interests') || $1::jsonb) "
//                   "WHERE user_id = (SELECT id FROM users WHERE email = $2)",
//                   "\"cooking\"", "john@example.com");
//
//     // Query updated data
//     cout << "\nUpdated JSON data:" << endl;
//     result R = W.exec("SELECT profile FROM user_profiles "
//                       "WHERE user_id = (SELECT id FROM users WHERE email = 'john@example.com')");
//
//     if (!R.empty()) {
//         cout << R[0]["profile"].as<string>() << endl;
//     }
//
//     W.commit();
// }
//
// TEST_CASE( "db: pq" ) {
// 	try {
// 		// Connect to database
// 		auto conn = pqxx::connection( "dbname=loh user=raz password=justice hostaddr=127.0.0.1 port=5432" );
//
// 		if ( conn.is_open() ) {
// 			std::cout << "Successfully connected to database " << conn.dbname() << std::endl;
// 		}
// 		else {
// 			std::cout << "Cannot connect to database" << std::endl;
// 			return;
// 		}
//
// 		auto work = pqxx::work( conn );
//
// 		pqxx::result R = work.exec(
// 			"SELECT table_name "
// 			"FROM information_schema.tables "
// 			"WHERE table_schema = 'public' "
// 			"ORDER BY table_name;"
// 		);
//
// 		// Print results
// 		std::cout << "Tables in database:" << std::endl;
// 		for ( const auto &row: R ) {
// 			std::cout << row[0].as<std::string>() << std::endl;
// 		}
//
// 		// Commit transaction (although this is just read operation, maintain good practice)
// 		work.commit();
//
// 	}
// 	catch ( const std::exception &e ) {
// 		std::cerr << e.what() << std::endl;
// 	}
// }
