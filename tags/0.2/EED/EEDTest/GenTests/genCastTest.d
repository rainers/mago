/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

import std.stdio;


string	Prefix;
int		Id;


// TODO: why don't template specializations work?
template AllowCase(T, U)
{
	bool AllowCase()
	{
		// TODO: these should work, DMD does it different than the other cases, I don't think it should

		static if ( is( U == uint ) || is( U == dchar ) || is( U == int* ) )
		{
			if ( is( T == float ) || is( T == double ) || is( T == real ) || is( T == cfloat ) || is( T == cdouble ) )
				return false;
		}
		else static if ( is( U == ulong ) )
		{
			if ( is( T == float ) || is( T == double ) || is( T == cfloat ) || is( T == cdouble ) )
				return false;
		}

		// TODO: different problem with DMD, NaN is treated differently in different cases

		static if ( is( U == bool ) )
		{
			if ( is( T == ifloat ) || is( T == idouble ) || is( T == ireal ) || is( T == cfloat ) || is( T == cdouble ) || is( T == creal ) )
				return false;
		}

		// TODO: these are disabled because of precision issues. Resolve them.
		//	In an expression the floating point precision is kept at 80-bit, but at the end when the result is used
		//	as in assigned to a variable or passed as an argument, it's shrunk to the intended type.
		//	We don't do that at the end of expression evaluation.

		static if ( is( U == ifloat ) )
		{
			if ( is( T == ireal ) || is( T == creal ) )
				return false;
		}
		return true;
	}
}


