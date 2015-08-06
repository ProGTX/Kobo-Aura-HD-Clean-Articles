#pragma once
// http://stackoverflow.com/a/10836193/793006

#ifdef _WIN32

#include <conio.h>
#include <string>
#include <windows.h>

int DeleteDirectory(
	const std::string&	refcstrRootDirectory,
	bool				bDeleteSubdirectories = true
) {
	bool			bSubdirectory = false;       // Flag, indicating whether
	// subdirectories have been found
	HANDLE			hFile;                       // Handle to directory
	std::string		strFilePath;                 // Filepath
	std::string		strPattern;                  // Pattern
	WIN32_FIND_DATA	FileInformation;             // File information


	strPattern = refcstrRootDirectory + "\\*.*";
	hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
	if(hFile != INVALID_HANDLE_VALUE) {
		do {
			if(FileInformation.cFileName[0] != '.') {
				strFilePath.erase();
				strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;

				if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if(bDeleteSubdirectories) {
						// Delete subdirectory
						int iRC = DeleteDirectory(strFilePath, bDeleteSubdirectories);
						if(iRC) {
							return iRC;
						}
					}
					else {
						bSubdirectory = true;
					}
				}
				else {
					// Set file attributes
					if(::SetFileAttributes(strFilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE) {
						return ::GetLastError();
					}

					// Delete file
					if(::DeleteFile(strFilePath.c_str()) == FALSE) {
						return ::GetLastError();
					}
				}
			}
		}
		while(::FindNextFile(hFile, &FileInformation) == TRUE);

		// Close handle
		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if(dwError != ERROR_NO_MORE_FILES) {
			return dwError;
		}
		else {
			if(!bSubdirectory) {
				// Set directory attributes
				if(::SetFileAttributes(refcstrRootDirectory.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE) {
					return ::GetLastError();
				}

				// Delete directory
				if(::RemoveDirectory(refcstrRootDirectory.c_str()) == FALSE) {
					return ::GetLastError();
				}
			}
		}
	}

	return 0;
}

#endif
