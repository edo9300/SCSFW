/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2017
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"
	Michael "mtheall" Theall

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/

#include <array>
#include <cstring>
#include <dirent.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "args.h"

using namespace std::string_view_literals;

static constexpr auto NDS_EXT = ".nds"sv;
static constexpr auto SRLDR_EXT = ".srldr"sv;
static constexpr auto ARG_EXT = ".argv"sv;
static constexpr auto EXT_EXT = ".ext"sv;
static constexpr auto EXT_DIR = "/nds"sv;
static constexpr auto SEPARATORS = "\n\r\t "sv;

/* Checks if s1 ends with s2, ignoring case.
   Returns true if it does, false otherwise.
 */
static bool strCaseEnd(std::string_view s1, std::string_view s2) {
	return (s1.size() >= s2.size() &&
			strcasecmp(s1.data() + s1.size() - s2.size(), s2.data()) == 0);
}

/* Parses the contents of the file given by filename into argarray. Arguments
   are tokenized based on whitespace.
 */
static bool parseArgFileAll(std::string_view filename, std::vector<std::string>& argarray) {
	FILE* argfile = fopen(filename.data(), "rb");
	if(!argfile) {
		return false;
	}

	char* line = nullptr;
	size_t lineSize = 0;
	int len = 0;
	while(((len = __getline(&line, &lineSize, argfile)) >= 0)) {
		std::string_view line_sv{ line, static_cast<std::string_view::size_type>(len) };
		// Find comment and end string there
		auto pos = line_sv.find_first_of('#');
		if(pos != std::string_view::npos) {
			line_sv.remove_suffix(pos - line_sv.size());
		}
		
		pos = line_sv.find_first_not_of(SEPARATORS);
		if(pos != std::string_view::npos) {
			line_sv.remove_prefix(pos);
		}
		
		while((pos = line_sv.find_first_of(SEPARATORS)) != std::string_view::npos) {
			auto substr = line_sv.substr(0, pos);
			argarray.emplace_back(substr);
			pos = line_sv.find_first_not_of(SEPARATORS, pos);
			if(pos == std::string_view::npos) {
				line_sv = "";
				break;
			}
			line_sv.remove_prefix(pos);
		}

		if(line_sv.size()) {
			argarray.emplace_back(line_sv);
		}
	}

	free(line);

	fclose(argfile);

	return argarray.size() > 0;
}

/* Parses the argument file given by filename and returns the NDS file that it
 * points to.
 */
static bool parseArgFileNds(std::string_view filename, std::string& ndsPath) {
	bool success = false;
	FILE* argfile = fopen(filename.data(), "rb");
	if(!argfile) {
		return false;
	}

	char* line = nullptr;
	size_t lineSize = 0;
	int len = 0;
	while((len = __getline(&line, &lineSize, argfile)) >= 0) {
		std::string_view line_sv{ line, static_cast<std::string_view::size_type>(len) };
		// Find comment and end string there
		auto pos = line_sv.find_first_of('#');
		if(pos != std::string_view::npos) {
			line_sv.remove_suffix(pos - line_sv.size());
		}

		pos = line_sv.find_first_not_of(SEPARATORS);
		if(pos != std::string_view::npos) {
			line_sv.remove_prefix(pos);
		}

		pos = line_sv.find_first_of(SEPARATORS);
		if(pos != std::string_view::npos) {
			line_sv = line_sv.substr(0, pos);
		}
		if(line_sv.size()) {
			success = true;
			ndsPath = line_sv;
			break;
		}
	}

	free(line);

	fclose(argfile);

	return success;
}

/* Converts a plain filename into an absolute path. If it's already an absolute
 * path, it is returned as-is. If basePath is nullptr, the current working directory
 * is used.
 * Returns true on success, false on failure.
 */
bool toAbsPath(std::string_view filename, std::string_view basePath, std::string& filePath) {
	// Copy existing absolute or empty paths
	if(filename.empty() || filename.starts_with('/') || filename.starts_with("fat:/")) {
		filePath = filename;
		return true;
	}

	if(basePath.empty()) {
		// Get current working directory (uses C-strings)
		std::array<char,PATH_MAX> cwd;
		if(getcwd(cwd.data(), cwd.size()) == nullptr) {
			// Path was too long, abort
			return false;
		}
		// Copy CWD into path
		filePath = cwd.data();
	} else {
		// Just copy the base path
		filePath = basePath;
	}

	// Ensure there's a path separator
	if(!filePath.ends_with('/')) {
		filePath += '/';
	}

	// Now append the filename
	filePath += filename;

	return true;
}

