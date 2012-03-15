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
		// TODO: maybe negate some of those above
		// TODO: maybe add some like 10 or 389
	}
	}

	void AddVal( T v )
	{
		vals.length = vals.length + 1;
		vals[ $ - 1 ] = v;
	}
}

template Binary(T, U)
{
	void Binary( void function( T, U ) func )
	{
		foreach ( t; IntVals!T.vals )
		{
			foreach ( u; IntVals!U.vals )
			{
			// TODO: make this automatic (for div and mod only)
				if ( func == &BinOp!(T, U).Div )
				{
					if ( __traits( isIntegral, T ) && __traits( isIntegral, U ) && (u == 0) )
						continue;
				}
				static if ( __traits( compiles, &BinOp!(T, U).Mod ) )
				{
					if ( func == &BinOp!(T, U).Mod )
					{
						if ( __traits( isIntegral, T ) && __traits( isIntegral, U ) && (u == 0) )
							continue;
					}
				}

				Id++;
				writefln( "  <verify id=\"%s_%d\">", Prefix, Id );

				func( t, u );

				writefln( "  </verify>" );
			}
		}
	}
}

template BinOp(T, U)
{
	void Add( T t, U u )
	{
		writeln( "    <add>" );
		CastTerm( t );
		CastTerm( u );
		writeln( "    </add>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( t + u ))();
		PrintTerm( t + u );
		writeln( "    </typedvalue>" );
	}

	void Sub( T t, U u )
	{
		writeln( "    <sub>" );
		CastTerm( t );
		CastTerm( u );
		writeln( "    </sub>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( t - u ))();
		PrintTerm( t - u );
		writeln( "    </typedvalue>" );
	}

	void Mul( T t, U u )
	{
		writeln( "    <mul>" );
		CastTerm( t );
		CastTerm( u );
		writeln( "    </mul>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( t * u ))();
		PrintTerm( t * u );
		writeln( "    </typedvalue>" );
	}

	void Div( T t, U u )
	{
		if ( __traits( isIntegral, T ) && __traits( isIntegral, U ) && (u == 0) )
		{
			writeln( "    <intvalue value=\"0\"/>" );
			writeln( "    <intvalue value=\"0\"/>" );
		}
		else
		{
			writeln( "    <div>" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </div>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t / u ))();
			PrintTerm( t / u );
			writeln( "    </typedvalue>" );
		}
	}

	static if ( (!is( U : cfloat) && !is( U : cdouble ) && !is( U : creal )) )
	{
		void Mod( T t, U u )
		{
			if ( __traits( isIntegral, T ) && __traits( isIntegral, U ) && (u == 0) )
			{
				writeln( "    <intvalue value=\"0\"/>" );
				writeln( "    <intvalue value=\"0\"/>" );
			}
			else
			{
				writeln( "    <mod>" );
				CastTerm( t );
				CastTerm( u );
				writeln( "    </mod>" );

				writeln( "    <typedvalue>" );
				PrintType!(typeof( t % u ))();
				PrintTerm( t % u );
				writeln( "    </typedvalue>" );
			}
		}
	}

	static if ( 
			(is( T == float) || is( T == double ) || is( T == real ))
		&&	(is( U == T ) || is( U == int ) || is( U == uint ))
		)
	{
		void Pow( T t, U u )
		{
			writeln( "    <pow>" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </pow>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t ^^ u ))();
			PrintTerm( t ^^ u );
			writeln( "    </typedvalue>" );
		}
	}

	static if (
		__traits( isIntegral, T ) && __traits( isIntegral, U )
		)
	{
		void BitOr( T t, U u )
		{
			writeln( "    <bitor>" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </bitor>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t | u ))();
			PrintTerm( t | u );
			writeln( "    </typedvalue>" );
		}

		void BitAnd( T t, U u )
		{
			writeln( "    <bitand>" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </bitand>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t & u ))();
			PrintTerm( t & u );
			writeln( "    </typedvalue>" );
		}

		void BitXor( T t, U u )
		{
			writeln( "    <bitxor>" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </bitxor>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t ^ u ))();
			PrintTerm( t ^ u );
			writeln( "    </typedvalue>" );
		}
	}

	static if (
		__traits( isIntegral, T ) && __traits( isIntegral, U )
		)
	{
		void ShiftLeft( T t, U u )
		{
			writeln( "    <shiftleft>" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </shiftleft>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t << u ))();
			PrintTerm( t << u );
			writeln( "    </typedvalue>" );
		}

		void ShiftRight( T t, U u )
		{
			writeln( "    <shiftright>" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </shiftright>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t >> u ))();
			PrintTerm( t >> u );
			writeln( "    </typedvalue>" );
		}
	}

	void And( T t, U u )
	{
		writeln( "    <and>" );
		CastTerm( t );
		CastTerm( u );
		writeln( "    </and>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( t && u ))();
		PrintTerm( t && u );
		writeln( "    </typedvalue>" );
	}

	void Or( T t, U u )
	{
		writeln( "    <or>" );
		CastTerm( t );
		CastTerm( u );
		writeln( "    </or>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( t || u ))();
		PrintTerm( t || u );
		writeln( "    </typedvalue>" );
	}

	void Equal( T t, U u )
	{
		writeln( "    <cmp op=\"==\">" );
		CastTerm( t );
		CastTerm( u );
		writeln( "    </cmp>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( t == u ))();
		PrintTerm( t == u );
		writeln( "    </typedvalue>" );
	}

	void NotEqual( T t, U u )
	{
		writeln( "    <cmp op=\"!=\">" );
		CastTerm( t );
		CastTerm( u );
		writeln( "    </cmp>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( t != u ))();
		PrintTerm( t != u );
		writeln( "    </typedvalue>" );
	}

	void Is( T t, U u )
	{
		writeln( "    <cmp op=\"is\">" );
		CastTerm( t );
		CastTerm( u );
		writeln( "    </cmp>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( t is u ))();
		PrintTerm( t is u );
		writeln( "    </typedvalue>" );
	}

	void NotIs( T t, U u )
	{
		writeln( "    <cmp op=\"!is\">" );
		CastTerm( t );
		CastTerm( u );
		writeln( "    </cmp>" );

		writeln( "    <typedvalue>" );
		PrintType!(typeof( t !is u ))();
		PrintTerm( t !is u );
		writeln( "    </typedvalue>" );
	}

	static if (
			!is( T == cfloat ) && !is( T == cdouble ) && !is( T == creal )
		&&	!is( U == cfloat ) && !is( U == cdouble ) && !is( U == creal )
		)
	{
		void Less( T t, U u )
		{
			writeln( "    <cmp op=\"&lt;\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t < u ))();
			PrintTerm( t < u );
			writeln( "    </typedvalue>" );
		}

		void LessEqual( T t, U u )
		{
			writeln( "    <cmp op=\"&lt;=\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t <= u ))();
			PrintTerm( t <= u );
			writeln( "    </typedvalue>" );
		}

		void Greater( T t, U u )
		{
			writeln( "    <cmp op=\">\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t > u ))();
			PrintTerm( t > u );
			writeln( "    </typedvalue>" );
		}

		void GreaterEqual( T t, U u )
		{
			writeln( "    <cmp op=\">=\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t >= u ))();
			PrintTerm( t >= u );
			writeln( "    </typedvalue>" );
		}
	}

	static if (
			!is( T == cfloat ) && !is( T == cdouble ) && !is( T == creal )
		&&	!is( U == cfloat ) && !is( U == cdouble ) && !is( U == creal )
		)
	{
		void Unordered( T t, U u )
		{
			writeln( "    <cmp op=\"!&lt;>=\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t !<>= u ))();
			PrintTerm( t !<>= u );
			writeln( "    </typedvalue>" );
		}

		void LessGreater( T t, U u )
		{
			writeln( "    <cmp op=\"&lt;>\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t <> u ))();
			PrintTerm( t <> u );
			writeln( "    </typedvalue>" );
		}

		void LessGreaterEqual( T t, U u )
		{
			writeln( "    <cmp op=\"&lt;>=\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t <>= u ))();
			PrintTerm( t <>= u );
			writeln( "    </typedvalue>" );
		}

		void UnorderedGreater( T t, U u )
		{
			writeln( "    <cmp op=\"!&lt;=\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t !<= u ))();
			PrintTerm( t !<= u );
			writeln( "    </typedvalue>" );
		}

		void UnorderedGreaterEqual( T t, U u )
		{
			writeln( "    <cmp op=\"!&lt;\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t !< u ))();
			PrintTerm( t !< u );
			writeln( "    </typedvalue>" );
		}

		void UnorderedLess( T t, U u )
		{
			writeln( "    <cmp op=\"!>=\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t !>= u ))();
			PrintTerm( t !>= u );
			writeln( "    </typedvalue>" );
		}

		void UnorderedLessEqual( T t, U u )
		{
			writeln( "    <cmp op=\"!>\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t !> u ))();
			PrintTerm( t !> u );
			writeln( "    </typedvalue>" );
		}

		void UnorderedEqual( T t, U u )
		{
			writeln( "    <cmp op=\"!&lt;>\">" );
			CastTerm( t );
			CastTerm( u );
			writeln( "    </cmp>" );

			writeln( "    <typedvalue>" );
			PrintType!(typeof( t !<> u ))();
			PrintTerm( t !<> u );
			writeln( "    </typedvalue>" );
		}
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

template BinaryList(T...)
{
	void Operation(U...)( string opName )
	{
		int	code = 0;

		switch ( std.string.tolower( opName ) )
		{
		case "add":	code = 0;	break;
		case "sub":	code = 1;	break;
		case "mul":	code = 2;	break;
		case "div":	code = 3;	break;
		case "mod":	code = 4;	break;
		case "pow":	code = 5;	break;
		case "bitand":	code = 6;	break;
		case "bitor":	code = 7;	break;
		case "bitxor":	code = 8;	break;
		case "shiftleft":	code = 9;	break;
		case "shiftright":	code = 10;	break;
		case "and":	code = 11;	break;
		case "or":	code = 12;	break;
		case "equal":				case "==":	code = 13;	break;
		case "notequal":			case "!=":	code = 14;	break;
		case "less":				case "<":	code = 15;	break;
		case "lessequal":			case "<=":	code = 16;	break;
		case "greater":				case ">":	code = 17;	break;
		case "greaterequal":		case ">=":	code = 18;	break;
		case "unordered":			case "!<>=":code = 19;	break;
		case "lessgreater":			case "<>":	code = 20;	break;
		case "lessgreaterequal":	case "<>=":	code = 21;	break;
		case "unorderedgreater":	case "!<=":	code = 22;	break;
		case "unorderedgreaterequal":	case "!<":	code = 23;	break;
		case "unorderedless":		case "!>=":	code = 24;	break;
		case "unorderedlessequal":	case "!>":	code = 25;	break;
		case "unorderedequal":		case "!<>":	code = 26;	break;
		case "is":				code = 27;	break;
		case "notis":			case "!is":	code = 28;	break;
		}

		foreach ( t; T )
		{
			foreach ( u; U )
			{
				switch ( code )
				{
				case 0:	Binary!(t, u)( &BinOp!(t, u).Add );	break;
				case 1:	Binary!(t, u)( &BinOp!(t, u).Sub );	break;
				case 2:	Binary!(t, u)( &BinOp!(t, u).Mul );	break;
				case 3:	Binary!(t, u)( &BinOp!(t, u).Div );	break;
				case 4:	
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).Mod ) ) )
							Binary!(t, u)( &BinOp!(t, u).Mod );
					break;
				case 5:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).Pow ) ) )
						Binary!(t, u)( &BinOp!(t, u).Pow );
					break;
				case 6:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).BitAnd ) ) )
						Binary!(t, u)( &BinOp!(t, u).BitAnd );
					break;
				case 7:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).BitOr ) ) )
						Binary!(t, u)( &BinOp!(t, u).BitOr );
					break;
				case 8:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).BitXor ) ) )
						Binary!(t, u)( &BinOp!(t, u).BitXor );
					break;
				case 9:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).ShiftLeft ) ) )
						Binary!(t, u)( &BinOp!(t, u).ShiftLeft );
					break;
				case 10:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).ShiftRight ) ) )
						Binary!(t, u)( &BinOp!(t, u).ShiftRight );
					break;
				case 11:	Binary!(t, u)( &BinOp!(t, u).And );	break;
				case 12:	Binary!(t, u)( &BinOp!(t, u).Or );	break;
				case 13:	Binary!(t, u)( &BinOp!(t, u).Equal );	break;
				case 14:	Binary!(t, u)( &BinOp!(t, u).NotEqual );	break;
				case 15:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).Less ) ) )
						Binary!(t, u)( &BinOp!(t, u).Less );
					break;
				case 16:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).LessEqual ) ) )
						Binary!(t, u)( &BinOp!(t, u).LessEqual );
					break;
				case 17:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).Greater ) ) )
						Binary!(t, u)( &BinOp!(t, u).Greater );
					break;
				case 18:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).GreaterEqual ) ) )
						Binary!(t, u)( &BinOp!(t, u).GreaterEqual );
					break;
				case 19:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).Unordered ) ) )
						Binary!(t, u)( &BinOp!(t, u).Unordered );
					break;
				case 20:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).LessGreater ) ) )
						Binary!(t, u)( &BinOp!(t, u).LessGreater );
					break;
				case 21:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).LessGreaterEqual ) ) )
						Binary!(t, u)( &BinOp!(t, u).LessGreaterEqual );
					break;
				case 22:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).UnorderedGreater ) ) )
						Binary!(t, u)( &BinOp!(t, u).UnorderedGreater );
					break;
				case 23:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).UnorderedGreaterEqual ) ) )
						Binary!(t, u)( &BinOp!(t, u).UnorderedGreaterEqual );
					break;
				case 24:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).UnorderedLess ) ) )
						Binary!(t, u)( &BinOp!(t, u).UnorderedLess );
					break;
				case 25:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).UnorderedLessEqual ) ) )
						Binary!(t, u)( &BinOp!(t, u).UnorderedLessEqual );
					break;
				case 26:
					static if ( __traits( compiles, Binary!(t, u)( &BinOp!(t, u).UnorderedEqual ) ) )
						Binary!(t, u)( &BinOp!(t, u).UnorderedEqual );
					break;
				case 27:	Binary!(t, u)( &BinOp!(t, u).Is );	break;
				case 28:	Binary!(t, u)( &BinOp!(t, u).NotIs );	break;
				}
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
		if ( op != "pow" )
		{
			BinaryList!(byte, ubyte, short, ushort, int, uint, long, ulong)
				.Operation!(byte, ubyte, short, ushort, int, uint, long, ulong)( op );
		}
	}
	else if ( set == 2 )
	{
		if ( op == "mod" )
		{
			BinaryList!(byte, ubyte, short, ushort, int, uint, long, ulong)
				.Operation!(float, double, real, ifloat, idouble, ireal)( op );
		}
		else if ( op != "pow" && op != "bitand" && op != "bitor" && op != "bitxor" )
		{
			BinaryList!(byte, ubyte, short, ushort, int, uint, long, ulong)
				.Operation!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)( op );
		}
	}
	else if ( set == 3 )
	{
		if ( op == "mod" )
		{
			BinaryList!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)
				.Operation!(byte, ubyte, short, ushort, int, uint, long, ulong)( op );
		}
		else if ( op == "pow" )
		{
			BinaryList!(float, double, real)
				.Operation!(int, uint)( op );
		}
		else if ( op != "bitand" && op != "bitor" && op != "bitxor" )
		{
			BinaryList!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)
				.Operation!(byte, ubyte, short, ushort, int, uint, long, ulong)( op );
		}
	}
	else if ( set == 4 )
	{
		if ( op == "mod" )
		{
			BinaryList!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)
				.Operation!(float, double, real, ifloat, idouble, ireal)( op );
		}
		else if ( op == "pow" )
		{
			BinaryList!(float)
				.Operation!(float)( op );
			BinaryList!(double)
				.Operation!(double)( op );
			BinaryList!(real)
				.Operation!(real)( op );
		}
		else if ( op != "bitand" && op != "bitor" && op != "bitxor" )
		{
			BinaryList!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)
				.Operation!(float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)( op );
		}
	}
	else if ( set == 5 )
	{
		// TODO: consider adding bool to all the int sets above, and getting rid of this set
		if ( op == "and" || op == "or" )
		{
			BinaryList!(bool)
				.Operation!(bool)( op );
			BinaryList!(bool)
				.Operation!(byte, ubyte, short, ushort, int, uint, long, ulong, float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)( op );
			BinaryList!(byte, ubyte, short, ushort, int, uint, long, ulong, float, double, real, ifloat, idouble, ireal, cfloat, cdouble, creal)
				.Operation!(bool)( op );
		}
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
