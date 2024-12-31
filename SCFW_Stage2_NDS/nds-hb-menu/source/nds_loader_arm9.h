/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2010
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy

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

#ifndef NDS_LOADER_ARM9_H
#define NDS_LOADER_ARM9_H


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	RUN_NDS_OK = 0,
	RUN_NDS_STAT_FAILED,
	RUN_NDS_GETCWD_FAILED,
	RUN_NDS_PATCH_DLDI_FAILED,
} eRunNdsRetCode;

#define LOAD_DEFAULT_NDS 0

eRunNdsRetCode runNds(const void* loader, u32 loaderSize, u32 cluster, bool initDisc, bool dldiPatchNds, int argc, const char** argv);

eRunNdsRetCode runNdsFile(const char* filename, int argc, const char** argv);

#ifdef __cplusplus

#include <vector>

extern "C++"
	template<typename T> int runNdsFile(const T & argarray) {
	std::vector<const char*> c_args;
	for(const auto& arg : argarray) {
		c_args.push_back(arg.data());
	}
	return runNdsFile(c_args[0], c_args.size(), &c_args[0]);
}
#endif

bool installBootStub();

#ifdef __cplusplus
}
#endif

#endif // NDS_LOADER_ARM7_H