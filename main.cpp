/*
 *	main.cpp
 *
 *	Created on	: 24. jul. 2015
 *	Author		: Peter Žužek
 *	License		: BSD
 */

#include "Debug.h"
#include "deletedir.h"
#include "tinydir.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

using std::string;
using std::vector;
using std::unordered_map;
using pos_t = std::remove_const<decltype(string::npos)>::type;
using sql_t = vector<unordered_map<string, string>>;

static const string NOT_INSERT = "X__NOTINSERT";
static const string TABLE_NAME = "X__TABLE";

vector<string> readFile(string filename) {
	DSELF();

	vector<string> file;
	std::ifstream database("sql\\kobo.sql");

	if(database.fail()) {
		Debug() << "Failed to open Kobo database";
		return file;
	}

	string line;

	while(std::getline(database, line)) {
		file.push_back(std::move(line));
	}

	return file;
}

bool startsWith(const string& haystack, const string& startNeedle) {
	return std::equal(startNeedle.begin(), startNeedle.end(), haystack.begin());
}

vector<string> parseKeys(const string& line, pos_t start, pos_t posValues) {
	vector<string> keys;
	auto prevComma = start;
	pos_t curComma;
	while(true) {
		++prevComma;

		curComma = line.find(',', prevComma);

		if(curComma > posValues) {
			keys.push_back(line.substr(prevComma, posValues - prevComma));
			break;
		}
		else {
			keys.push_back(line.substr(prevComma, curComma - prevComma));
		}

		prevComma = curComma;
	}
	return keys;
}

vector<string> parseValues(const string& line, pos_t start, const size_t size) {
	vector<string> values;
	values.reserve(size);
	auto prevComma = start;
	pos_t curComma;
	static const char apostrophe = '\'';

	while(true) {
		prevComma += 1;

		curComma = line.find(',', prevComma);

		if(curComma == string::npos) {
			curComma = line.find(')', prevComma);	// Actually last parenthesis
			values.push_back(line.substr(prevComma, curComma - prevComma));
			break;
		}
		else {
			if(line[prevComma] == apostrophe) {
				// Value is a string
				// Need to ignore commas and double apostrophes

				// Next separator could be apostrophe and comma
				string aposCommaSearch = string() + apostrophe + ',';

				// Also have to consider comma after double apostrophe
				string doubleAposCommaSearch = string() + apostrophe + aposCommaSearch;

				// Also have to consider comma after triple apostrophe
				string tripleAposCommaSearch = string() + apostrophe + doubleAposCommaSearch;

				pos_t aposComma;
				pos_t doubleAposComma;
				pos_t tripleAposComma;
				pos_t prevAposComma = prevComma;
				while(true) {
					++prevAposComma;

					aposComma = line.find(aposCommaSearch, prevAposComma);
					doubleAposComma = line.find(doubleAposCommaSearch, prevAposComma);
					tripleAposComma = line.find(tripleAposCommaSearch, prevAposComma);

					if(aposComma != doubleAposComma + 1 || doubleAposComma == tripleAposComma + 1) {
						// Proper closing apostrophe has been found
						break;
					}

					prevAposComma = aposComma;
				}

				// Or it's the last entry, so it ends with a parenthesis
				string aposParenthesisSearch = string() + apostrophe + ')';
				auto aposParenthesis = line.find(aposParenthesisSearch, prevComma + 1);

				// Move after closing apostrophe
#if MSVC_LOW
				curComma = min(aposComma, aposParenthesis) + 1;
#else
				curComma = std::min(aposComma, aposParenthesis) + 1;
#endif
			}

			values.push_back(line.substr(prevComma, curComma - prevComma));
		}

		prevComma = curComma;
	}

	return values;
}

unordered_map<string, string> parseSqlLine(const string& line) {
	unordered_map<string, string> sqlLine;

	static const string valuesString = ") VALUES (";

	pos_t firstParenthesis = line.find('(');
	pos_t posValues = line.find(valuesString, firstParenthesis);

	// Table name
	static const char backtick = '`';
	auto prevComma = line.find(backtick) + 1;
	auto curComma = line.find(backtick, prevComma);
	sqlLine[TABLE_NAME] = line.substr(prevComma, curComma - prevComma);

	// Keys
	auto keys = parseKeys(line, firstParenthesis, posValues);

	const auto size = keys.size();

	// Values
	auto values = parseValues(line, posValues + valuesString.size() - 1, size);

	// Entries
	for(pos_t i = 0; i < size; ++i) {
		sqlLine[keys[i]] = values[i];
	}

	return sqlLine;
}

sql_t parseSqlFile(const vector<string>& file) {
	DSELF();
	sql_t sql;
	sql.reserve(file.size());

	static const string insert = "INSERT";

	for(auto& line : file) {
		if(startsWith(line, insert)) {
			sql.push_back(parseSqlLine(line));
		}
		else {
			sql.emplace_back();
			sql.back().emplace(NOT_INSERT, line);
		}
	}

	return sql;
}

string trimApostrophes(string str) {
	return str.substr(1, str.length() - 2);
}