/* Convert a dataFilePath to the path of the ext file that specifies the
 * handler.
 * Returns true on success, false on failure
 */
static bool toExtPath(std::string_view dataFilePath, std::string& extFilePath) {
	// Figure out what the file extension is
	auto extPos = dataFilePath.rfind('.');
	if(extPos == std::string::npos) {
		return false;
	}

	extPos += 1;
	if(extPos >= dataFilePath.size()) {
		return false;
	}

	// Construct handler path from extension. Handlers are in the EXT_DIR and
	// end with EXT_EXT.
	auto ext = dataFilePath.substr(extPos);
	if(!toAbsPath(ext, EXT_DIR, extFilePath)) {
		return false;
	}

	extFilePath += EXT_EXT;

	return true;
}

bool argsNdsPath(std::string_view filePath, std::string& ndsPath) {
	if(strCaseEnd(filePath, NDS_EXT)) {
		ndsPath = filePath;
		return true;
	} else	if(strCaseEnd(filePath, ARG_EXT)) {
		return parseArgFileNds(filePath, ndsPath);
	} else {
		// This is a data file associated with a handler NDS by an ext file
		std::string extPath;
		if(!toExtPath(filePath, extPath)) {
			return false;
		}
		std::string ndsRelPath;
		if(!parseArgFileNds(extPath, ndsRelPath)) {
			return false;
		}
		// Handler is in EXT_DIR
		return toAbsPath(ndsRelPath, EXT_DIR, ndsPath);
	}

	return false;
}

bool argsFillArray(std::string_view filePath, std::vector<std::string>& argarray) {
	// Ensure argarray is empty
	argarray.clear();

	if(strCaseEnd(filePath, NDS_EXT)) {
		std::string absPath;
		if(!toAbsPath(filePath, {}, absPath)) {
			return false;
		}
		argarray.push_back(std::move(absPath));
	} else if(strCaseEnd(filePath, ARG_EXT)) {
		if(!parseArgFileAll(filePath, argarray)) {
			return false;
		}
		// Ensure argv[0] is absolute path
		std::string absPath;
		if(!toAbsPath(argarray[0], {}, absPath)) {
			return false;
		}
		std::swap(argarray[0], absPath);
	} else {
		// This is a data file associated with a handler NDS by an ext file
		std::string extPath;

		if(!toExtPath(filePath, extPath)) {
			return false;
		}

		// Read the arg file for the extension handler
		if(!parseArgFileAll(extPath, argarray)) {
			return false;
		}

		// Extension handler relative path is relative to EXT_DIR, not CWD
		std::string absPath;
		if(!toAbsPath(argarray[0], EXT_DIR, absPath)) {
			return false;
		}
		argarray[0] = absPath;

		// Add the data filename to the end. Its path is relative to CWD.
		if(!toAbsPath(filePath, {}, absPath)) {
			return false;
		}
		argarray.push_back(std::move(absPath));
	}

	return argarray.size() > 0 && strCaseEnd(argarray[0], NDS_EXT);
}

std::vector<std::string> argsGetExtensionList() {
	// Always supported files: NDS binaries and predefined argument lists
	std::vector extensionList{std::string{NDS_EXT},std::string{ARG_EXT}};

	// Get a list of extension files: argument lists associated with a file type
	auto* dir = opendir(EXT_DIR.data());
	if(dir) {
		for(auto* dirent = readdir(dir); dirent != nullptr; dirent = readdir(dir)) {
			// Add the name component of all files ending with EXT_EXT to the list
			if(dirent->d_type != DT_REG) {
				continue;
			}
			
			std::string_view name{dirent->d_name};

			if(!name.starts_with('.') && strCaseEnd(name, EXT_EXT)) {
				name.remove_suffix(EXT_EXT.size());
				extensionList.emplace_back(name);
			}
		}
		closedir(dir);
	}

	return extensionList;
}
