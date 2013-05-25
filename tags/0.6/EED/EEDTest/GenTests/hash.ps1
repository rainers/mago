#
#   Copyright (c) 2010 Aldo J. Nunez
#
#   Licensed under the Apache License, Version 2.0.
#   See the LICENSE text file for details.
#

function ByteToUpper( [byte] $b )
{
	return [byte] ($b -band 0xDF)
}

function IntToUpper( [uint32] $i )
{
	return [uint32] ($i -band 0xDFDFDFDF)
}


function GetSymbolNameHash( $name )
{
	[uint32] $end = 0
	[uint32] $wordLen = 0
	[uint32] $sum = 0

	$nameLen = $name.length

	while ( ($nameLen -band 3) -ne 0 )
	{
		$end = $end -bor (ByteToUpper( $name[ $nameLen - 1 ] ))
		$end = $end * 256	# << 8
		$nameLen--
	}

	$wordLen = $nameLen / 4

	for ( [uint32] $i = 0; $i -lt $wordLen; $i++ )
	{
		$wordName = 0

		for ( $j = 3; $j -ge 0; $j-- )
		{
			$wordName *= 256
			$wordName = $wordName -bor ($name[$i * 4 + $j] -band 0xFF)
		}

		$sum = $sum -bxor (IntToUpper( $wordName ))

		# rotl

		# $hi = $sum >> 28
		[uint32] $hi = ($sum -band 0xF0000000) / (256*256*256*16) -band 0xFF

		# $sum <<= 4
		[uint64] $longSum = $sum * 16
		$sum = [uint32] ($longSum -band 0x00000000FFFFFFFFL)

		$sum = $sum -bor $hi
	}

	$sum = $sum -bxor $end

	return $sum
}

"hash of '$args' = "
$hash = GetSymbolNameHash( $args[0] )
"{0} (0x{0:X})" -f $hash
