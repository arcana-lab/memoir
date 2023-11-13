BEGIN {
  found=0
}
{
  if (found > 0) {
    if ($1 ~ /NumSSA/) {
      if (found < 3) {
        printf "%-10d ┃ ", $3
      } else {
        printf "%-10d\n", $3
      }
      found+=1
    }
    if (found > 3) {
      found=0
    }
  } else {
    if ($0 ~ /STATS/) {
      found=1
    }
  }
}
