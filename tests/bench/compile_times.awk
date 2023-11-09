BEGIN {
  found=0
}
{
  if (found) {
    if ($0 ~ /Total Execution Time:/) {
      print $4 ;
      found=0
    }
  } else {
    if ($0 ~ /... Pass execution timing report .../) {
      found=1
    }
  }
}
