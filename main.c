/*
	DS3Remapper
	Copyright (C) 2014, Total_Noob

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <systemctrl.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("DS3Remapper", 0x1007, 1, 0);

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a);

#define HIJACK_FUNCTION(a, f, ptr) \
{ \
	u32 func = a; \
	static u32 patch_buffer[3]; \
	_sw(_lw(func), (u32)patch_buffer); \
	_sw(_lw(func + 4), (u32)patch_buffer + 8);\
	MAKE_JUMP((u32)patch_buffer + 4, func + 8); \
	_sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), func); \
	_sw(0, func + 4); \
	ptr = (void *)patch_buffer; \
}

int (* sceCtrlReadBuf)(SceCtrlData *pad, int nBufs, int a2, int mode);

int sceCtrlReadBufPatched(SceCtrlData *pad, int nBufs, int a2, int mode)
{
	/* If it is not an extended SceCtrlData structure */
	if(!(mode & 4))
	{
		/* A static buffer */
		static char buffer[64 * 48];

		/* Set k1 to zero to allow all buttons */
		int k1 = pspSdkSetK1(0);

		/* Call function with new arguments */
		int res = sceCtrlReadBuf((SceCtrlData *)buffer, nBufs, 1, mode | 4);

		/* Copy buffer to pad */
		int i;
		for(i = 0; i < nBufs; i++)
		{
			memcpy(&pad[i], buffer + (i * 48), sizeof(SceCtrlData));
		}	
		/* Restore k1 */
		pspSdkSetK1(k1);

		/* Return result */
		return res;
	}

	/* Call function with original arguments */
	return sceCtrlReadBuf(pad, nBufs, a2, mode);
}

int module_start(SceSize args, void *argp)
{
	/* Find ctrl.prx */
	SceModule2 *mod = sceKernelFindModuleByName("sceController_Service");

	/* Find function and hook it */
	int i;
	for(i = 0; i < mod->text_size; i += 4)
	{
		u32 addr = mod->text_addr + i;

		if(_lw(addr) == 0x35030104)
		{
			HIJACK_FUNCTION(addr - 0x1C, sceCtrlReadBufPatched, sceCtrlReadBuf);
			break;
		}
	}

	/* Clear caches */
	sceKernelDcacheWritebackAll();
	sceKernelIcacheClearAll();

	return 0;
}