vector<string> getExpiredArticleIds(sql_t& sql) {
	DSELF();
	vector<string> ids;

	for(auto& line : sql) {
		auto it = line.find("MimeType");
		if(
			it == line.end()									||
			it->second != "'application/x-kobo-html+pocket'"	||
			line["ContentType"] != "'6'"						||
			line["IsDownloaded"] != "'false'"
		) {
			continue;
		}

		string id = line["ContentID"];
		ids.push_back(trimApostrophes(std::move(id)));
	}

	return ids;
}

sql_t removeArticleLines(const sql_t& sql, const vector<string>& ids) {
	DSELF();
	sql_t trimmedSql;

	for(auto& line : sql) {
		auto idIt = line.find("volumeId");
		if(idIt == line.end()) {
			auto contentTypeIt = line.find("ContentType");

			if(contentTypeIt == line.end()) {
				trimmedSql.push_back(line);
				continue;
			}
			else if(contentTypeIt->second == "'6'") {
				idIt = line.find("ContentID");
			}
			else if(contentTypeIt->second == "'9'") {
				idIt = line.find("BookID");
			}
			else {
				trimmedSql.push_back(line);
				continue;
			}
		}

		auto expiredIdIt = std::find(ids.begin(), ids.end(), trimApostrophes(idIt->second));
		if(expiredIdIt == ids.end()) {
			trimmedSql.push_back(line);
		}
	}

	return trimmedSql;
}

void getAllFilenames(string folder, vector<string>& filenames, const size_t recursionStep, const size_t maxRecursion) {
	if(recursionStep > maxRecursion) {
		return;
	}

	tinydir_dir dir;
	tinydir_open(&dir, folder.c_str());

	// Skip . and ..
	tinydir_next(&dir);
	tinydir_next(&dir);

	while(dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		string filename = folder + file.name;
		if(file.is_dir) {
			filename += '/';
		}
		filenames.push_back(filename);

		if(file.is_dir) {
			getAllFilenames(filename, filenames, recursionStep + 1, maxRecursion);
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
}

bool containsId(const string& filename, const vector<string>& ids, vector<bool>& deletedIds) {
	for(pos_t i = 0; i < ids.size(); ++i) {
		if(deletedIds[i]) {
			continue;
		}
		if(filename.find(ids[i]) != string::npos) {
			deletedIds[i] = true;
			return true;
		}
	}
	return false;
}

void deleteFolder(string folder) {
	Debug() << "--- DELETE" << folder;

#ifdef _WIN32
	std::replace(folder.begin(), folder.end(), '/', '\\');
	auto err = DeleteDirectory(folder);
	if(err) {
		Debug() << "--- ERROR DELETE" << err;
	}
#endif
}

void deleteArticles(vector<string> ids, const vector<string>& filenames) {
	DSELF();
	vector<bool> deletedIds(ids.size(), false);

	for(auto& folder : filenames) {
		if(containsId(folder, ids, deletedIds)) {
			deleteFolder(folder);
		}
	}
}

void deleteImages(const vector<string>& ids, const vector<string>& filenames) {
	DSELF();
	vector<bool> deletedIds(ids.size(), false);

	pos_t i = 0;
	while(i < filenames.size()) {
		//Debug() << "--- IMAGE" << filenames[i];

		if(containsId(filenames[i], ids, deletedIds)) {
			string folder = filenames[i - 1];
			deleteFolder(folder);

			// Skip files in this folder
			do {
				++i;
			}
			while(i < filenames.size() && startsWith(filenames[i], folder));
		}
		else {
			++i;
		}
	}
}

void deleteArticleFiles(const vector<string>& ids) {
	DSELF();
	vector<string> filenames;
	getAllFilenames(".kobo/articles/", filenames, 0, 0);
	deleteArticles(ids, filenames);

	filenames.clear();
	getAllFilenames(".kobo-images/", filenames, 0, 2);
	deleteImages(ids, filenames);
}

void writeNewSqlFile(string filename, sql_t& sql) {
	DSELF();
	std::ofstream file(filename);

	for(auto& line : sql) {
		auto it = line.find(NOT_INSERT);

		if(it != line.end()) {
			file << it->second << '\n';
		}
		else {
			it = line.find(TABLE_NAME);
			file << "INSERT INTO `" << it->second << "` ";
			line.erase(it);

			string keys;
			string values;

			for(auto& entry : line) {
				keys += entry.first + ',';
				values += entry.second + ',';
			}

			// Delete last commas
			keys[keys.length() - 1] = ' ';
			values[values.length() - 1] = ' ';

			file << '(' << keys << ") VALUES (" << values << ");\n";
		}
	}
}

int main(int argc, char** argv) {
	Debug() << "Cleaning Kobo Aura HD";

	sql_t sql;

	{
		auto file = readFile("sql\\kobo.sql");

		if(file.empty()) {
			Debug() << "Unable to open SQL file";
			return -1;
		}

		sql = parseSqlFile(file);
		Debug() << "--- ORG SQL SIZE" << sql.size();
	}

	auto ids = getExpiredArticleIds(sql);
	Debug() << "--- IDS SIZE" << ids.size();

	sql = removeArticleLines(sql, ids);
	Debug() << "--- NEW SQL SIZE" << sql.size();

	deleteArticleFiles(ids);

	writeNewSqlFile("sql\\kobo_new.sql", sql);

	Debug() << "Done";

	return 0;
}
