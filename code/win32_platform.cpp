/*	TODO List
 * Main Loop
 * Input
 * Memory
 * Read File
 * Hot reloading
*/
#define  _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <windows.h>
#include "base.h"
#include "base_math.h"

//TODO: pull these out into their own file
struct Win32_Buffer {
	BITMAPINFO info;
	void *memory;
	u32 width;
	u32 height;
	u32 stride;
	u32 bytesPerPixel;
};

struct String8 {
	char *data;
	u64 size;
};

struct Color {
	f32 r, g, b;
};

//global variables
global b64 gIsRunning = false;
global Win32_Buffer gBackbuffer = {};

#define WIN32_DEFAULT_PAGE_SIZE Kilobytes(4)
struct MemoryArena {
	void *base;
	u64 maxSize;
	u64 allocIdx;
	u64 commitIdx;
};

//TODO: split arena stuff into base and platform layers - fine for awhile
internal MemoryArena Win32_MemoryArenaInit(u64 size) {
	MemoryArena arena = {};

	arena.base = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
	arena.maxSize = size;

	return arena;
}

internal void* Win32_MemoryArenaPush(MemoryArena *arena, u64 size) {
	void *result = NULL;
	if (arena->allocIdx + size < arena->maxSize) {
		result = (u8 *)arena->base + arena->allocIdx;
		arena->allocIdx += size;

		if (arena->allocIdx > arena->commitIdx) {
			u64 newCommitIdx = ClampTop(Pow2Align(arena->allocIdx, WIN32_DEFAULT_PAGE_SIZE), arena->maxSize);
			u64 memoryToCommit = newCommitIdx - arena->commitIdx;

			VirtualAlloc((u8*)arena->base + arena->commitIdx, memoryToCommit, MEM_COMMIT, PAGE_READWRITE);
			arena->commitIdx = newCommitIdx;
		}
	}

	return result;
}

internal void Win32_MemoryArenaPopTo(MemoryArena *arena, u64 newIdx) {
	if (newIdx < arena->allocIdx) {
		
		//keep memory committed, in future maybe we should pass a flag to determine whether we want to also decommit the popped memory
		arena->allocIdx = newIdx;
	}
}

internal String8 Win32_ReadEntireFile(char *fileName, MemoryArena *arena) {
	String8 result = {};
	HANDLE fileHandle = CreateFileA((LPCSTR)fileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fileHandle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER fileSizeStruct = {};
        
		if(GetFileSizeEx(fileHandle, &fileSizeStruct)) {
			u64 fileSize = (u64)fileSizeStruct.QuadPart + 1;
			result.size = (u64)fileSize;
            
			void *fileBuffer = Win32_MemoryArenaPush(arena, fileSize);
			result.data = (char *)fileBuffer;
            
			u32 bytesRead = 0;
			if(ReadFile(fileHandle, fileBuffer, (DWORD)fileSize, (LPDWORD)&bytesRead, NULL)) {
				result.size = bytesRead;
				result.data[fileSize - 1] = 0;
			}
		}
		CloseHandle(fileHandle);
	}
	return result;
}

internal void Win32_GetWindowDimensions(HWND hwnd, u32 *width, u32 *height) {
	RECT rect = {};
	GetWindowRect(hwnd, &rect);
	*width = rect.right - rect.left;
	*height = rect.bottom - rect.top;
}

internal void Win32_ClearBuffer(Color color) {
	u8 *row = (u8 *)gBackbuffer.memory;
	for (u32 y = 0; y < gBackbuffer.height; y++) {
		u32 *pixel = (u32 *)row;
		for (u32 x = 0; x < gBackbuffer.width; x++) {
			*pixel = (0 << 24) | (((u8)color.r) << 16) | (((u8)color.g) << 8) | ((u8)color.b);
			pixel++;
		}

		row += gBackbuffer.stride;
	}
}

internal void Win32_DrawPoint(V2 pos, Color color) {
	u32 *pixel  = ((u32*)gBackbuffer.memory) + ((u32)pos.x + (gBackbuffer.width * (u32)pos.y));
	*pixel = (0 << 24) | (((u8)color.r) << 16) | (((u8)color.g) << 8) | ((u8)color.b);
}

