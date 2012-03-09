#
#   Copyright (c) 2010 Aldo J. Nunez
#
#   Licensed under the Apache License, Version 2.0.
#   See the LICENSE text file for details.
#

param ( $binSets, $binOps, $prefix, $toolGen, [switch] $whatIf, [switch] $force )


$setSpecs = 
@(
	"Int",
	"Float"
)

$testSpecs = 
@{
	"negate" = "Negate";
	"bitnot" = "BitNot";
	"not" = "Not";
	"unaryadd" = "UnaryAdd";
}


if ( $toolGen -eq $null )
{
	$toolGen = ".\genUnaryTest"
}

if ( $prefix -eq $null )
{
	$prefix = "Hello"
}

if ( $binSets -eq $null )
{
	$binSets = 1..2
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