template IntVals(T)
{
	static T[] vals;
	static string namePrefix;

	static this()
	{
// TODO: pointer, function pointer, enum

		static if ( is( T == bool ) )
		{
//			AddVal( false );
			AddVal( true );
			namePrefix = "lambda";
		}
		static if ( is( T == byte ) || is( T == ubyte ) )
		{
			AddVal( 0x1c );
			AddVal( cast(T) 0x9c );
			namePrefix = "omega";
		}
		static if ( is( T == short ) || is( T == ushort ) )
		{
			AddVal( 0x7cce );
			AddVal( cast(T) 0xfcce );
			namePrefix = "omega";
		}
		static if ( is( T == int ) || is( T == uint ) )
		{
			AddVal( 0x299d51d1 );
			AddVal( cast(T) 0xa99d51d1 );
			namePrefix = "omega";
		}
		static if ( is( T == long ) || is( T == ulong ) )
		{
			AddVal( 0x18982d445b6bc177L );
			AddVal( cast(T) 0x98982d445b6bc177L );
			namePrefix = "omega";
		}
		static if ( is( T == char ) )
		{
			AddVal( 0x1c );
			AddVal( cast(T) 0x9c );
			namePrefix = "omega";
		}
		static if ( is( T == wchar ) )
		{
			AddVal( 0x7cce );
			AddVal( cast(T) 0xfcce );
			namePrefix = "omega";
		}
		static if ( is( T == dchar ) )
		{
			AddVal( 0x60307 );
			AddVal( cast(T) 0xa99d51d6 );
			namePrefix = "omega";
		}
		static if ( is( T == int* ) )
		{
			AddVal( cast(T) 0x00402010 );
			AddVal( cast(T) 0x80402010 );
			namePrefix = "omega";
		}
		static if ( is( T == float ) || is( T == double ) || is( T == real ) )
		{
			AddVal( 0.0 );
			AddVal( 0.0154 );
			AddVal( 1.0 );
			AddVal( 3298.514 );
			AddVal( T.nan );
			AddVal( T.infinity );
			AddVal( -0.0 );
			AddVal( -0.0154 );
			AddVal( -1.0 );
			AddVal( -3298.514 );
			AddVal( -T.nan );
			AddVal( -T.infinity );
			namePrefix = "omega";
		}
		static if ( is( T == ifloat ) || is( T == idouble ) || is( T == ireal ) )
		{
			AddVal( 0.0i );
			AddVal( 0.0154i );
			AddVal( 1.0i );
			AddVal( 3298.514i );
			AddVal( T.nan );
			AddVal( T.infinity );
			AddVal( -0.0i );
			AddVal( -0.0154i );
			AddVal( -1.0i );
			AddVal( -3298.514i );
			AddVal( -T.nan );
			AddVal( -T.infinity );
			namePrefix = "omega";
		}
		static if ( is( T == cfloat ) || is( T == cdouble ) || is( T == creal ) )
		{
			static if ( is( T == cfloat ) ) { alias float R; alias ifloat I; }
			else static if ( is( T == cdouble ) ) { alias double R; alias idouble I; }
			else static if ( is( T == creal ) ) { alias real R; alias ireal I; }

			const auto nan = R.nan;
			const auto infinity = R.infinity;
			const auto inan = I.nan;
			const auto iinfinity = I.infinity;

			AddVal( 0.0 + 0.0i );
			AddVal( 0.0 + 0.0154i );
			AddVal( 0.0 + 1.0i );
			AddVal( 0.0 + 3298.514i );
			AddVal( 0.0 + inan );
			AddVal( 0.0 + iinfinity );
			AddVal( 0.0 + -0.0i );
			AddVal( 0.0 + -0.0154i );
			AddVal( 0.0 + -1.0i );
			AddVal( 0.0 + -3298.514i );
			AddVal( 0.0 + -inan );
			AddVal( 0.0 + -iinfinity );

			AddVal( 0.0154 + 0.0i );
			AddVal( 0.0154 + 0.0154i );
			AddVal( 0.0154 + 1.0i );
			AddVal( 0.0154 + 3298.514i );
			AddVal( 0.0154 + inan );
			AddVal( 0.0154 + iinfinity );
			AddVal( 0.0154 + -0.0i );
			AddVal( 0.0154 + -0.0154i );
			AddVal( 0.0154 + -1.0i );
			AddVal( 0.0154 + -3298.514i );
			AddVal( 0.0154 + -inan );
			AddVal( 0.0154 + -iinfinity );

			AddVal( 1.0 + 0.0i );
			AddVal( 1.0 + 0.0154i );
			AddVal( 1.0 + 1.0i );
			AddVal( 1.0 + 3298.514i );
			AddVal( 1.0 + inan );
			AddVal( 1.0 + iinfinity );
			AddVal( 1.0 + -0.0i );
			AddVal( 1.0 + -0.0154i );
			AddVal( 1.0 + -1.0i );
			AddVal( 1.0 + -3298.514i );
			AddVal( 1.0 + -inan );
			AddVal( 1.0 + -iinfinity );

			AddVal( 3298.514 + 0.0i );
			AddVal( 3298.514 + 0.0154i );
			AddVal( 3298.514 + 1.0i );
			AddVal( 3298.514 + 3298.514i );
			AddVal( 3298.514 + inan );
			AddVal( 3298.514 + iinfinity );
			AddVal( 3298.514 + -0.0i );
			AddVal( 3298.514 + -0.0154i );
			AddVal( 3298.514 + -1.0i );
			AddVal( 3298.514 + -3298.514i );
			AddVal( 3298.514 + -inan );
			AddVal( 3298.514 + -iinfinity );

			AddVal( nan + 0.0i );
			AddVal( nan + 0.0154i );
			AddVal( nan + 1.0i );
			AddVal( nan + 3298.514i );
			AddVal( nan + inan );
			AddVal( nan + iinfinity );
			AddVal( nan + -0.0i );
			AddVal( nan + -0.0154i );
			AddVal( nan + -1.0i );
			AddVal( nan + -3298.514i );
			AddVal( nan + -inan );
			AddVal( nan + -iinfinity );

			AddVal( infinity + 0.0i );
			AddVal( infinity + 0.0154i );
			AddVal( infinity + 1.0i );
			AddVal( infinity + 3298.514i );
			AddVal( infinity + inan );
			AddVal( infinity + iinfinity );
			AddVal( infinity + -0.0i );
			AddVal( infinity + -0.0154i );
			AddVal( infinity + -1.0i );
			AddVal( infinity + -3298.514i );
			AddVal( infinity + -inan );
			AddVal( infinity + -iinfinity );

			AddVal( -0.0 + 0.0i );
			AddVal( -0.0 + 0.0154i );
			AddVal( -0.0 + 1.0i );
			AddVal( -0.0 + 3298.514i );
			AddVal( -0.0 + inan );
			AddVal( -0.0 + iinfinity );
			AddVal( -0.0 + -0.0i );
			AddVal( -0.0 + -0.0154i );
			AddVal( -0.0 + -1.0i );
			AddVal( -0.0 + -3298.514i );
			AddVal( -0.0 + -inan );
			AddVal( -0.0 + -iinfinity );

			AddVal( -0.0154 + 0.0i );
			AddVal( -0.0154 + 0.0154i );
			AddVal( -0.0154 + 1.0i );
			AddVal( -0.0154 + 3298.514i );
			AddVal( -0.0154 + inan );
			AddVal( -0.0154 + iinfinity );
			AddVal( -0.0154 + -0.0i );
			AddVal( -0.0154 + -0.0154i );
			AddVal( -0.0154 + -1.0i );
			AddVal( -0.0154 + -3298.514i );
			AddVal( -0.0154 + -inan );
			AddVal( -0.0154 + -iinfinity );

			AddVal( -1.0 + 0.0i );
			AddVal( -1.0 + 0.0154i );
			AddVal( -1.0 + 1.0i );
			AddVal( -1.0 + 3298.514i );
			AddVal( -1.0 + inan );
			AddVal( -1.0 + iinfinity );
			AddVal( -1.0 + -0.0i );
			AddVal( -1.0 + -0.0154i );
			AddVal( -1.0 + -1.0i );
			AddVal( -1.0 + -3298.514i );
			AddVal( -1.0 + -inan );
			AddVal( -1.0 + -iinfinity );

			AddVal( -3298.514 + 0.0i );
			AddVal( -3298.514 + 0.0154i );
			AddVal( -3298.514 + 1.0i );
			AddVal( -3298.514 + 3298.514i );
			AddVal( -3298.514 + inan );
			AddVal( -3298.514 + iinfinity );
			AddVal( -3298.514 + -0.0i );
			AddVal( -3298.514 + -0.0154i );
			AddVal( -3298.514 + -1.0i );
			AddVal( -3298.514 + -3298.514i );
			AddVal( -3298.514 + -inan );
			AddVal( -3298.514 + -iinfinity );

			AddVal( -nan + 0.0i );
			AddVal( -nan + 0.0154i );
			AddVal( -nan + 1.0i );
			AddVal( -nan + 3298.514i );
			AddVal( -nan + inan );
			AddVal( -nan + iinfinity );
			AddVal( -nan + -0.0i );
			AddVal( -nan + -0.0154i );
			AddVal( -nan + -1.0i );
			AddVal( -nan + -3298.514i );
			AddVal( -nan + -inan );
			AddVal( -nan + -iinfinity );

			AddVal( -infinity + 0.0i );
			AddVal( -infinity + 0.0154i );
			AddVal( -infinity + 1.0i );
			AddVal( -infinity + 3298.514i );
			AddVal( -infinity + inan );
			AddVal( -infinity + iinfinity );
			AddVal( -infinity + -0.0i );
			AddVal( -infinity + -0.0154i );
			AddVal( -infinity + -1.0i );
			AddVal( -infinity + -3298.514i );
			AddVal( -infinity + -inan );
			AddVal( -infinity + -iinfinity );
/*
*/
			namePrefix = "omega";
		}
	}

	void AddVal( T v )
	{
		vals.length = vals.length + 1;
		vals[ $ - 1 ] = v;
	}

	void PrintName( T val )
	{
		write( namePrefix, '_', T.sizeof, '_', T.mangleof );

		static if ( __traits( isIntegral, T ) )
		{
			if ( (val >>> (T.sizeof * 8 - 1)) != 0 )
				write( "_H" );			// for "high bit"
		}
		else static if ( is( T == int* ) )
		{
			if ( (cast(ptrdiff_t) val >>> (T.sizeof * 8 - 1)) != 0 )
				write( "_H" );			// for "high bit"
		}
	}
}

