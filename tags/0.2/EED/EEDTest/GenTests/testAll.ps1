#
#   Copyright (c) 2010 Aldo J. Nunez
#
#   Licensed under the Apache License, Version 2.0.
#   See the LICENSE text file for details.
#

param ( [switch] $self )

$t1 = get-date
$passedCount = 0

dir test*.xml |% `
{
	if ( $self.isPresent )
	{
		$selfArg = "-self"
	}

	..\..\release\eedtest -data ..\d1.xml -test $_ $selfArg
	if ( $LASTEXITCODE -ne 0 )
	{
		echo "Error in test file $($_.name)."
		exit 1
	}

	$passedCount++
}

$t2 = get-date
$span = $t2 - $t1

"Tests passed: $passedCount"
"Duration: $span"
