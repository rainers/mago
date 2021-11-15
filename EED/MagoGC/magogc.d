// provide a GC instance working an a separate heap, so it
// does not interfere with locks inside the GC or stdlib

import core.sys.windows.windows;

extern(Windows)
BOOL _dllEntry(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
	switch (ulReason)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
		default:
			break;
	}
	return TRUE;
}

import core.gc.gcinterface;
static import core.memory;

__gshared HANDLE heap;
__gshared MagoGC mgc = new MagoGC;

extern(Windows)
export GC initGC()
{
	heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0x10000, 0);
	if (!heap)
		return null;

	return mgc;
}

extern(Windows)
export bool termGC()
{
	if (!heap)
		return false;

	HeapDestroy(heap);
	heap = null;
	return true;
}

class MagoGC : GC
{
	Object o;

	this()
	{
	}

	~this()
	{
	}

	// overload Object methods to avoid emitting symbols
	override string toString() { return null; }
	override size_t toHash() @trusted nothrow { return 0; }
	override int opCmp(Object o) { return -1; }
	override bool opEquals(Object o) { return false; }

	void enable()
	{
	}

	void disable()
	{
	}

	void collect() nothrow
	{
	}

	void collectNoStack() nothrow
	{
	}

	void minimize() nothrow
	{
	}

	uint getAttr(void* p) nothrow
	{
		return 0;
	}

	uint setAttr(void* p, uint mask) nothrow
	{
		return 0;
	}

	uint clrAttr(void* p, uint mask) nothrow
	{
		return 0;
	}

	void* malloc(size_t size, uint bits, const TypeInfo ti) nothrow
	{
		void* p = HeapAlloc(heap, 0, size);
		return p;
	}

	BlkInfo qalloc(size_t size, uint bits, scope const TypeInfo ti) nothrow
	{
		BlkInfo retval;
		retval.base = HeapAlloc(heap, 0, size);
		retval.size = size;
		retval.attr = bits;
		return retval;
	}

	void* calloc(size_t size, uint bits, const TypeInfo ti) nothrow
	{
		void* p = HeapAlloc(heap, HEAP_ZERO_MEMORY, size);
		return p;
	}

	void* realloc(void* p, size_t size, uint bits, const TypeInfo ti) nothrow
	{
		void* q = HeapReAlloc(heap, 0, p, size);
		return q;
	}

	size_t extend(void* p, size_t minsize, size_t maxsize, const TypeInfo ti) nothrow
	{
		return 0;
	}

	size_t reserve(size_t size) nothrow
	{
		return 0;
	}

	void free(void* p) nothrow @nogc
	{
		HeapFree(heap, 0, p);
	}

	void* addrOf(void* p) nothrow @nogc
	{
		return null;
	}

	size_t sizeOf(void* p) nothrow @nogc
	{
		return 0;
	}

	BlkInfo query(void* p) nothrow
	{
		return BlkInfo.init;
	}

	core.memory.GC.Stats stats() nothrow
	{
		return typeof(return).init;
	}

	core.memory.GC.ProfileStats profileStats() nothrow
	{
		return typeof(return).init;
	}

	void addRoot(void* p) nothrow @nogc
	{
	}

	void removeRoot(void* p) nothrow @nogc
	{
	}

	@property RootIterator rootIter() return @nogc
	{
		return &rootsApply;
	}

	private int rootsApply(scope int delegate(ref Root) nothrow dg)
	{
		return 0;
	}

	void addRange(void* p, size_t sz, const TypeInfo ti = null) nothrow @nogc
	{
	}

	void removeRange(void* p) nothrow @nogc
	{
	}

	@property RangeIterator rangeIter() return @nogc
	{
		return &rangesApply;
	}

	private int rangesApply(scope int delegate(ref Range) nothrow dg)
	{
		return 0;
	}

	void runFinalizers(const scope void[] segment) nothrow
	{
	}

	bool inFinalizer() nothrow
	{
		return false;
	}

	ulong allocatedInCurrentThread() nothrow
	{
		return 0;
	}
}

// we don't need type info, replace with null pointers
immutable:
pragma(mangle, "_D6Object7__ClassZ")             void* dummy1;
pragma(mangle, "_D14TypeInfo_Class6__vtblZ")     void* dummy2;
pragma(mangle, "_D14TypeInfo_Const6__vtblZ")     void* dummy3;
pragma(mangle, "_D15TypeInfo_Struct6__vtblZ")    void* dummy4;
pragma(mangle, "_D18TypeInfo_Interface6__vtblZ") void* dummy5;
pragma(mangle, "_D4core2gc11gcinterface2GC11__InterfaceZ") void* dummy6;
pragma(mangle, "_D16TypeInfo_Pointer6__vtblZ")   void* dummy7;
pragma(mangle, "_D10TypeInfo_v6__initZ")         void* dummy8;
pragma(mangle, "_D4core6memory2GC12ProfileStats6__initZ") ubyte[core.memory.GC.ProfileStats.sizeof] ProfileStats_init;
//pragma(mangle, "_D4core6memory12__ModuleInfoZ")  void* dummy8;
//_D4core2gc11gcinterface2GC11__InterfaceZ
//_D4core2gc11gcinterface12__ModuleInfoZ