//TODO: Move these to a renderer eventually, also refine them if needed.
internal void Win32_DrawRect(V4 rect, Color color) {
	//ensure we aren't overflowing the buffer...
	//assuming we aren't doing anything silly like having x0 > gBackbuffer.width, or x0 being > x1
	ClampBottom(rect.x0, 0);
	ClampBottom(rect.y0, 0);
	ClampTop(rect.x1, gBackbuffer.width - 1);
	ClampTop(rect.y1, gBackbuffer.height - 1);

	//TODO: correctly handle rendering floating point coordinates by interpolating by the pixel coverage %

	//note above, really not handling this correctly in terms of floating point rounding...
	for (u32 y = (u32)rect.y0; y <= (u32)rect.y1; y++) {
		u32 rowIdx = gBackbuffer.width * y;
		for (u32 x = (u32)rect.x0; x <= (u32)rect.x1; x++) {
			u32 *pixel = (u32*)gBackbuffer.memory + (rowIdx + x);
			*pixel = (0 << 24) | (((u8)color.r) << 16) | (((u8)color.g) << 8) | ((u8)color.b);
		}
	}
	
}
//NOTE: This is just a super hacky simple way that makes sense to me, will obviously improve it
internal void Win32_DrawLine(V2 p0, V2 p1, Color color) {
	V2 lineDir = Normalize(p1 - p0);
	s32 xDir = Sign(lineDir.x);
	s32 yDir = Sign(lineDir.y);

	//NOTE: not handling lines outside screen boundaries, could currently cause writing outside buffer
	
	V2 currentPixel = p0;
	while (currentPixel != p1) {
		u32 *pixel = (u32 *)gBackbuffer.memory + ((u32)currentPixel.x + ((u32)currentPixel.y * gBackbuffer.width));
		*pixel = (((u8)color.r) << 16) | (((u8)color.g) << 8) | ((u8)color.b);

		V2 horizontalPixel = {currentPixel.x + xDir, currentPixel.y};
		V2 verticalPixel = {currentPixel.x, currentPixel.y + yDir};

		V2 xA = horizontalPixel - p0;
		V2 yA = verticalPixel - p0;
		f32 xDot = Dot(xA, lineDir);
		f32 yDot = Dot(yA, lineDir);

		f32 xPixelDistanceToLine = (f32)sqrt(Dot(xA, xA) - (xDot * xDot));
		f32 yPixelDistanceToLine = (f32)sqrt(Dot(yA, yA) - (yDot * yDot));

		currentPixel = xPixelDistanceToLine < yPixelDistanceToLine ? horizontalPixel : verticalPixel;
	}
}

//TODO: have this just be rendered as part of the normal rasterization pipeline
//NOTE: technique used for this is essentially just rasterization but on a rectangle instead of a triangle
internal void Win32_DrawLine(V2 p0, V2 p1, f32 thickness) {
	V2 lineDir = Normalize(p1 - p0);
	
	f32 halfThickness = thickness / 2;

	//Step 1, create the rect, proof for finding the 2 perpendicular vectors to a 2D vector, given only the vector is in slipbox
	V2 vec0 = {lineDir.y, -lineDir.x};
	vec0 = (vec0 * halfThickness);

	V2 vec1 = {-lineDir.y, lineDir.x};
	vec1 = (vec1 * halfThickness);

	//four points of the rectangle
	//assumed positions for clarity
	V2 bP0 = vec0 + p0; //top left
	V2 bP1 = vec1 + p0; //bottom left
	V2 bP2 = vec1 + p1; //bottom right
	V2 bP3 = vec0 + p1; //bottom right

	//Calculate bounding box as a rasterization optimization - rounded to correct pixel boundary
	u32 x0 = RoundAwayFromZeroF32ToU32(Min(Min(bP0.x, bP1.x), Min(bP2.x, bP3.x)));
	u32 y0 = RoundAwayFromZeroF32ToU32(Min(Min(bP0.y, bP1.y), Min(bP2.y, bP3.y)));
	u32 x1 = RoundAwayFromZeroF32ToU32(Max(Max(bP0.x, bP1.x), Max(bP2.x, bP3.x)));
	u32 y1 = RoundAwayFromZeroF32ToU32(Max(Max(bP0.y, bP1.y), Max(bP2.y, bP3.y)));

	//verify vertex ordering, calculate z component of (p1-p0) x (p2-p1), result should be positive
	V2 p0ToP1 = bP1 - bP0;
	V2 p1ToP2 = bP2 - bP1;
	V2 p2ToP3 = bP3 - bP2;
	V2 p3ToP0 = bP0 - bP3;
	
	/*we don't actually have to do this orientation check, 
	 * since we set our points based on the line dir, orientation is automatically taken care of.
	 * Just swap the order of our CrossZ arguments(because y is down in our backbuffer)
	 */
	f32 orientation_check = CrossZ(p0ToP1, p1ToP2);

	//swap order
	if (orientation_check < 0) {
		V2 temp = bP1;
		bP1 = bP3;
		bP3 = temp;

		p0ToP1 = bP1 - bP0;
		p1ToP2 = bP2 - bP1;
		p2ToP3 = bP3 - bP2;
		p3ToP0 = bP0 - bP3;
	}

	Color color = {255, 0, 0};

	//loop through bounding box and check each point for intersection.
	for (u32 y = y0; y <= y1; y++) {
		u32 rowIndex = gBackbuffer.width * y;
		for (u32 x = x0; x <= x1; x++) {
			u32 *pixel = (u32*)gBackbuffer.memory + (rowIndex + x);

			V2 p = { (f32)x, (f32)y };

			V2 pRelative = p - bP0;
			f32 edge1Test = CrossZ(p0ToP1, pRelative);

			pRelative = p - bP1;
			f32 edge2Test = CrossZ(p1ToP2, pRelative);

			pRelative = p - bP2;
			f32 edge3Test = CrossZ(p2ToP3, pRelative);

			pRelative = p - bP3;
			f32 edge4Test = CrossZ(p3ToP0, pRelative);

			if ((edge1Test >= 0.0f) && (edge2Test >= 0.0f) && (edge3Test >= 0.0f) && (edge4Test >= 0.0f)) {
				*pixel = (0 << 24) | (((u8)color.r) << 16) | (((u8)color.g) << 8) | ((u8)color.b);
			}
		}
	}
}

