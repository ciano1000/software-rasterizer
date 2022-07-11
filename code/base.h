#include <stdint.h>

//NOTE	Compiler Context
#ifdef	__clang__
#define COMPILER_CLANG	1
#define COMPILER_VS		0
#define COMPILER_GCC	0
#elif	_MSC_VER
#define COMPILER_VS		1
#define COMPILER_CLANG	0
#define COMPILER_GCC	0
#elif	__GNUC__
#define COMPILER_GCC	1
#define COMPILER_CLANG	0
#define COMPILER_VS		0
#else
#error "Can't determine compiler."
#endif 

#ifdef	_WIN64
#define OS_WIN64	1
#define OS_LINUX	0
#endif

#ifdef __gnu_linux__
#define OS_LINUX	1
#define OS_WIN64	0
#endif

//types
typedef uint8_t 	u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;
#define b8 			u8
#define b16			u16
#define b32 		u32
#define b64 		u64
#define true		1
#define false		0
typedef int8_t  	s8;
typedef int16_t 	s16;
typedef int32_t 	s32;
typedef int64_t 	s64;
typedef float 		f32;
typedef double  	f64;

#define global		static
#define internal	static

#define Kilobytes(kb) (((u64)kb) << 10 )
#define Megabytes(mb) (((u64)mb) << 20 )
#define Gigabytes(gb) (((u64)gb) << 30 )
#define Terabytes(tb) (((u64)tb) << 40 )

//Hackers Delight Constants for floating point conversions
#define C_521 0x4338000000000000UL
#define C_52  0x4330000000000000UL
#define C_231 0x4B400000
#define C_23  0x4B00000

//Generic Macro Utilities - macros prepended with "_" are only intended for use within other macros
#define _TokenPaste_(x, y) x##y				//Concatenate as a token, not a string
#define _TokenCat_(x,y) TOKEN_PASTE(x,y)	//Concatenate preproc strings
#define _DeferLoop_(begin, end, var) for (u32 var = (begin, 0); !var; ++var, end)	//Replicate basic odin/jai/golang like "defer" functionality
#define ArrayCount(array) (sizeof(array) / sizeof(array[0])) //get num items in a static array

#define F32_SIGN_BIT 0x80000000

//TODO: this is getting messy...
inline s32 Sign(s32 x) {
	return ((x) >> 31) | ((u32)-x >> 31);
}

inline s32 Sign(f32 f) {
	s32 result = 0;
	return f > 0 ? 1 : (f < 0 ? -1 : 0);
}

#define Min(a, b) (a < b ? a : b)
#define Max(a, b) (a > b ? a : b)
#define ClampTop(val, clamp) Min(val, clamp)
#define ClampBottom(val, clamp) Max(val, clamp)

//TODO: use intrinsics for rounding by setting the rounding mode
/*NOTE: rounds positive numbers up, negative numbers down, pretty crude, will want to use intrinsics at some point
 * Will cause issues if adding/subtracting 1 causes overflow/underflow, unlikely in most cases but is possible
*/
inline u32 RoundAwayFromZeroF32ToU32(f32 input) {
	u32 result = 0;
	
	if (input > 0.0f) {
		result = (u32)(input + 1.0f);
	} else if(input < 0.0f){
		result = (u32)(input - 1.0f);
	}

	return result;
}

#define Pow2Align(value, alignment) (((value) + (alignment - 1)) &~ (alignment - 1))
#define Pow2AlignDown(value, alignment) (((value)) &~ (alignment - 1))
//Constants
#define UNIQUE_INT CAT(prefix, __COUNTER__)	//Technically not a constant but whatever

