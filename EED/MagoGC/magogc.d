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
import core.thread.threadbase : ThreadBase;
static import core.memory;

__gshared HANDLE heap;
__gshared MagoGCInterface!2_108 mgc2_108 = new MagoGC!2_108;
__gshared MagoGCInterface!2_109 mgc2_109 = new MagoGC!2_109;
__gshared MagoGCInterface!2_111 mgc2_111 = new MagoGC!2_111;
__gshared MagoGCInterface!2_112 mgc2_112 = new MagoGC!2_112;

extern(Windows)
export MagoGCInterface!2_108 initGC_2_108()
{
	heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0x10000, 0);
	if (!heap)
		return null;

	return mgc2_108;
}

extern(Windows)
export MagoGCInterface!2_109 initGC_2_109()
{
	heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0x10000, 0);
	if (!heap)
		return null;

	return mgc2_109;
}

extern(Windows)
export MagoGCInterface!2_111 initGC_2_111()
{
	heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0x10000, 0);
	if (!heap)
		return null;

	return mgc2_111;
}

extern(Windows)
export MagoGCInterface!2_112 initGC_2_112()
{
	heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0x10000, 0);
	if (!heap)
		return null;

	return mgc2_112;
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

interface MagoGCInterface(int ver)
{
	void enable();
	void disable();
	void collect() nothrow;
	// method removed in dmd 2.109
	static if(ver < 2_109) void collectNoStack() nothrow;

	void minimize() nothrow;
	uint getAttr(void* p) nothrow;
	uint setAttr(void* p, uint mask) nothrow;
	uint clrAttr(void* p, uint mask) nothrow;
	void* malloc(size_t size, uint bits, const TypeInfo ti) nothrow;
	BlkInfo qalloc(size_t size, uint bits, scope const TypeInfo ti) nothrow;
	void* calloc(size_t size, uint bits, const TypeInfo ti) nothrow;
	void* realloc(void* p, size_t size, uint bits, const TypeInfo ti) nothrow;
	size_t extend(void* p, size_t minsize, size_t maxsize, const TypeInfo ti) nothrow;
	size_t reserve(size_t size) nothrow;
	void free(void* p) nothrow @nogc;
	void* addrOf(void* p) nothrow @nogc;
	size_t sizeOf(void* p) nothrow @nogc;
	BlkInfo query(void* p) nothrow;
	core.memory.GC.Stats stats() nothrow;
	core.memory.GC.ProfileStats profileStats() nothrow;
	void addRoot(void* p) nothrow @nogc;
	void removeRoot(void* p) nothrow @nogc;
	@property RootIterator rootIter() return @nogc;
	void addRange(void* p, size_t sz, const TypeInfo ti = null) nothrow @nogc;
	void removeRange(void* p) nothrow @nogc;
	@property RangeIterator rangeIter() return @nogc;
	void runFinalizers(const scope void[] segment) nothrow;
	bool inFinalizer() nothrow;
	ulong allocatedInCurrentThread() nothrow;

	static if(ver >= 2_111)
	{
		void[] getArrayUsed(void *ptr, bool atomic = false) nothrow;
		bool expandArrayUsed(void[] slice, size_t newUsed, bool atomic = false) nothrow @safe;
		size_t reserveArrayCapacity(void[] slice, size_t request, bool atomic = false) nothrow @safe;
		bool shrinkArrayUsed(void[] slice, size_t existingUsed, bool atomic = false) nothrow;
	}
	static if(ver >= 2_112)
	{
		void initThread(ThreadBase thread) nothrow @nogc;
		void cleanupThread(ThreadBase thread) nothrow @nogc;
	}
}

// verify GC interface is matching
// pragma(msg, "GC:   ", __traits(allMembers, GC));
// pragma(msg, "2_109:", __traits(allMembers, MagoGC!(2_109)));
// pragma(msg, "2_108:", __traits(allMembers, MagoGC!(2_108)));
// pragma(msg, "2_111:", __traits(allMembers, MagoGC!(2_111)));
pragma(msg, "2_112:", __traits(allMembers, MagoGC!(2_112)));