internal LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LRESULT result = NULL;

	switch (msg) {
		case WM_CLOSE: {
			gIsRunning = false;
		} break;
		case WM_DESTROY: {
			gIsRunning = false;
		} break;
		case WM_SIZE: {
			if (gIsRunning) {
				Win32_GetWindowDimensions(hwnd, &gBackbuffer.width, &gBackbuffer.height);
				gBackbuffer.stride = gBackbuffer.width * gBackbuffer.bytesPerPixel;
			}
			//TODO: investigate if constantly freeing and commiting on resize has a negative perf impact
			BITMAPINFO bitmapInfo = {};
			bitmapInfo.bmiHeader.biSize        = sizeof(bitmapInfo.bmiHeader);
			bitmapInfo.bmiHeader.biWidth       = gBackbuffer.width;
			bitmapInfo.bmiHeader.biHeight        = -(s32)gBackbuffer.height;
			bitmapInfo.bmiHeader.biPlanes      = 1;
			bitmapInfo.bmiHeader.biBitCount    = WORD(gBackbuffer.bytesPerPixel * 8);
			bitmapInfo.bmiHeader.biCompression = BI_RGB;			

			gBackbuffer.info = bitmapInfo;

			//TODO: look into the starting address, page size etc.
			if(gBackbuffer.memory)
				VirtualFree(gBackbuffer.memory, 0, MEM_RELEASE);
			gBackbuffer.memory = VirtualAlloc(0, gBackbuffer.stride * gBackbuffer.height, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		} break;
		default: {
			result = DefWindowProc(hwnd, msg, wParam, lParam);
		} break;
	}

	return result;
}

INT  WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
	char *wcName = "Default Window Class";

	WNDCLASS wc = {};
	wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = wcName;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	
	if (RegisterClassA(&wc)) {
		char *windowName = APP_NAME;
		auto hwnd = CreateWindowExA(
			NULL,
			wcName,
			windowName,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
			NULL,
			NULL,
			hInstance,
			NULL
		);

		if (hwnd) {
			HDC hdc = GetDC(hwnd);
			ShowWindow(hwnd,nCmdShow);

			//fill in the backbuffers width and height, could also set a default instead of querying from windows
			Win32_GetWindowDimensions(hwnd, &gBackbuffer.width, &gBackbuffer.height);
			gBackbuffer.bytesPerPixel = 4;
			gBackbuffer.stride = gBackbuffer.width * gBackbuffer.bytesPerPixel;

			BITMAPINFO bitmapInfo = {};
			bitmapInfo.bmiHeader.biSize        = sizeof(bitmapInfo.bmiHeader);
			bitmapInfo.bmiHeader.biWidth       = gBackbuffer.width;
			bitmapInfo.bmiHeader.biHeight        = -(s32)gBackbuffer.height;
			bitmapInfo.bmiHeader.biPlanes      = 1;
			bitmapInfo.bmiHeader.biBitCount    = WORD(gBackbuffer.bytesPerPixel * 8);
			bitmapInfo.bmiHeader.biCompression = BI_RGB;			

			gBackbuffer.info = bitmapInfo;

			//TODO: look into the starting address, page size etc.
			gBackbuffer.memory = VirtualAlloc(0, gBackbuffer.stride * gBackbuffer.height, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			gIsRunning = true;
			while (gIsRunning) {
				Win32_ClearBuffer({255.0f, 255.0f, 255.0f});
				//Test rect drawing
				//Win32_DrawRect( { 200, 200, 400, 400 }, {255.0f, 0.0f, 0.0f});
				//Win32_DrawLine( { 100, 100 }, { 800, 200 }, {255.0f, 0.0f, 0.0f});
				Win32_DrawLine( { 100, 200 }, { 400, 100 }, 50.0f);

				//NOTE: if we wanted the backbuffer to be smaller than the screen res and then scale it up, then this would be different
				StretchDIBits(
					hdc,
					0, 0, gBackbuffer.width, gBackbuffer.height,
					0, 0, gBackbuffer.width, gBackbuffer.height,
					gBackbuffer.memory,
					&gBackbuffer.info,
					DIB_RGB_COLORS,
					SRCCOPY
				);

				MSG msg = {};
				while (PeekMessageA(&msg, hwnd, NULL, NULL, PM_REMOVE)) {
					if(msg.message == WM_QUIT) {
						gIsRunning = false;
					}

					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
	}


	return 0;

}