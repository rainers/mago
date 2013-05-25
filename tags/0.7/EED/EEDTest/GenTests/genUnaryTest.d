/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

import std.stdio;
import std.random;

// DMD bugs:
//	- struct param in register
//	- unsigned shift
//	- assert in MulExp and DivExp::semantic

string	Prefix;
int		Id;


template IntVals(T)
{
	static T[] vals;

	static this()
	{
	static if ( !__traits( isFloating, T ) )
	{
		AddVal( T.min );
		AddVal( T.min + 1 );
		AddVal( cast(T) 2 );
		AddVal( cast(T) 1 );
		AddVal( cast(T) 0 );
		// TODO: AddVal( cast(T) ((cast(long) 1) << (T.size * 8 - 1)) );
		AddVal( T.max - 1 );
		AddVal( T.max );
/*
		Random rand;

		AddVal( cast(T) rand.front );
		rand.popFront();

		AddVal( cast(T) rand.front );
		rand.popFront();
*/
	}
	else
	{
		AddVal( T.nan );
		AddVal( T.infinity );
		AddVal( -T.infinity );
		AddVal( T.epsilon );
		AddVal( T.max );
		AddVal( T.min_normal );
		AddVal( cast(T) 0 );
		AddVal( cast(T) 1 );
		AddVal( cast(T) 2 );
		AddVal( cast(T) -1 );
		AddVal( cast(T) -2 );
		// TODO: maybe negate some of those above
		// TODO: maybe add some like 10 or 389
	}
	}

	void AddVal( T v )
	{
		vals.length = vals.length + 1;
		vals[ length - 1 ] = v;
	}
}

template Unary(T)
{
	void Unary( void function( T ) func )
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

template UnOp(T)
{
	void Negate( T t )
	{
		writeln( "    <negate>" );
		CastTerm( t );
		writeln( "    </negate>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( -t ))();
		PrintTerm( -t );
		writeln( "    </typedvalue>" );
	}

	static if ( __traits( isIntegral, T ) )
	{
		void BitNot( T t )
		{
			writeln( "    <bitnot>" );
			CastTerm( t );
			writeln( "    </bitnot>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( ~t ))();
			PrintTerm( ~t );
			writeln( "    </typedvalue>" );
		}
	}

	void Not( T t )
	{
		writeln( "    <not>" );
		CastTerm( t );
		writeln( "    </not>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( !t ))();
		PrintTerm( !t );
		writeln( "    </typedvalue>" );
	}

	void UnaryAdd( T t )
	{
		writeln( "    <unaryadd>" );
		CastTerm( t );
		writeln( "    </unaryadd>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( +t ))();
		PrintTerm( +t );
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
		else if ( is( X == ireal ) || is( X == idouble ) || is( X == ifloat ) )
		{
			writefln( "      <realvalue value=\"%ai\"/>", x );
		}
		else if ( __traits( isFloating, X ) )
		{
			writefln( "      <realvalue value=\"%a\"/>", x );
		}
		else if ( is( X == ulong ) || (is( X == long ) && (x < 0)) )
		{
			writefln( "      <intvalue value=\"%dUL\"/>", x );
		}
		else
		{
			writefln( "      <intvalue value=\"%d\"/>", x );
			//writefln( "      <intvalue value=\"0x%x\"/>", x );
		}
	}
}

template UnaryList(T...)
{
	void Operation( string opName )
	{
		int	code = 0;

		switch ( std.string.tolower( opName ) )
		{
		case "negate":	code = 0;	break;
		case "bitnot":	code = 1;	break;
		case "not":	code = 2;	break;
		case "unaryadd":code = 3;	break;
		}

		foreach ( t; T )
		{
			switch ( code )
			{
			case 0:	Unary!(t)( &UnOp!(t).Negate );	break;
			case 1:
				static if ( __traits( compiles, Unary!(t)( &UnOp!(t).BitNot ) ) )
					Unary!(t)( &UnOp!(t).BitNot );
				break;
			case 2:	Unary!(t)( &UnOp!(t).Not );	break;
			case 3:	Unary!(t)( &UnOp!(t).UnaryAdd );	break;
			}
		}
	}
}


void main( string[] args )
{
//	writeln( IntVals!int.vals );

//	Unary!(int, short)( &BinOp!(int, short).Add );
//	UnaryList!(byte, short).B2!(int, long, ifloat)();

	int		set = 0;
	string	op = null;

	if ( args.length >= 3 )
	{
		set = std.conv.parse!(int)( args[1] );
		op = args[2];

		if ( args.length >= 4 )
			Prefix = args[3];
	}

	writeln( "<test>" );

	if ( set == 1 )
	{
		// TODO: include bool
		UnaryList!(byte, ubyte, short, ushort, int, uint, long, ulong)
			.Operation( op );
	}
	else if ( set == 2 )
	{
		UnaryList!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)
			.Operation( op );
	}
	else if ( set == 4 )
	{
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
