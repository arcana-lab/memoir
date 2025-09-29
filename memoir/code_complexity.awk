#!/bin/awk

BEGIN {
  count=0
}
{
  if ($1 ~ "C++") count+=$NF
}
END {
  print count
}
