#
#   Copyright (c) 2010 Aldo J. Nunez
#
#   Licensed under the Apache License, Version 2.0.
#   See the LICENSE text file for details.
#

$toolGen = ".\genArithTest"

$prefix = "Hello"

$binSetSpecs = 
@(
	"IntInt",
	"IntFloat",
	"FloatInt",
	"FloatFloat"
)

$binTestSpecs = 
@(
	@{ op="<"; fileOp="Less" }
	@{ op="<="; fileOp="LessEqual" }
	@{ op=">"; fileOp="Greater" }
	@{ op=">="; fileOp="GreaterEqual" }
	@{ op="!<>="; fileOp="Unordered" }
	@{ op="<>"; fileOp="LessGreater" }
	@{ op="<>="; fileOp="LessGreaterEqual" }
	@{ op="!<="; fileOp="UnorderedGreater" }
	@{ op="!<"; fileOp="UnorderedGreaterEqual" }
	@{ op="!>="; fileOp="UnorderedLess" }
	@{ op="!>"; fileOp="UnorderedLessEqual" }
	@{ op="!<>"; fileOp="UnorderedEqual" }
)

foreach ( $i in 1..4 )
{
	foreach ( $test in $binTestSpecs )
	{
		$filename = "test{0}{1}.xml" -f $test.fileOp, $binSetSpecs[$i-1]
		if ( $forceOverwrite -or -not (test-path $filename) )
		{
			"$filename not found"
			#& $toolGen $i "$($test.op)" $prefix > $filename
			& $toolGen $i $test.op $prefix > $filename
		}
		else
		{
			"$filename found"
		}
	}
}
