#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#define VER_MESH	20
#define MAX_BONES	28
#define	MAX_GROUP	3

#define XE_NONE				0
#define XE_REFLECT			0x1
#define XE_OPACITY			0x2
#define XE_2SIDE			0x4
#define XE_SELF_ILLUMINATE	0x8
#define XE_HIGHLIGHT_OBJ	0x00002000

#define	MA_HIT		0x1
#define	MA_SOUND	0x2
#define	MA_QUAKE	0x4

#define	VER_BONE 4
#define	VER_MOTION	10
#define MAX_MDL_EVENT	8
#define MAX_ANIMATED_ELEMENTS	21

typedef uint8_t byte;
typedef uint16_t ushort;
typedef uint32_t uint;
typedef uint64_t ulong;
typedef uint64_t objid;

template<class T> void Delete(T*& val)
{
	delete val;
	val = nullptr;
}

template<class T> void DeleteArray(T*& val)
{
	delete[] val;
	val = nullptr;
}

enum EGMType
{
	GMT_ERROR = -1,
	GMT_NORMAL = 0,
	GMT_SKIN = 1,
	GMT_BONE = 2
};

enum EFillMode
{
	FILL_SOLID = 0,
	FILL_WIREFRAME = 1,
	FILL_POINT = 2
};

#endif // TYPES_H