version (LDC) static assert(__VERSION__ >= 2_111);
else static assert(__VERSION__ >= 2_112);

enum GC_members = __traits(allMembers, GC);
static assert(GC_members == __traits(allMembers, MagoGCInterface!(__VERSION__)));

class MagoGC(int ver) : MagoGCInterface!(ver)
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

	override void enable()
	{
	}

	override void disable()
	{
	}

	override void collect() nothrow
	{
	}

	// method removed in dmd 2.109
	static if(ver < 2_109)
	{
		override void collectNoStack() nothrow
		{
		}
	}

	override void minimize() nothrow
	{
	}

	override uint getAttr(void* p) nothrow
	{
		return 0;
	}

	override uint setAttr(void* p, uint mask) nothrow
	{
		return 0;
	}

	override uint clrAttr(void* p, uint mask) nothrow
	{
		return 0;
	}

	override void* malloc(size_t size, uint bits, const TypeInfo ti) nothrow
	{
		void* p = HeapAlloc(heap, 0, size);
		return p;
	}

	override BlkInfo qalloc(size_t size, uint bits, scope const TypeInfo ti) nothrow
	{
		BlkInfo retval;
		retval.base = HeapAlloc(heap, 0, size);
		retval.size = size;
		retval.attr = bits;
		return retval;
	}

	override void* calloc(size_t size, uint bits, const TypeInfo ti) nothrow
	{
		void* p = HeapAlloc(heap, HEAP_ZERO_MEMORY, size);
		return p;
	}

	override void* realloc(void* p, size_t size, uint bits, const TypeInfo ti) nothrow
	{
		void* q = HeapReAlloc(heap, 0, p, size);
		return q;
	}

	override size_t extend(void* p, size_t minsize, size_t maxsize, const TypeInfo ti) nothrow
	{
		return 0;
	}

	override size_t reserve(size_t size) nothrow
	{
		return 0;
	}

	override void free(void* p) nothrow @nogc
	{
		HeapFree(heap, 0, p);
	}

	override void* addrOf(void* p) nothrow @nogc
	{
		return null;
	}

	override size_t sizeOf(void* p) nothrow @nogc
	{
		return 0;
	}

	override BlkInfo query(void* p) nothrow
	{
		return BlkInfo.init;
	}

	override core.memory.GC.Stats stats() nothrow
	{
		return typeof(return).init;
	}

	override core.memory.GC.ProfileStats profileStats() nothrow
	{
		return typeof(return).init;
	}

	override void addRoot(void* p) nothrow @nogc
	{
	}

	override void removeRoot(void* p) nothrow @nogc
	{
	}

	override @property RootIterator rootIter() return @nogc
	{
		return &rootsApply;
	}

	private int rootsApply(scope int delegate(ref Root) nothrow dg)
	{
		return 0;
	}

	override void addRange(void* p, size_t sz, const TypeInfo ti = null) nothrow @nogc
	{
	}

	override void removeRange(void* p) nothrow @nogc
	{
	}

	override @property RangeIterator rangeIter() return @nogc
	{
		return &rangesApply;
	}

	private int rangesApply(scope int delegate(ref Range) nothrow dg)
	{
		return 0;
	}

	override void runFinalizers(const scope void[] segment) nothrow
	{
	}

	override bool inFinalizer() nothrow
	{
		return false;
	}

	override ulong allocatedInCurrentThread() nothrow
	{
		return 0;
	}

	static if(ver >= 2_111)
	{
		void[] getArrayUsed(void *ptr, bool atomic = false) nothrow
		{
			return null;
		}
		bool expandArrayUsed(void[] slice, size_t newUsed, bool atomic = false) nothrow @safe
		{
			return false;
		}

		size_t reserveArrayCapacity(void[] slice, size_t request, bool atomic = false) nothrow @safe
		{
			return 0;
		}

		bool shrinkArrayUsed(void[] slice, size_t existingUsed, bool atomic = false) nothrow
		{
			return false;
		}
	}
	static if(ver >= 2_112)
	{
		override void initThread(ThreadBase thread) nothrow @nogc
		{
		}
		override void cleanupThread(ThreadBase thread) nothrow @nogc
		{
		}
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