template Binary(T, U)
{
	void Binary( void function( T ) func )
	{
		foreach ( t; IntVals!T.vals )
		{
			Id++;
			writefln( "  <verify id=\"%s_%d\">", Prefix, Id );

			func( t );

			writefln( "  </verify>" );
		}
	}
}

template BinOp(T, U)
{
	void Add( T t )
	{
		writeln( "    <cast>" );
		PrintType!U();

		if ( __traits( isFloating, T ) )
		{
			PrintTerm( t );
		}
		else
		{
			write( "      <id name=\"" );
			IntVals!T.PrintName( t );
			writeln( "\"/>" );
		}
		writeln( "    </cast>" );

		writeln( "    <typedvalue>" );
		PrintType!( U )();
		PrintTerm( cast(U) t );
		//auto a = cast(U) t;
		writeln( "    </typedvalue>" );
	}


	void PrintType(X)()
	{
		static if ( is( X == byte )
			|| is( X == ubyte )
			|| is( X == short )
			|| is( X == ushort )
			|| is( X == int )
			|| is( X == uint )
			|| is( X == long )
			|| is( X == ulong )
			|| is( X == bool )
			|| is( X == float )
			|| is( X == double )
			|| is( X == real )
			|| is( X == ifloat )
			|| is( X == idouble )
			|| is( X == ireal )
			|| is( X == cfloat )
			|| is( X == cdouble )
			|| is( X == creal ) 
			|| is( X == char )
			|| is( X == wchar )
			|| is( X == dchar ) )
		{
			writefln( "      <basictype name=\"%s\"/>", typeid( X ) );
		}
		else if ( is( X == int* ) )
		{
			writeln( "      <pointertype>" );
			writeln( "        <basictype name=\"int\"/>" );
			writeln( "      </pointertype>" );
		}
		else
		{
			writefln( "      <reftype name=\"%s\"/>", typeid( X ) );
		}
	}

	void CastTerm(X)( X x )
	{
		writeln( "      <cast>" );
		PrintType!X();
		PrintTerm( x );
		writeln( "      </cast>" );
	}

	void PrintTerm(X)( X x )
	{
		// using %a for floating point numbers, because hex format is more accurate

		static if ( is( X == creal ) || is( X == cdouble ) || is( X == cfloat ) )
		{
			writeln( "      <group>" );
			writeln( "        <add>" );
			writefln( "          <realvalue value=\"%a\"/>", x.re );
			writefln( "          <realvalue value=\"%ai\"/>", x.im );
			writeln( "        </add>" );
			writeln( "      </group>" );
		}
		else static if ( is( X == ireal ) || is( X == idouble ) || is( X == ifloat ) )
		{
			writefln( "      <realvalue value=\"%ai\"/>", x );
		}
		else static if ( __traits( isFloating, X ) )
		{
			writefln( "      <realvalue value=\"%a\"/>", x );
		}
		else static if ( is( X == int* ) )
		{
			writefln( "      <intvalue value=\"0x%pUL\"/>", x );
		}
		else
		{
			static if ( is( X == ulong ) || is( X == long ) )
			{
				if ( is( X == long ) && (x >= 0) )
					writefln( "      <intvalue value=\"%d\"/>", x );
				else
					writefln( "      <intvalue value=\"%dUL\"/>", x );
			}
			else
			{
				writefln( "      <intvalue value=\"%d\"/>", x );
				//writefln( "      <intvalue value=\"0x%x\"/>", x );
			}
		}
	}
}

