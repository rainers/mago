#
#   Copyright (c) 2010 Aldo J. Nunez
#
#   Licensed under the Apache License, Version 2.0.
#   See the LICENSE text file for details.
#

param ( $binSets, $binOps, $prefix, $toolGen, [switch] $whatIf, [switch] $force )


$setSpecs = 
@(
	"IntInt",
	"IntFloat",
	"FloatInt",
	"FloatFloat"
)

$testSpecs = 
@{
	"add" = "Add";
	"sub" = "Sub";
	"mul" = "Mul";
	"div" = "Div";
	"mod" = "Mod";
	# skip Pow
	"bitand" = "BitAnd";
	"bitor" = "BitOr";
	"bitxor" = "BitXor";
	"shiftleft" = "ShiftLeft";
	"shiftright" = "ShiftRight";
	"and" = "And";
	"Or" = "Or";
	"==" = "Equal";
	"!=" = "NotEqual";
	"<" = "Less";
	"<=" = "LessEqual";
	">" = "Greater";
	">=" = "GreaterEqual";
	"!<>=" = "Unordered";
	"<>" = "LessGreater";
	"<>=" = "LessGreaterEqual";
	"!<=" = "UnorderedGreater";
	"!<" = "UnorderedGreaterEqual";
	"!>=" = "UnorderedLess";
	"!>" = "UnorderedLessEqual";
	"!<>" = "UnorderedEqual";
}


if ( $toolGen -eq $null )
{
	$toolGen = ".\genArithTest"
}

if ( $prefix -eq $null )
{
	$prefix = "Hello"
}

if ( $binSets -eq $null )
{
	$binSets = 1..4
}

if ( $binOps -eq $null )
{
	$binOps = $testSpecs.keys
}


foreach ( $i in $binSets )
{
	foreach ( $op in $binOps )
	{
		$fileOp = $testSpecs[$op]
		$filename = "test{0}{1}.xml" -f $fileOp, $setSpecs[$i-1]

		if ( $force.isPresent -or -not (test-path $filename) )
		{
			if ( $whatIf.isPresent )
			{
				"didn't find $filename"
			}
			else
			{
				"generating $filename"
				& $toolGen $i $op $prefix > $filename
			}
		}
		else
		{
			"found $filename"
		}
	}
}