template BinaryList(T...)
{
	void Operation(U...)( string opName )
	{
		foreach ( t; T )
		{
			foreach ( u; U )
			{
				if ( !AllowCase!(t, u) )
					continue;

				Binary!(t, u)( &BinOp!(t, u).Add );
			}
		}
	}
}


void main( string[] args )
{
//	writeln( IntVals!int.vals );

//	Binary!(int, short)( &BinOp!(int, short).Add );
//	BinaryList!(byte, short).B2!(int, long, ifloat)();

	int		set = 0;
	string	op = null;

	if ( args.length >= 2 )
	{
		set = std.conv.parse!(int)( args[1] );

		if ( args.length >= 3 )
			Prefix = args[2];
	}

	writeln( "<test>" );

	if ( set == 1 )
	{
		BinaryList!(bool, byte, ubyte, short, ushort, int, uint, long, ulong, char, wchar, dchar, int*)
			.Operation!(bool, byte, ubyte, short, ushort, int, uint, long, ulong, char, wchar, dchar, int*)( op );
	}
	else if ( set == 2 )
	{
		BinaryList!(bool, byte, ubyte, short, ushort, int, uint, long, ulong, char, wchar, dchar, int*)
			.Operation!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)( op );
	}
	else if ( set == 3 )
	{
// TODO: when not optimized, this causes a link error
//		BinaryList!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)
		BinaryList!(float, double, ifloat, idouble, ireal, cfloat, cdouble)
			.Operation!(bool, byte, ubyte, short, ushort, int, uint, long, ulong, char, wchar, dchar, int*)( op );
	}

	else if ( set == 4 )
	{
		BinaryList!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)
			.Operation!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)( op );
	}

	writeln( "</test>" );
}

template PrintNumberBytes( T )
{
	void PrintNumberBytes( T x )
	{
		byte*	b = cast(byte*) &x;

		for ( int i = 0; i < x.sizeof; i++ )
			std.stdio.writef( "%02x ", b[i] );

		std.stdio.writeln();
	}
}
